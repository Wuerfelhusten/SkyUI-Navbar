#include "Configuration.h"
#include <fstream>

namespace Navbar::Configuration
{
	namespace
	{
		ConfigData     config = ConfigData::Defaults();
		constexpr auto path = "Data/SKSE/Plugins/SkyUINavbar.json";
	}

	void Load()
	{
		try {
			std::ifstream file(path, std::ios::binary);
			if (!file) {
				SKSE::log::warn("{} not readable; using default navbar configuration", path);
				return;
			}
			std::string text(65537, '\0');
			file.read(text.data(), static_cast<std::streamsize>(text.size()));
			text.resize(static_cast<std::size_t>(file.gcount()));
			config = ConfigData::Parse(text);
			SKSE::log::info("Loaded {} menu definitions from {}", config.menus.size(), path);
			SKSE::log::info("Navbar sidebar scale: {}", config.navbarScale);
			SKSE::log::info("Navbar fade seconds: out={}, in={}", config.fadeOutSeconds, config.fadeInSeconds);
			SKSE::log::info("Navbar fade mode: {}", config.fadeMode);
		} catch (const std::exception& error) {
			SKSE::log::error("Invalid {}: {}; using default navbar configuration", path, error.what());
		}
	}

	const ConfigData& Get() { return config; }
}
