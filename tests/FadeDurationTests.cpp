#include "SwitchFade.h"
#include <cmath>
#include <stdexcept>

int main()
{
	using namespace std::chrono_literals;
	using Fade = Navbar::SwitchFade;
	const auto check = [](bool a_ok) {
		if (!a_ok) {
			throw std::runtime_error("Configurable fade duration failed");
		}
	};
	const auto start = Fade::Clock::time_point{};
	Fade       fade;
	fade.Start(start, 0.1, 0.25);
	check(fade.Tick(start + 50ms, false, false) == Fade::Action::kNone && fade.Alpha() == 0.5);
	check(fade.Tick(start + 100ms, false, false) == Fade::Action::kNone && fade.Alpha() == 1);
	check(fade.Tick(start + 120ms, false, false) == Fade::Action::kHideSource);
	check(fade.Tick(start + 160ms, true, false) == Fade::Action::kShowTarget);
	check(fade.Tick(start + 200ms, true, true) == Fade::Action::kNone);
	check(fade.Tick(start + 325ms, true, true) == Fade::Action::kNone && fade.Alpha() == 0.5);
	check(fade.Tick(start + 450ms, true, true) == Fade::Action::kFinished && fade.Alpha() == 0);

	fade.Start(start, 0, 0);
	check(fade.Tick(start, false, false) == Fade::Action::kNone && fade.Alpha() == 1);
	check(fade.Tick(start + 20ms, false, false) == Fade::Action::kHideSource);
	check(fade.Tick(start + 60ms, true, false) == Fade::Action::kShowTarget);
	check(fade.Tick(start + 100ms, true, true) == Fade::Action::kNone);
	check(fade.Tick(start + 100ms, true, true) == Fade::Action::kFinished && fade.Alpha() == 0);
	check(std::isfinite(fade.Alpha()));

	fade.Start(start, 10, 10);
	check(fade.Tick(start + 5s, false, false) == Fade::Action::kNone && fade.Alpha() == 0.5);
	check(fade.Tick(start + 10020ms, false, false) == Fade::Action::kHideSource);
	check(fade.Tick(start + 10060ms, true, false) == Fade::Action::kShowTarget);
	check(fade.Tick(start + 10100ms, true, true) == Fade::Action::kNone);
	check(fade.Tick(start + 15100ms, true, true) == Fade::Action::kNone && fade.Alpha() == 0.5);
	check(fade.Tick(start + 20100ms, true, true) == Fade::Action::kFinished);
	check(Fade::WatchdogDuration(10, 10) > 20100ms);
	check(Fade::WatchdogDuration(0.12, 0.12) == 6s);

	// A stalled owner still aborts, then uses the configured reveal time.
	fade.Start(start, 0.12, 0.1);
	fade.Tick(start + 120ms, false, false);
	check(fade.Tick(start + 5s, false, false) == Fade::Action::kAbort);
	check(fade.Tick(start + 5050ms, false, false) == Fade::Action::kNone && fade.Alpha() == 0.5);
	check(fade.Tick(start + 5100ms, false, false) == Fade::Action::kFinished);
}
