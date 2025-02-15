#pragma once

#include "../../Ini.hpp"

#include "../../configmanager.h"

class CraftingPullFromContainers {

public:
    static void Install();
    static float range;
    static bool IgnoreOwnership;
    static std::vector<LocalForm> permaLinks;

//protected:
    static int GetContainerItemCount(RE::TESObjectREFR* cont, bool bUseMechant, bool bUnkTrue);
    static RE::InventoryEntryData* GetInventoryItemEntryAtIdx(RE::TESObjectREFR* cont, int idx, bool bUseMerchant);
    static RE::BSPointerHandle<RE::TESObjectREFR, RE::BSUntypedPointerHandle<21, 5> >* RemoveItem(RE::TESObjectREFR* player, RE::BSPointerHandle<RE::TESObjectREFR, RE::BSUntypedPointerHandle<21, 5> >* result, RE::TESBoundObject* a_item, std::int32_t a_count, RE::ITEM_REMOVE_REASON a_reason, RE::ExtraDataList* a_extraList, RE::TESObjectREFR* a_moveToRef, const RE::NiPoint3* a_dropLoc, const RE::NiPoint3* a_rotate);
    static int GetInventoryItemCount(RE::InventoryChanges* inv, RE::TESBoundObject* item, void* filterClass);
    static RE::ExtraDataList* SetEnchantmentOnItem(RE::InventoryChanges* invchange, RE::TESBoundObject* selecteditem, RE::ExtraDataList* selecteditemExtra, RE::EnchantmentItem* enchantment, __int16 chargeamount);

//private:
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
    inline static REL::Relocation<decltype(SetEnchantmentOnItem)> _SetEnchantmentOnItem;

    struct CONT {
        RE::ObjectRefHandle cont;
        int itemCount;
    };
    static std::vector<CONT> cachedContainers;
    //struct INV {
    //    RE::InventoryEntryData* invEntry;
    //    void* retaddr;
    //};
    static std::unordered_map<RE::TESBoundObject*, RE::InventoryEntryData*> cachedStationInventory;
    //static std::uintptr_t totalItems;
    static RE::ObjectRefHandle usedFurniture;
};