#pragma once
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Navbar
{
	using TranslationCatalog = std::unordered_map<std::string, std::string>;

	inline std::string ResolveTranslation(std::string_view a_key, const TranslationCatalog& a_localized, const TranslationCatalog& a_english)
	{
		const std::string key(a_key);
		if (const auto found = a_localized.find(key); found != a_localized.end()) {
			return found->second;
		}
		if (const auto found = a_english.find(key); found != a_english.end()) {
			return found->second;
		}
		return key;
	}

	inline TranslationCatalog ParseTranslationCatalog(std::string_view a_text)
	{
		TranslationCatalog result;
		while (!a_text.empty()) {
			const auto end = a_text.find('\n');
			auto       line = a_text.substr(0, end);
			a_text = end == std::string_view::npos ? std::string_view{} : a_text.substr(end + 1);
			if (line.ends_with('\r')) {
				line.remove_suffix(1);
			}
			if (line.empty() || !line.starts_with('$')) {
				continue;
			}
			const auto tab = line.find('\t');
			if (tab == std::string_view::npos || tab < 2 || tab + 1 == line.size() || line.find('\t', tab + 1) != std::string_view::npos) {
				throw std::runtime_error("invalid translation key/value line");
			}
			if (!result.emplace(line.substr(0, tab), line.substr(tab + 1)).second) {
				throw std::runtime_error("duplicate translation key");
			}
		}
		return result;
	}
}
