#pragma once

namespace Navbar::MenuFade
{
	// Opacity is applied only while rendering, never to the host's animation state.
	void Begin(RE::GFxMovieView* a_source, std::string_view a_target);
	void Retarget(std::string_view a_target);
	void TrackTarget(RE::GFxMovieView* a_movie);
	void OnMenuEvent(std::string_view a_nativeName, bool a_opening);
	void OnMovieCreated(RE::GFxMovieView* a_movie);
	void SetCoverAlpha(double a_alpha);
	void End();
}
