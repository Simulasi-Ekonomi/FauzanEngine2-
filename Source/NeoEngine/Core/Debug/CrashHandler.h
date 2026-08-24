#pragma once
#include <string>

namespace NeoEngine {

constexpr const char* ENGINE_VERSION = "2.1.0";

class CrashHandler {
public:
    CrashHandler();
    
    void Install();
    void WriteCrashReport(const std::string& signalName);
    
private:
    static CrashHandler* instance;
};

} // namespace NeoEngine
