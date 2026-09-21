#include "EditorProvider.h"

#include <limits>

namespace NeoEngine {
bool EditorProvider::ExecuteCommand(const std::string& action, const std::string& data) {
    if (action.empty() || action.size() > 128U || data.size() > 8192U) {
        m_LastError = EditorProviderError::InvalidCommand;
        return false;
    }
    if (m_CommandSequence == std::numeric_limits<std::uint64_t>::max()) {
        m_LastError = EditorProviderError::Capacity;
        return false;
    }
    if (!m_OnCommand) {
        m_LastError = EditorProviderError::CallbackFailure;
        return false;
    }
    EditorCommand command{action, data, m_CommandSequence + 1U};
    try {
        m_OnCommand(command);
    } catch (...) {
        m_LastError = EditorProviderError::CallbackFailure;
        return false;
    }
    m_CommandSequence = command.sequence;
    m_LastError = EditorProviderError::None;
    return true;
}

std::string EditorProvider::GetEditorStateJSON() const {
    if (commandSequence_ == std::numeric_limits<uint64_t>::max()) return "{}";
    return "{\"paused\":" + std::string(m_Paused ? "true" : "false") +
           ",\"commandSequence\":" + std::to_string(m_CommandSequence) + "}";
}
} // namespace NeoEngine
