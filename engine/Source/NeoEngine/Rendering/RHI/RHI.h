#pragma once
#include <string>

namespace NeoEngine {

class RHI {
public:
    virtual ~RHI() = default;
    virtual bool Init(void* window) = 0;
    virtual void Shutdown() = 0;
    virtual void Clear(float r, float g, float b, float a) = 0;
    virtual void Present() = 0;
    virtual void SetViewport(int x, int y, int w, int h) = 0;
    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    virtual std::string GetRendererInfo() const = 0;
    virtual void* GetNativeDevice() = 0;
};

} // namespace NeoEngine
