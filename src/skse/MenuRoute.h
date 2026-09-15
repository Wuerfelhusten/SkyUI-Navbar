#pragma once
#include <string_view>

namespace Navbar
{
	enum class MenuProtocol
	{
		kNative,
		kBestiary,
		kCharacter,
		kMetaSkills,
		kAchievements,
		kWait
	};
	struct MenuRoute
	{
		std::string_view nativeName;
		MenuProtocol     protocol = MenuProtocol::kNative;
		const char*      root = "_root";
		const char*      openEvent = nullptr;
		const char*      closeEvent = nullptr;
	};
	constexpr MenuRoute RouteFor(std::string_view a_name)
	{
		if (a_name == "BestiaryMenu") {
			return { a_name, MenuProtocol::kBestiary, "_root.BestiaryMenu_mc" };
		}
		if (a_name == "CharacterSheet") {
			return { a_name, MenuProtocol::kCharacter, "_root.CharacterSheet_mc" };
		}
		if (a_name == "MetaSkillsMenu") {
			return { "CustomMenu", MenuProtocol::kMetaSkills, "_root.MetaController_mc", "MetaSkillMenu_Open" };
		}
		if (a_name == "AchievementMenu") {
			return { a_name, MenuProtocol::kAchievements, "_root.MenuFader_mc.Menu_mc",
				"AchievementsMenu_Open", "AchievementMenu_Close" };
		}
		if (a_name == "Sleep/Wait Menu") {
			return { a_name, MenuProtocol::kWait, "_root.SleepWaitMenu_mc" };
		}
		return { a_name };
	}
	constexpr bool NeedsUnpausedOpen(std::string_view a_name)
	{
		const auto protocol = RouteFor(a_name).protocol;
		return protocol == MenuProtocol::kBestiary || protocol == MenuProtocol::kMetaSkills || protocol == MenuProtocol::kWait;
	}
	constexpr bool BlocksNavigation(bool a_allowedHost, bool a_modal, bool a_cursor, bool a_context)
	{
		return !a_allowedHost && (a_modal || a_cursor || a_context);
	}
}
