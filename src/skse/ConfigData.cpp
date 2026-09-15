#include "ConfigData.h"

#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <unordered_set>

namespace Navbar
{
	ConfigData ConfigData::Defaults()
	{
		return { { { "InventoryMenu", true, true },
			{ "MagicMenu", true, true },
			{ "MapMenu", true, true },
			{ "BestiaryMenu", true, true },
			{ "StatsMenu", true, true },
			{ "MetaSkillsMenu", true, true },
			{ "Sleep/Wait Menu", true, true },
			{ "CharacterSheet", true, true },
			{ "AchievementMenu", true, true } } };
	}

	ConfigData ConfigData::Parse(std::string_view a_text)
	{
		if (a_text.size() > 65536) {
			throw std::runtime_error("configuration exceeds 64 KiB");
		}
		const auto root = nlohmann::json::parse(a_text, nullptr, true, true);  // JSONC: // and /* */ comments.
		if (!root.is_object()) {
			throw std::runtime_error("configuration must be an object");
		}
		for (const auto& [key, value] : root.items()) {
			if (key != "menus" && key != "navbarScale" && key != "showButtonHints" &&
				key != "fadeOutSeconds" && key != "fadeInSeconds" && key != "fadeMode") {
				throw std::runtime_error("unknown configuration property: " + key);
			}
		}
		if (!root.contains("menus") || !root.at("menus").is_array() || root.at("menus").size() > 32) {
			throw std::runtime_error("menus must be an array of at most 32 entries");
		}
		ConfigData result;
		result.fadeMode = root.value("fadeMode", result.fadeMode);
		if (result.fadeMode != "fullscreen" && result.fadeMode != "menu") {
			throw std::runtime_error("fadeMode must be fullscreen or menu");
		}
		result.showButtonHints = root.value("showButtonHints", true);
		const auto fadeSeconds = [&root](const char* a_key, double a_default) {
			if (!root.contains(a_key)) {
				return a_default;
			}
			const auto& value = root.at(a_key);
			if (!value.is_number()) {
				throw std::runtime_error(std::string(a_key) + " must be a number in seconds");
			}
			const auto seconds = value.get<double>();
			if (!std::isfinite(seconds) || seconds < 0 || seconds > 10) {
				throw std::runtime_error(std::string(a_key) + " must be between 0 and 10 seconds");
			}
			return seconds;
		};
		result.fadeOutSeconds = fadeSeconds("fadeOutSeconds", result.fadeOutSeconds);
		result.fadeInSeconds = fadeSeconds("fadeInSeconds", result.fadeInSeconds);
		if (root.contains("navbarScale")) {
			if (!root.at("navbarScale").is_number()) {
				throw std::runtime_error("navbarScale must be a number");
			}
			result.navbarScale = root.at("navbarScale").get<double>();
			if (!std::isfinite(result.navbarScale) || result.navbarScale < 0.25 || result.navbarScale > 3.0) {
				throw std::runtime_error("navbarScale must be between 0.25 and 3.0");
			}
		}
		std::unordered_set<std::string> seen;
		for (const auto& item : root.at("menus")) {
			if (!item.is_object()) {
				throw std::runtime_error("each menu must be an object");
			}
			for (const auto& [key, value] : item.items()) {
				if (key != "menu" && key != "showNavbar" && key != "showInNavbar") {
					throw std::runtime_error("unknown menu property: " + key);
				}
			}
			MenuEntry entry;
			entry.menu = item.at("menu").get<std::string>();
			entry.showNavbar = item.value("showNavbar", false);
			entry.showInNavbar = item.value("showInNavbar", false);
			const auto validText = [](const std::string& a_value, std::size_t a_max) {
				return !a_value.empty() && a_value.size() <= a_max &&
				       std::ranges::none_of(a_value, [](unsigned char a_c) { return a_c < 32 || a_c == 127; }) &&
				       std::ranges::any_of(a_value, [](unsigned char a_c) { return a_c != ' '; });
			};
			if (!validText(entry.menu, 128)) {
				throw std::runtime_error("menu must be nonempty text (maximum 128 UTF-8 bytes, no control characters)");
			}
			if (!seen.insert(entry.menu).second) {
				throw std::runtime_error("duplicate menu: " + entry.menu);
			}
			result.menus.push_back(std::move(entry));
		}
		return result;
	}

	int ConfigData::Find(std::string_view a_menu) const
	{
		for (std::size_t i = 0; i < menus.size(); ++i) {
			if (menus[i].menu == a_menu) {
				return static_cast<int>(i);
			}
		}
		return -1;
	}
}
