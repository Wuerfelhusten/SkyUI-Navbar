#pragma once
#include <algorithm>

namespace Navbar
{
	// Apply only for the duration of one draw; readiness/host animation always
	// observes its own alpha. Read returns optional<double>, write returns bool.
	template <class Read, class Write, class Draw>
	void RenderWithMenuOpacity(double a_factor, Read&& a_read, Write&& a_write, Draw&& a_draw)
	{
		if (a_factor >= 1.0) {
			a_draw();
			return;
		}
		const auto saved = a_read();
		if (!saved || !a_write(*saved * std::clamp(a_factor, 0.0, 1.0))) {
			a_draw();
			return;
		}
		struct Restore
		{
			Write& write;
			double alpha;
			~Restore() { write(alpha); }
		} restore{ a_write, *saved };
		a_draw();
	}
}
