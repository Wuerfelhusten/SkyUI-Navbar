#pragma once
#include <string>
#include <string_view>

namespace Navbar::Localization
{
	void        Load();
	std::string Get(std::string_view a_key);
}
