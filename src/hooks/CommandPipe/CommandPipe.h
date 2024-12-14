#pragma once

class CommandPipe {

public:
	static void Install();
	
protected:
    static void CompileAndRun(RE::Script* a_script, RE::ScriptCompiler* a_compiler, RE::COMPILER_NAME a_name,
                              RE::TESObjectREFR* a_targetRef);

private:
    CommandPipe() = delete;
    CommandPipe(const CommandPipe&) = delete;
    CommandPipe(CommandPipe&&) = delete;
    ~CommandPipe() = delete;

    CommandPipe& operator=(const CommandPipe&) = delete;
    CommandPipe& operator=(CommandPipe&&) = delete;

    inline static REL::Relocation<decltype(CompileAndRun)> _CompileAndRun;
};