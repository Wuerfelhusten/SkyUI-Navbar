#include "SkillsMenuHooks.h"
#include "Configuration.h"
#include "FadeOverlay.h"
#include "MenuController.h"
#include "SkillsBackground.h"
#include "SkillsMouseBridge.h"
#include "TransitionFrames.h"

#include <cmath>
#include <limits>

namespace Navbar::SkillsMenuHooks
{
	namespace
	{
		using Clock = std::chrono::steady_clock;
		bool                              installed = false;
		std::atomic<Clock::duration::rep> requestedUntil{ 0 };
		Clock::time_point                 requestDeadline;
		bool                              skipActive = false;
		bool                              cameraEndpointPending = false;
		bool                              uiSkipPending = false;
		bool                              closing = false;
		bool                              outroLogged = false;
		std::atomic_bool                  openingRequested{ false };
		bool                              restoreRootPending = false;
		std::atomic<std::uint32_t>        tracedMessages{ 0 };

		void FastForwardIntro(RE::StatsMenu* a_menu)
		{
			if (!skipActive) {
				return;
			}
			if (Clock::now() > requestDeadline) {
				skipActive = false;
				SKSE::log::warn("Skills intro: timed out waiting for the native sequence");
				return;
			}
			auto& data = a_menu->GetRuntimeData();
			if (!data.is3dInitialized || data.menuState != RE::StatsMenu::MenuState::kFadeIn) {
				return;
			}
			auto intro = data.cameraIntro;
			if (!intro || intro->Inactive()) {
				return;
			}
			if (intro->cycleType != RE::NiTimeController::CycleType::kClamp ||
				!std::isfinite(intro->endKeyTime) || !std::isfinite(intro->beginKeyTime) ||
				intro->endKeyTime < intro->beginKeyTime ||
				!std::isfinite(intro->frequency) || intro->frequency <= 0) {
				skipActive = false;
				SKSE::log::warn("Skills intro: nonstandard camera sequence; leaving it unchanged");
				return;
			}
			// The first engine sample initializes lastTime and resets the accumulator.
			if (!std::isfinite(intro->lastTime) || intro->lastTime == -(std::numeric_limits<float>::max)()) {
				return;
			}
			// Sample the endpoint on the next native update. Do not fake lastScaledTime:
			// StatsMenu must evaluate the camera before its normal intro-complete branch runs.
			intro->weightedLastTime = intro->endKeyTime;
			skipActive = false;
			cameraEndpointPending = true;
			SKSE::log::info("Skills intro fast-forward queued: {} -> {} seconds (native camera)",
				intro->lastScaledTime, intro->endKeyTime);
		}

		void FinishOpening(RE::StatsMenu* a_menu)
		{
			if (!cameraEndpointPending && !uiSkipPending) {
				return;
			}
			if (Clock::now() > requestDeadline) {
				SKSE::log::warn("Skills opening completion timed out: camera pending={}, UI pending={}",
					cameraEndpointPending, uiSkipPending);
				cameraEndpointPending = uiSkipPending = false;
				return;
			}
			const auto& data = a_menu->GetRuntimeData();
			if (!data.is3dInitialized || data.menuState != RE::StatsMenu::MenuState::kNormal || !a_menu->uiMovie) {
				return;
			}
			if (cameraEndpointPending && a_menu->fxDelegate) {
				// Intro completion starts another 300ms orientation interpolation. Use
				// the menu's registered MoveCamera endpoint callback, never a fixed pose.
				const auto callback = a_menu->fxDelegate->callbacks.GetAlt("MoveCamera");
				if (callback && callback->callback && callback->handler.get() == a_menu) {
					const RE::GFxValue       progress(1.0);
					const RE::FxDelegateArgs args(RE::GFxValue(0.0), a_menu, a_menu->uiMovie.get(), &progress, 1);
					callback->callback(args);
					cameraEndpointPending = false;
					SKSE::log::info("Skills camera placed at native final position/orientation (MoveCamera=1)");
				}
			}
			if (!uiSkipPending || !data.fadeInTriggered) {
				return;
			}
			RE::GFxValue root, base, total, current;
			if (!a_menu->uiMovie->GetVariable(&root, "_root") || !root.IsDisplayObject() ||
				!root.GetMember("StatsMenuBaseInstance", &base) || !base.IsDisplayObject() ||
				!root.GetMember("_totalframes", &total) || !total.IsNumber() ||
				!root.GetMember("_currentframe", &current) || !current.IsNumber()) {
				return;
			}
			// FFDec-verified vanilla/SkyUI timeline: FadeIn=17, endpoint=31.
			// A different root timeline needs its own adapter, not a guessed final frame.
			if (total.GetNumber() != 31) {
				SKSE::log::warn("Skills UI intro unchanged: unsupported root timeline ({} frames)", total.GetNumber());
				uiSkipPending = false;
				return;
			}
			if (current.GetNumber() < 17 || current.GetNumber() > 31) {
				return;
			}
			const RE::GFxValue lastFrame(31.0);
			if (!root.Invoke("gotoAndStop", nullptr, &lastFrame, 1)) {
				return;
			}
			RE::GFxValue::DisplayInfo display;
			display.SetAlpha(100);
			base.SetDisplayInfo(display);
			uiSkipPending = false;
			SKSE::log::info("Skills UI intro skipped: frame {} -> 31, base alpha=100; final frame retains SetFadedIn callback",
				current.GetNumber());
		}

		void FinishClosing(RE::StatsMenu* a_menu)
		{
			if (!closing) {
				return;
			}
			if (a_menu->uiMovie) {
				RE::GFxValue root;
				if (a_menu->uiMovie->GetVariable(&root, "_root") && root.IsDisplayObject()) {
					RE::GFxValue::DisplayInfo hidden;
					hidden.SetVisible(false);
					root.SetDisplayInfo(hidden);
				}
			}
			auto& data = a_menu->GetRuntimeData();
			if (data.menuState != RE::StatsMenu::MenuState::kFadeOut) {
				return;
			}
			const auto outro = data.cameraOutro;
			if (!outro || outro->Inactive() || outro->cycleType != RE::NiTimeController::CycleType::kClamp ||
				!std::isfinite(outro->endKeyTime) || !std::isfinite(outro->frequency) || outro->frequency <= 0 ||
				!std::isfinite(outro->lastTime) || outro->lastTime == -(std::numeric_limits<float>::max)()) {
				return;
			}
			outro->weightedLastTime = outro->endKeyTime;
			if (!outroLogged) {
				outroLogged = true;
				SKSE::log::info("Skills exit: full UI hidden; native outro endpoint queued ({} seconds)", outro->endKeyTime);
			}
		}

		struct ProcessMessageHook
		{
			static RE::UI_MESSAGE_RESULTS thunk(RE::StatsMenu* a_this, RE::UIMessage& a_message)
			{
				// Map can deliver Show off the main thread. Record it before the guard;
				// consume the lifecycle reset on the next main-thread message.
				if (a_message.type == RE::UI_MESSAGE_TYPE::kShow) {
					openingRequested.store(true);
					SKSE::log::info("Skills Show received on thread {}; main-thread reset pending", GetCurrentThreadId());
				}
				const auto main = RE::Main::GetSingleton();
				// Main omits RUNTIME_DATA in the cross-VR C++ layout, shifting its direct
				// threadID field. Use the verified runtime offset, not that C++ member.
				const auto mainThread = main ? REL::RelocateMember<std::uint32_t>(main, 0x28, 0x20) : 0u;
				const auto messageType = static_cast<std::uint32_t>(*a_message.type);
				const auto traceBit = messageType < 32 ? 1u << messageType : 0u;
				if (traceBit && (tracedMessages.fetch_or(traceBit) & traceBit) == 0) {
					SKSE::log::info("Skills message {}: thread={}, runtime main thread={}",
						messageType, GetCurrentThreadId(), mainThread);
				}
				if (!mainThread || mainThread != GetCurrentThreadId()) {
					return func(a_this, a_message);
				}
				if (openingRequested.exchange(false)) {
					const auto until = requestedUntil.exchange(0);
					requestDeadline = Clock::time_point(Clock::duration(until));
					const bool navbarOpening = until != 0 && Clock::now() <= requestDeadline;
					requestDeadline = Clock::now() + std::chrono::seconds(5);
					const auto ui = RE::UI::GetSingleton();
					// Ordinary Skills opens retain both native intro animations. An
					// expiring request alone is insufficient after a cancelled switch.
					const bool instant = SkipSkillsOpening(true,
						navbarOpening, ui && ui->IsMenuOpen(FadeOverlay::Name));
					skipActive = instant;
					uiSkipPending = instant;
					closing = false;
					outroLogged = false;
					restoreRootPending = true;
					cameraEndpointPending = false;
					MouseBridge::Reset(true);
					SKSE::log::info("Skills opening reset on main thread: camera={}, UI={}, navbar={}", skipActive, uiSkipPending, navbarOpening);
				}
				if (restoreRootPending && !closing) {
					if (a_this->uiMovie) {
						RE::GFxValue root;
						if (a_this->uiMovie->GetVariable(&root, "_root") && root.IsDisplayObject()) {
							RE::GFxValue::DisplayInfo visible;
							visible.SetVisible(true);
							root.SetDisplayInfo(visible);
							restoreRootPending = false;
						}
					}
				}
				if (a_message.type == RE::UI_MESSAGE_TYPE::kHide || a_message.type == RE::UI_MESSAGE_TYPE::kForceHide) {
					skipActive = false;
					cameraEndpointPending = uiSkipPending = false;
					MouseBridge::Reset(false);
					closing = SkipMenuClosing(true,
						MenuController::GetSingleton()->IsClosingForSwitch(RE::StatsMenu::MENU_NAME));
					restoreRootPending = false;
					FinishClosing(a_this);
				}
				if (a_message.type == RE::UI_MESSAGE_TYPE::kScaleformEvent && MouseBridge::Forward(a_this, a_message)) {
					return RE::UI_MESSAGE_RESULTS::kHandled;
				}
				if (a_message.type == RE::UI_MESSAGE_TYPE::kUpdate) {
					FastForwardIntro(a_this);
					FinishClosing(a_this);
				}
				const auto result = func(a_this, a_message);
				FinishClosing(a_this);
				if (a_message.type == RE::UI_MESSAGE_TYPE::kUpdate) {
					FastForwardIntro(a_this);
					FinishOpening(a_this);
				}
				return result;
			}
			inline static REL::Relocation<decltype(thunk)> func;
			static constexpr std::size_t                   idx = 4;
		};
	}

	void Install()
	{
		if (installed) {
			return;
		}
		const auto& config = Configuration::Get();
		SkillsBackground::Install();
		const int                       index = config.Find(RE::StatsMenu::MENU_NAME);
		const bool                      navbarHost = index >= 0 && config.menus[index].showNavbar;
		REL::Relocation<std::uintptr_t> menuVtable{ RE::StatsMenu::VTABLE[0] };
		ProcessMessageHook::func = menuVtable.write_vfunc(ProcessMessageHook::idx, ProcessMessageHook::thunk);
		MouseBridge::Install(navbarHost);
		installed = true;
		SKSE::log::info("Skills native hooks installed: mouse host={}, skip camera={}, skip UI={}",
			navbarHost, true, true);
	}

	bool ReadyForReveal()
	{
		const auto ui = RE::UI::GetSingleton();
		const auto menu = ui ? ui->GetMenu<RE::StatsMenu>() : nullptr;
		if (!menu || !menu->uiMovie) {
			return false;
		}
		if (!installed) {
			return true;
		}
		if (openingRequested.load() || closing || restoreRootPending || skipActive || cameraEndpointPending || uiSkipPending) {
			return false;
		}
		const auto&  data = menu->GetRuntimeData();
		RE::GFxValue visible, baseAlpha;
		return data.is3dInitialized && data.menuState == RE::StatsMenu::MenuState::kNormal &&
		       menu->uiMovie->GetVariable(&visible, "_root._visible") && visible.IsBool() && visible.GetBool() &&
		       menu->uiMovie->GetVariable(&baseAlpha, "_root.StatsMenuBaseInstance._alpha") &&
		       baseAlpha.IsNumber() && baseAlpha.GetNumber() >= 99;
	}

	void PrepareNavbarOpen()
	{
		requestedUntil.store(installed ?
								 (Clock::now() + std::chrono::seconds(5)).time_since_epoch().count() :
								 0);
	}
}
