#include "NavbarSelection.h"
#include "Configuration.h"
#include "FadeOverlay.h"
#include "MenuAdapters.h"
#include "MenuController.h"
#include "NavbarWidget.h"
#include "NavigationBridge.h"
#include <chrono>

namespace Navbar::Input
{
	namespace
	{
		std::int64_t Now()
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now().time_since_epoch())
			    .count();
		}
	}

	void SelectionController::ClosePicker()
	{
		const auto movie = Menus::Movie(pickerOwner.menu);
		if (movie && movie.get() == pickerOwner.movie) {
			Widget::SetPicker(movie.get(), false, "");
		}
		directionButtons = {};
		for (auto& stick : sticks) {
			stick.Cancel();
		}
		repeat = {};
	}

	bool SelectionController::OwnerValid()
	{
		const auto ui = RE::UI::GetSingleton();
		const auto host = TopHost(ui);
		const auto controls = RE::ControlMap::GetSingleton();
		const auto input = RE::BSInputDeviceManager::GetSingleton();
		return pickerOwner.owned && host.owned && host.movie == pickerOwner.movie && host.menu == pickerOwner.menu &&
		       !(controls && controls->GetRuntimeData().textEntryCount > 0) &&
		       !(ui && ui->IsMenuOpen(FadeOverlay::Name)) &&
		       (holdDevice != RE::INPUT_DEVICE::kGamepad || (input && input->IsGamepadEnabled())) &&
		       Menus::CanLeave(host.menu) && !Menus::HasBlocker(host.menu);
	}

	void SelectionController::MoveSelection(int a_direction)
	{
		if (!a_direction || !hold.Selecting()) {
			return;
		}
		const auto& config = Configuration::Get();
		selected = SelectionMenuIndex(config.menus, selected, a_direction, Menus::Available);
		if (selected >= 0 && !Widget::SetPicker(pickerOwner.movie, true, config.menus[selected].menu)) {
			Cancel();
		}
	}

	void SelectionController::UpdateSelection(std::int64_t a_now)
	{
		if ((!hold.Held() && !hold.Selecting()) || !pickerOwner.owned) {
			return;
		}
		if (!OwnerValid()) {
			Cancel();
			return;
		}
		if (hold.Tick(a_now)) {
			if (hold.Selecting()) {
				for (auto& stick : sticks) {
					stick.Begin();
				}
				const auto& config = Configuration::Get();
				selected = config.Find(pickerOwner.menu);
				if (selected < 0 || !Widget::SetPicker(pickerOwner.movie, true, pickerOwner.menu)) {
					Cancel();
					return;
				}
				SKSE::log::info("Navbar selection view enabled in {}", pickerOwner.menu);
			} else {
				ClosePicker();
				SKSE::log::info("Navbar selection view disabled in {}", pickerOwner.menu);
			}
		}
		if (hold.Selecting()) {
			const int direction = directionButtons.Held() ? directionButtons.Direction() : sticks[0].Direction();
			MoveSelection(repeat.Step(direction, a_now));
		}
	}

	void SelectionController::NavigateTo(int a_targetIndex)
	{
		const auto& config = Configuration::Get();
		if (a_targetIndex < 0 || a_targetIndex >= static_cast<int>(config.menus.size()) || !OwnerValid()) {
			return;
		}
		const auto source = pickerOwner.menu;
		const auto target = config.menus[a_targetIndex].menu;
		if (target == source) {
			return;
		}
		const bool accepted = MenuController::GetSingleton()->RequestSwitch(source, target);
		SKSE::log::info("Navbar navigation {} -> {}: {}", source, target, accepted);
		if (accepted) {
			Cancel();
		}
	}

	bool SelectionController::ConsumeCycle(RE::InputEvent* a_event, bool a_fading, std::int64_t a_eventTime, std::uint64_t a_cycleSession)
	{
		const auto button = a_event->AsButtonEvent();
		const auto device = a_event->GetDevice();
		if (!button || (device != RE::INPUT_DEVICE::kKeyboard && device != RE::INPUT_DEVICE::kMouse &&
						   device != RE::INPUT_DEVICE::kGamepad)) {
			return false;
		}
		const auto id = button->GetIDCode();
		if (hold.Held() && device == holdDevice && id == holdKey) {
			if (button->IsUp()) {
				const auto now = a_eventTime >= 0 ? a_eventTime : Now();
				UpdateSelection(now);
				const bool valid = OwnerValid();
				if (!valid) {
					hold.Cancel();
				}
				const auto action = hold.Release(now);
				if (action == HoldNavigation::ReleaseAction::kNext) {
					if (MenuController::GetSingleton()->RequestNext(pickerOwner.menu)) {
						Cancel();
					}
				}
				if (!hold.Selecting()) {
					ClosePicker();
					pickerOwner = {};
				}
			}
			return true;
		}
		if (ConsumeBurst(button, a_eventTime >= 0 ? a_eventTime : Now(), a_cycleSession)) {
			return true;
		}
		const auto code = device == RE::INPUT_DEVICE::kGamepad ? SKSE::InputMap::GamepadMaskToKeycode(id) :
		                                                         (device == RE::INPUT_DEVICE::kMouse ? 256 + id : id);
		const auto ui = RE::UI::GetSingleton();
		const auto host = TopHost(ui);
		const auto controls = RE::ControlMap::GetSingleton();
		const bool typing = controls && controls->GetRuntimeData().textEntryCount > 0;
		const bool eligible = code == Navigation::Key(device) && !typing && (a_fading || host.owned);
		if (button->IsDown() && code == Navigation::Key(device)) {
			SKSE::log::info("Navbar switch input: host='{}', owned={}, typing={}, fading={}, held={}, selected={}",
				host.menu, host.owned, typing, a_fading, hold.Held(), hold.Selecting());
		}
		if (drainedButtons.contains({ device, id })) {
			return false;
		}
		if (eligible && hold.Held()) {
			if (!button->IsUp()) {
				drainedButtons.insert({ device, id });
			}
			return false;  // A second device cannot start a parallel navigation gesture.
		}
		if (!eligible || !button->IsDown() || hold.Held()) {
			return false;
		}
		const bool inView = hold.Selecting();
		hold.Begin(a_eventTime >= 0 ? a_eventTime : Now());
		holdKey = id;
		holdDevice = device;
		pickerOwner = host;
		if (!inView) {
			selected = Configuration::Get().Find(host.menu);
		}
		if (a_fading || !OwnerValid()) {
			Cancel();
		}
		return true;
	}

	bool SelectionController::ConsumePicker(RE::InputEvent* a_event)
	{
		if (a_event->GetEventType() == RE::INPUT_EVENT_TYPE::kDeviceConnect) {
			Cancel();
			hold = {};
			burstTap = {};
			drainedButtons.clear();
			sticks = {};
			return false;
		}
		if (a_event->GetDevice() == RE::INPUT_DEVICE::kGamepad) {
			if (const auto stick = a_event->AsThumbstickEvent()) {
				const auto id = stick->GetIDCode();
				if (id != RE::ThumbstickEvent::InputType::kLeftThumbstick &&
					id != RE::ThumbstickEvent::InputType::kRightThumbstick) {
					return false;
				}
				const auto index = id == RE::ThumbstickEvent::InputType::kLeftThumbstick ? 0 : 1;
				return sticks[index].Route(stick->xValue, stick->yValue, hold.Selecting());
			}
		}
		if (const auto button = a_event->AsButtonEvent()) {
			const auto     device = a_event->GetDevice();
			const auto     id = button->GetIDCode();
			const ButtonID key{ device, id };
			if (!hold.Selecting()) {
				if (!drainedButtons.contains(key)) {
					return false;
				}
				if (button->IsUp()) {
					drainedButtons.erase(key);
				}
				return true;
			}
			if (button->IsUp()) {
				drainedButtons.erase(key);
			} else {
				drainedButtons.insert(key);
			}
			if (device == RE::INPUT_DEVICE::kKeyboard || device == RE::INPUT_DEVICE::kGamepad) {
				directionButtons.Route(device == RE::INPUT_DEVICE::kGamepad, id, button->IsDown(), button->IsUp());
			}
			const bool cancel = (device == RE::INPUT_DEVICE::kGamepad && id == 0x2000) ||
			                    (device == RE::INPUT_DEVICE::kKeyboard && id == 1);
			const bool confirm = (device == RE::INPUT_DEVICE::kGamepad || device == RE::INPUT_DEVICE::kKeyboard) &&
			                     SelectionConfirmKey(device == RE::INPUT_DEVICE::kGamepad, id);
			if (cancel && button->IsDown()) {
				Cancel();
			} else if (confirm && button->IsDown()) {
				NavigateTo(selected);
			}
			return true;  // Equip/zoom/clicks cannot act behind the modal selection.
		}
		return hold.Selecting() && (a_event->GetEventType() == RE::INPUT_EVENT_TYPE::kMouseMove ||
									   a_event->GetEventType() == RE::INPUT_EVENT_TYPE::kChar);
	}

	void SelectionController::Update() { UpdateSelection(Now()); }
	void SelectionController::Cancel()
	{
		burstTap.Cancel();
		hold.Cancel();
		ClosePicker();
		pickerOwner = {};
	}

	void SelectionController::OnMenuChanged()
	{
		if (!MenuController::GetSingleton()->CycleInputSession()) {
			burstTap.Cancel();
		}
		hold.Cancel();
		ClosePicker();
		pickerOwner = {};
	}
}
