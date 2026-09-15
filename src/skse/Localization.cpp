#include "Localization.h"
#include "TranslationCatalog.h"

namespace Navbar::Localization
{
	namespace
	{
		TranslationCatalog english, localized;

		TranslationCatalog ReadCatalog(const std::string& a_path)
		{
			RE::BSResourceNiBinaryStream file(a_path.c_str());
			if (!file.good()) {
				return {};  // Missing language files deliberately use English.
			}
			wchar_t ch{};
			if (!file.get(ch) || ch != 0xFEFF) {
				throw std::runtime_error("translations must be UTF-16 LE with BOM");
			}
			std::wstring text;
			while (file.get(ch)) {
				if (text.size() >= 131072) {
					throw std::runtime_error("translation file exceeds 256 KiB");
				}
				text.push_back(ch);
			}
			const auto utf8 = SKSE::stl::utf16_to_utf8(text);
			if (!utf8) {
				throw std::runtime_error("invalid UTF-16 translation text");
			}
			return ParseTranslationCatalog(*utf8);
		}

		void LoadCatalog(std::string_view a_language, TranslationCatalog& a_catalog)
		{
			const auto path = std::format("Interface/Translations/SkyUINavbar_{}.txt", a_language);
			try {
				a_catalog = ReadCatalog(path);
				SKSE::log::info("Navbar exact-case translations: {} keys from {}", a_catalog.size(), path);
			} catch (const std::exception& error) {
				SKSE::log::error("Navbar translation file {}: {}", path, error.what());
			}
		}
	}

	void Load()
	{
		// Retain the standard import for interoperability. Our own display values
		// bypass its cached strings and preserve the spelling in the source files.
		SKSE::Translation::ParseTranslation("SkyUINavbar");
		english.clear();
		localized.clear();
		LoadCatalog("english", english);
		const auto        settings = RE::INISettingCollection::GetSingleton();
		const auto        setting = settings ? settings->GetSetting("sLanguage:General") : nullptr;
		const std::string language = setting && setting->GetType() == RE::Setting::Type::kString && setting->data.s ?
		                                 setting->data.s :
		                                 "english";
		LoadCatalog(language, localized);
	}

	std::string Get(std::string_view a_key)
	{
		return ResolveTranslation(a_key, localized, english);
	}
}
