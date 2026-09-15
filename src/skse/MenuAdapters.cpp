#include "MenuAdapters.h"
#include "FadeOverlay.h"
#include "SkillsBackground.h"
#ifdef PlaySound
#	undef PlaySound
#endif

namespace Navbar::Menus
{
	namespace
	{
		std::string bestiaryEntry;
		bool        SendEvent(const char* a_name)
		{
			const auto source = SKSE::GetModCallbackEventSource();
			if (!source) {
				return false;
			}
			const SKSE::ModCallbackEvent event{ RE::BSFixedString(a_name), RE::BSFixedString(), 0.0f, nullptr };
			source->SendEvent(&event);
			return true;
		}
		bool HasAsset(const char* a_path)
		{
			RE::BSResourceNiBinaryStream stream(a_path);
			return stream.good();
		}
	}

	bool Matches(std::string_view a_name, RE::IMenu* a_menu)
	{
		const auto ui = RE::UI::GetSingleton();
		const auto route = RouteFor(a_name);
		if (!ui || !a_menu || ui->GetMenu(route.nativeName).get() != a_menu) {
			return false;
		}
		RE::GFxValue root;
		return a_menu->uiMovie && a_menu->uiMovie->GetVariable(&root, route.root) && root.IsDisplayObject();
	}
	RE::GPtr<RE::GFxMovieView> Movie(std::string_view a_name)
	{
		const auto ui = RE::UI::GetSingleton();
		const auto route = RouteFor(a_name);
		const auto menu = ui ? ui->GetMenu(route.nativeName) : nullptr;
		return Matches(a_name, menu.get()) ? menu->uiMovie : nullptr;
	}
	bool IsOpen(std::string_view a_name)
	{
		const auto ui = RE::UI::GetSingleton();
		const auto route = RouteFor(a_name);
		return ui && ui->IsMenuOpen(route.nativeName) &&
		       (route.protocol != MenuProtocol::kMetaSkills || Movie(a_name));
	}
	bool Available(std::string_view a_name)
	{
		const auto ui = RE::UI::GetSingleton();
		if (!ui) {
			return false;
		}
		const auto route = RouteFor(a_name);
		const auto it = ui->menuMap.find(RE::BSFixedString(route.nativeName));
		if (it == ui->menuMap.end() || (!it->second.create && !it->second.menu)) {
			return false;
		}
		if (route.protocol == MenuProtocol::kMetaSkills) {
			return HasAsset("Interface/MetaSkillsMenu/CustomMetaMenu.swf") && HasAsset("Scripts/metaSkillMenuScript.pex");
		}
		if (route.protocol == MenuProtocol::kAchievements) {
			const auto hud = ui->GetMenu(RE::HUDMenu::MENU_NAME);
			return hud && hud->uiMovie;  // The owner's open-event handler dereferences the HUD movie.
		}
		return true;
	}
	bool CanLeave(std::string_view a_name)
	{
		if (!IsOpen(a_name)) {
			return false;
		}
		if (RouteFor(a_name).protocol != MenuProtocol::kWait) {
			return true;
		}
		const auto menu = RE::UI::GetSingleton()->GetMenu<RE::SleepWaitMenu>();
		return menu && !menu->GetRuntimeData().isActive && !menu->GetRuntimeData().isSleeping;
	}
	bool ReadyForReveal(std::string_view a_name)
	{
		const auto route = RouteFor(a_name);
		if (route.protocol == MenuProtocol::kNative) {
			return true;
		}
		const auto movie = Movie(a_name);
		if (!movie) {
			return false;
		}
		const std::string path(route.root);
		for (std::size_t end = path.find('.');; end = path.find('.', end + 1)) {
			RE::GFxValue              clip;
			RE::GFxValue::DisplayInfo info;
			if (!movie->GetVariable(&clip, path.substr(0, end).c_str()) || !clip.IsDisplayObject() ||
				!clip.GetDisplayInfo(&info) || !info.GetVisible() || info.GetAlpha() < 99.0) {
				return false;
			}
			if (end == std::string::npos) {
				break;
			}
		}
		return true;
	}
	bool HasBlocker(std::string_view a_host, bool a_duringSwitch)
	{
		const auto ui = RE::UI::GetSingleton();
		if (!ui) {
			return true;
		}
		for (auto i = ui->menuStack.size(); i > 0; --i) {
			const auto menu = ui->menuStack[i - 1];
			if (!menu) {
				continue;
			}
			if (a_duringSwitch && (ui->GetMenu(FadeOverlay::Name).get() == menu.get() ||
									  ui->GetMenu(RE::TweenMenu::MENU_NAME).get() == menu.get())) {
				continue;
			}
			const bool ours = !a_host.empty() && Matches(a_host, menu.get());
			if (ours) {
				return false;
			}
			if (BlocksNavigation(false, menu->Modal(), menu->UsesCursor(), menu->UsesMenuContext())) {
				return true;
			}
		}
		return !a_duringSwitch;
	}
	bool Open(std::string_view a_name)
	{
		if (!Available(a_name) || IsOpen(a_name)) {
			return false;
		}
		const auto route = RouteFor(a_name);
		if (route.protocol == MenuProtocol::kMetaSkills && RE::UI::GetSingleton()->IsMenuOpen("CustomMenu")) {
			return false;
		}
		if (route.openEvent) {
			return SendEvent(route.openEvent);
		}
		if (route.protocol == MenuProtocol::kBestiary) {
			const auto                                               vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
			return vm && vm->DispatchStaticCall("Bestiary", "Open", RE::MakeFunctionArguments(), callback);
		}
		if (route.protocol == MenuProtocol::kWait) {
			RE::SleepWaitMenu::ToggleOpenMenu(false);
			return true;
		}
		const auto queue = RE::UIMessageQueue::GetSingleton();
		if (!queue) {
			return false;
		}
		if (route.nativeName == RE::StatsMenu::MENU_NAME) {
			// Queue the equal-depth FaderMenu before StatsMenu, not from Stats Show:
			// first creation after Stats puts its black rectangle above the Skills UI.
			SkillsBackground::Prepare();
		}
		queue->AddMessage(route.nativeName, RE::UI_MESSAGE_TYPE::kShow, nullptr);
		return true;
	}
	bool Close(std::string_view a_name)
	{
		if (!CanLeave(a_name)) {
			return false;
		}
		const auto route = RouteFor(a_name);
		const auto movie = Movie(a_name);
		const auto queue = RE::UIMessageQueue::GetSingleton();
		if (!queue) {
			return false;
		}
		if (route.closeEvent) {
			return SendEvent(route.closeEvent);
		}
		if (route.protocol == MenuProtocol::kMetaSkills) {
			return movie && movie->Invoke("_root.MetaController_mc.doClose", nullptr, nullptr, 0);
		}
		if (route.protocol == MenuProtocol::kCharacter) {
			return movie && movie->Invoke("_root.CharacterSheet_mc.CloseMenu", nullptr, nullptr, 0);
		}
		if (route.protocol == MenuProtocol::kBestiary) {
			RE::GFxValue entry;
			if (movie && movie->Invoke("_root.BestiaryMenu_mc.sendLastEntry", &entry, nullptr, 0) && entry.IsString()) {
				bestiaryEntry = entry.GetString();
			}
			// Bestiary's Hide helper has no exported API; mirror its balanced blur release.
			if (const auto blur = RE::UIBlurManager::GetSingleton()) {
				blur->DecrementBlurCount();
			}
			RE::PlaySound("UIJournalClose");
		}
		queue->AddMessage(route.nativeName, RE::UI_MESSAGE_TYPE::kHide, nullptr);
		return true;
	}
	void OnOpened(std::string_view a_name)
	{
		if (RouteFor(a_name).protocol != MenuProtocol::kBestiary || bestiaryEntry.empty()) {
			return;
		}
		if (const auto movie = Movie(a_name)) {
			const RE::GFxValue entry(bestiaryEntry.c_str());
			if (movie->Invoke("_root.BestiaryMenu_mc.getLastEntry", nullptr, &entry, 1)) {
				bestiaryEntry.clear();
			}
		}
	}
	void Reset() { bestiaryEntry.clear(); }
}
