#include "AnimationState.h"
#include <iostream>
#include <stdexcept>

int main()
{
	int        checks = 0;
	const auto check = [&checks](bool a_value) {
		++checks;
		if (!a_value) {
			throw std::runtime_error("Animation handoff check failed");
		}
	};
	try {
		Navbar::AnimationHandoff                handoff;
		const auto                              now = Navbar::AnimationHandoff::Clock::now();
		const std::vector<Navbar::TabAnimation> state{
			{ "InventoryMenu", 7.25, 0, false }, { "MagicMenu", 12.5, 22, true }
		};
		check(handoff.Take("MagicMenu", now).empty());
		handoff.Store("MagicMenu", state, now);
		check(handoff.Take("InventoryMenu", now).empty());
		handoff.ClearFor("InventoryMenu");  // Closing the source must preserve the handoff.
		const auto restored = handoff.Take("MagicMenu", now + std::chrono::milliseconds(500));
		check(restored.size() == 2);
		check(restored[0].menu == "InventoryMenu" && restored[0].position == 7.25 && restored[0].target == 0 && !restored[0].hover);
		check(restored[1].menu == "MagicMenu" && restored[1].position == 12.5 && restored[1].target == 22 && restored[1].hover);
		check(handoff.Take("MagicMenu", now).empty());  // Exactly one destination instance.
		handoff.Store("MagicMenu", state, now);
		handoff.ClearFor("MagicMenu");
		check(handoff.Take("MagicMenu", now).empty());
		handoff.Store("MagicMenu", state, now);
		check(handoff.Take("MagicMenu", now + std::chrono::seconds(5)).empty());
		handoff.Store("MagicMenu", state, now);
		handoff.Store("MapMenu", state, now);
		check(handoff.Take("MagicMenu", now).empty());
		check(handoff.Take("MapMenu", now).size() == 2);
		handoff.Store("MagicMenu", state, now);
		handoff.Clear();
		check(handoff.Take("MagicMenu", now).empty());
		handoff.Store("MagicMenu", state, now);
		handoff.Retarget("StatsMenu");
		check(handoff.Take("MagicMenu", now).empty());
		const auto skipped = handoff.Take("StatsMenu", now + std::chrono::milliseconds(200));
		check(skipped.size() == 2 && skipped[1].position == 12.5 && skipped[1].hover);
		handoff.Store("MagicMenu", state, now);
		handoff.Retarget("MapMenu");
		check(handoff.Take("MapMenu", now + std::chrono::seconds(5)).empty());  // Retargeting cannot extend stale state.
		std::cout << checks << " animation handoff checks passed\n";
	} catch (const std::exception& error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
