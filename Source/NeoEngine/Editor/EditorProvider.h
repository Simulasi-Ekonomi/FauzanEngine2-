#pragma once
#include <string>
#include <functional>
#include <cstdint>

namespace NeoEngine {

struct EditorCommand {
    std::string action;
    std::string data;
    std::uint64_t sequence = 0U;
};
enum class EditorProviderError : std::uint8_t { None, InvalidCommand, Capacity, CallbackFailure };

class EditorProvider {
public:
    static EditorProvider& Get() {
        static EditorProvider instance;
        return instance;
    }

    void SetPaused(bool paused) { m_Paused = paused; }
    bool IsPaused() const { return m_Paused; }

    bool ExecuteCommand(const std::string& action, const std::string& data = "");

    void SetCommandCallback(std::function<void(const EditorCommand&)> cb) {
        m_OnCommand = cb;
    }

    std::string GetEditorStateJSON() const;
    [[nodiscard]] std::uint64_t CommandSequence() const { return m_CommandSequence; }
    [[nodiscard]] EditorProviderError LastError() const { return m_LastError; }

private:
    EditorProvider() = default;
    bool m_Paused = false;
    std::function<void(const EditorCommand&)> m_OnCommand;
    std::uint64_t m_CommandSequence = 0U;
    EditorProviderError m_LastError = EditorProviderError::None;
};

} // namespace NeoEngine
