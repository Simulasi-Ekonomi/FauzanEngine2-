#pragma once
#include <string>
#include <functional>

namespace NeoEngine {

struct EditorCommand {
    std::string action;
    std::string data;
};

class EditorProvider {
public:
    static EditorProvider& Get() {
        static EditorProvider instance;
        return instance;
    }

    void SetPaused(bool paused) { m_Paused = paused; }
    bool IsPaused() const { return m_Paused; }

    void ExecuteCommand(const std::string& action, const std::string& data = "") {
        if (m_OnCommand) m_OnCommand({action, data});
    }

    void SetCommandCallback(std::function<void(const EditorCommand&)> cb) {
        m_OnCommand = cb;
    }

    std::string GetEditorStateJSON() const {
        return "{\"paused\":" + std::string(m_Paused ? "true" : "false") + "}";
    }

private:
    EditorProvider() = default;
    bool m_Paused = false;
    std::function<void(const EditorCommand&)> m_OnCommand;
};

} // namespace NeoEngine
