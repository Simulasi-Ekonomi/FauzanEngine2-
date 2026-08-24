#include "CoreValidation.h"
#include "Log.h"
#include "../Engine.h"
#include "../SubsystemManager.h"
#include <android/log.h>

namespace NeoEngine {

void CoreValidation::Run() {
    Log::Write(LogLevel::Info, LogChannel::Core, "Running Core Validation...");
    
    bool allValid = true;
    
    // Cek engine instance
    if (!Engine::Get().IsInitialized()) {
        Log::Write(LogLevel::Error, LogChannel::Core, "Engine not initialized");
        allValid = false;
    }
    
    // Cek subsystems
    if (!SubsystemManager::AreAllInitialized()) {
        Log::Write(LogLevel::Error, LogChannel::Core, "Not all subsystems initialized");
        allValid = false;
    }
    
    if (allValid) {
        Log::Write(LogLevel::Info, LogChannel::Core, "Core Validation PASSED");
    } else {
        Log::Write(LogLevel::Error, LogChannel::Core, "Core Validation FAILED");
    }
}

} // namespace NeoEngine
