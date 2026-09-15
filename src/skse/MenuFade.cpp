#include "MenuFade.h"
#include "MenuOpacity.h"
#include "MenuRoute.h"
#include <mutex>

namespace Navbar::MenuFade
{
	namespace
	{
		using Display = void (*)(RE::GFxMovieView*);
		std::mutex                                                 mutex;
		std::unordered_map<std::uintptr_t, std::array<Display, 2>> originals;
		RE::GPtr<RE::GFxMovieView>                                 source, target;
		std::string                                                targetName;
		bool                                                       active = false;
		double                                                     opacity = 1.0;
		thread_local RE::GFxMovieView*                             rendering = nullptr;

		template <std::size_t Index>
		struct DisplayHook
		{
			static void thunk(RE::GFxMovieView* a_this)
			{
				Display original;
				double  factor = 1.0;
				{
					std::scoped_lock lock(mutex);
					original = originals.at(*reinterpret_cast<std::uintptr_t*>(a_this))[Index];
					if (active && (source.get() == a_this || target.get() == a_this)) {
						factor = opacity;
					}
				}
				// Other movies (HUD, native FaderMenu, etc.) always pass through.
				// Pre-pass/display may nest: do not multiply the same movie twice.
				if (factor >= 1.0 || rendering == a_this) {
					original(a_this);
					return;
				}
				struct RestoreRendering
				{
					RE::GFxMovieView* previous;
					~RestoreRendering() { rendering = previous; }
				} restoreRendering{ rendering };
				rendering = a_this;
				RE::GFxValue root;
				RenderWithMenuOpacity(factor, [&]() -> std::optional<double> {
					RE::GFxValue::DisplayInfo info;
					if (!a_this->GetVariable(&root, "_root") || !root.IsDisplayObject() || !root.GetDisplayInfo(&info)) {
						return std::nullopt;
					}
					return info.GetAlpha(); }, [&](double a_alpha) {
					RE::GFxValue::DisplayInfo info;
					info.SetAlpha(a_alpha);
					return root.SetDisplayInfo(info); }, [&]() { original(a_this); });
			}
			// CommonLib GFxMovieView virtual interface, not runtime code offsets.
			static constexpr std::size_t idx = 0x26 + Index;
		};

		void InstallMovie(RE::GFxMovieView* a_movie)
		{
			if (!a_movie) {
				return;
			}
			std::scoped_lock lock(mutex);
			const auto       address = *reinterpret_cast<std::uintptr_t*>(a_movie);
			if (originals.contains(address)) {
				return;
			}
			const auto table = reinterpret_cast<Display*>(address);
			originals.emplace(address, std::array<Display, 2>{ table[0x26], table[0x27] });
			REL::Relocation<std::uintptr_t> vtable(address);
			vtable.write_vfunc(DisplayHook<0>::idx, DisplayHook<0>::thunk);
			vtable.write_vfunc(DisplayHook<1>::idx, DisplayHook<1>::thunk);
		}
	}

	void Begin(RE::GFxMovieView* a_source, std::string_view a_target)
	{
		InstallMovie(a_source);
		std::scoped_lock lock(mutex);
		source.reset(a_source);
		target.reset();
		targetName = RouteFor(a_target).nativeName;
		opacity = 1.0;
		active = true;
	}

	void Retarget(std::string_view a_target)
	{
		std::scoped_lock lock(mutex);
		if (active) {
			targetName = RouteFor(a_target).nativeName;
		}
	}

	void TrackTarget(RE::GFxMovieView* a_movie)
	{
		if (!a_movie) {
			return;
		}
		{
			std::scoped_lock lock(mutex);
			if (!active) {
				return;
			}
		}
		InstallMovie(a_movie);
		std::scoped_lock lock(mutex);
		if (active) {
			target.reset(a_movie);
		}
	}

	void OnMovieCreated(RE::GFxMovieView* a_movie)
	{
		{
			std::scoped_lock lock(mutex);
			if (!active) {
				return;
			}
		}
		InstallMovie(a_movie);
	}

	void OnMenuEvent(std::string_view a_nativeName, bool a_opening)
	{
		{
			std::scoped_lock lock(mutex);
			if (!active || !a_opening || a_nativeName != targetName) {
				return;
			}
		}
		// Capture synchronously in the native menu event, before its first draw.
		// No ActionScript is invoked here; rendering uses only retained identities.
		const auto ui = RE::UI::GetSingleton();
		const auto movie = ui ? ui->GetMovieView(a_nativeName) : nullptr;
		TrackTarget(movie.get());
	}

	void SetCoverAlpha(double a_alpha)
	{
		std::scoped_lock lock(mutex);
		opacity = 1.0 - std::clamp(a_alpha, 0.0, 1.0);
	}

	void End()
	{
		std::scoped_lock lock(mutex);
		active = false;
		opacity = 1.0;
		source.reset();
		target.reset();
		targetName.clear();
	}
}
