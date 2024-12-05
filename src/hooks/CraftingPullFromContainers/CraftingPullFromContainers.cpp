#include "CraftingPullFromContainers.h"
#include <MinHook.h>

std::vector<CraftingPullFromContainers::CONT> CraftingPullFromContainers::cachedContainers;
//RE::ObjectRefHandle CraftingPullFromContainers::usedFurniture;
//std::uintptr_t CraftingPullFromContainers::totalItems;

void CraftingPullFromContainers::Install()
{
	REL::Relocation<std::uintptr_t> GetItemCount{ RELOCATION_ID(19700, 19700) };
	REL::Relocation<std::uintptr_t> GetInvItem{ RELOCATION_ID(19699, 19699) };
	REL::Relocation<std::uintptr_t> GetInvCount{ RELOCATION_ID(16109, 16109) };
	
	MH_CreateHook((void*)GetItemCount.address(), GetContainerItemCount, reinterpret_cast<void**>(&_GetContainerItemCount));
	MH_CreateHook((void*)GetInvItem.address(), GetInventoryItemEntryAtIdx, reinterpret_cast<void**>(&_GetInventoryItemEntryAtIdx));
	MH_CreateHook((void*)GetInvCount.address(), GetInventoryItemCount, reinterpret_cast<void**>(&_GetInventoryItemCount));

	_RemoveItem = REL::Relocation<uintptr_t>(RE::VTABLE_PlayerCharacter[0]).write_vfunc(0x56, RemoveItem);
}

int CraftingPullFromContainers::GetContainerItemCount(RE::TESObjectREFR* cont, bool bUseMechant, bool bUnkTrue)
{
	if (!cont->IsPlayerRef() || RE::PlayerCharacter::GetSingleton()->GetOccupiedFurniture().native_handle() == 0) return _GetContainerItemCount(cont, bUseMechant, bUnkTrue);
	//if (usedFurniture == RE::PlayerCharacter::GetSingleton()->GetOccupiedFurniture()) return totalItems;

	
	//usedFurniture.reset();
	cachedContainers.clear();

	auto&& player = RE::PlayerCharacter::GetSingleton();
	int totalCount = 0;
	cachedContainers.push_back({ player, _GetContainerItemCount(player, false, true) });
	totalCount += cachedContainers.back().itemCount;

	auto&& cell = player->GetParentCell();
	cell->ForEachReferenceInRange(player->GetPosition(), 800, [&](RE::TESObjectREFR& obj) {

		if (obj.GetBaseObject()->Is(RE::FormType::Container)) {
			if (obj.IsAnOwner(RE::PlayerCharacter::GetSingleton(), true, false)) {
				cachedContainers.push_back({ &obj, _GetContainerItemCount(&obj, false, true) });
				totalCount += cachedContainers.back().itemCount;
			}
		}

		return RE::BSContainer::ForEachResult::kContinue;
		});
	
	//totalItems = totalCount;
	return totalCount;
}

RE::InventoryEntryData* CraftingPullFromContainers::GetInventoryItemEntryAtIdx(RE::TESObjectREFR* cont, int idx, bool bUseMerchant)
{
	if (!cont->IsPlayerRef() || RE::PlayerCharacter::GetSingleton()->GetOccupiedFurniture().native_handle() == 0) return _GetInventoryItemEntryAtIdx(cont, idx, bUseMerchant);
	
	int totalCumulation = 0;
	for (auto&& it : cachedContainers) {
		if (idx - totalCumulation < it.itemCount) {
			return _GetInventoryItemEntryAtIdx(it.cont, idx - totalCumulation, false);
		}
		totalCumulation += it.itemCount;
	}

	logger::warn("Somehow the idx for GIIEI got above all the total");
	return nullptr;
}

int CraftingPullFromContainers::GetInventoryItemCount(RE::InventoryChanges* inv, RE::TESBoundObject* item, void* filterClass)
{
	//static RE::ObjectRefHandle usedFurniture;
	if (!inv->owner->IsPlayerRef() || RE::PlayerCharacter::GetSingleton()->GetOccupiedFurniture().native_handle() == 0) return _GetInventoryItemCount(inv, item, filterClass);
	//if(usedFurniture)
	
	cachedContainers.clear();
	
	auto&& player = RE::PlayerCharacter::GetSingleton();
	int totalCount = 0;
	cachedContainers.push_back({ player, _GetInventoryItemCount(inv, item, filterClass) });
	totalCount += cachedContainers.back().itemCount;

	auto&& cell = player->GetParentCell();
	cell->ForEachReferenceInRange(player->GetPosition(), 800, [&](RE::TESObjectREFR& obj) {

		if (obj.GetBaseObject()->Is(RE::FormType::Container)) {
			if (obj.IsAnOwner(RE::PlayerCharacter::GetSingleton(), true, false)) {
				cachedContainers.push_back({ &obj, _GetInventoryItemCount(obj.GetInventoryChanges(), item, filterClass)});
				totalCount += cachedContainers.back().itemCount;
			}
		}

		return RE::BSContainer::ForEachResult::kContinue;
		});
	
	return totalCount;
}

void CraftingPullFromContainers::RemoveItem(RE::TESObjectREFR* player, RE::BSPointerHandle<RE::TESObjectREFR, RE::BSUntypedPointerHandle<21, 5> >* result, RE::TESBoundObject* a_item, std::int32_t a_count, RE::ITEM_REMOVE_REASON a_reason, RE::ExtraDataList* a_extraList, RE::TESObjectREFR* a_moveToRef, const RE::NiPoint3* a_dropLoc, const RE::NiPoint3* a_rotate)
{
	if (RE::PlayerCharacter::GetSingleton()->GetOccupiedFurniture().native_handle() == 0 || RE::PlayerCharacter::GetSingleton()->GetOccupiedFurniture().get()->GetBaseObject()->As<RE::TESFurniture>()->workBenchData.benchType == RE::TESFurniture::WorkBenchData::BenchType::kNone || a_reason != RE::ITEM_REMOVE_REASON::kRemove) return _RemoveItem(player, result, a_item, a_count, a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);

	int count = a_count;
	for (auto&& it : cachedContainers) {
		auto&& countsMap = it.cont->GetInventoryCounts();
		if (countsMap.contains(a_item)) {
			if (count > countsMap[a_item]) {
				count -= countsMap[a_item];
				if(it.cont->Is(RE::FormType::ActorCharacter)) _RemoveItem(it.cont, result, a_item, countsMap[a_item], a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
				else it.cont->RemoveItem(a_item, countsMap[a_item], a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
				
			}
			else {
				if (it.cont->Is(RE::FormType::ActorCharacter)) _RemoveItem(it.cont, result, a_item, count, a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
				else it.cont->RemoveItem(a_item, count, a_reason, a_extraList, a_moveToRef, a_dropLoc, a_rotate);
				break;
			}
		}
	}
	return;
}
