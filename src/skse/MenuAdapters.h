#pragma once
#include "MenuRoute.h"

namespace Navbar::Menus
{
	bool                       Available(std::string_view a_name);
	bool                       IsOpen(std::string_view a_name);
	RE::GPtr<RE::GFxMovieView> Movie(std::string_view a_name);
	bool                       Matches(std::string_view a_name, RE::IMenu* a_menu);
	bool                       CanLeave(std::string_view a_name);
	bool                       ReadyForReveal(std::string_view a_name);
	bool                       HasBlocker(std::string_view a_host, bool a_duringSwitch = false);
	bool                       Open(std::string_view a_name);
	bool                       Close(std::string_view a_name);
	void                       OnOpened(std::string_view a_name);
	void                       Reset();
}
