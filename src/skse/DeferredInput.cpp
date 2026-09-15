#include "DeferredInput.h"
#include "DeferredCaptureState.h"
#include "MenuAdapters.h"
#include "MenuController.h"
#include "NavbarInputHost.h"
#include "NavbarSelection.h"
#include "NavigationBridge.h"
#include "RuntimeThread.h"
#include <chrono>
#include <deque>
#include <mutex>

namespace Navbar::Input::Deferred
{
	namespace
	{
		struct Record
		{
			Host                 owner;
			RE::INPUT_EVENT_TYPE type;
			RE::INPUT_DEVICE     device;
			std::uint32_t        id = 0;
			float                value = 0, duration = 0, x = 0, y = 0;
			std::int64_t         time = 0;
			std::uint64_t        cycleSession = 0;
		};
		std::mutex           mutex;
		Host                 host;
		bool                 enabled = false, selecting = false;
		DeferredCaptureState raw, filtered;
		std::deque<Record>   records;
		bool                 overflow = false;
		std::uint64_t        cycleSession = 0;
	}

	void Publish(bool a_selecting)
	{
		const auto       ui = RE::UI::GetSingleton();
		const auto       current = TopHost(ui);
		const auto       menu = current.owned && ui ? ui->GetMenu(RouteFor(current.menu).nativeName) : nullptr;
		const auto       controls = RE::ControlMap::GetSingleton();
		const auto       session = MenuController::GetSingleton()->CycleInputSession();
		std::scoped_lock lock(mutex);
		const bool       wasEnabled = enabled;
		const auto       previousMenu = host.menu;
		host = current;
		cycleSession = session;
		// Unpaused custom menus dispatch input on workers. Do not change their
		// pause flags just to force the engine onto its paused-menu input path.
		enabled = ((menu && !menu->PausesGame()) || session) && !(controls && controls->GetRuntimeData().textEntryCount > 0);
		selecting = enabled && a_selecting;
		if (wasEnabled != enabled || previousMenu != host.menu) {
			SKSE::log::info("Navbar raw input host: '{}', enabled={}, textEntry={}", host.menu, enabled,
				controls ? controls->GetRuntimeData().textEntryCount : 0);
		}
	}

	static bool Route(RE::InputEvent* a_event, bool a_capture)
	{
		std::scoped_lock lock(mutex);
		auto&            state = a_capture ? raw : filtered;
		Record           record{ host, a_event->GetEventType(), a_event->GetDevice() };
		record.cycleSession = cycleSession;
		record.time = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch())
		                  .count();
		if (const auto button = a_event->AsButtonEvent()) {
			record.id = button->GetIDCode();
			record.value = button->Value();
			record.duration = button->HeldDuration();
			const auto code = record.device == RE::INPUT_DEVICE::kGamepad ? SKSE::InputMap::GamepadMaskToKeycode(record.id) :
			                                                                (record.device == RE::INPUT_DEVICE::kMouse ? record.id + 256 : record.id);
			const bool cycle = enabled && (record.device == RE::INPUT_DEVICE::kKeyboard || record.device == RE::INPUT_DEVICE::kMouse || record.device == RE::INPUT_DEVICE::kGamepad) &&
			                   code == Navigation::Key(record.device);
			if (!state.Button(static_cast<std::uint32_t>(record.device), record.id, button->IsDown(), record.value > 0, cycle, selecting)) {
				return false;
			}
			if (a_capture && cycle && (button->IsDown() || button->IsUp())) {
				SKSE::log::info("Navbar raw switch captured in {}: device={}, key={}, down={}", host.menu,
					static_cast<int>(record.device), record.id, button->IsDown());
			}
		} else if (const auto stick = a_event->AsThumbstickEvent()) {
			if (record.device != RE::INPUT_DEVICE::kGamepad || (!stick->IsLeft() && !stick->IsRight())) {
				return selecting;
			}
			const auto index = stick->IsLeft() ? 0u : 1u;
			if (!state.Stick(index, stick->xValue, stick->yValue, selecting)) {
				return false;
			}
			record.id = stick->GetIDCode();
			record.x = stick->xValue;
			record.y = stick->yValue;
		} else if (record.type == RE::INPUT_EVENT_TYPE::kDeviceConnect) {
			if (!state.Disconnect(enabled)) {
				return false;
			}
		} else {
			return selecting;  // Text and mouse motion must not reach the host picker underneath.
		}
		if (!a_capture) {
			return true;
		}
		if (records.size() < 512) {
			records.push_back(std::move(record));
		} else {
			overflow = true;
		}
		return true;
	}

	bool Capture(RE::InputEvent* a_event) { return Route(a_event, true); }
	bool Filter(RE::InputEvent* a_event) { return Route(a_event, false); }

	void Drain(SelectionController& a_selection)
	{
		std::deque<Record> pending;
		bool               cancelled;
		{
			std::scoped_lock lock(mutex);
			pending.swap(records);
			cancelled = std::exchange(overflow, false);
		}
		if (cancelled) {
			a_selection.ResetDeferred();
			SKSE::log::warn("Navbar deferred input overflow; selection cancelled");
			return;
		}
		for (const auto& record : pending) {
			const auto current = TopHost(RE::UI::GetSingleton());
			const bool sameOwner = current.owned && record.owner.owned && current.menu == record.owner.menu &&
			                       current.movie == record.owner.movie;
			const bool sameCycle = record.cycleSession && record.cycleSession == MenuController::GetSingleton()->CycleSession();
			if (record.type == RE::INPUT_EVENT_TYPE::kButton && record.value > 0 && record.duration == 0) {
				SKSE::log::info("Navbar deferred button replay: captured='{}', current='{}', sameOwner={}, device={}, key={}, thread={}, main={}",
					record.owner.menu, current.menu, sameOwner, static_cast<int>(record.device), record.id, GetCurrentThreadId(), OnMainThread());
			}
			if (record.type == RE::INPUT_EVENT_TYPE::kDeviceConnect) {
				a_selection.ResetDeferred();
				continue;
			}
			// Never begin a delayed gesture in a replacement/closed menu. Releases
			// still drain the captured physical input across a menu/thread change.
			if (!sameOwner && !sameCycle && record.type == RE::INPUT_EVENT_TYPE::kButton && record.value > 0) {
				continue;
			}
			if (record.type == RE::INPUT_EVENT_TYPE::kButton) {
				const auto button = RE::ButtonEvent::Create(record.device, RE::BSFixedString(), record.id,
					record.value, record.duration);
				if (!button) {
					SKSE::log::warn("Navbar deferred button allocation failed");
					a_selection.ResetDeferred();
					continue;
				}
				if (!a_selection.ConsumeCycle(button, !sameOwner && !sameCycle, record.time, sameCycle ? record.cycleSession : 0)) {
					a_selection.ConsumePicker(button);
				}
				// Factory allocation, no retained user-event string or engine-owned links.
				button->SetUserEvent(RE::BSFixedString());
				RE::free(button);
			} else if (sameOwner) {
				const auto stick = RE::malloc_runtime<RE::ThumbstickEvent>(sizeof(RE::ThumbstickEvent), sizeof(RE::ThumbstickEvent));
				if (!stick) {
					a_selection.ResetDeferred();
					continue;
				}
				RE::stl::emplace_vtable<RE::ThumbstickEvent>(stick);
				stick->eventType = RE::INPUT_EVENT_TYPE::kThumbstick;
				stick->Init(static_cast<RE::ThumbstickEvent::InputType>(record.id), record.device, record.x, record.y);
				a_selection.ConsumePicker(stick);
				stick->userEvent = RE::BSFixedString();
				RE::free(stick);
			}
			a_selection.UpdateAt(record.time);
		}
	}
}
