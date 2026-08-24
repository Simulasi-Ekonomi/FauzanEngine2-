#pragma once

namespace NeoEngine {

class Engine {
public:
    static Engine& Get();

    void Start();
    bool IsInitialized() const;
    bool IsRunning() const;
    void Stop();

private:
    bool initialized = false;
    bool running = true;
};

} // namespace NeoEngine
