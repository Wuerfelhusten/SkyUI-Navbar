#include "NavbarInputHost.h"
#include "Configuration.h"
#include "MenuAdapters.h"

namespace Navbar::Input
{
	Host TopHost(RE::UI* a_ui)
	{
		if (!a_ui) {
			return {};
		}
		const auto& config = Configuration::Get();
		for (auto i = a_ui->menuStack.size(); i > 0; --i) {
			const auto menu = a_ui->menuStack[i - 1];
			if (!menu) {
				continue;
			}
			for (const auto& entry : config.menus) {
				if (entry.showNavbar && Menus::Matches(entry.menu, menu.get())) {
					return { true, entry.menu, menu->uiMovie.get() };
				}
			}
			if (BlocksNavigation(false, menu->Modal(), menu->UsesCursor(), menu->UsesMenuContext())) {
				return {};
			}
		}
		return {};
	}

}
