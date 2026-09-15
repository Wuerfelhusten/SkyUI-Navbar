#pragma once

namespace Navbar::Navigation
{
	bool          SetKeys(double a_pc, double a_gamepad, unsigned a_priority = 2);
	std::uint32_t Key(RE::INPUT_DEVICE a_device);
	void          Configure(RE::GFxMovieView* a_movie, RE::GFxValue& a_widget);
	void          Refresh(RE::GFxMovieView* a_movie, RE::GFxValue& a_widget);
}
