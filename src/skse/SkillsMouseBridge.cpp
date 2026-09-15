#include "SkillsMouseBridge.h"
#include "NavbarWidget.h"
#include "SkillsInput.h"
#include <cmath>

namespace Navbar::SkillsMenuHooks::MouseBridge
{
	namespace
	{
		bool                       navbarHost = false;
		MouseCapture               movieMouse;
		std::atomic<std::uint32_t> nativeButtons{ 0 };
		bool                       mouseLogged = false;
		std::atomic<std::uint32_t> tracedClicks{ 0 };

		struct CanProcessHook
		{
			static bool thunk(RE::MenuEventHandler* a_this, RE::InputEvent* a_event)
			{
				const bool accepted = func(a_this, a_event);
				if (!navbarHost || !a_event || a_event->GetDevice() != RE::INPUT_DEVICE::kMouse) {
					return accepted;
				}
				auto ui = RE::UI::GetSingleton();
				auto cursor = RE::MenuCursor::GetSingleton();
				if (!ui || !cursor || !ui->IsMenuOpen(RE::StatsMenu::MENU_NAME) || ui->IsModalMenuOpen()) {
					return accepted;
				}
				auto        menu = ui->GetMenu<RE::StatsMenu>();
				const auto& position = cursor->GetRuntimeData();
				const bool  inside = menu && Widget::HitTest(menu->uiMovie.get(), position.cursorPosX, position.cursorPosY);
				bool        captured = nativeButtons.load() != 0;
				if (const auto button = a_event->AsButtonEvent(); button && button->GetIDCode() < 8) {
					if (button->IsDown() && tracedClicks.fetch_add(1) < 8) {
						SKSE::log::info("Skills native mouse-down: button={}, screen=({}, {}), navbar hit={}, native accepted={}",
							button->GetIDCode(), position.cursorPosX, position.cursorPosY, inside, accepted);
					}
					const auto bit = 1u << button->GetIDCode();
					captured = (nativeButtons.load() & bit) != 0;
					if (button->IsDown() && inside) {
						nativeButtons.fetch_or(bit);
					}
					if (button->IsUp()) {
						nativeButtons.fetch_and(~bit);
					}
				}
				// Only suppress the 3D skill handler; generic mouse-to-Scaleform dispatch stays intact.
				return (inside || captured) ? false : accepted;
			}

			inline static REL::Relocation<decltype(thunk)> func;
			static constexpr std::size_t                   idx = 1;
		};
	}

	bool Forward(RE::StatsMenu* a_menu, RE::UIMessage& a_message)
	{
		if (!navbarHost || !a_menu->uiMovie || !a_message.data) {
			return false;
		}
		const auto data = static_cast<RE::BSUIScaleformData*>(a_message.data);
		if (!data->scaleformEvent) {
			return false;
		}
		const auto type = data->scaleformEvent->type;
		using Type = RE::GFxEvent::EventType;
		if (type != Type::kMouseMove && type != Type::kMouseDown && type != Type::kMouseUp) {
			return false;
		}
		const auto& event = *static_cast<RE::GFxMouseEvent*>(data->scaleformEvent);
		if (event.mouseIndex != 0 || !std::isfinite(event.x) || !std::isfinite(event.y)) {
			return false;
		}
		const bool inside = Widget::HitTest(a_menu->uiMovie.get(), event.x, event.y);
		if (type == Type::kMouseDown) {
			SKSE::log::info("Skills Scaleform mouse-down: button={}, screen=({}, {}), navbar hit={}",
				event.button, event.x, event.y, inside);
		}
		const auto action = type == Type::kMouseMove ? MouseCapture::Event::kMove :
		                                               (type == Type::kMouseDown ? MouseCapture::Event::kDown : MouseCapture::Event::kUp);
		const bool forward = movieMouse.Route(action, event.button, inside);
		if (!forward) {
			return false;
		}
		if (!mouseLogged) {
			mouseLogged = true;
			SKSE::log::info("Skills navbar mouse forwarding active at {}, {}", event.x, event.y);
		}
		if (type == Type::kMouseDown) {
			const RE::GFxMouseEvent move(Type::kMouseMove, 0, event.x, event.y);
			a_menu->uiMovie->HandleEvent(move);
		}
		a_menu->uiMovie->HandleEvent(event);
		return true;
	}

	void Reset(bool a_opening)
	{
		movieMouse.Reset();
		nativeButtons.store(0);
		if (a_opening) {
			mouseLogged = false;
			tracedClicks.store(0);
		}
	}

	void Install(bool a_enabled)
	{
		navbarHost = a_enabled;
		REL::Relocation<std::uintptr_t> table{ RE::StatsMenu::VTABLE[1] };
		CanProcessHook::func = table.write_vfunc(CanProcessHook::idx, CanProcessHook::thunk);
	}
}
