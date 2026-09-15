#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace Navbar
{
	struct MenuEntry
	{
		std::string menu;
		bool        showNavbar{ false };
		bool        showInNavbar{ false };
	};

	struct ConfigData
	{
		std::vector<MenuEntry> menus;
		double                 navbarScale{ 1.0 };
		bool                   showButtonHints{ true };
		double                 fadeOutSeconds{ 0.05 };
		double                 fadeInSeconds{ 0.05 };
		std::string            fadeMode{ "menu" };
		static ConfigData      Defaults();
		static ConfigData      Parse(std::string_view a_text);
		[[nodiscard]] int      Find(std::string_view a_menu) const;
	};
}
