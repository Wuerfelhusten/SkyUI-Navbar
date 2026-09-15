#include "MenuRoute.h"
#include <iostream>
#include <stdexcept>

int main()
{
	using namespace Navbar;
	int        checks = 0;
	const auto check = [&checks](bool a_value, const char* a_message) {
		++checks;
		if (!a_value) {
			throw std::runtime_error(a_message);
		}
	};
	try {
		for (const auto name : { "InventoryMenu", "MagicMenu", "MapMenu", "StatsMenu", "OtherModMenu" }) {
			const auto route = RouteFor(name);
			check(route.nativeName == name && route.protocol == MenuProtocol::kNative, "generic native contract preserved");
			check(!route.openEvent && !route.closeEvent, "no event dispatched for generic menus");
		}
		const auto meta = RouteFor("MetaSkillsMenu");
		check(meta.nativeName == "CustomMenu", "Meta Skills maps to SKSE CustomMenu");
		check(std::string_view(meta.root) == "_root.MetaController_mc", "shared menu must match owner root");
		check(std::string_view(meta.openEvent) == "MetaSkillMenu_Open", "owner initializes skill data and validates dependencies");
		check(!meta.closeEvent, "MetaSkillMenu_Close alone does not close its movie");
		check(meta.nativeName != RouteFor("StatsMenu").nativeName, "CSM selector and CSF skill tree are separate hosts");
		check(RouteFor("StatsMenu").nativeName == "StatsMenu", "CSF reuses the existing StatsMenu host, not CustomMenu");
		check(meta.openEvent && std::string_view(meta.openEvent) != "MetaSkillMenu_Selection",
			"CSM tab always requests the selector, never the last selected skill tree");
		check(RouteFor("CustomMenu").protocol != MenuProtocol::kMetaSkills, "unrelated CustomMenu is not implicitly Meta Skills");
		const auto achievements = RouteFor("AchievementMenu");
		check(std::string_view(achievements.openEvent) == "AchievementsMenu_Open", "plural open event");
		check(std::string_view(achievements.closeEvent) == "AchievementMenu_Close", "singular close event");
		check(std::string_view(achievements.root) == "_root.MenuFader_mc.Menu_mc", "nested achievement movie");
		check(RouteFor("CharacterSheet").protocol == MenuProtocol::kCharacter, "character native name");
		check(RouteFor("BestiaryMenu").protocol == MenuProtocol::kBestiary, "bestiary owner API");
		check(RouteFor("Sleep/Wait Menu").protocol == MenuProtocol::kWait, "wait native name");
		for (const auto name : { "BestiaryMenu", "MetaSkillsMenu", "Sleep/Wait Menu" }) {
			check(NeedsUnpausedOpen(name), "owner opening must not inherit overlay pause");
		}
		check(!NeedsUnpausedOpen("InventoryMenu"), "standard menu transitions retain pause");
		check(!BlocksNavigation(true, true, true, true), "modal navbar host itself is permitted");
		check(BlocksNavigation(false, true, false, false), "another modal blocks navigation");
		check(BlocksNavigation(false, false, true, false), "another cursor menu blocks navigation");
		check(BlocksNavigation(false, false, false, true), "another context menu blocks navigation");
		check(!BlocksNavigation(false, false, false, false), "passive overlay does not block navigation");
		std::cout << checks << " menu route checks passed\n";
		return 0;
	} catch (const std::exception& error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
