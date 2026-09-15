#pragma once
namespace Navbar::Transitions
{
	void Install();
	struct MapRevealState
	{
		bool           cameraReady;
		bool           faderActive;
		constexpr bool Ready(bool a_uiReady) const { return a_uiReady && cameraReady && !faderActive; }
	};
	// Main thread only. Observe native completion without interrupting callbacks.
	MapRevealState GetMapRevealState();
}
