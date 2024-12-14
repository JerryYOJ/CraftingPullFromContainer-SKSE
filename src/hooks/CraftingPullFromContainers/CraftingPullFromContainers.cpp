#include <MinHook.h>
#include <boost/algorithm/string.hpp>

#include "CraftingPullFromContainers.h"

#undef GetObject

RE::ObjectRefHandle CraftingPullFromContainers::usedFurniture;

//Cache
std::vector<CraftingPullFromContainers::CONT> CraftingPullFromContainers::cachedContainers;
std::unordered_map<RE::TESBoundObject*, RE::InventoryEntryData*> CraftingPullFromContainers::cachedStationInventory;

//Config
std::vector<RE::FormID> CraftingPullFromContainers::permaLinks;
inicpp::IniManager CraftingPullFromContainers::cfg("Data/SKSE/Plugins/CraftingPullFromContainers.ini");
float CraftingPullFromContainers::range = 800.f;
bool CraftingPullFromContainers::IgnoreOwnership = false;

//bool GottenItemCounts = false;

void CraftingPullFromContainers::Install()
{
	REL::Relocation<std::uintptr_t> GetItemCount{ RELOCATION_ID(19274, 19700) };
	REL::Relocation<std::uintptr_t> GetInvItem{ RELOCATION_ID(19273, 19699) };
	REL::Relocation<std::uintptr_t> GetInvCount{ RELOCATION_ID(15869, 16109) };
	REL::Relocation<std::uintptr_t> SetEnchOnItem{ RELOCATION_ID(15906, 16146) };
	
	MH_CreateHook((void*)GetItemCount.address(), GetContainerItemCount, reinterpret_cast<void**>(&_GetContainerItemCount));
	MH_CreateHook((void*)GetInvItem.address(), GetInventoryItemEntryAtIdx, reinterpret_cast<void**>(&_GetInventoryItemEntryAtIdx));
	MH_CreateHook((void*)GetInvCount.address(), GetInventoryItemCount, reinterpret_cast<void**>(&_GetInventoryItemCount));
	MH_CreateHook((void*)SetEnchOnItem.address(), SetEnchantmentOnItem, reinterpret_cast<void**>(&_SetEnchantmentOnItem));

	_RemoveItem = REL::Relocation<uintptr_t>(RE::VTABLE_PlayerCharacter[0]).write_vfunc(0x56, RemoveItem);

	if (!cfg["CraftingPullFromContainers"].isKeyExist("Range")) cfg.modify("CraftingPullFromContainers", "Range", "800");
	if (!cfg["CraftingPullFromContainers"].isKeyExist("IgnoreOwnership")) cfg.modify("CraftingPullFromContainers", "IgnoreOwnership", "false", "true/false");
	if (!cfg["CraftingPullFromContainers"].isKeyExist("PermaLinks")) cfg.modify("CraftingPullFromContainers", "PermaLinks", "");
	cfg.parse();
	
	range = std::stof(cfg["CraftingPullFromContainers"]["Range"]);
	IgnoreOwnership = cfg["CraftingPullFromContainers"]["IgnoreOwnership"].contains("true");
	std::vector<std::string> IDs;
	if(!cfg["CraftingPullFromContainers"]["PermaLinks"].empty()) boost::split(IDs, cfg["CraftingPullFromContainers"]["PermaLinks"], boost::is_any_of(","));
	if(!IDs.empty()) for (auto&& it : IDs) permaLinks.push_back(std::stoll(it, nullptr, 16));

	logger::info("[LoadConfig] Range:{}  bIgnoreOwnership:{}  permaLinksCount:{}", range, IgnoreOwnership, permaLinks.size());
}

int CraftingPullFromContainers::GetContainerItemCount(RE::TESObjectREFR* cont, bool bUseMechant, bool bUnkTrue)
{
	if (!cont->IsPlayerRef() || RE::PlayerCharacter::GetSingleton()->GetOccupiedFurniture().native_handle() == 0) return _GetContainerItemCount(cont, bUseMechant, bUnkTrue);
	if (usedFurniture == RE::PlayerCharacter::GetSingleton()->GetOccupiedFurniture()) {
		int totalCount = 0;

		auto Validate = [&]() -> bool {
			if (cachedContainers.empty()) return false;

			for (auto&& it : cachedContainers) {
				if (!it.cont || it.cont.get() == nullptr) return false;
				it.itemCount = _GetContainerItemCount(it.cont.get().get(), false, true);
				totalCount += it.itemCount;
			}
			return true;
		};

		
		if (Validate()) {
			cachedStationInventory.clear();
			return totalCount;
		}
	}

	cachedStationInventory.clear();
	usedFurniture = RE::PlayerCharacter::GetSingleton()->GetOccupiedFurniture();
	cachedContainers.clear();

	auto&& player = RE::PlayerCharacter::GetSingleton();
	int totalCount = 0;
	cachedContainers.push_back({ player->CreateRefHandle(), _GetContainerItemCount(player, false, true) - 1});
	totalCount += cachedContainers.back().itemCount + 1;

	auto&& cell = player->GetParentCell();
	cell->ForEachReferenceInRange(player->GetPosition(), range, [&](RE::TESObjectREFR& obj) {
		if (obj.GetBaseObject()->Is(RE::FormType::Container)) {
			if (obj.IsAnOwner(RE::PlayerCharacter::GetSingleton(), true, false) || IgnoreOwnership) {
				if (std::find(permaLinks.begin(), permaLinks.end(), obj.formID) != permaLinks.end()) 
					return RE::BSContainer::ForEachResult::kContinue;


				cachedContainers.push_back({ obj.CreateRefHandle(), _GetContainerItemCount(&obj, false, true) - 1});
				totalCount += cachedContainers.back().itemCount + 1;
			}
		}

		return RE::BSContainer::ForEachResult::kContinue;
		});
	
	for (auto&& it : permaLinks) {
		auto&& ref = RE::TESObjectREFR::LookupByID<RE::TESObjectREFR>(it);
		if (ref && ref->GetBaseObject()->Is(RE::FormType::Container)) {
			if (ref->IsAnOwner(RE::PlayerCharacter::GetSingleton(), true, false) || IgnoreOwnership) {
				cachedContainers.push_back({ ref->CreateRefHandle(), _GetContainerItemCount(ref, false, true) - 1});
				totalCount += cachedContainers.back().itemCount + 1;

				logger::debug("Added PermaLink Container {:X}({})", ref->formID, cachedContainers.back().cont.native_handle());
			}
		}
	}

	//totalItems = totalCount;
	return totalCount;
}

RE::InventoryEntryData* CraftingPullFromContainers::GetInventoryItemEntryAtIdx(RE::TESObjectREFR* cont, int idx, bool bUseMerchant)
{
	if (!cont->IsPlayerRef() || RE::PlayerCharacter::GetSingleton()->GetOccupiedFurniture().native_handle() == 0) return _GetInventoryItemEntryAtIdx(cont, idx, bUseMerchant);

	int totalCumulation = 0;

	//if (idx < totalCumulation) {
	//	totalCumulation = 0;
	//	cachedContainers.clear();
	//}

	for (auto&& it : cachedContainers) {
		if (idx - totalCumulation <= it.itemCount) {
			auto* invEntry = _GetInventoryItemEntryAtIdx(it.cont.get().get(), idx - totalCumulation, false);
			if (invEntry == nullptr) return invEntry;
			if ((invEntry->GetObject()->IsWeapon() || invEntry->GetObject()->IsArmor()) && it.cont.get().get() != RE::PlayerCharacter::GetSingleton()) return nullptr;
			if (!invEntry->IsOwnedBy(RE::PlayerCharacter::GetSingleton()) && !IgnoreOwnership) return nullptr;

			auto obj = invEntry->GetObject();

			if (cachedStationInventory.contains(obj)) {
				if (cachedStationInventory[invEntry->GetObject()]->object != invEntry->GetObject()) {
					cachedStationInventory.erase(invEntry->GetObject());
					return invEntry;
				}
				else {
					cachedStationInventory[obj]->countDelta += invEntry->countDelta;
					RE::MemoryManager::GetSingleton()->Deallocate(invEntry, false);
					return nullptr;
				}
			}
			cachedStationInventory[obj] = invEntry;
			return invEntry;
			//return _GetInventoryItemEntryAtIdx(it.cont.get().get(), idx - totalCumulation, false);
		}
		totalCumulation += it.itemCount + 1;
	}

	if(!cachedContainers.empty()) logger::debug("Somehow the idx {} for GIIEI got above all the total", idx);
	return _GetInventoryItemEntryAtIdx(cont, idx, bUseMerchant);
}

int CraftingPullFromContainers::GetInventoryItemCount(RE::InventoryChanges* inv, RE::TESBoundObject* item, void* filterClass)
{
	//static RE::ObjectRefHandle usedFurniture;
	if (!inv->owner->IsPlayerRef() || RE::PlayerCharacter::GetSingleton()->GetOccupiedFurniture().native_handle() == 0) return _GetInventoryItemCount(inv, item, filterClass);
	if (usedFurniture == RE::PlayerCharacter::GetSingleton()->GetOccupiedFurniture()) {
		int totalCount = 0;
		auto Validate = [&]() -> bool {
			if (cachedContainers.empty()) return false;

			for (auto&& it : cachedContainers) {
				if (!it.cont || it.cont.get() == nullptr) return false;
				totalCount += _GetInventoryItemCount(it.cont.get().get()->GetInventoryChanges(), item, filterClass);
			}
			return true;
			};
		if (Validate()) {
			//cachedStationInventory.clear();
			return totalCount;
		}
	}

	usedFurniture = RE::PlayerCharacter::GetSingleton()->GetOccupiedFurniture();
	cachedContainers.clear();
	
	auto&& player = RE::PlayerCharacter::GetSingleton();
	int totalCount = 0;
	cachedContainers.push_back({ player->CreateRefHandle(), _GetInventoryItemCount(inv, item, filterClass) - 1});
	totalCount += cachedContainers.back().itemCount + 1;

	auto&& cell = player->GetParentCell();
	cell->ForEachReferenceInRange(player->GetPosition(), range, [&](RE::TESObjectREFR& obj) {

		if (obj.GetBaseObject()->Is(RE::FormType::Container)) {
			if (obj.IsAnOwner(RE::PlayerCharacter::GetSingleton(), true, false) || IgnoreOwnership) {
				if (std::find(permaLinks.begin(), permaLinks.end(), obj.formID) != permaLinks.end()) return RE::BSContainer::ForEachResult::kContinue;

				cachedContainers.push_back({ obj.CreateRefHandle(), _GetInventoryItemCount(obj.GetInventoryChanges(), item, filterClass) - 1});
				totalCount += cachedContainers.back().itemCount + 1;
			}
		}

		return RE::BSContainer::ForEachResult::kContinue;
	});
	
	for (auto&& it : permaLinks) {
		auto&& ref = RE::TESObjectREFR::LookupByID<RE::TESObjectREFR>(it);
		if (ref && ref->GetBaseObject()->Is(RE::FormType::Container)) {
			if (ref->IsAnOwner(RE::PlayerCharacter::GetSingleton(), true, false) || IgnoreOwnership) {
				cachedContainers.push_back({ ref->CreateRefHandle(), _GetInventoryItemCount(ref->GetInventoryChanges(), item ,filterClass) - 1});
				totalCount += cachedContainers.back().itemCount + 1;

				logger::debug("Added PermaLink Container {:X}({})", ref->formID, cachedContainers.back().cont.native_handle());
			}
		}
	}

	return totalCount;
}

RE::ExtraDataList* CraftingPullFromContainers::SetEnchantmentOnItem(RE::InventoryChanges* invchange, RE::TESBoundObject* selecteditem, RE::ExtraDataList* selecteditemExtra, RE::EnchantmentItem* enchantment, __int16 chargeamount)
{
	for (auto&& it : cachedContainers) {
		auto&& invCounts = it.cont.get()->GetInventoryCounts();
		if (invCounts.contains(selecteditem) && invCounts[selecteditem] >= 1) {
			logger::debug("Enchanted Item {} with {} charge", selecteditem->GetName(), chargeamount);

			return _SetEnchantmentOnItem(it.cont.get()->GetInventoryChanges(), selecteditem, selecteditemExtra, enchantment, chargeamount);
		}
	}

	logger::error("Enchanted Item, but the item doesnt exist in all cachedContainers???");

	return nullptr;
}

RE::BSPointerHandle<RE::TESObjectREFR, RE::BSUntypedPointerHandle<21, 5> >* CraftingPullFromContainers::RemoveItem(RE::TESObjectREFR* player, RE::BSPointerHandle<RE::TESObjectREFR, RE::BSUntypedPointerHandle<21, 5> >* result, RE::TESBoundObject* a_item, std::int32_t a_count, RE::ITEM_REMOVE_REASON a_reason, RE::ExtraDataList* a_extraList, RE::TESObjectREFR* a_moveToRef, const RE::NiPoint3* a_dropLoc, const RE::NiPoint3* a_rotate)
{
	if (RE::PlayerCharacter::GetSingleton()->GetOccupiedFurniture().native_handle() == 0 || RE::PlayerCharacter::GetSingleton()->GetOccupiedFurniture().get()->GetBaseObject()->As<RE::TESFurniture>()->workBenchData.benchType == RE::TESFurniture::WorkBenchData::BenchType::kNone || a_reason != RE::ITEM_REMOVE_REASON::kRemove) return _RemoveItem(player, result, a_item, a_count, a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);

	int count = a_count;
	for (auto&& it : cachedContainers) {
		auto&& countsMap = it.cont.get()->GetInventoryCounts();
		if (countsMap.contains(a_item)) {
			if (count > countsMap[a_item]) {
				count -= countsMap[a_item];
				if(it.cont.get()->Is(RE::FormType::ActorCharacter)) return _RemoveItem(it.cont.get().get(), result, a_item, countsMap[a_item], a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
				else it.cont.get()->RemoveItem(a_item, countsMap[a_item], a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
				
				logger::debug("Removed {}:{} from {}", a_item->GetName(), countsMap[a_item], it.cont.get().get()->GetDisplayFullName());
			}
			else {
				if (it.cont.get()->Is(RE::FormType::ActorCharacter)) return _RemoveItem(it.cont.get().get(), result, a_item, count, a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
				else it.cont.get()->RemoveItem(a_item, count, a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
				
				logger::debug("Removed {}:{} from {}", a_item->GetName(), countsMap[a_item], it.cont.get().get()->GetDisplayFullName());
				break;
			}
		}
	}

	return _RemoveItem(player, result, a_item, a_count, a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
}
