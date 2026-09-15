#pragma once

namespace Navbar
{
	inline bool OnMainThread()
	{
		const auto main = RE::Main::GetSingleton();
		return main && REL::RelocateMember<std::uint32_t>(main, 0x28, 0x20) == GetCurrentThreadId();
	}
}
