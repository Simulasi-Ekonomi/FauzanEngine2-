#pragma once
#include <cstdint>

namespace NeoEngine::Networking {

enum class NetworkPermission : uint8_t {
    Observe = 1u << 0,
    Replicate = 1u << 1,
    RemoteCommand = 1u << 2,
    MutateAuthority = 1u << 3,
    Administrative = 1u << 4
};

constexpr uint8_t permissionMask(NetworkPermission permission) {
    return static_cast<uint8_t>(permission);
}

struct AuthorizationIdentity {
    uint64_t peerId{};
    uint64_t identityHash{};
    bool authenticated{};
    uint8_t permissions{};
};

class NetworkAuthorizationPolicy {
public:
    static bool allows(const AuthorizationIdentity& identity, NetworkPermission permission) {
        if (!identity.peerId || !identity.authenticated) return false;
        return (identity.permissions & permissionMask(permission)) != 0;
    }
};

} // namespace NeoEngine::Networking
