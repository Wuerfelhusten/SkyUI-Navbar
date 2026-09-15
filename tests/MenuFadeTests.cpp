#include "FadePresentation.h"
#include "MenuOpacity.h"
#include "SwitchFade.h"
#include <iostream>
#include <optional>
#include <stdexcept>

namespace
{
	void Check(bool a_value)
	{
		if (!a_value) {
			throw std::runtime_error("menu fade regression");
		}
	}
}

int main()
{
	using namespace Navbar;
	using namespace std::chrono_literals;
	try {
		// Source is deliberately not an input: leaving Map/Skills must not
		// request a screen cover when the destination is an ordinary menu.
		for (const auto target : { "InventoryMenu", "MagicMenu", "MapMenu", "StatsMenu", "BestiaryMenu", "MetaSkillsMenu", "Sleep/Wait Menu", "CharacterSheet", "AchievementMenu", "ExtraMenu" }) {
			const auto old = PresentationFor("fullscreen", target);
			const auto menu = PresentationFor("menu", target);
			Check(!old.menu && old.fullscreen);
			Check(menu.menu);
			const bool scene = std::string_view(target) == "MapMenu" || std::string_view(target) == "StatsMenu";
			Check(menu.fullscreen == scene);
		}
		for (const double hostAlpha : { 0.0, 35.0, 100.0 }) {
			for (const double factor : { 0.0, 0.25, 0.5, 1.0 }) {
				double alpha = hostAlpha;
				int    draws = 0;
				RenderWithMenuOpacity(factor, [&]() { return std::optional(alpha); }, [&](double a_value) { alpha = a_value; return true; }, [&]() {
						Check(alpha == hostAlpha * factor);
						++draws; });
				Check(draws == 1 && alpha == hostAlpha);
			}
		}
		int draws = 0, writes = 0;
		RenderWithMenuOpacity(0.5, []() { return std::optional<double>(); }, [&](double) { ++writes; return true; }, [&]() { ++draws; });
		Check(draws == 1 && writes == 0);
		RenderWithMenuOpacity(0.5, []() { return std::optional(100.0); }, [&](double) { ++writes; return false; }, [&]() { ++draws; });
		Check(draws == 2 && writes == 1);
		double alpha = 75.0;
		try {
			RenderWithMenuOpacity(0.5, [&]() { return std::optional(alpha); }, [&](double a_value) { alpha = a_value; return true; }, []() { throw 1; });
		} catch (int) {}
		Check(alpha == 75.0);
		// Presentation changes neither close/show scheduling nor readiness waits.
		for (const auto duration : { 0.0, 0.1, 0.12, 1.0 }) {
			SwitchFade old, menu;
			const auto start = SwitchFade::Clock::now();
			old.Start(start, duration, duration);
			menu.Start(start, duration, duration);
			bool closed = false, ready = false;
			for (int ms = 0; ms < 3000; ms += 10) {
				const auto a = old.Tick(start + std::chrono::milliseconds(ms), closed, ready);
				const auto b = menu.Tick(start + std::chrono::milliseconds(ms), closed, ready);
				Check(a == b && old.Alpha() == menu.Alpha());
				if (a == SwitchFade::Action::kHideSource) {
					closed = true;
				}
				if (a == SwitchFade::Action::kShowTarget) {
					ready = true;
				}
			}
			Check(!old.Active() && !menu.Active());
		}
		std::cout << "Menu/fullscreen/both routing, render-only alpha restoration and timing passed\n";
		return 0;
	} catch (const std::exception& error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
