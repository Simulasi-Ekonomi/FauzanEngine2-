#pragma once

namespace NeoEngine
{

class CrashHandler
{
public:

    static void Install();

private:

    static void SignalHandler(int signal);
};

}
