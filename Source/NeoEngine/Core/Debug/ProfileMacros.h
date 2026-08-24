#pragma once
#include "FrameProfiler.h"

#define PROFILE_BEGIN_FRAME()       NeoEngine::FrameProfiler::BeginFrame()
#define PROFILE_END_FRAME()         NeoEngine::FrameProfiler::EndFrame()
#define PROFILE_SECTION(name)       NeoEngine::ScopedProfile __profile_##__LINE__(name)

namespace NeoEngine {

class ScopedProfile {
public:
    explicit ScopedProfile(const char* name) : m_Name(name) {
        FrameProfiler::BeginSection(m_Name);
    }
    ~ScopedProfile() {
        FrameProfiler::EndSection(m_Name);
    }
private:
    const char* m_Name;
};

} // namespace NeoEngine
