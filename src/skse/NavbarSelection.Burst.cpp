#include "NavbarSelection.h"
#include "MenuController.h"
#include "NavigationBridge.h"

namespace Navbar::Input
{
	bool SelectionController::ConsumeBurst(RE::ButtonEvent* a_button, std::int64_t a_now, std::uint64_t a_session)
	{
		const auto controller = MenuController::GetSingleton();
		const auto device = a_button->GetDevice();
		const auto id = a_button->GetIDCode();
		if (burstTap.Held() && device == burstDevice && id == burstKey) {
			if (a_button->IsUp() && burstTap.Release(a_now, controller->CycleSession())) {
				const auto host = TopHost(RE::UI::GetSingleton());
				const bool accepted = controller->RequestNext(host.menu, controller->CycleSession());
				SKSE::log::info("Navbar transition tap: accepted={}", accepted);
			}
			return true;
		}
		const auto code = device == RE::INPUT_DEVICE::kGamepad ? SKSE::InputMap::GamepadMaskToKeycode(id) :
		                                                         (device == RE::INPUT_DEVICE::kMouse ? 256 + id : id);
		const auto session = a_session ? a_session : controller->CycleInputSession();
		if (!session || session != controller->CycleSession() || code != Navigation::Key(device)) {
			return false;
		}
		if (a_button->IsDown() && !burstTap.Held()) {
			burstDevice = device;
			burstKey = id;
			burstTap.Begin(a_now, session);
		}
		return true;
	}
}
