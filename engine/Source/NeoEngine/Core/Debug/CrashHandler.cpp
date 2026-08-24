#include "CrashHandler.h"

#include <signal.h>
#include <iostream>
#include <fstream>
#include <ctime>

namespace NeoEngine
{

static std::ofstream crashFile;

void CrashHandler::Install()
{
    signal(SIGSEGV, SignalHandler);
    signal(SIGABRT, SignalHandler);
    signal(SIGILL,  SignalHandler);
    signal(SIGFPE,  SignalHandler);
}

void CrashHandler::SignalHandler(int signal)
{
    crashFile.open("neoengine_crash_dump.txt", std::ios::app);

    std::time_t now = std::time(nullptr);

    crashFile << "============================" << std::endl;
    crashFile << "NeoEngine Crash Dump" << std::endl;
    crashFile << "Time: " << std::ctime(&now);
    crashFile << "Signal: " << signal << std::endl;
    crashFile << "============================" << std::endl;

    crashFile.close();

    std::cerr << "NeoEngine crash detected. Dump written to neoengine_crash_dump.txt\n";

    std::_Exit(1);
}

}
