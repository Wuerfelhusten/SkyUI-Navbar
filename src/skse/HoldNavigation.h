#pragma once
#include <array>
#include <cmath>
#include <cstdint>

namespace Navbar
{
	constexpr bool SelectionConfirmKey(bool a_gamepad, std::uint32_t a_key)
	{
		return a_gamepad ? a_key == 0x1000 : a_key == 0x12 || a_key == 0x1C || a_key == 0x9C;  // A / E, Enter, keypad Enter.
	}

	class HoldNavigation
	{
	public:
		enum class ReleaseAction
		{
			kNone,
			kNext
		};
		void Begin(std::int64_t a_now)
		{
			held = true;
			cancelled = toggled = false;
			start = a_now;
		}
		bool Tick(std::int64_t a_now)
		{
			if (!held || cancelled || toggled || a_now - start < 350) {
				return false;
			}
			selecting = !selecting;
			toggled = true;
			return true;
		}
		ReleaseAction Release(std::int64_t a_now)
		{
			if (!held) {
				return ReleaseAction::kNone;
			}
			Tick(a_now);  // A missing timer tick must never turn a long hold into Next.
			// A short press dismisses the latched picker. Only normal view cycles.
			const auto action = cancelled || toggled || selecting ? ReleaseAction::kNone : ReleaseAction::kNext;
			if (!cancelled && !toggled && selecting) {
				selecting = false;
			}
			held = false;
			return action;
		}
		void Cancel()
		{
			cancelled = true;
			selecting = false;
		}
		bool Held() const { return held; }
		bool Selecting() const { return selecting; }

	private:
		std::int64_t start = 0;
		bool         held = false, cancelled = false, selecting = false, toggled = false;
	};

	class SelectionRepeat
	{
	public:
		int Step(int a_direction, std::int64_t a_now)
		{
			if (!a_direction) {
				direction = 0;
				return 0;
			}
			if (a_direction != direction) {
				direction = a_direction;
				next = a_now + 350;
				return a_direction;
			}
			if (a_now < next) {
				return 0;
			}
			next = a_now + 120;  // No catch-up burst after a slow frame.
			return a_direction;
		}

	private:
		int          direction = 0;
		std::int64_t next = 0;
	};

	class SelectionButtons
	{
	public:
		void Route(bool a_gamepad, std::uint32_t a_key, bool a_down, bool a_up)
		{
			// W/A/Up/Left = previous; S/D/Down/Right = next. D-pad mirrors this.
			constexpr std::array<std::uint32_t, 8> keyboard{ 0x11, 0x1E, 0xC8, 0xCB, 0x1F, 0x20, 0xD0, 0xCD };
			constexpr std::array<std::uint32_t, 4> dpad{ 0x1, 0x4, 0x2, 0x8 };
			const auto                             count = a_gamepad ? dpad.size() : keyboard.size();
			for (std::size_t i = 0; i < count; ++i) {
				if (a_key != (a_gamepad ? dpad[i] : keyboard[i])) {
					continue;
				}
				const auto bit = 1u << (i + (a_gamepad ? 8 : 0));
				if (a_up) {
					held &= ~bit;
				} else if (a_down) {
					held |= bit;
				}
				return;
			}
		}
		bool Held() const { return held != 0; }
		int  Direction() const { return static_cast<int>((held & 0xCF0) != 0) - static_cast<int>((held & 0x30F) != 0); }

	private:
		std::uint32_t held = 0;
	};

	class SelectionAxis
	{
	public:
		void Begin()
		{
			armed = neutral;
			direction = 0;
		}
		void Cancel()
		{
			direction = 0;
			armed = false;
		}
		bool Route(float a_x, float a_y, bool a_selecting)
		{
			neutral = std::isfinite(a_x) && std::isfinite(a_y) && std::abs(a_x) < 0.3f && std::abs(a_y) < 0.3f;
			const bool consume = a_selecting || draining;
			if (a_selecting && neutral) {
				armed = true;
			}
			direction = a_selecting && armed && std::isfinite(a_y) && std::abs(a_y) >= 0.55f ? (a_y > 0 ? -1 : 1) : 0;
			if (consume) {
				draining = !neutral;
			}
			return consume;
		}
		int Direction() const { return direction; }

	private:
		bool neutral = true, armed = false, draining = false;
		int  direction = 0;
	};

	template <class Entries, class Available>
	int SelectionMenuIndex(const Entries& a_entries, int a_from, int a_direction, Available a_available)
	{
		const int count = static_cast<int>(a_entries.size());
		if (!count || a_from < 0 || a_from >= count || (a_direction != -1 && a_direction != 1)) {
			return a_from;
		}
		for (int step = 1; step < count; ++step) {
			const int index = (a_from + a_direction * step + count) % count;
			if (a_entries[index].showInNavbar && a_available(a_entries[index].menu)) {
				return index;
			}
		}
		return a_from;
	}
}
