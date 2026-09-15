#pragma once

namespace Navbar::Input
{
	struct Host
	{
		bool              owned = false;
		std::string       menu;
		RE::GFxMovieView* movie = nullptr;  // Identity only, revalidated before UI dispatch.
	};
	Host TopHost(RE::UI* a_ui);
}
