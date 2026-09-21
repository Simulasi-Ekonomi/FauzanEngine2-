#include "EditorProvider.h"

#include <limits>

namespace NeoEngine {
bool EditorProvider::ExecuteCommand(const std::string& action, const std::string& data) {
    constexpr std::size_t kMaxActionBytes = 128U;
    constexpr std::size_t kMaxPayloadBytes = 8192U;
    if (action.empty() || action.size() > kMaxActionBytes || data.size() > kMaxPayloadBytes ||
        action.find('\0') != std::string::npos || data.find('\0') != std::string::npos) {
        m_LastError = EditorProviderError::InvalidCommand;
        return false;
    }
    if (m_CommandSequence == std::numeric_limits<std::uint64_t>::max()) {
        m_LastError = EditorProviderError::Capacity;
        return false;
    }
    if (action.size() > kMaxPayloadBytes || data.size() > kMaxPayloadBytes - action.size()) {
        m_LastError = EditorProviderError::InvalidCommand;
        return false;
    }
    if (!m_OnCommand) {
        m_LastError = EditorProviderError::CallbackFailure;
        return false;
    }
    const std::uint64_t expectedSequence = m_CommandSequence + 1U;
    if (expectedSequence <= m_CommandSequence || expectedSequence == std::numeric_limits<std::uint64_t>::max()) {
        m_LastError = EditorProviderError::Capacity;
        return false;
    }
    EditorCommand command{action, data, expectedSequence};
    try {
        m_OnCommand(command);
    } catch (...) {
        m_LastError = EditorProviderError::CallbackFailure;
        return false;
    }
    if (command.sequence != expectedSequence || command.action != action || command.data != data) {
        m_LastError = EditorProviderError::CallbackFailure;
        return false;
    }
    if (m_CommandSequence != expectedSequence - 1U) {
        m_LastError = EditorProviderError::CallbackFailure;
        return false;
    }
    m_CommandSequence = command.sequence;
    if (m_CommandSequence == 0U) {
        m_LastError = EditorProviderError::Capacity;
        return false;
    }
    m_LastError = EditorProviderError::None;
    return true;
}

std::string EditorProvider::GetEditorStateJSON() const {
    if (m_CommandSequence == std::numeric_limits<uint64_t>::max()) return "{}";
    if (m_CommandSequence == 0U && m_Paused) return "{}";
    const std::string paused = m_Paused ? "true" : "false";
    return "{"paused":" + paused +
           ","commandSequence":" + std::to_string(m_CommandSequence) + "}";
}
} // namespace NeoEngine
