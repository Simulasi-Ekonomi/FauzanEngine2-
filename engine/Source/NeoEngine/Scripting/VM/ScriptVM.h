#pragma once
#include "ScriptValue.h"
#include <vector>
#include <unordered_map>
#include <functional>

namespace Neo {
    class ScriptVM {
        [[maybe_unused]] std::vector<ScriptValue> stack;
        std::unordered_map<std::string, std::function<void(ScriptVM&)>> nativeFuncs;

    public:
        void push(ScriptValue v) { stack.push_back(v); }

        ScriptValue pop() {
            if(stack.empty()) return 0;
            ScriptValue v = stack.back();
            stack.pop_back();
            return v;
        }

        void registerNative(const std::string& name, std::function<void(ScriptVM&)> func) {
            nativeFuncs[name] = func;
        }

        void execute(const std::string& funcName) {
            if(nativeFuncs.count(funcName)) {
                nativeFuncs[funcName](*this);
            }
        }
    };
}
