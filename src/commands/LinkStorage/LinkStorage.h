#include "../../commandmanager.h"

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
					CraftingPullFromContainers::permaLinks.push_back(cont->formID);

					auto orig = CraftingPullFromContainers::cfg["CraftingPullFromContainers"]["PermaLinks"];

					if(orig == "") orig += std::format("0x{:X}", cont->formID);
					else orig += std::format(",0x{:X}", cont->formID);

					CraftingPullFromContainers::cfg.modify("CraftingPullFromContainers", "PermaLinks", orig);
					PRINT("Added 0x%X", cont->formID);
				}
				else {
					PRINT("Not an owner of this container!");
				}
			}
			});
		CommandManager::Register("ListLink", [](cmd) {
			for (auto&& it : CraftingPullFromContainers::permaLinks) {
				PRINT("0x%X", it);
			}
			});
		CommandManager::Register("RemoveLink", [](cmd) {
			if (args.size() == 1) {
				auto&& ID = std::stoll(args[0], nullptr, 16);
				bool found = false;
				CraftingPullFromContainers::permaLinks.erase(std::remove_if(CraftingPullFromContainers::permaLinks.begin(), CraftingPullFromContainers::permaLinks.end(), [&](RE::FormID id) {
					if (id == ID) {
						found = true;
						return id == ID;
					}
					return false;
					}), CraftingPullFromContainers::permaLinks.end());
				if (found) {
					std::string str = std::format("0x{:X}", ID);
					auto orig = CraftingPullFromContainers::cfg["CraftingPullFromContainers"]["PermaLinks"];
					auto&& idx = orig.find(str);
					if (idx != std::string::npos && idx <= orig.size()) {
						if (idx == 0) orig.erase(idx, idx + str.size());
						else orig.erase(idx - 1, idx + str.size() - 1);
					}

					CraftingPullFromContainers::cfg.modify("CraftingPullFromContainers", "PermaLinks", orig);
				}
				else {
					PRINT("Not Found");
				}
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

			using SetPersistent_t = void(*)(RE::TESObjectREFR*, bool);
			REL::Relocation<SetPersistent_t>SetPersistent{ RELOCATION_ID(19182, 19597) };

			bool Switch = args[0] == "true";

			if (Switch) {
				auto cell = a_targetRef->GetSaveParentCell();

				if (cell->GetRuntimeData().worldSpace == nullptr) {
					cell->GetRuntimeData().worldSpace = RE::TESForm::LookupByID<RE::TESWorldSpace>(0x0000003C);
					cell->formFlags |= RE::TESObjectCELL::RecordFlags::kPersistent;
					cell->cellFlags.reset(RE::TESObjectCELL::Flag::kIsInteriorCell);

					SetPersistent(a_targetRef, Switch);

					cell->cellFlags.set(RE::TESObjectCELL::Flag::kIsInteriorCell);
					cell->formFlags &= ~RE::TESObjectCELL::RecordFlags::kPersistent;
					cell->GetRuntimeData().worldSpace = nullptr;
				}
				else if ((cell->formFlags & RE::TESObjectCELL::RecordFlags::kPersistent) == 0) {
					cell->formFlags |= RE::TESObjectCELL::RecordFlags::kPersistent;

					SetPersistent(a_targetRef, Switch);

					cell->formFlags &= ~RE::TESObjectCELL::RecordFlags::kPersistent;
				}

				SetPersistent(a_targetRef, Switch);
			}
			else SetPersistent(a_targetRef, Switch);

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
			if (form && form->Is(RE::FormType::Reference) && form->As<RE::TESObjectREFR>()->GetBaseObject()->Is(RE::FormType::Container)) cont = form->As<RE::TESObjectREFR>();
			
			if (cont) cont->ActivateRef(RE::PlayerCharacter::GetSingleton(), 0, nullptr, cont->extraList.GetCount(), false);
			else {
				PRINT("Cannot find ref container");
			}
			});
	}
}