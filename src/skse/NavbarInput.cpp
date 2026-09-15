#include "NavbarInput.h"
#include "DeferredInput.h"
#include "FadeOverlay.h"
#include "FilteredEvents.h"
#include "NavbarSelection.h"
#include "NavbarWidget.h"
#include "NavigationBridge.h"
#include "RuntimeThread.h"
#include <mutex>

namespace Navbar::Input
{
	namespace
	{
		std::array<Host, 8> captures;
		SelectionController selection;
		// Scheduled engine tasks and the paused-menu input callback may use
		// different threads. Keep picker state serialized, including callbacks
		// made synchronously by Scaleform during a picker update.
		std::recursive_mutex selectionMutex;

		class RawInputSink final : public RE::BSTEventSink<RE::InputEvent*>
		{
		public:
			RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_events, RE::BSTEventSource<RE::InputEvent*>*) override
			{
				// CharacterSheet's own EventProcessor uses this source as well. Unpaused
				// input must be copied here, not inferred from downstream MenuControls.
				for (auto event = a_events ? *a_events : nullptr; event; event = event->next) {
					Deferred::Capture(event);
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};
		RawInputSink rawInputSink;

		bool Consume(RE::InputEvent* a_event)
		{
			if (a_event->GetDevice() != RE::INPUT_DEVICE::kMouse) {
				return false;
			}
			const auto button = a_event->AsButtonEvent();
			if (!button || button->GetIDCode() > 9) {
				return false;
			}
			const auto  id = button->GetIDCode();
			const auto  cursor = RE::MenuCursor::GetSingleton();
			const auto  ui = RE::UI::GetSingleton();
			auto        host = TopHost(ui);
			const float x = cursor ? cursor->GetRuntimeData().cursorPosX : 0;
			const float y = cursor ? cursor->GetRuntimeData().cursorPosY : 0;
			const bool  inside = cursor && host.owned && Widget::HitTest(host.movie, x, y);
			if (id >= captures.size()) {
				return inside;
			}  // Wheel up/down.
			auto& capture = captures[id];
			if (button->IsDown()) {
				capture = inside ? host : Host{};
			}
			const auto owner = capture;
			const bool consume = inside || owner.owned;
			if (consume && owner.owned && (button->IsDown() || button->IsUp())) {
				const bool down = button->IsDown();
				SKSE::GetTaskInterface()->AddUITask([owner, id, down, x, y]() {
					const auto currentUI = RE::UI::GetSingleton();
					const auto current = TopHost(currentUI);
					if (current.owned && current.menu == owner.menu && current.movie == owner.movie) {
						Widget::DispatchButton(current.movie, id, down, x, y);
					}
				});
				if (down) {
					SKSE::log::info("Navbar captured mouse button {} in {}; host input suppressed", id, owner.menu);
				}
			}
			if (button->IsUp()) {
				capture = {};
			}
			return consume;
		}

		struct ProcessInputHook
		{
			static RE::BSEventNotifyControl thunk(RE::MenuControls* a_this, RE::InputEvent* const* a_events,
				RE::BSTEventSource<RE::InputEvent*>* a_source)
			{
				if (!OnMainThread()) {
					if (!a_events) {
						return func(a_this, a_events, a_source);
					}
					FilteredEvents<RE::InputEvent> filtered(*a_events, Deferred::Filter);
					return func(a_this, filtered.Head(), a_source);
				}
				std::unique_lock lock(selectionMutex);
				Deferred::Drain(selection);
				selection.Update();
				Deferred::Publish(selection.Selecting());
				if (!a_events) {
					lock.unlock();
					return func(a_this, a_events, a_source);
				}
				const auto ui = RE::UI::GetSingleton();
				const bool fading = ui && ui->IsMenuOpen(FadeOverlay::Name);
				if (fading) {
					for (auto& capture : captures) {
						capture = {};
					}
				}
				FilteredEvents<RE::InputEvent> filtered(*a_events, [fading](auto a_event) {
					if (Deferred::Filter(a_event)) {
						return true;
					}
					const bool cycle = selection.ConsumeCycle(a_event, fading);  // Track key-up even while black.
					const bool picker = cycle ? false : selection.ConsumePicker(a_event);
					selection.Update();  // Preserve even a quick direction down/up in one event batch.
					return fading || cycle || picker || Consume(a_event);
				});
				selection.Update();
				lock.unlock();  // Never hold our state lock while running native handlers.
				return func(a_this, filtered.Head(), a_source);
			}
			inline static REL::Relocation<decltype(thunk)> func;
			static constexpr std::size_t                   idx = 1;
		};
	}

	void CancelSelection()
	{
		std::scoped_lock lock(selectionMutex);
		selection.Cancel();
		Deferred::Publish(false);
	}

	void OnMenuChanged()
	{
		std::scoped_lock lock(selectionMutex);
		selection.OnMenuChanged();
		Deferred::Publish(false);
	}

	void TickFromTask()
	{
		// MenuController already marshals this call through SKSE's task queue.
		// Do not apply the raw-input thread rejection a second time here: it
		// can starve the mailbox while an unpaused host remains open.
		std::scoped_lock lock(selectionMutex);
		Deferred::Drain(selection);
		selection.Update();
		Deferred::Publish(selection.Selecting());
	}

	void Install()
	{
		if (const auto input = RE::BSInputDeviceManager::GetSingleton()) {
			input->AddEventSink(&rawInputSink);
			SKSE::log::info("Navbar raw input observer installed for unpaused hosts");
		}
		REL::Relocation<std::uintptr_t> table{ RE::VTABLE_MenuControls[0] };
		ProcessInputHook::func = table.write_vfunc(ProcessInputHook::idx, ProcessInputHook::thunk);
		SKSE::log::info("Navbar input capture installed before native handlers and Scaleform click dispatch");
	}
}
