#pragma once
#include <cstddef>

namespace Navbar
{
	template <class Entries, class Available>
	int NextMenuIndex(const Entries& a_entries, int a_source, Available a_available)
	{
		if (a_source < 0 || a_source >= static_cast<int>(a_entries.size())) {
			return -1;
		}
		for (std::size_t step = 1; step < a_entries.size(); ++step) {
			const int index = static_cast<int>((a_source + step) % a_entries.size());
			if (a_entries[index].showInNavbar && a_available(a_entries[index].menu)) {
				return index;
			}
		}
		return -1;
	}

}
