#include "TelemetryOutbox.h"

#include <algorithm>
#include <cctype>
#include <json/json.h>
#include <new>
#include <sstream>

namespace NeoEngine {
namespace {

template <typename T>
void Put(std::vector<uint8_t>& out, T value) {
    for (size_t i = 0; i < sizeof(T); ++i) {
        out.push_back(static_cast<uint8_t>(
            (static_cast<uint64_t>(value) >> (i * 8U)) & 0xffU));
    }
}

template <typename T>
bool Get(const std::vector<uint8_t>& bytes, size_t& offset, T& value) {
    if (offset > bytes.size() || sizeof(T) > bytes.size() - offset) return false;

    uint64_t raw = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
        raw |= static_cast<uint64_t>(bytes[offset + i]) << (i * 8U);
    }
    offset += sizeof(T);
    value = static_cast<T>(raw);
    return true;
}

bool ValidJsonObject(const std::string& json) {
    if (json.empty() || json.size() > TelemetryOutbox::kMaxEnvelopeBytes ||
        json.front() != '{') {
        return false;
    }

    Json::CharReaderBuilder builder;
    Json::Value root;
    std::string errors;
    std::istringstream input(json);
    return Json::parseFromStream(builder, input, &root, &errors) && root.isObject();
}

} // namespace

bool TelemetryOutbox::ValidId(const std::string& id) {
    if (id.empty() || id.size() > 96U) return false;
    for (unsigned char c : id) {
        if (!std::isalnum(c) && c != '-' && c != '_') return false;
    }
    return true;
}

bool TelemetryOutbox::Enqueue(std::string id, std::string json) {
    if (!ValidId(id) || json.empty() || json.size() > kMaxEnvelopeBytes ||
        !ValidJsonObject(json) ||
        std::any_of(m_Pending.begin(), m_Pending.end(),
                    [&](const auto& entry) { return entry.id == id; }) ||
        m_Pending.size() >= kMaxEnvelopes) {
        return false;
    }

    try {
        m_Pending.push_back({std::move(id), std::move(json)});
    } catch (const std::bad_alloc&) {
        return false;
    }
    return true;
}

bool TelemetryOutbox::Acknowledge(const std::string& id) {
    auto it = std::find_if(m_Pending.begin(), m_Pending.end(),
                           [&](const auto& entry) { return entry.id == id; });
    if (it == m_Pending.end()) return false;
    m_Pending.erase(it);
    return true;
}

bool TelemetryOutbox::AcknowledgeBatch(const std::vector<std::string>& ids) {
    if (ids.empty() || ids.size() > kMaxEnvelopes) return false;

    try {
        std::vector<TelemetryEnvelope> candidate = m_Pending;
        for (const std::string& id : ids) {
            auto it = std::find_if(candidate.begin(), candidate.end(),
                                   [&](const auto& entry) { return entry.id == id; });
            if (it == candidate.end()) return false;
            candidate.erase(it);
        }
        m_Pending = std::move(candidate);
    } catch (const std::bad_alloc&) {
        return false;
    }
    return true;
}

std::vector<uint8_t> TelemetryOutbox::Serialize() const {
    std::vector<uint8_t> out;
    try {
        out.reserve(kMaxSerializedBytes);
        Put<uint32_t>(out, 0x5842544FU);
        Put<uint16_t>(out, 1U);
        Put<uint32_t>(out, static_cast<uint32_t>(m_Pending.size()));

        for (const auto& entry : m_Pending) {
            Put<uint16_t>(out, static_cast<uint16_t>(entry.id.size()));
            out.insert(out.end(), entry.id.begin(), entry.id.end());
            Put<uint32_t>(out, static_cast<uint32_t>(entry.json.size()));
            out.insert(out.end(), entry.json.begin(), entry.json.end());
        }
    } catch (const std::bad_alloc&) {
        return {};
    }
    return out;
}

bool TelemetryOutbox::Deserialize(const std::vector<uint8_t>& bytes) {
    if (bytes.empty() || bytes.size() > kMaxSerializedBytes) return false;

    size_t offset = 0;
    uint32_t magic = 0;
    uint16_t version = 0;
    uint32_t count = 0;
    if (!Get(bytes, offset, magic) || !Get(bytes, offset, version) ||
        !Get(bytes, offset, count) || magic != 0x5842544FU ||
        version != 1U || count > kMaxEnvelopes) {
        return false;
    }

    try {
        std::vector<TelemetryEnvelope> next;
        next.reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            uint16_t idLength = 0;
            uint32_t jsonLength = 0;
            if (!Get(bytes, offset, idLength) ||
                idLength > bytes.size() - std::min(offset, bytes.size())) {
                return false;
            }

            std::string id(reinterpret_cast<const char*>(bytes.data() + offset), idLength);
            offset += idLength;

            if (!Get(bytes, offset, jsonLength) ||
                jsonLength > kMaxEnvelopeBytes ||
                offset > bytes.size() ||
                jsonLength > bytes.size() - offset ||
                !ValidId(id)) {
                return false;
            }

            std::string json(reinterpret_cast<const char*>(bytes.data() + offset), jsonLength);
            offset += jsonLength;

            if (!ValidJsonObject(json) ||
                std::any_of(next.begin(), next.end(),
                            [&](const auto& entry) { return entry.id == id; })) {
                return false;
            }
            next.push_back({std::move(id), std::move(json)});
        }

        if (offset != bytes.size()) return false;
        m_Pending = std::move(next);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

bool TelemetryOutbox::PruneOlderThan(uint64_t nowMs, uint64_t maxAgeMs) {
    if (maxAgeMs == 0U) return false;

    const uint64_t cutoff = nowMs > maxAgeMs ? nowMs - maxAgeMs : 0U;
    std::vector<TelemetryEnvelope> retained;
    try {
        retained.reserve(m_Pending.size());
        bool changed = false;

        for (const TelemetryEnvelope& envelope : m_Pending) {
            Json::CharReaderBuilder builder;
            Json::Value root;
            std::string errors;
            std::istringstream input(envelope.json);
            if (!Json::parseFromStream(builder, input, &root, &errors) ||
                !root.isObject()) {
                retained.push_back(envelope);
                continue;
            }

            const Json::Value& occurredAt = root["occurredAtMs"];
            if (!occurredAt.isUInt64() && !occurredAt.isUInt()) {
                retained.push_back(envelope);
                continue;
            }

            const uint64_t occurredAtMs = occurredAt.isUInt64()
                ? occurredAt.asUInt64()
                : static_cast<uint64_t>(occurredAt.asUInt());

            if (occurredAtMs >= cutoff) retained.push_back(envelope);
            else changed = true;
        }

        if (changed) m_Pending.swap(retained);
        return changed;
    } catch (const std::bad_alloc&) {
        return false;
    }
}
} // namespace NeoEngine
