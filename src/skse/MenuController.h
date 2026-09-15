#pragma once
#include "AnimationState.h"
#include "CycleBurst.h"
#include "FadePresentation.h"
#include "SwitchFade.h"
#include <chrono>
#include <thread>
#include <vector>

namespace Navbar
{
	class MenuController final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		static MenuController* GetSingleton();
		void                   Install();
		void                   ScheduleTransitionTick();
		[[nodiscard]] bool     RequestSwitch(std::string_view a_source, std::string_view a_target);
		[[nodiscard]] bool     RequestNext(std::string_view a_source, std::uint64_t a_session = 0);
		std::uint64_t          CycleInputSession() const;
		std::uint64_t          CycleSession() const { return cycle.Session(); }
		void                   CancelQueuedCycles()
		{
			cycle.Clear();
			cycleContinue = false;
		}
		[[nodiscard]] const char* GetCurrentMenuName() const;
		[[nodiscard]] static bool IsRegistered(std::string_view a_menu);
		[[nodiscard]] bool        IsClosingForSwitch(std::string_view a_menu) const;
		[[nodiscard]] bool        IsOpeningForSwitch(std::string_view a_menu) const;
		std::vector<TabAnimation> TakeAnimationState(std::string_view a_menu, RE::GFxMovieView* a_movie);
		RE::BSEventNotifyControl  ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
			RE::BSTEventSource<RE::MenuOpenCloseEvent>*                     a_source) override;

	private:
		using Clock = std::chrono::steady_clock;

		struct MenuState
		{
			bool              open{ false };
			bool              timedOut{ false };
			RE::GFxMovieView* loadedMovie{ nullptr };  // Identity only; never dereferenced.
			Clock::time_point openedAt;
		};

		void               Tick();
		bool               StartSwitch(std::string_view a_source, std::string_view a_target, bool a_continuation);
		bool               ContinueCycle();
		void               RetargetCycle();
		void               OnMenuEvent(int a_index, bool a_opening);
		void               RestoreSource();
		void               CheckTransitionTimeout(Clock::time_point a_now);
		[[nodiscard]] bool TickTransition(Clock::time_point a_now);

		// Menu/movie state stays on the main thread. The worker only reads the
		// atomic scheduling flags and enqueues tasks; it never accesses a movie.
		std::vector<MenuState> states;
		std::atomic<int>       current{ -1 };
		std::atomic_bool       watching{ false };
		std::atomic_bool       taskQueued{ false };
		bool                   transitioning{ false };
		CycleBurst             cycle;
		bool                   cycleContinue = false;
		AnimationHandoff       animationHandoff;
		std::string            pendingSource;
		std::string            pendingTarget;
		SwitchFade             fade;
		FadePresentation       presentation{ false, true };
		int                    mapReadinessLog = -1;
		bool                   fadeWaiting = false;
		std::atomic_bool       frameQueued{ false };
		Clock::time_point      transitionDeadline;
		bool                   installed{ false };
		std::jthread           worker;
	};
}
