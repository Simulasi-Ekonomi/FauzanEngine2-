#pragma once
#include <string>

namespace Neo {
    class Actor {
    public:
        std::string name;
        virtual ~Actor() = default;
        virtual void onUpdate(float dt) = 0;
    };

    class ScriptActor : public Actor {
        std::string scriptFunc;
    public:
        void bindScript(const std::string& funcName) { scriptFunc = funcName; }
        void onUpdate(float dt) override {
            // Logika panggil VM akan ditaruh di sini
        }
    };
}
