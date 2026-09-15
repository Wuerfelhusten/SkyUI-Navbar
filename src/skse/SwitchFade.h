#pragma once
#include <algorithm>
#include <chrono>

namespace Navbar
{
	class SwitchFade
	{
	public:
		using Clock = std::chrono::steady_clock;
		enum class Phase
		{
			kIdle,
			kCover,
			kClosing,
			kOpening,
			kReveal
		};
		enum class Action
		{
			kNone,
			kHideSource,
			kShowTarget,
			kFinished,
			kAbort
		};
		static Clock::duration WatchdogDuration(double a_fadeOutSeconds, double a_fadeInSeconds)
		{
			return std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(
				6.0 + (std::max)(0.0, a_fadeOutSeconds - 0.12) + (std::max)(0.0, a_fadeInSeconds - 0.12)));
		}
		void Start(Clock::time_point a_now, double a_fadeOutSeconds = 0.12, double a_fadeInSeconds = 0.12)
		{
			coverMs = a_fadeOutSeconds * 1000.0;
			revealMs = a_fadeInSeconds * 1000.0;
			phase = Phase::kCover;
			start = changed = a_now;
			alpha = 0;
		}
		Action Tick(Clock::time_point a_now, bool a_sourceClosed, bool a_targetReady)
		{
			if (phase == Phase::kIdle) {
				return Action::kNone;
			}
			// User-selected fade time is not a stalled native menu. Preserve the
			// original readiness budget while allowing deliberately slow fades.
			if (a_now - start >= std::chrono::duration<double, std::milli>(5000 + (std::max)(0.0, coverMs - 120)) && phase != Phase::kReveal) {
				phase = Phase::kReveal;
				changed = a_now;
				revealFrom = alpha;
				return Action::kAbort;
			}
			const auto elapsed = std::chrono::duration<double, std::milli>(a_now - changed).count();
			if (phase == Phase::kCover) {
				alpha = coverMs == 0 ? 1.0 : std::clamp(elapsed / coverMs, 0.0, 1.0);
				if (elapsed >= coverMs + 20) {
					phase = Phase::kClosing;
					changed = a_now;
					return Action::kHideSource;
				}
			} else if (phase == Phase::kClosing && a_sourceClosed && elapsed >= 40) {
				phase = Phase::kOpening;
				changed = a_now;
				return Action::kShowTarget;
			} else if (phase == Phase::kOpening && a_targetReady && elapsed >= 40) {
				phase = Phase::kReveal;
				changed = a_now;
				revealFrom = 1;
			} else if (phase == Phase::kReveal) {
				alpha = revealMs == 0 ? 0.0 : revealFrom * (1.0 - std::clamp(elapsed / revealMs, 0.0, 1.0));
				if (elapsed >= revealMs) {
					phase = Phase::kIdle;
					return Action::kFinished;
				}
			}
			return Action::kNone;
		}
		void Cancel(Clock::time_point a_now)
		{
			phase = Phase::kReveal;
			changed = a_now;
			revealFrom = alpha;
		}
		bool   Active() const { return phase != Phase::kIdle; }
		double Alpha() const { return alpha; }
		Phase  State() const { return phase; }

	private:
		Phase             phase = Phase::kIdle;
		Clock::time_point start, changed;
		double            alpha = 0;
		double            revealFrom = 1;
		double            coverMs = 120;
		double            revealMs = 120;
	};
}
