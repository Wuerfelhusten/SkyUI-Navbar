#include "MenuController.h"
#include "Configuration.h"
#include "CycleNavigation.h"
#include "FadeOverlay.h"
#include "MenuAdapters.h"
#include "MenuFade.h"
#include "NavbarWidget.h"
#include "RuntimeThread.h"

namespace Navbar
{
	bool MenuController::IsOpeningForSwitch(std::string_view a_menu) const
	{
		return OnMainThread() && transitioning && fade.State() == SwitchFade::Phase::kOpening &&
		       pendingTarget == a_menu && Clock::now() < transitionDeadline;
	}

	bool MenuController::IsClosingForSwitch(std::string_view a_menu) const
	{
		return OnMainThread() && transitioning && fade.State() == SwitchFade::Phase::kClosing &&
		       pendingSource == a_menu && Clock::now() < transitionDeadline;
	}

	bool MenuController::RequestNext(std::string_view a_source, std::uint64_t a_session)
	{
		const auto& config = Configuration::Get();
		if (a_session && a_session != cycle.Session()) {
			return false;
		}
		if (transitioning || cycleContinue || (a_session && CycleInputSession())) {
			if (!CycleInputSession()) {
				return false;
			}
			const int next = NextMenuIndex(config.menus, cycle.Target(), IsRegistered);
			if (next < 0) {
				return false;
			}
			cycle.SetTarget(next);
			if (!transitioning) {
				cycleContinue = true;
			}
			SKSE::log::info("Navbar queued next destination: {}", config.menus[next].menu);
			return true;
		}
		const int  next = NextMenuIndex(config.menus, config.Find(a_source), IsRegistered);
		const bool accepted = next >= 0 && StartSwitch(a_source, config.menus[next].menu, a_session != 0);
		if (accepted) {
			if (a_session) {
				cycle.SetTarget(next);
			}
			Widget::BeginNavigation(Menus::Movie(a_source).get());
		}
		return accepted;
	}

	bool MenuController::RequestSwitch(std::string_view a_source, std::string_view a_target)
	{
		return StartSwitch(a_source, a_target, false);
	}

	bool MenuController::StartSwitch(std::string_view a_source, std::string_view a_target, bool a_continuation)
	{
		const auto& config = Configuration::Get();
		const int   sourceIndex = config.Find(a_source);
		const int   targetIndex = config.Find(a_target);
		if (sourceIndex < 0 || targetIndex < 0 || a_source == a_target ||
			!config.menus[sourceIndex].showNavbar || !config.menus[targetIndex].showInNavbar) {
			return false;
		}
		auto ui = RE::UI::GetSingleton();
		if (!ui || !Menus::CanLeave(a_source) || Menus::HasBlocker(a_source) || transitioning) {
			return false;
		}
		if (!IsRegistered(a_target)) {
			SKSE::log::warn("Cannot open unregistered menu {}", a_target);
			return false;
		}
		transitioning = true;
		if (!a_continuation) {
			cycle.Begin(targetIndex);
		}
		cycleContinue = false;
		pendingSource = a_source;
		pendingTarget = a_target;
		transitionDeadline = Clock::now() + SwitchFade::WatchdogDuration(config.fadeOutSeconds, config.fadeInSeconds);
		watching.store(true);
		SKSE::GetTaskInterface()->AddTask([this, sourceIndex, targetIndex]() {
			const auto& menus = Configuration::Get().menus;
			auto        queue = RE::UIMessageQueue::GetSingleton();
			auto        ui = RE::UI::GetSingleton();
			const auto& source = menus[sourceIndex].menu;
			const auto& target = menus[targetIndex].menu;
			if (!queue || !ui || !Menus::CanLeave(source) || Menus::HasBlocker(source) || !IsRegistered(target)) {
				transitioning = false;
				CancelQueuedCycles();
				pendingSource.clear();
				pendingTarget.clear();
				animationHandoff.Clear();
				return;
			}
			SKSE::log::info("Switching {} -> {}", source, target);
			Widget::BeginNavigation(Menus::Movie(source).get());
			animationHandoff.Clear();
			if (menus[targetIndex].showNavbar) {
				const auto movie = Menus::Movie(source);
				animationHandoff.Store(target, Widget::CaptureAnimationState(movie.get()), Clock::now(),
					std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(5 + Configuration::Get().fadeOutSeconds)));
			}
			pendingSource = source;
			pendingTarget = target;
			mapReadinessLog = -1;
			fadeWaiting = true;
			presentation = PresentationFor(Configuration::Get().fadeMode, target);
			MenuFade::End();
			if (presentation.menu) {
				MenuFade::Begin(Menus::Movie(source).get(), target);
			}
			SKSE::log::info("Navbar fade presentation: menu={}, fullscreen={}", presentation.menu, presentation.fullscreen);
			FadeOverlay::Prepare(!NeedsUnpausedOpen(source) && !NeedsUnpausedOpen(target));
			queue->AddMessage(FadeOverlay::Name, RE::UI_MESSAGE_TYPE::kShow, nullptr);
		});
		return true;
	}

	std::vector<TabAnimation> MenuController::TakeAnimationState(std::string_view a_menu, RE::GFxMovieView* a_movie)
	{
		const auto ui = RE::UI::GetSingleton();
		if (!ui || !Menus::IsOpen(a_menu) || Menus::Movie(a_menu).get() != a_movie) {
			return {};
		}
		return animationHandoff.Take(a_menu, Clock::now());
	}
}
