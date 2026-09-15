#pragma once

namespace Navbar::Input
{
	class SelectionController;
	namespace Deferred
	{
		// Capture copies values only; UI and selection state stay on the main thread.
		bool Capture(RE::InputEvent* a_event);
		bool Filter(RE::InputEvent* a_event);
		void Drain(SelectionController& a_selection);
		void Publish(bool a_selecting);
	}
}
