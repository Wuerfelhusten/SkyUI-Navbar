#pragma once
#include <string_view>

namespace Navbar
{
	struct FadePresentation
	{
		bool menu;
		bool fullscreen;
	};

	constexpr FadePresentation PresentationFor(std::string_view a_mode, std::string_view a_target)
	{
		const bool menu = a_mode == "menu";
		const bool sceneChange = a_target == "MapMenu" || a_target == "StatsMenu";
		return { menu, !menu || sceneChange };
	}
}
