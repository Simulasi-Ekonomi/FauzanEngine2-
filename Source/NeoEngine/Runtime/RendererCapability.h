#pragma once
#include <string_view>
namespace NeoEngine {
enum class RendererCapabilityState : unsigned char { ReadyHeadless, ReadyPresent, NotImplemented, UnsupportedPlatform };
struct RendererCapability { RendererCapabilityState state; std::string_view backend; std::string_view reason; };
class RendererCapabilityProbe { public: static RendererCapability Query(); static bool CanRender(); static bool CanPresent(); };
} // namespace NeoEngine
