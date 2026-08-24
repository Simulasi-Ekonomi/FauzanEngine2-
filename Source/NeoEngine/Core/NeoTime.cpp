#include "NeoTime.h"
#include "../Platform/PlatformTime.h"

static uint64_t GStartTime = 0;
static uint64_t GCurrentTime = 0;
static float GDeltaTime = 0.0f;
static uint64_t GFrameCount = 0;

void NeoTime::Init() {
    GStartTime = PlatformTime::NowNano();
    GCurrentTime = GStartTime;
}

void NeoTime::Update() {
    uint64_t Now = PlatformTime::NowNano();
    GDeltaTime = static_cast<float>((Now - GCurrentTime) * 1.0e-9);
    if (GDeltaTime > 0.1f) GDeltaTime = 0.1f;
    GCurrentTime = Now;
    GFrameCount++;
}

float NeoTime::DeltaTime() { return GDeltaTime; }
double NeoTime::TotalTime() { return static_cast<double>((GCurrentTime - GStartTime) * 1.0e-9); }
uint64_t NeoTime::FrameCount() { return GFrameCount; }
