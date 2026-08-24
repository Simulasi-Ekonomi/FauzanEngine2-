#pragma once

#include "FrameProfiler.h"

#define PROFILE_BEGIN_FRAME() \
NeoEngine::FrameProfiler::BeginFrame()

#define PROFILE_END_FRAME() \
NeoEngine::FrameProfiler::EndFrame()

#define PROFILE_BEGIN(name) \
NeoEngine::FrameProfiler::BeginSection(name)

#define PROFILE_END(name) \
NeoEngine::FrameProfiler::EndSection(name)
