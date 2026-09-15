#include "Configuration.h"
#include "FadeOverlay.h"
#include "Localization.h"
#include "MapFade.h"
#include "MenuAdapters.h"
#include "MenuController.h"
#include "MenuFade.h"
#include "MenuTransitions.h"
#include "NavbarInput.h"
#include "NavbarResources.h"
#include "NavigationBridge.h"
#include "ScaleformAPI.h"
#include "SkillsMenuHooks.h"

SKSE_PLUGIN_VERSION = []() {
	SKSE::PluginVersionData version;
	version.PluginName(Plugin::NAME);
	version.PluginVersion(Plugin::VERSION);
	version.AuthorName(Plugin::AUTHOR);
	version.UsesAddressLibrary();
	version.UsesUpdatedStructs();
	return version;
}();

// Pre-AE SKSE uses Query; newer loaders use the version export above.
SKSE_PLUGIN_QUERY(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	if (!a_skse || !a_info || a_skse->IsEditor()) {
		return false;
	}
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION.pack();
	return true;
}

namespace
{
	void OnSKSEMessage(SKSE::MessagingInterface::Message* a_message)
	{
		if (a_message && (a_message->type == SKSE::MessagingInterface::kPreLoadGame ||
							 a_message->type == SKSE::MessagingInterface::kNewGame)) {
			Navbar::Input::CancelSelection();
			Navbar::MenuFade::End();
			Navbar::MenuController::GetSingleton()->CancelQueuedCycles();
			Navbar::Menus::Reset();
		}
		if (a_message && a_message->type == SKSE::MessagingInterface::kDataLoaded) {
			Navbar::Localization::Load();
			Navbar::Resources::Install();
			Navbar::SkillsMenuHooks::Install();
			Navbar::Input::Install();
			Navbar::Transitions::Install();
			Navbar::MapFade::Install();
			Navbar::FadeOverlay::Install();
			Navbar::MenuController::GetSingleton()->Install();
		}
	}
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse, { .log = true, .trampoline = true, .trampolineSize = 128 });
	spdlog::set_pattern("[%H:%M:%S:%e] [%l] %v"s);
	spdlog::flush_on(spdlog::level::info);

#ifndef NDEBUG
	spdlog::set_level(spdlog::level::trace);
#else
	spdlog::set_level(spdlog::level::info);
#endif

	Navbar::Configuration::Load();
	const auto scaleform = SKSE::GetScaleformInterface();
	const auto messaging = SKSE::GetMessagingInterface();
	if (!scaleform || !scaleform->Register(Navbar::ScaleformAPI::Register, "SkyUINavbar")) {
		SKSE::log::critical("Could not register the SkyUINavbar Scaleform API");
		return false;
	}
	if (!messaging || !messaging->RegisterListener(OnSKSEMessage)) {
		SKSE::log::critical("Could not register the SKSE message listener");
		return false;
	}

	SKSE::log::info("SkyUI Navbar {} loaded for runtime {}", Plugin::VERSION.string(), a_skse->RuntimeVersion().string());
	return true;
}
