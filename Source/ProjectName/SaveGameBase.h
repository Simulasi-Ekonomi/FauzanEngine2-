#pragma once

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

class USaveGameBase {
public:
    std::string SaveSlotName;
    uint32_t UserIndex = 0;
    int32_t PlayerLevel = 1;
    float PlayTimeSeconds = 0.0f;

    bool SaveToFile(const std::string& filePath) const {
        if (filePath.empty() || SaveSlotName.size() > kMaxSlotNameBytes || !IsValidState()) return false;
        std::vector<uint8_t> payload;
        payload.reserve(4 + SaveSlotName.size() + sizeof(UserIndex) + sizeof(PlayerLevel) + sizeof(PlayTimeSeconds));
        AppendU32(payload, static_cast<uint32_t>(SaveSlotName.size()));
        payload.insert(payload.end(), SaveSlotName.begin(), SaveSlotName.end());
        AppendU32(payload, UserIndex);
        AppendI32(payload, PlayerLevel);
        AppendU32(payload, FloatBits(PlayTimeSeconds));
        if (payload.size() > kMaxPayloadBytes) return false;

        const uint32_t checksum = Fnv1a(payload);
        const std::string tempPath = MakeTempPath(filePath);
        std::ofstream outFile(tempPath, std::ios::binary | std::ios::trunc);
        if (!outFile.is_open()) return false;
        const bool headerOk = WriteU32(outFile, kMagic) && WriteU32(outFile, kVersion) &&
                              WriteU32(outFile, static_cast<uint32_t>(payload.size())) && WriteU32(outFile, checksum);
        if (!headerOk || (!payload.empty() && !WriteBytes(outFile, payload))) {
            outFile.close(); std::remove(tempPath.c_str()); return false;
        }
        outFile.flush();
        const bool writeOk = outFile.good();
        outFile.close();
        if (!writeOk) { std::remove(tempPath.c_str()); return false; }
        if (std::rename(tempPath.c_str(), filePath.c_str()) != 0) {
            std::remove(tempPath.c_str()); return false;
        }
        return true;
    }

    bool LoadFromFile(const std::string& filePath) {
        if (filePath.empty()) return false;
        std::ifstream inFile(filePath, std::ios::binary);
        if (!inFile.is_open()) return false;
        uint32_t magic = 0, version = 0, payloadSize = 0, expectedChecksum = 0;
        if (!ReadU32(inFile, magic) || !ReadU32(inFile, version) || !ReadU32(inFile, payloadSize) ||
            !ReadU32(inFile, expectedChecksum)) return false;
        if (magic != kMagic || version != kVersion || payloadSize > kMaxPayloadBytes) return false;

        std::vector<uint8_t> payload(payloadSize);
        if (payloadSize != 0 && !ReadBytes(inFile, payload)) return false;
        char trailingByte = 0;
        if (inFile.read(&trailingByte, 1)) return false;
        if (!inFile.eof() || Fnv1a(payload) != expectedChecksum) return false;

        size_t cursor = 0;
        uint32_t slotNameSize = 0, loadedUserIndex = 0, playTimeBits = 0;
        int32_t loadedPlayerLevel = 0;
        if (!ReadU32(payload, cursor, slotNameSize) || slotNameSize > kMaxSlotNameBytes ||
            cursor > payload.size() || slotNameSize > payload.size() - cursor) return false;
        std::string loadedSlotName(reinterpret_cast<const char*>(payload.data() + cursor), slotNameSize);
        cursor += slotNameSize;
        if (!ReadU32(payload, cursor, loadedUserIndex) || !ReadI32(payload, cursor, loadedPlayerLevel) ||
            !ReadU32(payload, cursor, playTimeBits) || cursor != payload.size()) return false;
        const float loadedPlayTime = BitsFloat(playTimeBits);
        if (loadedPlayerLevel < 1 || !std::isfinite(loadedPlayTime) || loadedPlayTime < 0.0f) return false;

        SaveSlotName = std::move(loadedSlotName);
        UserIndex = loadedUserIndex;
        PlayerLevel = loadedPlayerLevel;
        PlayTimeSeconds = loadedPlayTime;
        return true;
    }

private:
    static constexpr uint32_t kMagic = 0x465A5347U;
    static constexpr uint32_t kVersion = 1U;
    static constexpr size_t kMaxSlotNameBytes = 4096U;
    static constexpr size_t kMaxPayloadBytes = 1U << 20U;

    static std::string MakeTempPath(const std::string& filePath) {
        static std::atomic<uint64_t> sequence{0};
        const uint64_t id = sequence.fetch_add(1, std::memory_order_relaxed);
        return filePath + ".tmp." + std::to_string(id);
    }
    bool IsValidState() const { return PlayerLevel >= 1 && std::isfinite(PlayTimeSeconds) && PlayTimeSeconds >= 0.0f; }
    static uint32_t FloatBits(float value) { uint32_t bits = 0; std::memcpy(&bits, &value, sizeof(bits)); return bits; }
    static float BitsFloat(uint32_t bits) { float value = 0.0f; std::memcpy(&value, &bits, sizeof(value)); return value; }
    static void AppendU32(std::vector<uint8_t>& out, uint32_t value) {
        out.push_back(static_cast<uint8_t>(value)); out.push_back(static_cast<uint8_t>(value >> 8U));
        out.push_back(static_cast<uint8_t>(value >> 16U)); out.push_back(static_cast<uint8_t>(value >> 24U));
    }
    static void AppendI32(std::vector<uint8_t>& out, int32_t value) { AppendU32(out, static_cast<uint32_t>(value)); }
    static bool ReadU32(const std::vector<uint8_t>& data, size_t& cursor, uint32_t& value) {
        if (cursor > data.size() || data.size() - cursor < 4U) return false;
        value = static_cast<uint32_t>(data[cursor]) | (static_cast<uint32_t>(data[cursor + 1U]) << 8U) |
                (static_cast<uint32_t>(data[cursor + 2U]) << 16U) | (static_cast<uint32_t>(data[cursor + 3U]) << 24U);
        cursor += 4U; return true;
    }
    static bool ReadI32(const std::vector<uint8_t>& data, size_t& cursor, int32_t& value) {
        uint32_t raw = 0; if (!ReadU32(data, cursor, raw)) return false; value = static_cast<int32_t>(raw); return true;
    }
    static bool WriteU32(std::ofstream& out, uint32_t value) {
        const uint8_t bytes[4] = {static_cast<uint8_t>(value), static_cast<uint8_t>(value >> 8U),
                                   static_cast<uint8_t>(value >> 16U), static_cast<uint8_t>(value >> 24U)};
        out.write(reinterpret_cast<const char*>(bytes), sizeof(bytes)); return out.good();
    }
    static bool WriteBytes(std::ofstream& out, const std::vector<uint8_t>& bytes) {
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())); return out.good();
    }
    static bool ReadU32(std::ifstream& in, uint32_t& value) {
        uint8_t bytes[4] = {}; in.read(reinterpret_cast<char*>(bytes), sizeof(bytes));
        if (in.gcount() != static_cast<std::streamsize>(sizeof(bytes))) return false;
        value = static_cast<uint32_t>(bytes[0]) | (static_cast<uint32_t>(bytes[1]) << 8U) |
                (static_cast<uint32_t>(bytes[2]) << 16U) | (static_cast<uint32_t>(bytes[3]) << 24U); return true;
    }
    static bool ReadBytes(std::ifstream& in, std::vector<uint8_t>& bytes) {
        in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        return in.gcount() == static_cast<std::streamsize>(bytes.size());
    }
    static uint32_t Fnv1a(const std::vector<uint8_t>& data) {
        uint32_t hash = 2166136261U; for (uint8_t byte : data) { hash ^= byte; hash *= 16777619U; } return hash;
    }
};
