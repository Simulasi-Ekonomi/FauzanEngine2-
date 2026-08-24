#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace NeoEngine {
enum class RuntimePersistenceError : uint8_t { None, InvalidKey, InvalidValue, SensitiveContent, Capacity, PayloadLimit, Corrupt, UnsupportedVersion, ChecksumMismatch, TrailingBytes };
class RuntimeSettingsStore {
public:
    static constexpr uint8_t kVersion = 1;
    static constexpr uint8_t kMaxEntries = 64;
    static constexpr uint8_t kMaxKeyBytes = 48;
    static constexpr uint16_t kMaxValueBytes = 256;
    bool Set(std::string key, std::string value);
    [[nodiscard]] const std::string* Find(std::string_view key) const;
    bool Serialize(std::vector<uint8_t>& out) const;
    bool Deserialize(const std::vector<uint8_t>& bytes);
    [[nodiscard]] size_t Count() const { return entries_.size(); }
    [[nodiscard]] RuntimePersistenceError LastError() const { return lastError_; }
private:
    struct Entry { std::string key; std::string value; };
    std::vector<Entry> entries_;
    RuntimePersistenceError lastError_ = RuntimePersistenceError::None;
};

struct RuntimeSaveEnvelope { std::string kind; uint64_t revision = 0; std::vector<uint8_t> payload; };
class RuntimeSaveCodec {
public:
    static constexpr uint8_t kVersion = 1;
    static constexpr uint8_t kMaxKindBytes = 48;
    static constexpr size_t kMaxPayloadBytes = 1024U * 1024U;
    static bool Serialize(const RuntimeSaveEnvelope& envelope, std::vector<uint8_t>& out, RuntimePersistenceError& error);
    static bool Deserialize(const std::vector<uint8_t>& bytes, RuntimeSaveEnvelope& envelope, RuntimePersistenceError& error);
};
} // namespace NeoEngine
