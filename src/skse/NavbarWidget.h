#pragma once
#include "AnimationState.h"

namespace Navbar::Widget
{
	void                      UpdateLayout(RE::GFxMovieView* a_movie);
	void                      BeginNavigation(RE::GFxMovieView* a_movie);
	bool                      SetPicker(RE::GFxMovieView* a_movie, bool a_active, const std::string& a_selected);
	[[nodiscard]] bool        HitTest(RE::GFxMovieView* a_movie, float a_screenX, float a_screenY);
	void                      DispatchButton(RE::GFxMovieView* a_movie, std::uint32_t a_button, bool a_down, float a_screenX, float a_screenY);
	std::vector<TabAnimation> CaptureAnimationState(RE::GFxMovieView* a_movie);
}
