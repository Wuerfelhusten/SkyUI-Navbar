#pragma once
#include <utility>
#include <vector>

namespace Navbar
{
	// MenuControls sees a filtered chain; other event sinks still see the original.
	template <class Event>
	class FilteredEvents
	{
	public:
		template <class Predicate>
		FilteredEvents(Event* a_first, Predicate a_consume)
		{
			for (auto event = a_first; event; event = event->next) {
				links.emplace_back(event, event->next);
			}
			Event** tail = &head;
			for (const auto& [event, next] : links) {
				if (!a_consume(event)) {
					*tail = event;
					tail = &event->next;
				}
			}
			*tail = nullptr;
		}
		~FilteredEvents()
		{
			for (const auto& [event, next] : links) {
				event->next = next;
			}
		}
		FilteredEvents(const FilteredEvents&) = delete;
		FilteredEvents& operator=(const FilteredEvents&) = delete;
		Event* const*   Head() const { return &head; }

	private:
		Event*                                 head = nullptr;
		std::vector<std::pair<Event*, Event*>> links;
	};
}
