#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <set>
#include <utility>

namespace Navbar::Input
{
	// Raw event observation and MenuControls filtering see the same physical input
	// in either order. Each owns its own release bookkeeping; only raw events replay.
	class DeferredCaptureState
	{
	public:
		bool Button(std::uint32_t a_device, std::uint32_t a_id, bool a_down, bool a_pressed, bool a_cycle, bool a_selecting)
		{
			const auto key = std::pair{ a_device, a_id };
			if (!buttons.contains(key) && !a_selecting && !(a_cycle && a_down)) {
				return false;
			}
			if (a_pressed) {
				buttons.insert(key);
			} else {
				buttons.erase(key);
			}
			return true;
		}
		bool Stick(std::size_t a_index, float a_x, float a_y, bool a_selecting)
		{
			if (a_index >= sticks.size()) {
				return a_selecting;
			}
			if (!a_selecting && !sticks[a_index]) {
				return false;
			}
			sticks[a_index] = std::abs(a_x) >= 0.3f || std::abs(a_y) >= 0.3f;
			return true;
		}
		bool Disconnect(bool a_enabled)
		{
			const bool captured = a_enabled || !buttons.empty() || sticks[0] || sticks[1];
			buttons.clear();
			sticks = {};
			return captured;
		}

	private:
		std::set<std::pair<std::uint32_t, std::uint32_t>> buttons;
		std::array<bool, 2>                               sticks{};
	};
}
