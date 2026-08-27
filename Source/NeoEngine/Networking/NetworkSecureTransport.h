#pragma once
#include <cstddef>
#include <cstdint>

namespace NeoEngine::Networking {

enum class SecureTransportState : uint8_t {
    Idle,
    Handshaking,
    Established,
    Failed,
    Closed
};

enum class SecureTransportError : uint8_t {
    None,
    CertificateRejected,
    PeerAuthenticationFailed,
    HandshakeFailed,
    NotEstablished,
    Closed
};

struct SecurePeerIdentity {
    uint64_t peerId{};
    uint64_t identityHash{};
    bool verified{};
};

class ISecureTransport {
public:
    virtual ~ISecureTransport() = default;
    virtual SecureTransportState state() const = 0;
    virtual SecureTransportError lastError() const = 0;
    virtual bool beginHandshake() = 0;
    virtual bool peerVerified() const = 0;
    virtual SecurePeerIdentity peerIdentity() const = 0;
    virtual std::ptrdiff_t read(void* buffer, std::size_t capacity) = 0;
    virtual std::ptrdiff_t write(const void* data, std::size_t size) = 0;
    virtual void close() = 0;
};

} // namespace NeoEngine::Networking
