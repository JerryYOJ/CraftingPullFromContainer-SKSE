#include "CommandPipe.h"
#include "../../commandmanager.h"

void CommandPipe::Install() {
    int offset = 0x52;
    if (REL::Module::GetRuntime() == REL::Module::Runtime::SE) 
        offset = 0xE2;

    logger::info("Installing hooks for class {} using offsets {:X}", typeid(CommandPipe).name(), offset);

    REL::Relocation<std::uintptr_t> hookPoint{RELOCATION_ID(52065, 52952), offset};
    auto& trampoline = SKSE::GetTrampoline();
    _CompileAndRun = trampoline.write_call<5>(hookPoint.address(), CompileAndRun);
}

void CommandPipe::CompileAndRun(RE::Script* a_script, RE::ScriptCompiler* a_compiler, RE::COMPILER_NAME a_name,
                                RE::TESObjectREFR* a_targetRef) {
    
    if (CommandManager::Dispatch(a_script, a_compiler, a_name, a_targetRef)) return;
    return _CompileAndRun(a_script, a_compiler, a_name, a_targetRef);
}
