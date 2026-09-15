#pragma once
namespace Navbar::FadeOverlay
{
	constexpr auto Name = "SkyUINavbarTransition";
	void           Install();
	void           Prepare(bool a_pauseGame);
	bool           Ready();
	void           SetAlpha(double a_alpha);
}
