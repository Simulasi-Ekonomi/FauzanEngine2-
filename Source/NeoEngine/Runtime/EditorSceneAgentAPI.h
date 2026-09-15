#pragma once

#include "EditorSceneSession.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace NeoEngine {

enum class EditorAgentError : uint8_t {
    None,
    EmptyRequest,
    InvalidJson,
    RootNotObject,
    MissingOperation,
    InvalidOperation,
    UnknownField,
    InvalidArgument,
    UnknownActor,
    OperationFailed
};

// Strict, in-memory JSON command seam for editor automation. It never performs
// filesystem/network access and delegates scene validation/mutation to the session.
class EditorSceneAgentAPI {
public:
    bool Execute(std::string_view request, EditorSceneSession& session, const AssetRegistry& assets, std::string& response) const;
    [[nodiscard]] EditorAgentError LastError() const { return lastError_; }

private:
    bool Fail(EditorAgentError error, std::string& response) const;
    mutable EditorAgentError lastError_ = EditorAgentError::None;
};

} // namespace NeoEngine
