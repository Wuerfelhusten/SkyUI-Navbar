#include "ConfigData.h"
#include "CycleNavigation.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace
{
	int  checks = 0;
	void Check(bool a_condition, const char* a_message)
	{
		++checks;
		if (!a_condition) {
			throw std::runtime_error(a_message);
		}
	}
	void Reject(std::string_view a_text)
	{
		bool rejected = false;
		try {
			(void)Navbar::ConfigData::Parse(a_text);
		} catch (const std::exception&) {
			rejected = true;
		}
		Check(rejected, "invalid configuration was accepted");
	}
}

int main(int argc, char** argv)
{
	try {
		const auto defaults = Navbar::ConfigData::Defaults();
		Check(defaults.menus.size() == 9, "all nine supported menus listed");
		for (std::size_t i = 0; i < defaults.menus.size(); ++i) {
			Check(defaults.menus[i].showNavbar && defaults.menus[i].showInNavbar, "supported menus enabled; availability is detected at runtime");
		}
		Check(defaults.showButtonHints, "hints on by default");
		Check(defaults.fadeMode == "menu", "menu-only presentation is the release default");
		Check(Navbar::ConfigData::Parse(R"({"menus":[]})").fadeMode == "menu", "missing mode uses the release default");
		Check(Navbar::ConfigData::Parse(R"({"fadeMode":"menu","menus":[]})").fadeMode == "menu", "menu-only fade can be selected");
		Check(Navbar::ConfigData::Parse(R"({"fadeMode":"fullscreen","menus":[]})").fadeMode == "fullscreen", "original mode can be restored");
		for (const auto value : { "true", "null", "1", "[]", "\"MENU\"", "\"invalid\"" }) {
			Reject(std::string("{\"menus\":[],\"fadeMode\":") + value + "}");
		}
		Check(defaults.fadeOutSeconds == 0.05 && defaults.fadeInSeconds == 0.05, "release fade durations are 50 milliseconds");
		const auto noFades = Navbar::ConfigData::Parse(R"({"menus":[]})");
		Check(noFades.fadeOutSeconds == 0.05 && noFades.fadeInSeconds == 0.05, "omitted durations use release defaults");
		for (const auto field : { "fadeOutSeconds", "fadeInSeconds" }) {
			for (const auto value : { 0.0, 0.1, 0.125, 1.0, 10.0 }) {
				const auto parsed = Navbar::ConfigData::Parse(std::string("{\"menus\":[],\"") + field + "\":" + std::to_string(value) + "}");
				Check((std::string_view(field) == "fadeOutSeconds" ? parsed.fadeOutSeconds : parsed.fadeInSeconds) == value, "decimal fade seconds parsed exactly");
				Check((std::string_view(field) == "fadeOutSeconds" ? parsed.fadeInSeconds : parsed.fadeOutSeconds) == 0.05, "fade directions are independent");
			}
			for (const auto value : { "-0.1", "10.001", "true", "null", "\"0.1\"", "[]", "1e999" }) {
				Reject(std::string("{\"menus\":[],\"") + field + "\":" + value + "}");
			}
		}
		Check(defaults.navbarScale == 1.0, "default sidebar size is 100 percent of the SWF baseline");
		Check(Navbar::ConfigData::Parse(R"({"menus":[]})").navbarScale == 1.0, "missing scale uses current default");
		for (const double scale : { 0.25, 0.8, 1.0, 1.25, 3.0 }) {
			const auto sized = Navbar::ConfigData::Parse("{\"menus\":[],\"navbarScale\":" + std::to_string(scale) + "}");
			Check(sized.navbarScale == scale, "scale parsed correctly");
		}
		for (const auto value : { "0", "-1", "0.249", "3.001", "true", "null", "\"1.0\"", "[]", "1e999" }) {
			Reject(std::string("{\"menus\":[],\"navbarScale\":") + value + "}");
		}
		Check(!Navbar::ConfigData::Parse(R"({"showButtonHints":false,"menus":[]})").showButtonHints, "hints can be hidden");
		Check(Navbar::ConfigData::Parse(R"({"menus":[]})").showButtonHints, "missing hint setting defaults on");
		for (const auto value : { "1", "null", "\"false\"" }) {
			Reject(std::string("{\"menus\":[],\"showButtonHints\":") + value + "}");
		}
		for (const auto field : { "disableMenuTransitions", "skipSkillsIntro", "skipSkillsUiIntro", "followSkyUIBindings", "tutorial", "bindings", "adapters" }) {
			Reject(std::string("{\"menus\":[],\"") + field + "\":false}");
		}
		Check(defaults.Find("StatsMenu") == 4, "skills follows bestiary");
		const std::vector<std::string> expectedOrder{ "InventoryMenu", "MagicMenu", "MapMenu", "BestiaryMenu",
			"StatsMenu", "MetaSkillsMenu", "Sleep/Wait Menu", "CharacterSheet", "AchievementMenu" };
		for (std::size_t i = 0; i < expectedOrder.size(); ++i) {
			Check(defaults.menus[i].menu == expectedOrder[i], "requested default menu order");
			Check(Navbar::NextMenuIndex(defaults.menus, static_cast<int>(i), [](const auto&) { return true; }) ==
					  static_cast<int>((i + 1) % expectedOrder.size()),
				"next-menu follows config array including wrap");
		}
		const auto commented = Navbar::ConfigData::Parse(R"json(
// Settings before the object.
{
  /* Inline block comment. */
  "fadeInSeconds": 0.1,
  "menus": [
    // The order is intentionally different from defaults.
    {"menu":"https://example.test/a/*literal*/", "showInNavbar":true},
    {"menu":"Quote: \" // still text", "showInNavbar":true}
  ]
} // Comment at EOF.
)json");
		Check(commented.menus[0].menu == "https://example.test/a/*literal*/", "comment markers in strings are literal");
		Check(commented.menus[1].menu == "Quote: \" // still text", "escaped quotes do not start comments");
		Check(commented.fadeInSeconds == 0.1, "decimal seconds work in JSONC");
		Check(Navbar::NextMenuIndex(commented.menus, 0, [](const auto&) { return true; }) == 1, "custom order drives cycling");
		Reject(R"({"menus":[]} /* unfinished)");
		Reject(R"({"menus":[],})");
		Check(defaults.Find("Unknown") == -1, "unknown menu lookup");
		const auto config = Navbar::ConfigData::Parse(R"({"menus":[
            {"menu":"HostOnly","showNavbar":true,"showInNavbar":false},
            {"menu":"TabOnly","showNavbar":false,"showInNavbar":true},
            {"menu":"Both","showNavbar":true,"showInNavbar":true},
            {"menu":"Neither","showNavbar":false,"showInNavbar":false},
            {"menu":"Defaults"}
        ]})");
		Check(config.menus.size() == 5, "custom menu count");
		Check(config.Find("HostOnly") == 0 && config.Find("TabOnly") == 1, "preserve JSON order");
		Check(config.menus[0].showNavbar && !config.menus[0].showInNavbar, "host-only toggle");
		Check(!config.menus[1].showNavbar && config.menus[1].showInNavbar, "tab-only toggle");
		Check(config.menus[2].showNavbar && config.menus[2].showInNavbar, "both toggles");
		Check(!config.menus[3].showNavbar && !config.menus[3].showInNavbar, "neither toggle");
		Check(!config.menus[4].showNavbar && !config.menus[4].showInNavbar, "safe missing toggle defaults");
		Check(Navbar::ConfigData::Parse(R"({"menus":[]})").menus.empty(), "explicit empty configuration");
		Reject("{");
		Reject("[]");
		Reject(R"({"schemaVersion":3,"menus":[]})");
		Reject(R"({"schemaVersion":1.0,"menus":[]})");
		Reject(R"({"schemaVersion":2,"menus":[]})");
		Reject(R"({"menus":{}})");
		Reject(R"({"menus":[null]})");
		Reject(R"({"menus":[{"label":"Missing menu"}]})");
		Reject(R"({"menus":[{"menu":""}]})");
		Reject(R"({"menus":[{"menu":" "}]})");
		Reject(R"({"menus":[{"menu":"M","label":""}]})");
		Reject(R"({"menus":[{"menu":"M\u0000N"}]})");
		Reject(R"({"menus":[{"menu":"M","showNavbar":"true"}]})");
		Reject(R"({"menus":[{"menu":"M","showNavbar":1}]})");
		Reject(R"({"menus":[{"menu":"M","showInNavBar":true}]})");
		Reject(R"({"menues":[],"menus":[]})");
		Reject(R"({"menus":[{"menu":"M"},{"menu":"M"}]})");
		Reject(std::string(65537, ' '));
		Reject(std::string(R"({"menus":[{"menu":")") + std::string(129, 'x') + R"("}]})");
		std::string many = R"({"menus":[)";
		for (int i = 0; i < 33; ++i) {
			if (i) {
				many += ',';
			}
			many += "{\"menu\":\"Menu" + std::to_string(i) + "\"}";
		}
		many += "]}";
		Reject(many);
		Check(argc == 2, "default config path argument missing");
		std::ifstream file(argv[1], std::ios::binary);
		Check(file.good(), "default config file missing");
		const std::string text{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
		const auto        shipped = Navbar::ConfigData::Parse(text);
		Check(shipped.fadeOutSeconds == defaults.fadeOutSeconds && shipped.fadeInSeconds == defaults.fadeInSeconds, "shipped durations match fallback");
		Check(shipped.showButtonHints == defaults.showButtonHints, "shipped hints match fallback");
		Check(shipped.fadeMode == defaults.fadeMode, "shipped presentation matches fallback");
		Check(shipped.navbarScale == defaults.navbarScale, "shipped scale matches fallback");
		Check(shipped.menus.size() == defaults.menus.size(), "shipped default count");
		for (std::size_t i = 0; i < defaults.menus.size(); ++i) {
			Check(shipped.menus[i].menu == defaults.menus[i].menu &&
					  shipped.menus[i].showNavbar == defaults.menus[i].showNavbar &&
					  shipped.menus[i].showInNavbar == defaults.menus[i].showInNavbar,
				"shipped defaults differ from fallback");
		}
		std::cout << checks << " configuration checks passed\n";
		return 0;
	} catch (const std::exception& error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
