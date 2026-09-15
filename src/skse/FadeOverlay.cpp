#include "FadeOverlay.h"
#include "MenuController.h"

namespace Navbar::FadeOverlay
{
	namespace
	{
		bool pauseNextOpen = true;
		class Overlay final : public RE::IMenu
		{
		public:
			Overlay()
			{
				depthPriority = 100;
				menuFlags.set(RE::UI_MENU_FLAGS::kUsesMenuContext,
					RE::UI_MENU_FLAGS::kRequiresUpdate, RE::UI_MENU_FLAGS::kTopmostRenderedMenu,
					RE::UI_MENU_FLAGS::kSkipRenderDuringFreezeFrameScreenshot);
				if (pauseNextOpen) {
					menuFlags.set(RE::UI_MENU_FLAGS::kPausesGame);
				}
				const auto manager = RE::BSScaleformManager::GetSingleton();
				if (!manager || !manager->LoadMovie(this, uiMovie, "NavBar")) {
					SKSE::log::error("Navbar transition overlay could not load NavBar.swf");
				}
			}
			static RE::IMenu*      Create() { return new Overlay(); }
			RE::UI_MESSAGE_RESULTS ProcessMessage(RE::UIMessage& a_message) override
			{
				if (a_message.type == RE::UI_MESSAGE_TYPE::kScaleformEvent || a_message.type == RE::UI_MESSAGE_TYPE::kUserEvent) {
					return RE::UI_MESSAGE_RESULTS::kHandled;
				}
				return RE::IMenu::ProcessMessage(a_message);
			}
			void AdvanceMovie(float a_interval, std::uint32_t a_time) override
			{
				RE::IMenu::AdvanceMovie(a_interval, a_time);
				MenuController::GetSingleton()->ScheduleTransitionTick();
			}
		};
	}
	void Install() { RE::UI::GetSingleton()->Register(Name, Overlay::Create); }
	void Prepare(bool a_pauseGame) { pauseNextOpen = a_pauseGame; }
	bool Ready()
	{
		const auto   ui = RE::UI::GetSingleton();
		const auto   movie = ui ? ui->GetMovieView(Name) : nullptr;
		RE::GFxValue widget;
		return movie && movie->GetVariable(&widget, "_root.navbar") && widget.IsDisplayObject();
	}
	void SetAlpha(double a_alpha)
	{
		const auto ui = RE::UI::GetSingleton();
		const auto movie = ui ? ui->GetMovieView(Name) : nullptr;
		if (!movie) {
			return;
		}
		const auto                        rect = movie->GetVisibleFrameRect();
		const std::array<RE::GFxValue, 5> args{ RE::GFxValue(a_alpha * 100), RE::GFxValue(rect.left),
			RE::GFxValue(rect.top), RE::GFxValue(rect.right), RE::GFxValue(rect.bottom) };
		movie->Invoke("_root.navbar.setTransitionCover", nullptr, args.data(), static_cast<std::uint32_t>(args.size()));
	}
}
