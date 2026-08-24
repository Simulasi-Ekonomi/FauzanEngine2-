#include "PlatformTime.h"
#include "Platform.h"

extern Platform* GPlatform;

uint64 PlatformTime::NowNano() {
    return GPlatform->GetTimeNano();
}

double PlatformTime::NowSeconds() {
    return static_cast<double>(GPlatform->GetTimeNano()) * 1e-9;
}
