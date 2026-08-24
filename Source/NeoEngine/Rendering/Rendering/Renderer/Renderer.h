#pragma once

class Renderer {
public:
    static Renderer& Get() {
        static Renderer instance;
        return instance;
    }

    void Init();
    void Render();

private:
    Renderer() = default;
};
