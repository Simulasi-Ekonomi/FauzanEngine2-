#include "CrashHandler.h"
#include "Log.h"
#include <signal.h>
#include <fstream>
#include <ctime>
#include <android/log.h>

namespace NeoEngine {

static CrashHandler* g_CrashHandler = nullptr;

void SignalHandler(int sig) {
    const char* signalName = "UNKNOWN";
    switch (sig) {
        case SIGSEGV: signalName = "SIGSEGV (Segmentation Fault)"; break;
        case SIGABRT: signalName = "SIGABRT (Abort)"; break;
        case SIGFPE:  signalName = "SIGFPE (Floating Point Error)"; break;
        case SIGILL:  signalName = "SIGILL (Illegal Instruction)"; break;
        case SIGBUS:  signalName = "SIGBUS (Bus Error)"; break;
    }
    
    Log::Write(LogLevel::Fatal, LogChannel::Core,
        std::string("CRASH DETECTED: ") + signalName);
    
    if (g_CrashHandler) {
        g_CrashHandler->WriteCrashReport(signalName);
    }
    
    // Restore default handler dan re-raise signal
    signal(sig, SIG_DFL);
    raise(sig);
}

CrashHandler::CrashHandler() {
    g_CrashHandler = this;
}

void CrashHandler::Install() {
    signal(SIGSEGV, SignalHandler);
    signal(SIGABRT, SignalHandler);
    signal(SIGFPE, SignalHandler);
    signal(SIGILL, SignalHandler);
    signal(SIGBUS, SignalHandler);
    Log::Write(LogLevel::Info, LogChannel::Core, "Crash Handler Installed");
}

void CrashHandler::WriteCrashReport(const std::string& signalName) {
    auto now = time(nullptr);
    std::string filename = "/sdcard/FauzanEngine/Crashes/crash_" + 
                           std::to_string(now) + ".log";
    
    std::ofstream report(filename);
    if (report.is_open()) {
        report << "=== FauzanEngine Crash Report ===\n";
        report << "Timestamp: " << ctime(&now);
        report << "Signal: " << signalName << "\n";
        report << "Version: " << ENGINE_VERSION << "\n";
        report.close();
    }
}

} // namespace NeoEngine
