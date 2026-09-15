#include "SkillsInput.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

int main()
{
	int        checks = 0;
	const auto check = [&checks](bool a_value) {
		++checks;
		if (!a_value) {
			throw std::runtime_error("Skills input check " + std::to_string(checks) + " failed");
		}
	};
	try {
		using Event = Navbar::MouseCapture::Event;
		Navbar::MouseCapture mouse;
		check(!mouse.Route(Event::kMove, 0, false));
		check(mouse.Route(Event::kMove, 0, true));
		check(mouse.Route(Event::kMove, 0, false));  // One rollout on exit.
		check(!mouse.Route(Event::kMove, 0, false));
		check(!mouse.Route(Event::kDown, 0, false));
		check(!mouse.Route(Event::kUp, 0, true));  // No press inside, no click.
		check(mouse.Route(Event::kDown, 0, true));
		check(mouse.Route(Event::kMove, 0, false));
		check(mouse.Route(Event::kUp, 0, false));  // Release-outside reaches the SWF.
		check(!mouse.Route(Event::kUp, 0, true));
		check(!mouse.Route(Event::kMove, 0, false));
		check(!mouse.Route(Event::kDown, 32, true));
		check(mouse.Route(Event::kDown, 0, true));
		check(mouse.Route(Event::kDown, 1, true));
		check(mouse.Route(Event::kUp, 0, false));
		check(mouse.Route(Event::kMove, 0, false));  // Other button remains captured.
		mouse.Reset();
		check(!mouse.Route(Event::kUp, 1, true));
		check(!mouse.Route(Event::kMove, 0, false));
		auto p = Navbar::ScreenToStage(2400, 40, 0, 0, 2560, 1440, 0, 0, 1280, 720);
		check(p && p->x == 1200 && p->y == 20);
		p = Navbar::ScreenToStage(1700, 70, 100, 50, 1920, 1080, -320, -40, 1600, 1040);
		check(p && p->x == 1280 && p->y == -20);  // Viewport offset and widescreen frame.
		check(!Navbar::ScreenToStage(1, 2, 0, 0, 0, 720, 0, 0, 1280, 720));
		check(!Navbar::ScreenToStage(1, 2, 0, 0, 1280, 720, 1280, 0, 0, 720));
		check(!Navbar::ScreenToStage(std::numeric_limits<float>::quiet_NaN(), 2, 0, 0, 1280, 720, 0, 0, 1280, 720));
		std::cout << checks << " Skills input checks passed (logic only; native dispatch needs in-game testing)\n";
	} catch (const std::exception& error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
