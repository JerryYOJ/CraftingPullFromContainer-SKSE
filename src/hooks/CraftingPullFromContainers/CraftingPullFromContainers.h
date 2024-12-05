#pragma once

class CraftingPullFromContainers {

public:
    static void Install();

protected:
    static int GetContainerItemCount(RE::TESObjectREFR* cont, bool bUseMechant, bool bUnkTrue);
    static RE::InventoryEntryData* GetInventoryItemEntryAtIdx(RE::TESObjectREFR* cont, int idx, bool bUseMerchant);
    static void RemoveItem(RE::TESObjectREFR* player, RE::BSPointerHandle<RE::TESObjectREFR, RE::BSUntypedPointerHandle<21, 5> >* result, RE::TESBoundObject* a_item, std::int32_t a_count, RE::ITEM_REMOVE_REASON a_reason, RE::ExtraDataList* a_extraList, RE::TESObjectREFR* a_moveToRef, const RE::NiPoint3* a_dropLoc, const RE::NiPoint3* a_rotate);
    static int GetInventoryItemCount(RE::InventoryChanges* inv, RE::TESBoundObject* item, void* filterClass);

private:
    CraftingPullFromContainers() = delete;
    CraftingPullFromContainers(const CraftingPullFromContainers&) = delete;
    CraftingPullFromContainers(CraftingPullFromContainers&&) = delete;
    ~CraftingPullFromContainers() = delete;

    CraftingPullFromContainers& operator=(const CraftingPullFromContainers&) = delete;
    CraftingPullFromContainers& operator=(CraftingPullFromContainers&&) = delete;

    inline static REL::Relocation<decltype(GetContainerItemCount)> _GetContainerItemCount;
    inline static REL::Relocation<decltype(GetInventoryItemEntryAtIdx)> _GetInventoryItemEntryAtIdx;
    inline static REL::Relocation<decltype(RemoveItem)> _RemoveItem;
    inline static REL::Relocation<decltype(GetInventoryItemCount)> _GetInventoryItemCount;

    struct CONT {
        RE::TESObjectREFR* cont;
        int itemCount;
    };
    static std::vector<CONT> cachedContainers;
    //static std::uintptr_t totalItems;
    //static RE::ObjectRefHandle usedFurniture;
};