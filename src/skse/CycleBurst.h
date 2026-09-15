#pragma once
#include "SwitchFade.h"
#include <cstdint>

namespace Navbar
{
	constexpr bool CanRetargetCycle(SwitchFade::Phase a_phase, bool a_waiting, bool a_paused, bool a_needsUnpaused)
	{
		return (a_waiting || a_phase == SwitchFade::Phase::kCover || a_phase == SwitchFade::Phase::kClosing) &&
		       !(a_paused && a_needsUnpaused);
	}

	// Logical destination advances independently of the menu currently opening.
	class CycleBurst
	{
	public:
		void Begin(int a_target)
		{
			++session;
			desired = a_target;
		}
		void Clear()
		{
			++session;
			desired = -1;
		}
		void          SetTarget(int a_target) { desired = a_target; }
		int           Target() const { return desired; }
		std::uint64_t Session() const { return desired >= 0 ? session : 0; }
		bool          Pending(int a_committed) const { return desired >= 0 && desired != a_committed; }

	private:
		std::uint64_t session = 0;
		int           desired = -1;
	};

	// A tap begun under the cover survives our own source-close/target-open events.
	// Holds under the cover are drained, never converted to a delayed Next action.
	class CycleBurstTap
	{
	public:
		void Begin(std::int64_t a_now, std::uint64_t a_session)
		{
			start = a_now;
			session = a_session;
			held = true;
		}
		bool Release(std::int64_t a_now, std::uint64_t a_session)
		{
			const bool next = held && session != 0 && session == a_session && a_now >= start && a_now - start < 350;
			held = false;
			return next;
		}
		void Cancel() { session = 0; }
		bool Held() const { return held; }

	private:
		std::int64_t  start = 0;
		std::uint64_t session = 0;
		bool          held = false;
	};
}
