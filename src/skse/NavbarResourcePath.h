#pragma once
#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Navbar
{
	inline constexpr auto kNavbarResourceURL = "NavBarForSkyUI/__widget__.swf";

	// A private namespace, not a general Skyrim path rewrite. Only our own
	// widget and the assets loaded relative to it are routed back to Interface.
	inline std::optional<std::string> ResolveNavbarResource(std::string_view a_requested)
	{
		if (a_requested.empty() || a_requested.find(':') != std::string_view::npos ||
			std::ranges::any_of(a_requested, [](unsigned char a_c) { return a_c < 32; })) {
			return std::nullopt;
		}
		std::string path(a_requested);
		std::replace(path.begin(), path.end(), '\\', '/');
		std::string lower(path);
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char a_c) { return static_cast<char>(std::tolower(a_c)); });
		constexpr std::string_view marker = "navbarforskyui/";
		const auto                 found = lower.find(marker);
		if (found == std::string::npos || (found != 0 && path[found - 1] != '/')) {
			return std::nullopt;
		}
		std::vector<std::string> parts;
		auto                     start = found + marker.size();
		if (start == path.size() || path.back() == '/') {
			return std::nullopt;
		}
		while (start < path.size()) {
			const auto end = path.find('/', start);
			const auto part = path.substr(start, end == std::string::npos ? end : end - start);
			if (part == "..") {
				if (parts.empty()) {
					return std::nullopt;
				}
				parts.pop_back();
			} else if (!part.empty() && part != ".") {
				parts.push_back(part);
			}
			if (end == std::string::npos) {
				break;
			}
			start = end + 1;
		}
		if (parts.empty()) {
			return std::nullopt;
		}
		if (parts.size() == 1 && parts[0] == "__widget__.swf") {
			return "Interface/NavBar.swf";
		}
		std::string result = "Interface";
		for (const auto& part : parts) {
			result += "/" + part;
		}
		return result;
	}
}
