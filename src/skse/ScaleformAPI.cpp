#include "ScaleformAPI.h"

#include "Localization.h"
#include "MenuController.h"
#include "MenuFade.h"
#include "NavigationBridge.h"

namespace Navbar::ScaleformAPI
{
	namespace
	{
		class Translate final : public RE::GFxFunctionHandler
		{
		public:
			void Call(Params& a_params) override
			{
				if (!a_params.retVal) {
					return;
				}
				a_params.retVal->SetUndefined();
				if (a_params.argCount != 1 || !a_params.args[0].IsString()) {
					return;
				}
				const auto text = Localization::Get(a_params.args[0].GetString());
				a_params.movie->CreateString(a_params.retVal, text.c_str());
			}
		};

		class GetCurrentMenu final : public RE::GFxFunctionHandler
		{
		public:
			void Call(Params& a_params) override
			{
				a_params.retVal->SetString(MenuController::GetSingleton()->GetCurrentMenuName());
			}
		};

		class SwitchTo final : public RE::GFxFunctionHandler
		{
		public:
			void Call(Params& a_params) override
			{
				RE::GFxValue source;
				const bool   accepted = a_params.argCount == 1 &&
				                        a_params.args[0].IsString() &&
				                        a_params.thisPtr && a_params.thisPtr->GetMember("menuName", &source) &&
				                        source.IsString() &&
				                        MenuController::GetSingleton()->RequestSwitch(source.GetString(), a_params.args[0].GetString());
				if (source.IsString() && a_params.argCount == 1 && a_params.args[0].IsString()) {
					SKSE::log::info("Navbar click {} -> {}: {}", source.GetString(),
						a_params.args[0].GetString(), accepted ? "accepted" : "rejected by navigation guard");
				}
				a_params.retVal->SetBoolean(accepted);
			}
		};

		class NextMenu final : public RE::GFxFunctionHandler
		{
		public:
			void Call(Params& a_params) override
			{
				RE::GFxValue source;
				a_params.retVal->SetBoolean(a_params.thisPtr && a_params.thisPtr->GetMember("menuName", &source) &&
											source.IsString() && MenuController::GetSingleton()->RequestNext(source.GetString()));
			}
		};

		class SetCycleKeys final : public RE::GFxFunctionHandler
		{
		public:
			void Call(Params& a_params) override
			{
				a_params.retVal->SetBoolean((a_params.argCount == 2 || (a_params.argCount == 3 && a_params.args[2].IsBool())) &&
											a_params.args[0].IsNumber() && a_params.args[1].IsNumber() &&
											Navigation::SetKeys(a_params.args[0].GetNumber(), a_params.args[1].GetNumber(),
												a_params.argCount == 3 && a_params.args[2].GetBool() ? 1 : 2));
			}
		};

		class Ready final : public RE::GFxFunctionHandler
		{
		public:
			void Call(Params& a_params) override
			{
				const auto phase = a_params.argCount > 0 && a_params.args[0].IsString() ?
				                       a_params.args[0].GetString() :
				                       "unknown";
				SKSE::log::info("Navbar ActionScript ready: {}", phase);
				a_params.retVal->SetBoolean(true);
			}
		};

		template <class Handler>
		void AddFunction(RE::GFxMovie* a_view, RE::GFxValue* a_root, const char* a_name)
		{
			RE::GFxValue                     function;
			RE::GPtr<RE::GFxFunctionHandler> handler(new Handler());
			handler->Release();  // GPtr retains its own reference; release the allocation's reference.
			a_view->CreateFunction(std::addressof(function), handler.get());
			a_root->SetMember(a_name, function);
		}
	}

	void Populate(RE::GFxMovie* a_view, RE::GFxValue* a_root)
	{
		AddFunction<Translate>(a_view, a_root, "Translate");
		AddFunction<GetCurrentMenu>(a_view, a_root, "GetCurrentMenu");
		AddFunction<SwitchTo>(a_view, a_root, "SwitchTo");
		AddFunction<NextMenu>(a_view, a_root, "NextMenu");
		AddFunction<SetCycleKeys>(a_view, a_root, "SetCycleKeys");
		AddFunction<Ready>(a_view, a_root, "Ready");
	}

	bool Register(RE::GFxMovieView* a_view, RE::GFxValue* a_root)
	{
		MenuFade::OnMovieCreated(a_view);
		Populate(a_view, a_root);
		return true;
	}
}
