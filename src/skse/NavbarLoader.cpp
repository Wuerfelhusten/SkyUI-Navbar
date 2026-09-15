#include "NavbarLoader.h"
#include "Configuration.h"
#include "MenuAdapters.h"
#include "MovieAssetPath.h"
#include "NavbarInitialization.h"
#include "NavbarResourcePath.h"
#include "NavbarResources.h"
#include "NavigationBridge.h"

namespace Navbar::Loader
{
	namespace
	{
		constexpr auto kInstance = "NavBarForSkyUI";

		void RemoveClip(RE::GFxValue& a_root, const char* a_name)
		{
			RE::GFxValue holder, content, widget, listener, loader;
			if (!a_root.GetMember(a_name, &holder) || !holder.IsDisplayObject()) {
				return;
			}
			if (holder.GetMember("listener", &listener) && listener.IsObject()) {
				listener.SetMember("cancelled", RE::GFxValue(true));
				if (holder.GetMember("loader", &loader) && loader.IsObject()) {
					loader.Invoke("removeListener", nullptr, &listener, 1);
				}
				listener.DeleteMember("holder");
			}
			if (holder.GetMember("content", &content) && content.IsDisplayObject() &&
				content.GetMember("navbar", &widget) && widget.IsDisplayObject()) {
				widget.Invoke("detachHints");
			}
			holder.Invoke("removeMovieClip");
		}

		void MarkFailed(RE::GFxValue& a_listener)
		{
			RE::GFxValue holder;
			if (a_listener.GetMember("holder", &holder) && holder.IsDisplayObject()) {
				holder.SetMember("loadFailed", RE::GFxValue(true));
			}
		}

		class LoadEvent final : public RE::GFxFunctionHandler
		{
		public:
			explicit LoadEvent(const char* a_event) : event(a_event) {}

			void Call(Params& a_params) override
			{
				if (a_params.retVal) {
					a_params.retVal->SetUndefined();
				}
				RE::GFxValue cancelled;
				if (!a_params.thisPtr || (a_params.thisPtr->GetMember("cancelled", &cancelled) &&
											 cancelled.IsBool() && cancelled.GetBool())) {
					return;
				}
				RE::GFxValue name;
				const char*  menu = "unknown";
				if (a_params.thisPtr->GetMember("menuName", &name) && name.IsString()) {
					menu = name.GetString();
				}
				SKSE::log::info("Navbar loader {} in {}", event, menu);
				if (event == "onLoadError"sv) {
					MarkFailed(*a_params.thisPtr);
					RE::GFxValue path;
					a_params.thisPtr->GetMember("loadPath", &path);
					if (a_params.argCount > 1 && a_params.args[1].IsString()) {
						SKSE::log::error("Navbar load error in {}: {} ({})", menu, a_params.args[1].GetString(),
							path.IsString() ? path.GetString() : "unknown path");
					}
					return;
				}
				if (event != "onLoadInit"sv || a_params.argCount < 1 || !a_params.args[0].IsDisplayObject()) {
					return;
				}
				const auto index = Configuration::Get().Find(menu);
				if (index < 0 || !Configuration::Get().menus[index].showNavbar ||
					!Menus::IsOpen(menu) || Menus::Movie(menu).get() != a_params.movie) {
					return;  // The async load's host was closed, replaced or disabled.
				}

				RE::GFxValue widget;
				if (!a_params.args[0].GetMember("navbar", &widget) || !widget.IsDisplayObject()) {
					MarkFailed(*a_params.thisPtr);
					SKSE::log::error("Navbar SWF initialized but exported navbar instance is missing");
					return;
				}

				auto view = static_cast<RE::GFxMovieView*>(a_params.movie);
				if (!InitializeWidget(view, menu, widget)) {
					MarkFailed(*a_params.thisPtr);
					return;
				}

				// Show the holder only after initialization has completed.
				RE::GFxValue root, holder;
				if (!view->GetVariable(&root, "_root") || !root.IsDisplayObject() ||
					!a_params.thisPtr->GetMember("holder", &holder) || !holder.IsDisplayObject()) {
					return;
				}
				holder.SetMember("_visible", RE::GFxValue(true));
				Navigation::Configure(view, widget);
			}

		private:
			const char* event;
		};
	}

	void Load(RE::GFxMovieView* a_movie, const char* a_menuName)
	{
		RE::GFxValue root;
		if (!a_movie || !a_movie->GetVariable(&root, "_root") || !root.IsDisplayObject()) {
			SKSE::log::error("Navbar: no root in {}", a_menuName);
			return;
		}

		RemoveClip(root, kInstance);
		RE::GFxValue nextDepth;
		std::int32_t depth = 10000;
		if (root.Invoke("getNextHighestDepth", &nextDepth) && nextDepth.IsNumber()) {
			depth = (std::max)(depth, static_cast<std::int32_t>(nextDepth.GetNumber()));
		}
		RE::GFxValue holder;
		RE::GFxValue content;
		if (!root.CreateEmptyMovieClip(&holder, kInstance, depth) ||
			!holder.CreateEmptyMovieClip(&content, "content", 1)) {
			SKSE::log::error("Navbar: cannot create load holder in {}", a_menuName);
			return;
		}
		holder.SetMember("_visible", RE::GFxValue(false));
		// Fail closed until loadClip has accepted the request. Explicit failure
		// lets the transition reveal the owner's UI without waiting five seconds.
		holder.SetMember("loadFailed", RE::GFxValue(true));
		const auto        definition = a_movie->GetMovieDef();
		const auto        url = definition ? definition->GetFileURL() : nullptr;
		const std::string hostURL = url ? url : "";
		// Keep the already working root-level route. Nested hosts use our own
		// namespace because the ../ route still fails in Skyrim's file opener.
		auto path = NavbarMoviePath(hostURL);
		if (path && *path != "NavBar.swf" && Resources::Installed()) {
			path = kNavbarResourceURL;
		}
		if (!path) {
			SKSE::log::error("Navbar: cannot resolve Interface asset relative to host URL '{}' in {}", hostURL, a_menuName);
			return;
		}
		// The holder persists while loadClip replaces only its content child.
		RE::GFxValue loader;
		RE::GFxValue listener;
		a_movie->CreateObject(&loader, "MovieClipLoader");
		a_movie->CreateObject(&listener);
		if (!loader.IsObject() || !listener.IsObject()) {
			SKSE::log::error("Navbar: MovieClipLoader creation failed");
			return;
		}
		RE::GFxValue menuName;
		a_movie->CreateString(&menuName, a_menuName);
		listener.SetMember("menuName", menuName);
		RE::GFxValue loadPath;
		a_movie->CreateString(&loadPath, path->c_str());
		listener.SetMember("loadPath", loadPath);
		listener.SetMember("holder", holder);
		for (const auto event : { "onLoadStart", "onLoadComplete", "onLoadInit", "onLoadError" }) {
			RE::GFxValue                     function;
			RE::GPtr<RE::GFxFunctionHandler> handler(new LoadEvent(event));
			handler->Release();  // GPtr retains its own reference; release the allocation's reference.
			a_movie->CreateFunction(&function, handler.get());
			listener.SetMember(event, function);
		}
		holder.SetMember("loader", loader);
		holder.SetMember("listener", listener);
		const std::array<RE::GFxValue, 1> listeners{ listener };
		RE::GFxValue                      added;
		if (!loader.Invoke("addListener", &added, listeners.data(), listeners.size())) {
			SKSE::log::error("Navbar: could not attach MovieClipLoader listener");
			return;
		}
		const std::array<RE::GFxValue, 2> args{ loadPath, content };
		RE::GFxValue                      accepted;
		holder.SetMember("loadFailed", RE::GFxValue(false));
		if (!loader.Invoke("loadClip", &accepted, args.data(), args.size()) ||
			!accepted.IsBool() || !accepted.GetBool()) {
			holder.SetMember("loadFailed", RE::GFxValue(true));
			SKSE::log::error("Navbar: loadClip rejected {} in {}", *path, a_menuName);
			return;
		}
		SKSE::log::info("Navbar loadClip accepted {} in {} at depth {} (host URL '{}'); waiting for loader events",
			*path, a_menuName, depth, hostURL);
	}

	bool Failed(RE::GFxMovieView* a_movie)
	{
		RE::GFxValue failed;
		return a_movie && a_movie->GetVariable(&failed, "_root.NavBarForSkyUI.loadFailed") && failed.IsBool() && failed.GetBool();
	}

}
