#include "NavigationBridge.h"
#include <cmath>

namespace Navbar::Navigation
{
	namespace
	{
		std::atomic<std::uint32_t> pcKey{ 56 }, gamepadKey{ 271 };
		unsigned                   bindingPriority = 0;  // Defaults < SkyUI config.txt < live SkyUI bindings.
	}

	bool SetKeys(double a_pc, double a_gamepad, unsigned a_priority)
	{
		if (!std::isfinite(a_pc) || !std::isfinite(a_gamepad) || a_pc != std::floor(a_pc) || a_gamepad != std::floor(a_gamepad) ||
			a_pc < 1 || a_pc > 263 || a_pc == 255 || a_gamepad < 266 || a_gamepad > 281 || a_priority < bindingPriority) {
			return false;
		}
		bindingPriority = a_priority;
		pcKey.store(static_cast<std::uint32_t>(a_pc));
		gamepadKey.store(static_cast<std::uint32_t>(a_gamepad));
		return true;
	}

	std::uint32_t Key(RE::INPUT_DEVICE a_device)
	{
		return a_device == RE::INPUT_DEVICE::kGamepad ? gamepadKey.load() : pcKey.load();
	}

	void Configure(RE::GFxMovieView* a_movie, RE::GFxValue& a_widget)
	{
		RE::GFxValue root;
		if (!a_movie->GetVariable(&root, "_root")) {
			return;
		}
		// Adapter contracts belong to the compiled UI, not an editable SKSE config.
		const std::array<RE::GFxValue, 2> args{ RE::GFxValue(), root };
		a_widget.Invoke("configureNavigation", nullptr, args.data(), args.size());
		Refresh(a_movie, a_widget);
	}

	void Refresh(RE::GFxMovieView*, RE::GFxValue& a_widget)
	{
		const auto                        input = RE::BSInputDeviceManager::GetSingleton();
		const bool                        gamepad = input && input->IsGamepadEnabled();
		const std::array<RE::GFxValue, 3> args{ RE::GFxValue(static_cast<double>(pcKey.load())),
			RE::GFxValue(static_cast<double>(gamepadKey.load())), RE::GFxValue(gamepad) };
		a_widget.Invoke("updateNavigation", nullptr, args.data(), args.size());
	}
}
