#include "../../commandmanager.h"

#include "../../configmanager.h"

class CraftingPullFromContainers;
namespace LinkStorage {
	void Install() {

#define cmd                                                                                     \
					const std::vector<std::string> &args, RE::Script *a_script, RE::ScriptCompiler *a_compiler, \
						RE::COMPILER_NAME a_name, RE::TESObjectREFR *a_targetRef
#define PRINT RE::ConsoleLog::GetSingleton()->Print

		CommandManager::Register("AddLink", [](cmd) {
			RE::TESObjectREFR* cont = nullptr;

			if (args.size() == 1) {
				auto&& ID = std::stoll(args[0], nullptr, 16);
				auto&& form = RE::TESObjectREFR::LookupByID(ID);
				if (form && form->Is(RE::FormType::Reference) && form->As<RE::TESObjectREFR>()->GetBaseObject()->Is(RE::FormType::Container)) cont = form->As<RE::TESObjectREFR>();
			}
			else if (a_targetRef) cont = a_targetRef;
			else {
				auto&& ui = RE::UI::GetSingleton();
				auto&& menu = ui ? ui->GetMenu<RE::ContainerMenu>() : nullptr;
				auto&& handle = menu ? menu->GetTargetRefHandle() : 0;
				auto&& container = handle ? RE::TESObjectREFR::LookupByHandle(handle) : nullptr;

				if (container) {
					cont = container.get();
					//PRINT("Ref:%d", container->_refCount);
				}
			}

			if (!cont) return;

			if (cont->GetBaseObject()->Is(RE::FormType::Container)) {
				if (cont->IsAnOwner(RE::PlayerCharacter::GetSingleton(), true, false)) {

					ConfigManager::getInstance().AddToFormList("PermaLinks", CraftingPullFromContainers::permaLinks, cont);

					PRINT("Added 0x%X", cont->formID);
				}
				else {
					PRINT("Not an owner of this container!");
				}
			}
			});
		CommandManager::Register("ListLink", [](cmd) {
			for (auto&& it : CraftingPullFromContainers::permaLinks) {
				PRINT("0x%X", it.Get());
			}
			});
		CommandManager::Register("RemoveLink", [](cmd) {
			if (args.size() == 1) {
				auto&& ID = std::stoll(args[0], nullptr, 16);
				
				if (!ConfigManager::getInstance().RemoveFromFormList("PermaLinks", CraftingPullFromContainers::permaLinks, ID)) {
					PRINT("Not Found, check log");
				}

				CraftingPullFromContainers::usedFurniture.reset();
			}
			else {
				PRINT("Argument must be one ref id");
			}
			});

		CommandManager::Register("SetPersistent", [](cmd) {
			if (args.size() != 1) {
				PRINT("SetPersistent <true/false>");
				return;
			}

			using PromoteReference_t = bool(*)(void* manager, RE::TESObjectREFR*, RE::TESForm* owner);
			static REL::Relocation<PromoteReference_t>PromoteReference{ RELOCATION_ID(15157, 15330) };

			using DemoteReference_t = bool(*)(void* manager, RE::TESObjectREFR*, RE::TESForm* owner, bool a_allowForActors);
			static REL::Relocation<DemoteReference_t>DemoteReference{ RELOCATION_ID(15158, 15331) };

			bool Switch = args[0] == "true";

			if (Switch) {
				PromoteReference(nullptr, a_targetRef, RE::PlayerCharacter::GetSingleton()->GetBaseObject());
			}
			else DemoteReference(nullptr, a_targetRef, RE::PlayerCharacter::GetSingleton()->GetBaseObject(), true);

			PRINT("Set %X to %d", a_targetRef->formID, Switch);
			});

		CommandManager::Register("OpenContainer", [](cmd) {
			if (args.size() != 1) {
				PRINT("OpenContainer <RefID>");
				return;
			}
			
			RE::TESObjectREFR* cont = nullptr;

			auto&& ID = std::stoll(args[0], nullptr, 16);
			auto&& form = RE::TESObjectREFR::LookupByID(ID);
			if (form) if (form->Is(RE::FormType::ActorCharacter) || (form->Is(RE::FormType::Reference) && form->As<RE::TESObjectREFR>()->GetBaseObject()->Is(RE::FormType::Container))) cont = form->As<RE::TESObjectREFR>();
			
			using OpenContainer_t = void(*)(RE::TESObjectREFR*, int opening_type);
			static REL::Relocation<OpenContainer_t>OpenContainer{ RELOCATION_ID(50211, 51140) };

			if (cont) OpenContainer(cont, 0);
			else {
				PRINT("Cannot find ref");
			}
			});
	}
}