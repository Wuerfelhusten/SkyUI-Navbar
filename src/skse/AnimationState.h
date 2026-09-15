#pragma once
#include <chrono>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Navbar
{
	struct TabAnimation
	{
		std::string menu;
		double      position = 0;
		double      target = 0;
		bool        hover = false;
	};

	class AnimationHandoff
	{
	public:
		using Clock = std::chrono::steady_clock;
		void Store(std::string_view a_target, std::vector<TabAnimation> a_tabs, Clock::time_point a_now,
			Clock::duration a_lifetime = std::chrono::seconds(5))
		{
			target = a_target;
			tabs = std::move(a_tabs);
			expires = a_now + a_lifetime;
		}
		std::vector<TabAnimation> Take(std::string_view a_target, Clock::time_point a_now)
		{
			if (a_now >= expires) {
				Clear();
			}
			if (a_target != target) {
				return {};
			}
			auto snapshot = std::move(tabs);
			Clear();
			return snapshot;
		}
		void ClearFor(std::string_view a_target)
		{
			if (a_target == target) {
				Clear();
			}
		}
		void Retarget(std::string_view a_target) { target = a_target; }
		void Clear()
		{
			target.clear();
			tabs.clear();
		}

	private:
		std::string               target;
		std::vector<TabAnimation> tabs;
		Clock::time_point         expires{};
	};
}
