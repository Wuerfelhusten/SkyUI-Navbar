#pragma once
namespace Navbar::Input
{
	void Install();
	// Only from a scheduled SKSE controller task, never from the timer/input worker.
	void TickFromTask();
	void CancelSelection();
	void OnMenuChanged();
}
