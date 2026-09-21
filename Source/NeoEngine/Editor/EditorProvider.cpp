#include "EditorProvider.h"

#include <limits>

namespace NeoEngine {
bool EditorProvider::ExecuteCommand(const std::string& action, const std::string& data) {
    if (action.empty() || action.size() > 128U || data.size() > 8192U || action.find('\0') != std::string::npos || data.find('\0') != std::string::npos) {
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
    if (m_CommandSequence == std::numeric_limits<uint64_t>::max()) return "{}";
    const std::string paused = m_Paused ? "true" : "false";
    return "{\"paused\":" + paused +
           ",\"commandSequence\":" + std::to_string(m_CommandSequence) + "}";
}
} // namespace NeoEngine
