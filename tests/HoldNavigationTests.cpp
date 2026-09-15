#include "HoldNavigation.h"
#include <stdexcept>
#include <string>
#include <vector>

void Check(bool a_condition)
{
	if (!a_condition) {
		throw std::runtime_error("Hold navigation regression");
	}
}
int main()
{
	using Action = Navbar::HoldNavigation::ReleaseAction;
	for (const auto key : { 0x12u, 0x1Cu, 0x9Cu }) {
		Check(Navbar::SelectionConfirmKey(false, key));
	}
	for (const auto key : { 0u, 1u, 0x11u, 0x1Eu, 0x1Fu, 0x20u, 0x38u, 0x1000u }) {
		Check(!Navbar::SelectionConfirmKey(false, key));
	}
	Check(Navbar::SelectionConfirmKey(true, 0x1000));
	Check(!Navbar::SelectionConfirmKey(true, 0x12) && !Navbar::SelectionConfirmKey(true, 0x2000));
	Navbar::HoldNavigation hold;
	Check(hold.Release(100) == Action::kNone);
	hold.Begin(100);
	Check(hold.Held() && !hold.Selecting());
	Check(!hold.Tick(449));
	Check(hold.Release(449) == Action::kNext);
	Check(!hold.Held() && hold.Release(450) == Action::kNone);
	hold.Begin(1000);
	Check(hold.Tick(1350) && hold.Selecting());
	Check(!hold.Tick(2000));
	Check(hold.Release(2100) == Action::kNone);
	Check(hold.Selecting() && !hold.Held());  // Releasing does not close or confirm the view.
	hold.Begin(3000);
	Check(hold.Selecting());
	Check(hold.Release(3350) == Action::kNone);  // Second long press toggles off, even without timer.
	Check(!hold.Selecting());
	hold.Begin(4000);
	hold.Cancel();
	Check(hold.Held() && !hold.Tick(5000));  // Still owns the release after menu loss.
	Check(hold.Release(5000) == Action::kNone);
	hold.Begin(6000);
	Check(hold.Tick(6350));
	hold.Cancel();
	Check(!hold.Selecting() && hold.Release(6400) == Action::kNone);
	hold.Begin(7000);
	Check(hold.Tick(7350) && hold.Selecting());
	Check(!hold.Tick(12000) && hold.Selecting());  // One toggle per physical hold, never oscillate.
	Check(hold.Release(12000) == Action::kNone && hold.Selecting());
	hold.Begin(13000);
	Check(hold.Release(13100) == Action::kNone && !hold.Selecting());  // Short press dismisses the picker, never cycles.
	Check(!hold.Held() && hold.Release(13101) == Action::kNone);
	hold.Begin(14000);
	Check(hold.Release(14001) == Action::kNext);  // A fresh press in normal view cycles again.
	hold.Cancel();
	Check(!hold.Selecting());                       // B/menu loss works even when no switch key is held.
	Check(!hold.Tick(20000) && !hold.Selecting());  // No automatic resume in the destination.
	Check(hold.Release(20001) == Action::kNone);
	hold.Begin(21000);  // A fresh long press is required to select again.
	Check(hold.Tick(21350) && hold.Selecting());
	hold.Cancel();  // Confirm another menu while the switch key is still held.
	Check(!hold.Selecting() && hold.Held());
	Check(!hold.Tick(22000) && !hold.Selecting());
	Check(hold.Release(22001) == Action::kNone && !hold.Selecting());
	Check(!hold.Tick(30000) && !hold.Selecting());

	// Deferred delivery must use capture timestamps, not the later drain time.
	Navbar::HoldNavigation deferred;
	deferred.Begin(1000);
	Check(deferred.Release(1349) == Action::kNext);
	deferred.Begin(2000);
	Check(deferred.Release(2350) == Action::kNone && deferred.Selecting());
	deferred.Begin(3000);
	Check(deferred.Release(3001) == Action::kNone && !deferred.Selecting());

	// CharacterSheet's observed 131 ms tap and 825 ms hold, replayed by the
	// controller's 100 ms task cadence. An empty input batch must still tick.
	Navbar::HoldNavigation character;
	character.Begin(1000);
	Check(!character.Tick(1100));
	Check(character.Release(1131) == Action::kNext);
	character.Begin(2000);
	for (const auto time : { 2100, 2200, 2300 }) {
		Check(!character.Tick(time));
	}
	Check(character.Tick(2400) && character.Selecting());
	Check(!character.Tick(2800));
	Check(character.Release(2825) == Action::kNone && character.Selecting());
	character.Begin(3000);
	Check(character.Release(3131) == Action::kNone && !character.Selecting());

	Navbar::SelectionRepeat repeat;
	Check(repeat.Step(0, 0) == 0);
	Check(repeat.Step(1, 0) == 1);
	Check(repeat.Step(1, 349) == 0);
	Check(repeat.Step(1, 350) == 1);
	Check(repeat.Step(1, 469) == 0);
	Check(repeat.Step(1, 470) == 1);
	Check(repeat.Step(-1, 471) == -1);  // Changing direction is immediate.
	Check(repeat.Step(0, 472) == 0);
	Check(repeat.Step(-1, 473) == -1);  // Releasing rearms a fresh tap.
	Check(repeat.Step(-1, 10000) == -1);
	Check(repeat.Step(-1, 10000) == 0);  // No catch-up burst.

	for (const auto key : { 0x11u, 0x1Eu, 0xC8u, 0xCBu, 0x1Fu, 0x20u, 0xD0u, 0xCDu }) {
		Navbar::SelectionButtons buttons;
		const bool               previous = key == 0x11 || key == 0x1E || key == 0xC8 || key == 0xCB;
		buttons.Route(false, key, true, false);
		Check(buttons.Held() && buttons.Direction() == (previous ? -1 : 1));
		buttons.Route(false, key, false, true);
		Check(!buttons.Held() && buttons.Direction() == 0);
	}
	for (const auto key : { 1u, 2u, 4u, 8u }) {
		Navbar::SelectionButtons buttons;
		buttons.Route(true, key, true, false);
		Check(buttons.Direction() == (key == 1 || key == 4 ? -1 : 1));
	}
	Navbar::SelectionButtons buttons;
	buttons.Route(false, 0x11, true, false);
	buttons.Route(false, 0xC8, true, false);
	buttons.Route(false, 0x11, false, true);
	Check(buttons.Direction() == -1);  // Releasing W does not release a still-held Up key.
	buttons.Route(true, 2, true, false);
	Check(buttons.Held() && buttons.Direction() == 0);  // Opposite directions cancel.
	buttons.Route(true, 2, false, true);
	Check(buttons.Direction() == -1);
	buttons = {};
	buttons.Route(true, 0x1000, true, false);
	buttons.Route(true, 0xC, true, false);
	Check(!buttons.Held());  // Confirm/right-stick identifier is not directional navigation.

	Navbar::SelectionAxis axis;
	Check(!axis.Route(0, 0.1f, false));  // Normal host input outside selection.
	axis.Begin();
	Check(axis.Route(0, 0.4f, true) && axis.Direction() == 0);  // Drift below threshold.
	Check(axis.Route(0, 0.8f, true) && axis.Direction() == -1);
	axis.Cancel();
	Check(axis.Route(0, 0.8f, false) && axis.Direction() == 0);  // Held stick does not reach destination.
	Check(axis.Route(0, 0, false));                              // Consume the neutral release too.
	Check(!axis.Route(0, -0.8f, false));                         // Original host behavior restored.
	axis.Begin();                                                // Already tilted before opening: neutral required before navigation.
	Check(axis.Route(0, -0.8f, true) && axis.Direction() == 0);
	Check(axis.Route(0, 0, true) && axis.Direction() == 0);
	Check(axis.Route(0, -0.8f, true) && axis.Direction() == 1);
	Check(axis.Route(0.8f, 0, true) && axis.Direction() == 0);  // Horizontal movement swallowed, not navigation.

	struct Entry
	{
		std::string menu;
		bool        showInNavbar;
	};
	const std::vector<Entry> entries{ { "inventory", true }, { "hidden", false },
		{ "missing", true }, { "magic", true }, { "map", true }, { "host-only", false } };
	const auto               available = [](const auto& a_menu) { return a_menu != "missing"; };
	Check(Navbar::SelectionMenuIndex(entries, 0, 1, available) == 3);
	Check(Navbar::SelectionMenuIndex(entries, 0, -1, available) == 4);
	Check(Navbar::SelectionMenuIndex(entries, 4, 1, available) == 0);
	Check(Navbar::SelectionMenuIndex(entries, 3, -1, available) == 0);  // Back to owner: cancel on release.
	Check(Navbar::SelectionMenuIndex(entries, 5, 1, available) == 0);
	Check(Navbar::SelectionMenuIndex(entries, 5, -1, available) == 4);
	Check(Navbar::SelectionMenuIndex(entries, 0, 0, available) == 0);
	Check(Navbar::SelectionMenuIndex(entries, -1, 1, available) == -1);
	Check(Navbar::SelectionMenuIndex(entries, 0, 1, [](auto&) { return false; }) == 0);
	const std::vector<Entry> one{ { "owner", true } }, empty;
	Check(Navbar::SelectionMenuIndex(one, 0, 1, available) == 0);
	Check(Navbar::SelectionMenuIndex(empty, -1, 1, available) == -1);
}
