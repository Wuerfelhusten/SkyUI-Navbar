#include "CycleBurst.h"
#include "CycleNavigation.h"
#include <stdexcept>
#include <string>
#include <vector>

void Check(bool a_condition)
{
	if (!a_condition) {
		throw std::runtime_error("cycle assertion failed");
	}
}
int main()
{
	struct Entry
	{
		std::string menu;
		bool        showInNavbar;
	};
	std::vector<Entry> entries{ { "inventory", true }, { "disabled", false }, { "missing", true },
		{ "magic", true }, { "map", true }, { "host-only", false } };
	const auto         available = [](const auto& a_name) { return a_name != "missing"; };
	Check(Navbar::NextMenuIndex(entries, 0, available) == 3);
	Check(Navbar::NextMenuIndex(entries, 3, available) == 4);
	Check(Navbar::NextMenuIndex(entries, 4, available) == 0);
	Check(Navbar::NextMenuIndex(entries, 5, available) == 0);
	Check(Navbar::NextMenuIndex(entries, -1, available) == -1);
	Check(Navbar::NextMenuIndex(entries, 6, available) == -1);
	Navbar::CycleBurst burst;
	Check(!burst.Session() && !burst.Pending(0));
	burst.Begin(Navbar::NextMenuIndex(entries, 0, available));
	const auto session = burst.Session();
	Check(burst.Target() == 3);
	burst.SetTarget(Navbar::NextMenuIndex(entries, burst.Target(), available));
	Check(burst.Target() == 4 && burst.Pending(3));
	burst.SetTarget(Navbar::NextMenuIndex(entries, burst.Target(), available));
	Check(burst.Target() == 0 && burst.Session() == session);  // Three taps wrap, even with the first target still opening.
	Check(!burst.Pending(0));
	for (int i = 0; i < 300; ++i) {
		burst.SetTarget(Navbar::NextMenuIndex(entries, burst.Target(), available));
	}
	Check(burst.Target() == 0);  // No lost taps or fixed-length queue overflow.
	Navbar::CycleBurstTap tap;
	tap.Begin(1000, session);
	Check(tap.Held() && tap.Release(1130, session));
	Check(!tap.Held() && !tap.Release(1131, session));  // No duplicate release.
	tap.Begin(2000, session);
	Check(tap.Release(2349, session));  // May finish after the cover has disappeared.
	tap.Begin(3000, session);
	Check(!tap.Release(3350, session));  // Holding in the cover never counts as spam.
	tap.Begin(4000, session);
	Check(!tap.Release(5000, session));
	tap.Begin(6000, session);
	tap.Cancel();
	Check(tap.Held() && !tap.Release(6010, session));
	tap.Begin(7000, session);
	burst.Clear();
	Check(!burst.Session() && !tap.Release(7010, burst.Session()));
	burst.Begin(3);
	Check(burst.Session() != session);
	tap.Begin(8000, session);
	Check(!tap.Release(8010, burst.Session()));  // Old capture cannot act in a new navigation session.
	using Phase = Navbar::SwitchFade::Phase;
	for (const auto phase : { Phase::kIdle, Phase::kCover, Phase::kClosing, Phase::kOpening, Phase::kReveal }) {
		const bool beforeShow = phase == Phase::kCover || phase == Phase::kClosing;
		Check(Navbar::CanRetargetCycle(phase, false, false, true) == beforeShow);
		Check(Navbar::CanRetargetCycle(phase, false, true, false) == beforeShow);
		Check(!Navbar::CanRetargetCycle(phase, false, true, true));
	}
	Check(Navbar::CanRetargetCycle(Phase::kIdle, true, false, true));
	// Four visible vanilla destinations: exactly three taps from Inventory land on Skills.
	const std::vector<Entry> vanilla{ { "inventory", true }, { "magic", true }, { "map", true }, { "skills", true } };
	burst.Begin(Navbar::NextMenuIndex(vanilla, 0, available));
	for (int i = 0; i < 2; ++i) {
		burst.SetTarget(Navbar::NextMenuIndex(vanilla, burst.Target(), available));
	}
	Check(burst.Target() == 3 && burst.Pending(1));
	entries = { { "only", true } };
	Check(Navbar::NextMenuIndex(entries, 0, available) == -1);
	entries.clear();
	Check(Navbar::NextMenuIndex(entries, 0, available) == -1);
}
