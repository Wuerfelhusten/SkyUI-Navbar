#include "MenuTransitions.h"
#include "MenuController.h"
#include "NativeCode.h"
#include "RuntimeThread.h"
#include "TransitionFrames.h"

namespace Navbar::Transitions
{
	namespace
	{
		void FinishItemFade(RE::IMenu* a_menu, std::string_view a_name)
		{
			if (!a_menu->uiMovie) {
				return;
			}
			RE::GFxValue root, body, total, current;
			if (!a_menu->uiMovie->GetVariable(&root, "_root") || !root.IsDisplayObject() ||
				!root.GetMember("Menu_mc", &body) || !body.IsDisplayObject() ||
				!root.GetMember("_totalframes", &total) || !total.IsNumber() ||
				!root.GetMember("_currentframe", &current) || !current.IsNumber()) {
				return;
			}
			const int endpoint = ItemTransitionEndpoint(static_cast<int>(total.GetNumber()), static_cast<int>(current.GetNumber()),
				MenuController::GetSingleton()->IsClosingForSwitch(a_name));
			if (!endpoint) {
				return;
			}
			// Frame 6 runs onFadeCompletion; frame 1 restores SetFadedIn and input.
			const RE::GFxValue frame(static_cast<double>(endpoint));
			if (root.Invoke("gotoAndStop", nullptr, &frame, 1)) {
				SKSE::log::info("Item-menu transition skipped: {} -> {}", current.GetNumber(), endpoint);
			}
		}

		template <std::size_t Index>
		struct AdvanceHook
		{
			static void thunk(RE::IMenu* a_this, float a_interval, std::uint32_t a_time)
			{
				FinishItemFade(a_this, Index == 0 ? RE::InventoryMenu::MENU_NAME : RE::MagicMenu::MENU_NAME);
				func(a_this, a_interval, a_time);
				FinishItemFade(a_this, Index == 0 ? RE::InventoryMenu::MENU_NAME : RE::MagicMenu::MENU_NAME);
			}
			inline static REL::Relocation<decltype(thunk)> func;
			static constexpr std::size_t                   idx = 5;
		};

		template <std::size_t Index>
		struct MessageHook
		{
			static RE::UI_MESSAGE_RESULTS thunk(RE::IMenu* a_this, RE::UIMessage& a_message)
			{
				const auto result = func(a_this, a_message);
				if (OnMainThread()) {
					FinishItemFade(a_this, Index == 0 ? RE::InventoryMenu::MENU_NAME : RE::MagicMenu::MENU_NAME);
				}
				return result;
			}
			inline static REL::Relocation<decltype(thunk)> func;
			static constexpr std::size_t                   idx = 4;
		};

		struct MapUpdateHook
		{
			static void thunk(RE::TESCameraState* a_this, RE::BSTSmartPointer<RE::TESCameraState>& a_next)
			{
				// 1.7.104: Transition is larger than the current CommonLib declaration.
				// Verified Update reads direction at 0x80 and interpolation duration at 0x5C.
				const auto direction = REL::RelocateMember<std::uint32_t>(a_this, 0x80, 0x80);
				if (!SkipMapTransition(true, direction,
						MenuController::GetSingleton()->IsOpeningForSwitch(RE::MapMenu::MENU_NAME),
						MenuController::GetSingleton()->IsClosingForSwitch(RE::MapMenu::MENU_NAME))) {
					func(a_this, a_next);
					return;
				}
				RE::BSTSmartPointer<RE::TESCameraState> keepAlive(a_this);
				auto&                                   duration = REL::RelocateMember<float>(a_this, 0x5C, 0x5C);
				const float                             saved = duration;
				duration = 0.0f;
				// Zero duration samples the endpoint; native threshold callbacks and state cleanup still run.
				func(a_this, a_next);
				duration = saved;
			}
			inline static REL::Relocation<decltype(thunk)> func;
			static constexpr std::size_t                   idx = 3;
		};
	}

	MapRevealState GetMapRevealState()
	{
		const auto ui = RE::UI::GetSingleton();
		if (!ui) {
			return { false, false };
		}
		const auto map = ui->GetMenu<RE::MapMenu>();
		const auto fader = ui->GetMenu<RE::FaderMenu>();
		const bool fading = fader && fader->GetRuntimeData().isActive;
		if (!map || !ui->IsMenuOpen(RE::MapMenu::MENU_NAME)) {
			return { false, fading };
		}
		const auto runtime = map->GetRuntimeData2();
		if (!runtime) {
			return { false, fading };
		}
		const auto& camera = runtime->camera;
		// 1.7.104 RVA 0x996A00: currentState == worldStates[activeMode].
		// MapMenu::AdvanceMovie uses this camera at +0x304A0. Do not infer readiness
		// from the Transition duration or from the navbar's independently loaded SWF.
		const bool normal = camera.cameraRoot && camera.currentState && camera.unk60 < 2 &&
		                    camera.currentState.get() == camera.unk68[camera.unk60].get();
		// FaderMenu +0x38 is cleared by its native completion callback (RVA 0x92C19E).
		// IsMenuOpen is unsuitable: this is an always-open menu.
		return { normal, fading };
	}

	void Install()
	{
		REL::Relocation<std::uintptr_t> inventory{ RE::InventoryMenu::VTABLE[0] };
		REL::Relocation<std::uintptr_t> magic{ RE::MagicMenu::VTABLE[0] };
		AdvanceHook<0>::func = inventory.write_vfunc(AdvanceHook<0>::idx, AdvanceHook<0>::thunk);
		AdvanceHook<1>::func = magic.write_vfunc(AdvanceHook<1>::idx, AdvanceHook<1>::thunk);
		MessageHook<0>::func = inventory.write_vfunc(MessageHook<0>::idx, MessageHook<0>::thunk);
		MessageHook<1>::func = magic.write_vfunc(MessageHook<1>::idx, MessageHook<1>::thunk);
		SKSE::log::info("Item-menu transition hooks installed");
		REL::Relocation<std::uintptr_t> map{ RE::MapCameraStates::Transition::VTABLE[0] };
		if (!map.address()) {
			SKSE::log::warn("Map camera skip unavailable: CommonLib has no transition vtable address");
			return;
		}
		const auto update = *reinterpret_cast<const std::uintptr_t*>(map.address() + MapUpdateHook::idx * sizeof(void*));
		const auto code = NativeCode::Read(update, 0x80);
		const auto interpolate = Binary::CallTarget(code, update, 0x4C);
		if (!Binary::MapDirection(code) || !interpolate ||
			!Binary::MapDuration(NativeCode::Read(*interpolate, 0x90))) {
			SKSE::log::warn("Map camera skip unavailable: native field-access contract differs; item-menu/input hooks remain enabled");
			return;
		}
		MapUpdateHook::func = map.write_vfunc(MapUpdateHook::idx, MapUpdateHook::thunk);
		SKSE::log::info("Map camera transition hook installed after instruction-contract validation");
	}
}
