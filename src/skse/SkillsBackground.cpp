#include "SkillsBackground.h"
#include "NativeCode.h"
#include "RuntimeThread.h"

namespace Navbar::SkillsBackground
{
	namespace
	{
		using FadeOutGame = void (*)(bool, bool, float, bool, float);
		FadeOutGame fadeOutGame = nullptr;
	}

	void Install()
	{
		// Resolve before MapFade replaces these calls. CommonLib selects the
		// runtime vtable; the two verified calls must name the same native helper.
		// MapMenu has no own VTABLE member: MapMenu::VTABLE resolves to IMenu.
		const REL::Relocation<std::uintptr_t> map{ RE::VTABLE_MapMenu[0] };
		const auto                            message = *reinterpret_cast<const std::uintptr_t*>(map.address() + 4 * sizeof(void*));
		const auto                            code = NativeCode::Read(message, Binary::kMapHideCall + 5);
		const auto                            helper = Binary::CallTarget(code, message, Binary::kMapShowCall);
		if (!Binary::MapFadeCalls(code) || !helper ||
			helper != Binary::CallTarget(code, message, Binary::kMapHideCall) ||
			!Binary::FadeQueue(NativeCode::Read(*helper, Binary::kFadeQueueCall + 5))) {
			SKSE::log::warn("Skills background: native fade helper contract differs; leaving it untouched");
			return;
		}
		fadeOutGame = reinterpret_cast<FadeOutGame>(*helper);
	}

	void Prepare()
	{
		if (!fadeOutGame || !OnMainThread()) {
			return;
		}
		// Match the native Skills opening setup (also used by Custom Skills).
		// FaderMenu sits behind StatsMenu's 3D scene. Our own cover sits above it.
		// Keep a positive duration: zero breaks the stock AS2 interpolation.
		// Native StatsMenu Hide reverses this fade; do not clear the global fader.
		fadeOutGame(true, true, 0.001f, true, 0.0f);
		SKSE::log::info("Navbar Skills opening: native black scene background requested");
	}
}
