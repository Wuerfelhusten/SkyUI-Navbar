#include "NavbarResources.h"
#include "NativeCode.h"
#include "NavbarResourcePath.h"

namespace Navbar::Resources
{
	namespace
	{
		// BSScaleformFileOpener::OpenFileEx: opaque file/log pointers, unchanged
		// ownership. Verified vfunc 3 forwards the filename to BSResource on
		// runtimes 1.7.104 (RVA 0x1174150) and 1.6.1170 (RVA 0xFB4490).
		// OpenFile vfunc 1 delegates here on both verified binaries.
		bool installed = false;

		struct OpenFileHook
		{
			static void* thunk(void* a_this, const char* a_path, void* a_log, int a_flags, int a_mode)
			{
				const auto canonical = ResolveNavbarResource(a_path ? a_path : "");
				if (!canonical) {
					return func(a_this, a_path, a_log, a_flags, a_mode);
				}
				const auto file = func(a_this, canonical->c_str(), a_log, a_flags, a_mode);
				SKSE::log::info("Navbar resource '{}' -> '{}': {}", a_path, *canonical, file ? "opened" : "FAILED");
				return file;
			}
			inline static REL::Relocation<decltype(thunk)> func;
			static constexpr std::size_t                   idx = 3;
		};
	}

	bool Installed() { return installed; }

	void Install()
	{
		if (installed) {
			return;
		}
		const auto                      runtime = REL::Module::get().version();
		REL::Relocation<std::uintptr_t> table{ RE::VTABLE_BSScaleformFileOpener[0] };
		const auto                      current = *reinterpret_cast<const std::uintptr_t*>(table.address() + OpenFileHook::idx * sizeof(std::uintptr_t));
		if (NativeCode::Read(current, 1).empty()) {
			SKSE::log::warn("Navbar resource routing skipped: file opener points outside executable code (possibly another plugin)");
			return;
		}
		OpenFileHook::func = table.write_vfunc(OpenFileHook::idx, OpenFileHook::thunk);
		installed = true;
		SKSE::log::info("Navbar private resource namespace installed for runtime {}; unrelated file requests pass through unchanged", runtime.string());
	}
}
