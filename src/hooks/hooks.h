#include <MinHook.h>

#include "CraftingPullFromContainers/CraftingPullFromContainers.h"

namespace Hooks {
    void Install() { 
        MH_Initialize();

        CraftingPullFromContainers::Install();

        MH_EnableHook(MH_ALL_HOOKS);
        return;
    }
}