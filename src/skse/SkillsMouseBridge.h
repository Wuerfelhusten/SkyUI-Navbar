#pragma once

namespace Navbar::SkillsMenuHooks::MouseBridge
{
	void Install(bool a_enabled);
	void Reset(bool a_opening);
	bool Forward(RE::StatsMenu* a_menu, RE::UIMessage& a_message);
}
