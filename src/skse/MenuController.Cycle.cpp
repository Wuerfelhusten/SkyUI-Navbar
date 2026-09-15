#include "MenuController.h"
#include "Configuration.h"
#include "FadeOverlay.h"
#include "MenuAdapters.h"
#include "MenuFade.h"

namespace Navbar
{
	std::uint64_t MenuController::CycleInputSession() const
	{
		const auto ui = RE::UI::GetSingleton();
		const bool cover = ui && ui->IsMenuOpen(FadeOverlay::Name);
		if ((!transitioning && !cycleContinue && !cover) || !cycle.Session() || Clock::now() >= transitionDeadline) {
			return 0;
		}
		const auto controls = RE::ControlMap::GetSingleton();
		const auto owner = Menus::IsOpen(pendingSource) ? pendingSource : pendingTarget;
		return ui && !(controls && controls->GetRuntimeData().textEntryCount > 0) && !Menus::HasBlocker(owner, true) ? cycle.Session() : 0;
	}

	void MenuController::RetargetCycle()
	{
		// Once Show has been sent, the owner's opening protocol must finish.
		// A paused cover also cannot be reused for an API requiring unpaused play.
		if (!transitioning || !cycle.Pending(Configuration::Get().Find(pendingTarget))) {
			return;
		}
		const auto& target = Configuration::Get().menus[cycle.Target()].menu;
		const auto  ui = RE::UI::GetSingleton();
		const auto  overlay = ui ? ui->GetMenu(FadeOverlay::Name) : nullptr;
		if (!overlay || !CanRetargetCycle(fade.State(), fadeWaiting, overlay->PausesGame(), NeedsUnpausedOpen(target)) || !IsRegistered(target)) {
			return;
		}
		SKSE::log::info("Navbar skips unopened destination {} -> {}", pendingTarget, target);
		pendingTarget = target;
		// Once a scene-changing route needs the screen cover, retain it through
		// this switch even if rapid taps retarget again; never drop a partial cover.
		presentation.fullscreen = presentation.fullscreen || PresentationFor(Configuration::Get().fadeMode, target).fullscreen;
		MenuFade::Retarget(target);
		animationHandoff.Retarget(target);
	}

	bool MenuController::ContinueCycle()
	{
		if (!cycleContinue || transitioning) {
			return false;
		}
		const auto ui = RE::UI::GetSingleton();
		if (ui && ui->IsMenuOpen(FadeOverlay::Name)) {
			return true;
		}
		const auto  source = pendingTarget;
		const auto& config = Configuration::Get();
		if (!CycleInputSession() || !cycle.Pending(config.Find(source)) ||
			!StartSwitch(source, config.menus[cycle.Target()].menu, true)) {
			CancelQueuedCycles();
			pendingTarget.clear();
			return false;
		}
		return true;
	}
}
