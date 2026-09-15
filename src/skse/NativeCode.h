#pragma once
#include "BinaryContracts.h"

namespace Navbar::NativeCode
{
	inline std::span<const std::uint8_t> Read(std::uintptr_t a_address, std::size_t a_length)
	{
		const auto segment = REL::Module::get().segment(REL::Segment::textx);
		if (a_address < segment.address() || a_address - segment.address() > segment.size() ||
			a_length > segment.size() - (a_address - segment.address())) {
			return {};
		}
		return { reinterpret_cast<const std::uint8_t*>(a_address), a_length };
	}
}
