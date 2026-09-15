#pragma once
#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Navbar
{
	// MovieClipLoader resolves against the host movie directory. Derive the
	// route back to Interface instead of assuming every menu SWF lives there.
	inline std::optional<std::string> NavbarMoviePath(std::string_view a_movieURL)
	{
		if (a_movieURL.empty()) {
			return std::nullopt;
		}
		std::string path(a_movieURL.substr(0, a_movieURL.find_first_of("?#")));
		if (path.empty()) {
			return std::nullopt;
		}
		std::replace(path.begin(), path.end(), '\\', '/');
		if (path.find("://") != std::string::npos && !path.starts_with("file://")) {
			return std::nullopt;
		}
		std::vector<std::string> parts;
		std::size_t              start = 0;
		while (start < path.size()) {
			const auto end = path.find('/', start);
			parts.push_back(path.substr(start, end == std::string::npos ? end : end - start));
			if (end == std::string::npos) {
				break;
			}
			start = end + 1;
		}
		if (parts.empty() || path.back() == '/') {
			return std::nullopt;
		}
		std::size_t first = 0;
		bool        interfaceRoot = false;
		for (std::size_t i = 0; i + 1 < parts.size(); ++i) {
			auto part = parts[i];
			std::transform(part.begin(), part.end(), part.begin(), [](unsigned char a_c) { return static_cast<char>(std::tolower(a_c)); });
			if (part == "interface") {
				first = i + 1;
				interfaceRoot = true;
			}
		}
		if (!interfaceRoot && (path.front() == '/' || path.find(':') != std::string::npos)) {
			return std::nullopt;
		}
		std::size_t depth = 0;
		for (auto i = first; i + 1 < parts.size(); ++i) {
			if (parts[i].empty() || parts[i] == ".") {
				continue;
			}
			if (parts[i] == "..") {
				if (depth == 0) {
					return std::nullopt;
				}
				--depth;
			} else {
				++depth;
			}
		}
		std::string relative;
		for (std::size_t i = 0; i < depth; ++i) {
			relative += "../";
		}
		return relative + "NavBar.swf";
	}
}
