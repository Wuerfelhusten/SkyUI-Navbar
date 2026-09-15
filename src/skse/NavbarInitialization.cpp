#include "NavbarInitialization.h"
#include "Configuration.h"
#include "MenuController.h"
#include "ScaleformAPI.h"

namespace Navbar::Loader
{
	bool InitializeWidget(RE::GFxMovieView* a_movie, const char* a_menuName, RE::GFxValue& a_widget)
	{
		// Supply the bridge explicitly; no dependency on global AS lookup
		// or constructor timing inside a host movie.
		RE::GFxValue api;
		a_movie->CreateObject(&api);
		ScaleformAPI::Populate(a_movie, &api);
		RE::GFxValue name;
		a_movie->CreateString(&name, a_menuName);
		api.SetMember("menuName", name);
		RE::GFxValue entries;
		a_movie->CreateArray(&entries);
		const auto    animation = MenuController::GetSingleton()->TakeAnimationState(a_menuName, a_movie);
		std::uint32_t count = 0;
		for (const auto& entry : Configuration::Get().menus) {
			if (!entry.showInNavbar || !MenuController::IsRegistered(entry.menu)) {
				continue;
			}
			RE::GFxValue item, menuName;
			a_movie->CreateObject(&item);
			a_movie->CreateString(&menuName, entry.menu.c_str());
			item.SetMember("menu", menuName);
			item.SetMember("available", RE::GFxValue(true));
			for (const auto& state : animation) {
				if (state.menu != entry.menu) {
					continue;
				}
				item.SetMember("animationY", RE::GFxValue(state.position));
				item.SetMember("hoverTarget", RE::GFxValue(state.target));
				item.SetMember("isHover", RE::GFxValue(state.hover));
				break;
			}
			entries.PushBack(item);
			++count;
		}
		const auto   rect = a_movie->GetVisibleFrameRect();
		RE::GFxValue bounds;
		a_movie->CreateObject(&bounds);
		bounds.SetMember("left", RE::GFxValue(rect.left));
		bounds.SetMember("top", RE::GFxValue(rect.top));
		bounds.SetMember("right", RE::GFxValue(rect.right));
		bounds.SetMember("bottom", RE::GFxValue(rect.bottom));
		bounds.SetMember("scale", RE::GFxValue(Configuration::Get().navbarScale));
		bounds.SetMember("showButtonHints", RE::GFxValue(Configuration::Get().showButtonHints));
		const std::array<RE::GFxValue, 4> args{ api, name, entries, bounds };
		RE::GFxValue                      result;
		if (!a_widget.Invoke("initialize", &result, args.data(), args.size()) ||
			!result.IsNumber() || result.GetNumber() != count) {
			SKSE::log::error("Navbar initialize failed: class linkage or AS2 execution error");
			return false;
		}

		SKSE::log::info("Navbar initialized: {} tabs in {}; restored {} animation states", count, a_menuName, animation.size());
		return true;
	}
}
