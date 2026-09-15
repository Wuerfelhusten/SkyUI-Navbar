#pragma once
#include <cmath>
#include <cstdint>
#include <optional>

namespace Navbar
{
	struct StagePoint
	{
		float x;
		float y;
	};

	inline std::optional<StagePoint> ScreenToStage(float a_x, float a_y,
		float a_viewportLeft, float a_viewportTop, float a_viewportWidth, float a_viewportHeight,
		float a_left, float a_top, float a_right, float a_bottom)
	{
		if (a_viewportWidth <= 0 || a_viewportHeight <= 0 || a_right <= a_left || a_bottom <= a_top) {
			return std::nullopt;
		}
		StagePoint point{ a_left + (a_x - a_viewportLeft) * (a_right - a_left) / a_viewportWidth,
			a_top + (a_y - a_viewportTop) * (a_bottom - a_top) / a_viewportHeight };
		if (!std::isfinite(point.x) || !std::isfinite(point.y)) {
			return std::nullopt;
		}
		return point;
	}

	class MouseCapture
	{
	public:
		enum class Event
		{
			kMove,
			kDown,
			kUp
		};
		bool Route(Event a_event, std::uint32_t a_button, bool a_inside)
		{
			const auto bit = a_button < 32 ? (1u << a_button) : 0u;
			if (a_event == Event::kMove) {
				const bool forward = a_inside || over || buttons != 0;
				over = a_inside;
				return forward;
			}
			if (a_event == Event::kDown && bit) {
				if (a_inside) {
					buttons |= bit;
				}
				return a_inside;
			}
			if (a_event == Event::kUp && bit) {
				const bool captured = (buttons & bit) != 0;
				buttons &= ~bit;
				return captured;
			}
			return false;
		}
		void Reset()
		{
			over = false;
			buttons = 0;
		}

	private:
		bool          over = false;
		std::uint32_t buttons = 0;
	};
}
