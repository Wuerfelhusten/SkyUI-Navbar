#include "MovieAssetPath.h"
#include "NavbarResourcePath.h"
#include <stdexcept>

int main()
{
	const auto check = [](std::string_view a_url, std::string_view a_expected) {
		const auto actual = Navbar::NavbarMoviePath(a_url);
		if (!actual || *actual != a_expected) {
			throw std::runtime_error("Incorrect movie-relative asset path: " + std::string(a_url));
		}
	};
	check("Interface/inventorymenu.swf", "NavBar.swf");
	check("Interface/statsmenu.swf", "NavBar.swf");
	check("Interface/MetaSkillsMenu/CustomMetaMenu.swf", "../NavBar.swf");
	check("Interface/Exported/statsmenu.gfx", "../NavBar.swf");
	check("Data\\Interface\\MetaSkillsMenu\\CustomMetaMenu.swf", "../NavBar.swf");
	check("E:/Skyrim/Data/Interface/Mods/UI/menu.swf", "../../NavBar.swf");
	check("file:///E:/Skyrim/Data/INTERFACE/MetaSkillsMenu/menu.swf", "../NavBar.swf");
	check("MetaSkillsMenu/CustomMetaMenu.swf", "../NavBar.swf");
	check("NavBar.swf", "NavBar.swf");
	check("Interface/A/../B/./menu.swf?x=1", "../NavBar.swf");
	for (const auto url : { "", "?x=1", "#anchor", "Interface/", "Interface/../escape.swf", "../menu.swf", "https://example.com/menu.swf", "https://example.com/Interface/menu.swf", "C:/Other/menu.swf" }) {
		if (Navbar::NavbarMoviePath(url)) {
			throw std::runtime_error("Unexpected unknown/escaping movie URL accepted");
		}
	}
	const auto resource = [](std::string_view a_raw, std::string_view a_expected) {
		const auto actual = Navbar::ResolveNavbarResource(a_raw);
		if (!actual || *actual != a_expected) {
			throw std::runtime_error("Incorrect private resource mapping");
		}
	};
	resource(Navbar::kNavbarResourceURL, "Interface/NavBar.swf");
	resource("Interface/MetaSkillsMenu/NavBarForSkyUI/__widget__.swf", "Interface/NavBar.swf");
	resource("Interface/Exported/Mod/Nested/NavBarForSkyUI/__widget__.swf", "Interface/NavBar.swf");
	resource("Interface\\MetaSkillsMenu\\NavBarForSkyUI\\__widget__.swf", "Interface/NavBar.swf");
	resource("Interface/MetaSkillsMenu/NavBarForSkyUI/skyui/buttonart.swf", "Interface/skyui/buttonart.swf");
	resource("Interface/MetaSkillsMenu/NavBarForSkyUI/skyui/config.txt", "Interface/skyui/config.txt");
	resource("Interface/MetaSkillsMenu/NavBarForSkyUI/skyui/../gfxfontlib.swf", "Interface/gfxfontlib.swf");
	resource("Interface/MetaSkillsMenu/navbarforskyui/./skyui/buttonart.swf", "Interface/skyui/buttonart.swf");
	for (const auto raw : { "Interface/MetaSkillsMenu/MSMData.json", "Interface/NavBar.swf", "Interface/skyui/buttonart.swf",
			 "Interface/OtherNavBarForSkyUI/foo.swf", "NavBarForSkyUI/../escape.swf", "NavBarForSkyUI/",
			 "NavBarForSkyUI/skyui/../../escape.swf", "https://example.com/NavBarForSkyUI/foo.swf" }) {
		if (Navbar::ResolveNavbarResource(raw)) {
			throw std::runtime_error("Non-owned/escaping file request must not be rewritten");
		}
	}
}
