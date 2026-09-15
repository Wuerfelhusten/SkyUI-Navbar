#include "MapFade.h"
#include "MenuController.h"
#include "NativeCode.h"
#include "PendingMapFades.h"
#include "RuntimeThread.h"
#include "TransitionFrames.h"

namespace Navbar::MapFade
{
	namespace
	{
		PendingMapFades   pending;
		thread_local bool mapRequest = false;

		template <std::size_t Index>
		struct RequestHook
		{
			static void thunk(bool a_fadeOut, bool a_black, float a_duration, bool a_hold, float a_minimum)
			{
				const bool previous = mapRequest;
				mapRequest = OnMainThread() && SkipMapTransition(true, Index == 0 ? 1u : 2u,
												   MenuController::GetSingleton()->IsOpeningForSwitch(RE::MapMenu::MENU_NAME),
												   MenuController::GetSingleton()->IsClosingForSwitch(RE::MapMenu::MENU_NAME));
				func(a_fadeOut, a_black, a_duration, a_hold, a_minimum);
				mapRequest = previous;
			}
			inline static REL::Relocation<decltype(thunk)> func;
		};

		struct QueueHook
		{
			static void thunk(RE::FaderData* a_data)
			{
				// The native helper creates/owns the data exactly as before. Tag only
				// the requests from the two verified MapMenu call sites, not other fades.
				if (mapRequest && a_data) {
					pending.Mark(a_data);
				}
				func(a_data);
			}
			inline static REL::Relocation<decltype(thunk)> func;
		};

		struct DeleteHook
		{
			static void* thunk(RE::FaderData* a_this, std::uint32_t a_flags)
			{
				pending.Take(a_this);
				return func(a_this, a_flags);
			}
			inline static REL::Relocation<decltype(thunk)> func;
			static constexpr std::size_t                   idx = 0;
		};

		void Complete(RE::FaderMenu* a_menu)
		{
			if (!a_menu->uiMovie || !a_menu->GetRuntimeData().isActive) {
				return;
			}
			RE::GFxValue clip, duration, minimum, start, end;
			if (!a_menu->uiMovie->GetVariable(&clip, "mc_FaderMenu") || !clip.IsDisplayObject() ||
				!clip.GetMember("fFadeDuration", &duration) || !duration.IsNumber() ||
				!clip.GetMember("fMinNumSeconds", &minimum) || !minimum.IsNumber() ||
				!clip.GetMember("iStartAlpha", &start) || !start.IsNumber() ||
				!clip.GetMember("iEndAlpha", &end) || !end.IsNumber()) {
				SKSE::log::warn("Map fade: unknown FaderMenu contract; retaining native fade");
				return;
			}
			const auto step = MapFadeCompletionStep(duration.GetNumber(), minimum.GetNumber(),
				start.GetNumber(), end.GetNumber());
			if (!step) {
				SKSE::log::warn("Map fade: unsupported endpoint/timing; retaining native fade");
				return;
			}
			// Invoke AFTER native ProcessMessage has retained the completion callback.
			// updateFade clamps to the transparent endpoint and calls native FadeDone,
			// including the normal flag reset, callback dispatch and reference release.
			// Never zero the duration, hide the global menu, or clear isActive ourselves.
			const RE::GFxValue elapsed(*step);
			const bool         invoked = a_menu->uiMovie->Invoke("mc_FaderMenu.updateFade", nullptr, &elapsed, 1);
			if (invoked && !a_menu->GetRuntimeData().isActive) {
				SKSE::log::info("Map native fade completed immediately (native FadeDone retained)");
			} else {
				SKSE::log::warn("Map fade endpoint did not complete; retaining native processing");
			}
		}

		struct MessageHook
		{
			static RE::UI_MESSAGE_RESULTS thunk(RE::FaderMenu* a_this, RE::UIMessage& a_message)
			{
				const bool tagged = pending.Take(a_message.data);
				const auto result = func(a_this, a_message);
				if (tagged && OnMainThread() &&
					result == RE::UI_MESSAGE_RESULTS::kHandled &&
					(a_message.type == RE::UI_MESSAGE_TYPE::kShow || a_message.type == RE::UI_MESSAGE_TYPE::kUpdate)) {
					Complete(a_this);
				}
				return result;
			}
			inline static REL::Relocation<decltype(thunk)> func;
			static constexpr std::size_t                   idx = 4;
		};
	}

	void Install()
	{
		// CommonLib's VariantID selects the vtable for the current runtime.
		// Derive the helper from the real call target, not an AE-only ID or RVA.
		// MapMenu has no own VTABLE member: MapMenu::VTABLE resolves to IMenu.
		const REL::Relocation<std::uintptr_t> map{ RE::VTABLE_MapMenu[0] };
		const auto                            mapMessage = *reinterpret_cast<const std::uintptr_t*>(map.address() + 4 * sizeof(void*));
		const auto                            mapCode = NativeCode::Read(mapMessage, Binary::kMapHideCall + 5);
		const auto                            helper = Binary::CallTarget(mapCode, mapMessage, Binary::kMapShowCall);
		const auto                            hideHelper = Binary::CallTarget(mapCode, mapMessage, Binary::kMapHideCall);
		if (!Binary::MapFadeCalls(mapCode) || !helper || helper != hideHelper) {
			SKSE::log::warn("Map fade skip: native call-site contract differs; preserving native fade");
			return;
		}
		const auto                      helperCode = NativeCode::Read(*helper, Binary::kFadeQueueCall + 5);
		const auto                      queue = Binary::CallTarget(helperCode, *helper, Binary::kFadeQueueCall);
		REL::Relocation<std::uintptr_t> fader{ RE::FaderMenu::VTABLE[0] };
		REL::Relocation<std::uintptr_t> data{ RE::FaderData::VTABLE[0] };
		if (!Binary::FadeQueue(helperCode) || !queue || NativeCode::Read(*queue, 1).empty() ||
			NativeCode::Read(*reinterpret_cast<const std::uintptr_t*>(fader.address() + 4 * sizeof(void*)), 1).empty() ||
			NativeCode::Read(*reinterpret_cast<const std::uintptr_t*>(data.address()), 1).empty()) {
			SKSE::log::warn("Map fade skip: modified helper/vtable contract; preserving native fade");
			return;
		}
		const auto showCall = mapMessage + Binary::kMapShowCall;
		const auto hideCall = mapMessage + Binary::kMapHideCall;
		const auto queueCall = *helper + Binary::kFadeQueueCall;
		auto&      trampoline = SKSE::GetTrampoline();
		DeleteHook::func = data.write_vfunc(DeleteHook::idx, DeleteHook::thunk);
		MessageHook::func = fader.write_vfunc(MessageHook::idx, MessageHook::thunk);
		QueueHook::func = trampoline.write_call<5>(queueCall, QueueHook::thunk);
		RequestHook<0>::func = trampoline.write_call<5>(showCall, RequestHook<0>::thunk);
		RequestHook<1>::func = trampoline.write_call<5>(hideCall, RequestHook<1>::thunk);
		SKSE::log::info("Targeted Map native fade completion installed");
	}
}
