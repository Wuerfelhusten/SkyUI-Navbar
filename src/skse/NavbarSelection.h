#pragma once
#include "CycleBurst.h"
#include "HoldNavigation.h"
#include "NavbarInputHost.h"
#include <set>

namespace Navbar::Input
{
	// Serialized by NavbarInput at the engine-task / paused-input boundary.
	class SelectionController
	{
	public:
		void Update();
		void UpdateAt(std::int64_t a_time) { UpdateSelection(a_time); }
		void Cancel();
		void OnMenuChanged();
		bool ConsumeCycle(RE::InputEvent* a_event, bool a_fading, std::int64_t a_eventTime = -1, std::uint64_t a_cycleSession = 0);
		bool ConsumePicker(RE::InputEvent* a_event);
		bool Selecting() const { return hold.Selecting(); }
		void ResetDeferred()
		{
			Cancel();
			hold = {};
			burstTap = {};
		}

	private:
		using ButtonID = std::pair<RE::INPUT_DEVICE, std::uint32_t>;

		void ClosePicker();
		bool OwnerValid();
		void MoveSelection(int a_direction);
		void UpdateSelection(std::int64_t a_now);
		void NavigateTo(int a_targetIndex);
		bool ConsumeBurst(RE::ButtonEvent* a_button, std::int64_t a_now, std::uint64_t a_session);

		HoldNavigation               hold;
		CycleBurstTap                burstTap;
		RE::INPUT_DEVICE             burstDevice = RE::INPUT_DEVICE::kNone;
		std::uint32_t                burstKey = 0;
		SelectionRepeat              repeat;
		Host                         pickerOwner;
		int                          selected = -1;
		RE::INPUT_DEVICE             holdDevice = RE::INPUT_DEVICE::kNone;
		std::uint32_t                holdKey = 0;
		std::set<ButtonID>           drainedButtons;
		std::array<SelectionAxis, 2> sticks;
		SelectionButtons             directionButtons;
	};
}
