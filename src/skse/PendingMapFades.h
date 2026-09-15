#pragma once
#include <cmath>
#include <mutex>
#include <optional>
#include <unordered_set>

namespace Navbar
{
	// Identity only. Consumption and native destruction both remove the marker.
	// Thus a discarded message cannot accidentally tag a later reused allocation.
	class PendingMapFades
	{
	public:
		void Mark(const void* a_data)
		{
			if (!a_data) {
				return;
			}
			const std::scoped_lock lock(mutex);
			pending.insert(a_data);
		}
		bool Take(const void* a_data)
		{
			const std::scoped_lock lock(mutex);
			return pending.erase(a_data) != 0;
		}

	private:
		std::mutex                      mutex;
		std::unordered_set<const void*> pending;
	};

	inline std::optional<double> MapFadeCompletionStep(double a_duration, double a_minimum,
		double a_startAlpha, double a_endAlpha)
	{
		// Only the verified black-to-transparent Map entry/exit requests.
		if (!std::isfinite(a_duration) || !std::isfinite(a_minimum) || a_duration <= 0 ||
			a_duration > 60 || a_minimum < 0 || a_minimum > 60 || a_startAlpha != 100 || a_endAlpha != 0) {
			return std::nullopt;
		}
		return a_duration + a_minimum + 1.0;
	}
}
