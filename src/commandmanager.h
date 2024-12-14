#pragma once

#include <boost/algorithm/string.hpp>

class CommandManager {
    friend CommandPipe;

public:
    using cmdfunc = void(const std::vector<std::string>& args, RE::Script* a_script, RE::ScriptCompiler* a_compiler, RE::COMPILER_NAME a_name,
                         RE::TESObjectREFR* a_targetRef);
    static void Register(std::string name, std::function<cmdfunc> exec) { Registry[name] = exec;}

protected:
    static bool Dispatch(RE::Script* a_script, RE::ScriptCompiler* a_compiler, RE::COMPILER_NAME a_name,
        RE::TESObjectREFR* a_targetRef) {
        //logger::debug("0");
        std::vector<std::string> args;
        boost::split(args, a_script->GetCommand(), boost::is_any_of("\t "));
        if (Registry.contains(args[0])) {
            std::string name = args[0];
            args.erase(args.begin());
            //logger::debug("0.5");
            Registry[name](args, a_script, a_compiler, a_name, a_targetRef);
            return true;
        }

        return false;
    }

private:
    static std::map<std::string, std::function<cmdfunc>> Registry;

};