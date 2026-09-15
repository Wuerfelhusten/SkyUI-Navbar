#include "BinaryContracts.h"
#include <array>
#include <stdexcept>
#include <vector>

namespace
{
	void Require(bool a_condition)
	{
		if (!a_condition) {
			throw std::runtime_error("Native instruction contract regression");
		}
	}
	void Put(std::vector<std::uint8_t>& a_code, std::size_t a_offset, std::initializer_list<std::uint8_t> a_bytes)
	{
		std::copy(a_bytes.begin(), a_bytes.end(), a_code.begin() + a_offset);
	}
}

int main()
{
	using namespace Navbar::Binary;
	Require(!MapDirection({}) && !MapDuration({}) && !MapFadeCalls({}) && !FadeQueue({}));
	Require(!CallTarget({}, 0x1000, 0));
	const std::array<std::uint8_t, 5> backward{ 0xE8, 0xF6, 0xFF, 0xFF, 0xFF };
	Require(CallTarget(backward, 0x1000, 0) == 0xFFB);
	Require(!CallTarget(backward, 0x1000, 1));
	Require(!Matches(backward, 10, { 0xE8 }));

	// Both locally inspected Update instruction layouts, independent of version strings.
	for (const auto direction : { 0x65u, 0x76u }) {
		std::vector<std::uint8_t> update(0x80);
		Put(update, 0x46, { 0x48, 0x8B, 0xD6, 0x48, 0x8B, 0xCB, 0xE8 });
		Put(update, direction, { 0x8B, 0x83, 0x80, 0, 0, 0, 0x83, 0xF8, 1 });
		Require(MapDirection(update));
		update[direction + 2] = 0x88;
		Require(!MapDirection(update));
	}
	std::vector<std::uint8_t> duration(0x90);
	Put(duration, 0x55, { 0xF3, 0x0F, 0x10, 0x49, 0x5C });
	Put(duration, 0x67, { 0x41, 0x0F, 0x2E, 0xCE, 0x74, 0x17 });
	Put(duration, 0x84, { 0x41, 0x0F, 0x28, 0xC5, 0xF3, 0x0F, 0x11, 0x41, 0x6C });
	Require(MapDuration(duration));
	duration[0x59] = 0x60;
	Require(!MapDuration(duration));

	std::vector<std::uint8_t> map(kMapHideCall + 5);
	Put(map, kMapShowCall - 13, { 0xF3, 0x0F, 0x11, 0x44, 0x24, 0x20, 0x45, 0x33, 0xC9, 0xB2, 1, 0x33, 0xC9, 0xE8 });
	Put(map, kMapHideCall - 13, { 0xF3, 0x0F, 0x11, 0x74, 0x24, 0x20, 0x45, 0x33, 0xC9, 0xB2, 1, 0x33, 0xC9, 0xE8 });
	Require(MapFadeCalls(map));
	// Call displacement changes with relocation, without changing the contract.
	map[kMapShowCall + 1] = 0x7F;
	Require(MapFadeCalls(map));
	map[kMapHideCall] = 0xE9;
	Require(!MapFadeCalls(map));

	std::vector<std::uint8_t> queue(kFadeQueueCall + 5);
	Put(queue, kFadeQueueCall - 8, { 0xF3, 0x0F, 0x11, 0x40, 0x1C, 0x48, 0x8B, 0xC8, 0xE8 });
	Require(FadeQueue(queue));
	queue[kFadeQueueCall - 4] = 0x20;
	Require(!FadeQueue(queue));
}
