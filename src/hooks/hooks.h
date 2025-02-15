#include <MinHook.h>

#include "CraftingPullFromContainers/CraftingPullFromContainers.h"
#include "CommandPipe/CommandPipe.h"

namespace Hooks {
    void Install() { 
        MH_Initialize();

        CraftingPullFromContainers::Install();
        CommandPipe::Install();

        MH_EnableHook(MH_ALL_HOOKS);
        return;
    }

    void InstallLate() {

        CraftingPullFromContainers::InstallLate();

    }
}