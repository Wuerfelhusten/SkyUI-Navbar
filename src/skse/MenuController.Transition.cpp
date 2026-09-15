#include "MenuController.h"
#include "Configuration.h"
#include "FadeOverlay.h"
#include "MenuAdapters.h"
#include "MenuFade.h"
#include "MenuTransitions.h"
#include "NavbarLoader.h"
#include "SkillsMenuHooks.h"

namespace Navbar
{
	void MenuController::CheckTransitionTimeout(Clock::time_point a_now)
	{
		if (transitioning && a_now >= transitionDeadline) {
			MenuFade::End();
			RestoreSource();
			CancelQueuedCycles();
			transitioning = false;
			animationHandoff.Clear();
			pendingSource.clear();
			pendingTarget.clear();
			fadeWaiting = false;
			fade = {};
			if (const auto queue = RE::UIMessageQueue::GetSingleton()) {
				queue->AddMessage(FadeOverlay::Name, RE::UI_MESSAGE_TYPE::kHide, nullptr);
			}
			SKSE::log::warn("Menu transition timed out; navigation unlocked");
		}
	}

	bool MenuController::TickTransition(Clock::time_point a_now)
	{
		const auto& menus = Configuration::Get().menus;
		RetargetCycle();
		if (fadeWaiting && FadeOverlay::Ready()) {
			fadeWaiting = false;
			fade.Start(a_now, Configuration::Get().fadeOutSeconds, Configuration::Get().fadeInSeconds);
		}
		if (fade.Active()) {
			const auto queue = RE::UIMessageQueue::GetSingleton();
			bool       targetReady = false;
			if (Menus::IsOpen(pendingTarget)) {
				const auto movie = Menus::Movie(pendingTarget);
				if (presentation.menu) {
					MenuFade::TrackTarget(movie.get());
				}
				RE::GFxValue initialized;
				const int    index = Configuration::Get().Find(pendingTarget);
				targetReady = movie && index >= 0 && (!menus[index].showNavbar || (movie->GetVariable(&initialized, "_root.NavBarForSkyUI.content.navbar.initialized") && initialized.IsBool() && initialized.GetBool()));
				targetReady = targetReady && Menus::ReadyForReveal(pendingTarget);
				if (!targetReady && Loader::Failed(movie.get()) && fade.State() == SwitchFade::Phase::kOpening) {
					fade.Cancel(a_now);
					CancelQueuedCycles();
					animationHandoff.Clear();
					SKSE::log::warn("Navbar failed in {}; revealing the already open owner menu without waiting for the readiness timeout", pendingTarget);
				}
				if (pendingTarget == RE::StatsMenu::MENU_NAME) {
					targetReady = targetReady && SkillsMenuHooks::ReadyForReveal();
				}
			}
			if (pendingTarget == RE::MapMenu::MENU_NAME && fade.State() == SwitchFade::Phase::kOpening) {
				const auto native = Transitions::GetMapRevealState();
				const int  state = (targetReady ? 1 : 0) | (native.cameraReady ? 2 : 0) | (native.faderActive ? 4 : 0);
				if (state != mapReadinessLog) {
					SKSE::log::info("Map reveal readiness: UI={}, camera={}, nativeFade={}",
						targetReady, native.cameraReady, native.faderActive);
					mapReadinessLog = state;
				}
				// Native completion is authoritative; no Map-only black hold before the shared fade-in.
				targetReady = native.Ready(targetReady);
				if (targetReady) {
					SKSE::log::info("Map native presentation ready; starting configured fade-in");
				}
			}
			auto action = fade.Tick(a_now, !Menus::IsOpen(pendingSource), targetReady);
			FadeOverlay::SetAlpha(presentation.fullscreen ? fade.Alpha() : 0.0);
			if (presentation.menu) {
				MenuFade::SetCoverAlpha(fade.Alpha());
			}
			if (queue && (action == SwitchFade::Action::kHideSource || action == SwitchFade::Action::kShowTarget)) {
				if (Menus::HasBlocker(pendingSource, true) || !IsRegistered(pendingTarget)) {
					fade.Cancel(a_now);
					CancelQueuedCycles();
					animationHandoff.Clear();
					RestoreSource();
				} else if (action == SwitchFade::Action::kHideSource) {
					if (!Menus::Close(pendingSource)) {
						fade.Cancel(a_now);
						CancelQueuedCycles();
						animationHandoff.Clear();
					}
					RE::TweenMenu::CloseTweenMenu();
					SKSE::log::info("Navbar fade-out complete; closing {}", pendingSource);
				} else {
					if (pendingTarget == RE::StatsMenu::MENU_NAME) {
						SkillsMenuHooks::PrepareNavbarOpen();
					}
					RE::TweenMenu::CloseTweenMenu();
					if (!Menus::Open(pendingTarget)) {
						fade.Cancel(a_now);
						CancelQueuedCycles();
						animationHandoff.Clear();
						RestoreSource();
						SKSE::log::warn("Navbar adapter rejected opening {}", pendingTarget);
					} else {
						SKSE::log::info("Navbar fade: source closed; opening {}", pendingTarget);
					}
				}
			}
			if (action == SwitchFade::Action::kAbort) {
				CancelQueuedCycles();
				animationHandoff.Clear();
				RestoreSource();
				SKSE::log::warn("Navbar fade readiness timeout; revealing available scene");
			}
			if (action == SwitchFade::Action::kFinished) {
				MenuFade::End();
				if (queue) {
					queue->AddMessage(FadeOverlay::Name, RE::UI_MESSAGE_TYPE::kHide, nullptr);
				}
				transitioning = false;
				cycleContinue = cycle.Pending(Configuration::Get().Find(pendingTarget));
				pendingSource.clear();
				animationHandoff.Clear();
				SKSE::log::info("Navbar fade complete");
			}
			return true;
		}
		return false;
	}

	void MenuController::RestoreSource()
	{
		if (pendingSource.empty() || Menus::IsOpen(pendingSource) || Menus::IsOpen(pendingTarget) ||
			Menus::HasBlocker({}, true)) {
			return;
		}
		RE::TweenMenu::CloseTweenMenu();
		if (pendingSource == RE::StatsMenu::MENU_NAME) {
			SkillsMenuHooks::PrepareNavbarOpen();
		}
		SKSE::log::warn("Navbar failed destination; source {} restore request: {}", pendingSource,
			Menus::Open(pendingSource) ? "accepted" : "rejected");
	}
}
