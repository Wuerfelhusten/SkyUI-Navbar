#pragma once

namespace Navbar
{
	constexpr bool SkipSkillsOpening(bool a_transitionsDisabled, bool a_navbarRequest, bool a_navbarFadeActive)
	{
		return a_transitionsDisabled && a_navbarRequest && a_navbarFadeActive;
	}

	constexpr bool SkipMenuClosing(bool a_transitionsDisabled, bool a_navbarClosing)
	{
		return a_transitionsDisabled && a_navbarClosing;
	}

	constexpr bool SkipMapTransition(bool a_transitionsDisabled, unsigned a_direction, bool a_navbarOpening, bool a_navbarClosing)
	{
		return a_transitionsDisabled && ((a_direction == 1 && a_navbarOpening) || (a_direction == 2 && a_navbarClosing));
	}

	constexpr int ItemTransitionEndpoint(int a_total, int a_current, bool a_navbarClosing)
	{
		if (a_total != 11) {
			return 0;
		}
		if (a_current >= 2 && a_current < 6 && a_navbarClosing) {
			return 6;
		}
		if (a_current >= 7 && a_current <= 11) {
			return 1;
		}
		return 0;
	}
}
