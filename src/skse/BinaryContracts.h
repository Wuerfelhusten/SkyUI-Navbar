#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <optional>
#include <span>

namespace Navbar::Binary
{
	inline constexpr std::size_t kMapShowCall = 0x1A6;
	inline constexpr std::size_t kMapHideCall = 0x4F4;
	inline constexpr std::size_t kFadeQueueCall = 0x94;
	inline bool                  Matches(std::span<const std::uint8_t> a_code, std::size_t a_offset,
		std::initializer_list<std::uint8_t> a_expected)
	{
		return a_offset <= a_code.size() && a_expected.size() <= a_code.size() - a_offset &&
		       std::equal(a_expected.begin(), a_expected.end(), a_code.begin() + a_offset);
	}

	inline std::optional<std::uintptr_t> CallTarget(std::span<const std::uint8_t> a_code,
		std::uintptr_t a_address, std::size_t a_offset)
	{
		if (a_offset > a_code.size() || a_code.size() - a_offset < 5 || a_code[a_offset] != 0xE8) {
			return std::nullopt;
		}
		std::int32_t displacement;
		std::memcpy(&displacement, a_code.data() + a_offset + 1, sizeof(displacement));
		return a_address + a_offset + 5 + static_cast<std::intptr_t>(displacement);
	}

	// These are instruction contracts, not executable-version allowlists.
	// Check both observed compiler layouts of Transition::Update, then its
	// interpolation callee before accessing fields missing from CommonLib.
	inline bool MapDirection(std::span<const std::uint8_t> a_code)
	{
		return Matches(a_code, 0x46, { 0x48, 0x8B, 0xD6, 0x48, 0x8B, 0xCB, 0xE8 }) &&
		       (Matches(a_code, 0x65, { 0x8B, 0x83, 0x80, 0, 0, 0, 0x83, 0xF8, 1 }) ||
				   Matches(a_code, 0x76, { 0x8B, 0x83, 0x80, 0, 0, 0, 0x83, 0xF8, 1 }));
	}

	inline bool MapDuration(std::span<const std::uint8_t> a_code)
	{
		return Matches(a_code, 0x55, { 0xF3, 0x0F, 0x10, 0x49, 0x5C }) &&
		       Matches(a_code, 0x67, { 0x41, 0x0F, 0x2E, 0xCE, 0x74, 0x17 }) &&
		       Matches(a_code, 0x84, { 0x41, 0x0F, 0x28, 0xC5, 0xF3, 0x0F, 0x11, 0x41, 0x6C });
	}

	inline bool MapFadeCalls(std::span<const std::uint8_t> a_code)
	{
		return Matches(a_code, kMapShowCall - 13,
				   { 0xF3, 0x0F, 0x11, 0x44, 0x24, 0x20, 0x45, 0x33, 0xC9, 0xB2, 1, 0x33, 0xC9, 0xE8 }) &&
		       Matches(a_code, kMapHideCall - 13,
				   { 0xF3, 0x0F, 0x11, 0x74, 0x24, 0x20, 0x45, 0x33, 0xC9, 0xB2, 1, 0x33, 0xC9, 0xE8 });
	}

	inline bool FadeQueue(std::span<const std::uint8_t> a_code)
	{
		return Matches(a_code, kFadeQueueCall - 8, { 0xF3, 0x0F, 0x11, 0x40, 0x1C, 0x48, 0x8B, 0xC8, 0xE8 });
	}
}
