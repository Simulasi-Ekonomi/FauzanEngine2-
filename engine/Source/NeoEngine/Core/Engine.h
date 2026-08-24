#pragma once

class Engine {
public:
    static Engine& Get();

    void Init();
    void Run();
    void Shutdown();

    bool IsRunning() const { return bIsRunning; }
    void Stop() { bIsRunning = false; }

private:
    Engine() : bIsRunning(true) {}
    bool bIsRunning;
};
