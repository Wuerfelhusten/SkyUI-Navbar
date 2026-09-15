#include "MenuController.h"
#include "Configuration.h"
#include "MenuAdapters.h"
#include "MenuFade.h"
#include "NavbarInput.h"
#include "NavbarLoader.h"
#include "NavbarWidget.h"

namespace Navbar
{
	MenuController* MenuController::GetSingleton()
	{
		static MenuController singleton;
		return std::addressof(singleton);
	}

	void MenuController::Install()
	{
		if (installed) {
			return;
		}
		auto ui = RE::UI::GetSingleton();
		if (!ui) {
			SKSE::log::error("UI singleton was unavailable at DataLoaded");
			return;
		}
		states.resize(Configuration::Get().menus.size());
		ui->AddEventSink(this);
		installed = true;
		// Only schedule work here. All menu/movie access stays on the main thread.
		worker = std::jthread([this](std::stop_token a_stop) {
			while (!a_stop.stop_requested()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				if (!a_stop.stop_requested() && watching.load() && !taskQueued.exchange(true)) {
					SKSE::GetTaskInterface()->AddTask([this]() {
						Tick();
						taskQueued.store(false);
					});
				}
			}
		});
		SKSE::log::info("Listening for {} configured menus", states.size());
	}

	bool MenuController::IsRegistered(std::string_view a_menu)
	{
		return Menus::Available(a_menu);
	}

	void MenuController::ScheduleTransitionTick()
	{
		if (frameQueued.exchange(true)) {
			return;
		}
		SKSE::GetTaskInterface()->AddTask([this]() { Tick(); frameQueued.store(false); });
	}

	const char* MenuController::GetCurrentMenuName() const
	{
		const int index = current.load();
		return index >= 0 ? Configuration::Get().menus[index].menu.c_str() : "";
	}

	RE::BSEventNotifyControl MenuController::ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
		RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (a_event) {
			MenuFade::OnMenuEvent(a_event->menuName.c_str(), a_event->opening);
			// Marshal lifecycle handling to the same task queue as navigation.
			const std::string name = a_event->menuName.c_str();
			const bool        opening = a_event->opening;
			SKSE::GetTaskInterface()->AddTask([this, name, opening]() {
				const auto& menus = Configuration::Get().menus;
				for (std::size_t i = 0; i < menus.size(); ++i) {
					if (RouteFor(menus[i].menu).nativeName != name ||
						(!menus[i].showNavbar && !menus[i].showInNavbar)) {
						continue;
					}
					OnMenuEvent(static_cast<int>(i), opening);
				}
			});
		}
		return RE::BSEventNotifyControl::kContinue;
	}

	void MenuController::OnMenuEvent(int a_index, bool a_opening)
	{
		if (!transitioning && !cycleContinue) {
			CancelQueuedCycles();
			pendingTarget.clear();
		}
		Input::OnMenuChanged();  // Every menu change returns to normal (non-picker) input.
		states[a_index] = MenuState{};
		states[a_index].open = a_opening;
		states[a_index].openedAt = Clock::now();
		const auto& entry = Configuration::Get().menus[a_index];
		if (!a_opening && !(transitioning && pendingSource == entry.menu)) {
			animationHandoff.ClearFor(entry.menu);
		}
		SKSE::log::info("Menu event: {} {}", entry.menu, a_opening ? "opened" : "closed");
		if (a_opening) {
			Menus::OnOpened(entry.menu);
			current.store(a_index);
			if (pendingTarget.empty()) {
				transitioning = false;
			}
			watching.store(true);
		} else if (current.load() == a_index) {
			current.store(-1);
		}
		Tick();
	}

	void MenuController::Tick()
	{
		Input::TickFromTask();  // Hold threshold/repeat also progress without fresh input events.
		auto ui = RE::UI::GetSingleton();
		if (!ui) {
			return;
		}
		const auto now = Clock::now();
		CheckTransitionTimeout(now);
		bool        keepWatching = ContinueCycle() || transitioning;
		const auto& menus = Configuration::Get().menus;
		for (std::size_t i = 0; i < states.size(); ++i) {
			auto& state = states[i];
			if (!state.open || !menus[i].showNavbar) {
				continue;
			}
			if (!ui->IsMenuOpen(RouteFor(menus[i].menu).nativeName)) {
				continue;
			}
			auto movie = Menus::Movie(menus[i].menu);
			if (movie && state.loadedMovie == movie.get()) {
				Widget::UpdateLayout(movie.get());
				keepWatching = true;
				continue;
			}
			if (state.timedOut) {
				continue;
			}
			keepWatching = true;
			RE::GFxValue root;
			if (movie && movie->GetVariable(&root, "_root") && root.IsDisplayObject()) {
				// A cached/replaced movie gets exactly one load attempt per identity/open.
				Menus::OnOpened(menus[i].menu);
				Loader::Load(movie.get(), menus[i].menu.c_str());
				state.loadedMovie = movie.get();
			} else if (now - state.openedAt >= std::chrono::seconds(5)) {
				state.timedOut = true;
				SKSE::log::error("Navbar: movie/root not ready in {} after 5 seconds", menus[i].menu);
			}
		}
		keepWatching = TickTransition(now) || keepWatching;
		watching.store(keepWatching);
	}
}
