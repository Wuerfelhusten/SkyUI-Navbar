#include "NavbarWidget.h"
#include "Configuration.h"
#include "NavigationBridge.h"
#include "SkillsInput.h"
#include <cmath>

namespace Navbar::Widget
{
	std::vector<TabAnimation> CaptureAnimationState(RE::GFxMovieView* a_movie)
	{
		std::vector<TabAnimation> result;
		RE::GFxValue              widget, snapshot;
		if (!a_movie || !a_movie->GetVariable(&widget, "_root.NavBarForSkyUI.content.navbar") ||
			!widget.IsDisplayObject() || !widget.Invoke("captureAnimationState", &snapshot) ||
			!snapshot.IsArray() || snapshot.GetArraySize() > 32) {
			return result;
		}
		for (std::uint32_t i = 0; i < snapshot.GetArraySize(); ++i) {
			RE::GFxValue item, menu, position, target, hover;
			if (!snapshot.GetElement(i, &item) || !item.IsObject() ||
				!item.GetMember("menu", &menu) || !menu.IsString() ||
				!item.GetMember("position", &position) || !position.IsNumber() ||
				!item.GetMember("target", &target) || !target.IsNumber() ||
				!item.GetMember("hover", &hover) || !hover.IsBool() ||
				!std::isfinite(position.GetNumber()) || !std::isfinite(target.GetNumber())) {
				continue;
			}
			const auto& config = Configuration::Get();
			const int   index = config.Find(menu.GetString());
			if (index < 0 || !config.menus[index].showInNavbar) {
				continue;
			}
			result.push_back({ config.menus[index].menu, std::clamp(position.GetNumber(), 0.0, 22.0),
				std::clamp(target.GetNumber(), 0.0, 22.0), hover.GetBool() });
		}
		return result;
	}

	bool SetPicker(RE::GFxMovieView* a_movie, bool a_active, const std::string& a_selected)
	{
		RE::GFxValue widget, result, menu;
		if (!a_movie || !a_movie->GetVariable(&widget, "_root.NavBarForSkyUI.content.navbar") || !widget.IsDisplayObject()) {
			return false;
		}
		a_movie->CreateString(&menu, a_selected.c_str());
		const std::array<RE::GFxValue, 2> args{ RE::GFxValue(a_active), menu };
		return widget.Invoke("setPicker", &result, args.data(), args.size()) && result.IsBool() && result.GetBool();
	}

	void BeginNavigation(RE::GFxMovieView* a_movie)
	{
		RE::GFxValue widget;
		if (a_movie && a_movie->GetVariable(&widget, "_root.NavBarForSkyUI.content.navbar") && widget.IsDisplayObject()) {
			widget.Invoke("beginNavigation", nullptr);
		}
	}

	bool HitTest(RE::GFxMovieView* a_movie, float a_screenX, float a_screenY)
	{
		if (!a_movie || !std::isfinite(a_screenX) || !std::isfinite(a_screenY)) {
			return false;
		}
		RE::GFxValue widget;
		if (!a_movie->GetVariable(&widget, "_root.NavBarForSkyUI.content.navbar") || !widget.IsDisplayObject()) {
			return false;
		}
		RE::GViewport viewport;
		a_movie->GetViewport(&viewport);
		const auto frame = a_movie->GetVisibleFrameRect();
		const auto point = ScreenToStage(a_screenX, a_screenY, static_cast<float>(viewport.left),
			static_cast<float>(viewport.top), static_cast<float>(viewport.width), static_cast<float>(viewport.height),
			frame.left, frame.top, frame.right, frame.bottom);
		if (!point) {
			return false;
		}
		const std::array<RE::GFxValue, 2> args{ RE::GFxValue(point->x), RE::GFxValue(point->y) };
		RE::GFxValue                      hit;
		return widget.Invoke("containsPoint", &hit, args.data(), args.size()) && hit.IsBool() && hit.GetBool();
	}

	void DispatchButton(RE::GFxMovieView* a_movie, std::uint32_t a_button, bool a_down, float a_screenX, float a_screenY)
	{
		RE::GFxValue widget;
		if (!a_movie || !a_movie->GetVariable(&widget, "_root.NavBarForSkyUI.content.navbar") || !widget.IsDisplayObject()) {
			return;
		}
		RE::GViewport viewport;
		a_movie->GetViewport(&viewport);
		const auto frame = a_movie->GetVisibleFrameRect();
		const auto point = ScreenToStage(a_screenX, a_screenY, static_cast<float>(viewport.left),
			static_cast<float>(viewport.top), static_cast<float>(viewport.width), static_cast<float>(viewport.height),
			frame.left, frame.top, frame.right, frame.bottom);
		if (!point) {
			return;
		}
		const std::array<RE::GFxValue, 4> args{ RE::GFxValue(a_button), RE::GFxValue(a_down),
			RE::GFxValue(point->x), RE::GFxValue(point->y) };
		widget.Invoke("nativeButton", nullptr, args.data(), args.size());
	}

	void UpdateLayout(RE::GFxMovieView* a_movie)
	{
		RE::GFxValue widget;
		if (!a_movie || !a_movie->GetVariable(&widget, "_root.NavBarForSkyUI.content.navbar") || !widget.IsDisplayObject()) {
			return;
		}
		const auto rect = a_movie->GetVisibleFrameRect();
		if (!std::isfinite(rect.left) || !std::isfinite(rect.top) ||
			!std::isfinite(rect.right) || !std::isfinite(rect.bottom) || rect.right <= rect.left) {
			return;
		}
		const std::array<RE::GFxValue, 4> args{ RE::GFxValue(rect.left), RE::GFxValue(rect.top),
			RE::GFxValue(rect.right), RE::GFxValue(rect.bottom) };
		widget.Invoke("layout", nullptr, args.data(), args.size());
		Navigation::Refresh(a_movie, widget);
		// A restored hover may precede Scaleform's first rollover in the new movie.
		// Reconcile only that inherited hover so a mouse exit cannot leave it stuck.
		const auto cursor = RE::MenuCursor::GetSingleton();
		if (cursor) {
			RE::GViewport viewport;
			a_movie->GetViewport(&viewport);
			const auto& mouse = cursor->GetRuntimeData();
			const auto  point = ScreenToStage(mouse.cursorPosX, mouse.cursorPosY,
				static_cast<float>(viewport.left), static_cast<float>(viewport.top),
				static_cast<float>(viewport.width), static_cast<float>(viewport.height),
				rect.left, rect.top, rect.right, rect.bottom);
			if (point) {
				const std::array<RE::GFxValue, 2> pointer{ RE::GFxValue(point->x), RE::GFxValue(point->y) };
				widget.Invoke("reconcileRestoredHover", nullptr, pointer.data(), pointer.size());
			}
		}
	}
}
