#include "DeferredCaptureState.h"
#include "FilteredEvents.h"
#include "MenuTransitions.h"
#include "PendingMapFades.h"
#include "SwitchFade.h"
#include "TransitionFrames.h"
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
	struct Event
	{
		int    id;
		Event* next;
	};
	int  checks = 0;
	void Check(bool a_value)
	{
		++checks;
		if (!a_value) {
			throw std::runtime_error("transition/input check failed");
		}
	}
}
int main()
{
	try {
		// Observation must work even if the downstream MenuControls callback never runs.
		Navbar::Input::DeferredCaptureState rawOnly;
		Check(rawOnly.Button(0, 56, true, true, true, false));
		Check(rawOnly.Button(0, 56, false, false, true, false));
		for (const bool rawFirst : { false, true }) {
			Navbar::Input::DeferredCaptureState raw, filtered;
			auto&                               first = rawFirst ? raw : filtered;
			auto&                               second = rawFirst ? filtered : raw;
			Check(first.Button(0, 56, true, true, true, false));
			Check(second.Button(0, 56, true, true, true, false));
			// Filter and replay bookkeeping do not consume each other's releases.
			Check(first.Button(0, 56, false, false, false, false));
			Check(second.Button(0, 56, false, false, false, false));
			Check(!raw.Button(0, 56, false, false, false, false));
			Check(!filtered.Button(0, 56, false, false, false, false));
			Check(!raw.Button(0, 30, true, true, false, false));
			Check(!filtered.Button(0, 30, true, true, false, false));
		}
		Navbar::Input::DeferredCaptureState captured;
		Check(captured.Button(2, 0x20, true, true, true, false));
		Check(captured.Button(2, 0x20, false, true, false, false));  // Drain after host/device changes.
		Check(captured.Button(2, 0x20, false, false, false, false));
		Check(captured.Button(0, 18, true, true, false, true));  // Picker confirm.
		Check(captured.Button(0, 18, false, false, false, false));
		Check(!captured.Stick(0, 0, 1, false));
		Check(captured.Stick(0, 0, 1, true));
		Check(captured.Stick(0, 0, 0, false));
		Check(!captured.Stick(0, 0, 0, false));
		Check(captured.Stick(1, 1, 0, true));
		Check(captured.Disconnect(false));
		Check(!captured.Disconnect(false));
		for (unsigned flags = 0; flags < 8; ++flags) {
			Check(Navbar::SkipSkillsOpening((flags & 1) != 0, (flags & 2) != 0, (flags & 4) != 0) == (flags == 7));
		}  // No skip for normal entry, a cancelled/stale navbar request, or disabled skip setting.
		Event                   c{ 3, nullptr }, b{ 2, &c }, a{ 1, &b };
		Navbar::PendingMapFades pending;
		Check(!pending.Take(&a));  // Unrelated native fades are never selected.
		pending.Mark(&a);
		pending.Mark(&b);
		Check(!pending.Take(&c));
		Check(pending.Take(&a));
		Check(!pending.Take(&a));  // Exactly once, even before native destruction.
		Check(pending.Take(&b));   // Also the cleanup path for a discarded request.
		Check(!pending.Take(&b));  // Reused allocation is not tagged accidentally.
		pending.Mark(&b);
		Check(pending.Take(&b));  // A new Map request can reuse that address.
		pending.Mark(nullptr);
		Check(!pending.Take(nullptr));
		Check(Navbar::MapFadeCompletionStep(0.5, 0.2, 100, 0) == 1.7);
		Check(Navbar::MapFadeCompletionStep(1, 0, 100, 0) == 2);
		Check(!Navbar::MapFadeCompletionStep(0, 0, 100, 0));
		Check(!Navbar::MapFadeCompletionStep(-1, 0, 100, 0));
		Check(!Navbar::MapFadeCompletionStep(1, -1, 100, 0));
		Check(!Navbar::MapFadeCompletionStep(1, 0, 0, 100));
		Check(!Navbar::MapFadeCompletionStep(1, 0, 100, 50));
		Check(!Navbar::MapFadeCompletionStep(61, 0, 100, 0));
		Check(!Navbar::MapFadeCompletionStep(1, 61, 100, 0));
		Check(!Navbar::MapFadeCompletionStep(std::numeric_limits<double>::infinity(), 0, 100, 0));
		Check(!Navbar::MapFadeCompletionStep(1, std::numeric_limits<double>::quiet_NaN(), 100, 0));
		for (unsigned mask = 0; mask < 8; ++mask) {
			{
				Navbar::FilteredEvents<Event> filtered(&a, [mask](auto a_event) { return (mask & (1u << (a_event->id - 1))) != 0; });
				auto                          current = *filtered.Head();
				for (int id = 1; id <= 3; ++id) {
					if (mask & (1u << (id - 1))) {
						continue;
					}
					Check(current && current->id == id);
					current = current->next;
				}
				Check(!current);
			}
			Check(a.next == &b && b.next == &c && c.next == nullptr);
		}
		Navbar::FilteredEvents<Event> empty(nullptr, [](auto) { return true; });
		Check(!*empty.Head());
		for (unsigned flags = 0; flags < 4; ++flags) {
			const bool disabled = (flags & 1) != 0;
			const bool closing = (flags & 2) != 0;
			Check(Navbar::SkipMenuClosing(disabled, closing) == (flags == 3));
			for (const bool opening : { false, true }) {
				Check(Navbar::SkipMapTransition(disabled, 1, opening, closing) == (disabled && opening));
				Check(Navbar::SkipMapTransition(disabled, 2, opening, closing) == (disabled && closing));
				Check(!Navbar::SkipMapTransition(disabled, 0, opening, closing));
				Check(!Navbar::SkipMapTransition(disabled, 3, opening, closing));
			}
		}
		Check(Navbar::ItemTransitionEndpoint(11, 1, true) == 0);
		for (int i = 2; i < 6; ++i) {
			Check(Navbar::ItemTransitionEndpoint(11, i, true) == 6);
			Check(Navbar::ItemTransitionEndpoint(11, i, false) == 0);
		}
		Check(Navbar::ItemTransitionEndpoint(11, 6, true) == 0);
		for (int i = 7; i <= 11; ++i) {
			Check(Navbar::ItemTransitionEndpoint(11, i, true) == 1);
			Check(Navbar::ItemTransitionEndpoint(11, i, false) == 1);
		}
		Check(Navbar::ItemTransitionEndpoint(31, 3, true) == 0);
		Check(Navbar::ItemTransitionEndpoint(11, 12, true) == 0);
		Check(Navbar::ItemTransitionEndpoint(11, -1, true) == 0);
		using namespace std::chrono_literals;
		using Fade = Navbar::SwitchFade;
		const auto start = Fade::Clock::time_point{};
		for (unsigned flags = 0; flags < 8; ++flags) {
			const Navbar::Transitions::MapRevealState native{ (flags & 2) != 0, (flags & 4) != 0 };
			Check(native.Ready((flags & 1) != 0) == (flags == 3));
		}
		// Map readiness starts the same fade immediately, with no extra dwell timer.
		const Navbar::Transitions::MapRevealState readyMap{ true, false };
		Fade                                      ordinary, map;
		ordinary.Start(start);
		map.Start(start);
		for (const auto elapsed : { 140ms, 180ms, 220ms, 280ms, 340ms }) {
			const bool ready = elapsed >= 220ms;
			Check(ordinary.Tick(start + elapsed, true, ready) == map.Tick(start + elapsed, true, readyMap.Ready(ready)));
			Check(ordinary.Alpha() == map.Alpha() && ordinary.State() == map.State());
			if (elapsed == 220ms) {
				Check(map.State() == Fade::Phase::kReveal);
			}
			if (elapsed == 280ms) {
				Check(map.Alpha() == 0.5);
			}
		}
		Check(!map.Active() && map.Alpha() == 0);
		Fade fade;
		fade.Start(start);
		Check(fade.Active() && fade.Alpha() == 0);
		Check(fade.Tick(start + 60ms, false, false) == Fade::Action::kNone);
		Check(fade.Alpha() == 0.5);
		Check(fade.Tick(start + 120ms, false, false) == Fade::Action::kNone);
		Check(fade.Alpha() == 1);
		Check(fade.Tick(start + 140ms, false, false) == Fade::Action::kHideSource);
		Check(fade.Tick(start + 200ms, false, true) == Fade::Action::kNone);
		Check(fade.Tick(start + 220ms, true, false) == Fade::Action::kShowTarget);
		Check(fade.Tick(start + 300ms, true, false) == Fade::Action::kNone && fade.Alpha() == 1);
		Check(fade.Tick(start + 350ms, true, true) == Fade::Action::kNone);
		Check(fade.State() == Fade::Phase::kReveal);
		Check(fade.Tick(start + 410ms, true, true) == Fade::Action::kNone && fade.Alpha() == 0.5);
		Check(fade.Tick(start + 470ms, true, true) == Fade::Action::kFinished);
		Check(!fade.Active() && fade.Alpha() == 0);
		fade.Start(start);
		fade.Tick(start + 140ms, false, false);
		Check(fade.Tick(start + 5s, false, false) == Fade::Action::kAbort);
		Check(fade.Tick(start + 5200ms, false, false) == Fade::Action::kFinished);
		Check(!fade.Active() && fade.Alpha() == 0);
		fade.Start(start);
		fade.Tick(start + 60ms, false, false);
		fade.Cancel(start + 60ms);
		Check(fade.Tick(start + 120ms, false, false) == Fade::Action::kNone && fade.Alpha() == 0.25);
		Check(fade.Tick(start + 180ms, false, false) == Fade::Action::kFinished);
		fade.Start(start);
		Check(fade.Tick(start + 140ms, false, false) == Fade::Action::kHideSource);
		Check(fade.Tick(start + 200ms, true, false) == Fade::Action::kShowTarget);
		// A confirmed MovieClipLoader failure reveals the owner's open menu
		// immediately; the missing navbar must not hold black until timeout.
		fade.Cancel(start + 220ms);
		Check(fade.Tick(start + 280ms, true, false) == Fade::Action::kNone && fade.Alpha() == 0.5);
		Check(fade.Tick(start + 340ms, true, false) == Fade::Action::kFinished);
		Check(!fade.Active() && fade.Alpha() == 0);
		std::cout << checks << " transition/input checks passed\n";
	} catch (const std::exception& error) {
		std::cerr << error.what();
		return 1;
	}
}
