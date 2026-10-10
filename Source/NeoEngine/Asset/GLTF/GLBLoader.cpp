#include "GLBLoader.h"

#include <rapidjson/document.h>
#include <filesystem>
#include <fstream>
#include <limits>

namespace {
constexpr std::uint32_t kGlbMagic = 0x46546C67U;
constexpr std::uint32_t kGlbVersion = 2U;
constexpr std::uint32_t kJsonChunk = 0x4E4F534AU;
constexpr std::uint32_t kBinChunk = 0x004E4942U;
constexpr std::uint32_t kHeaderBytes = 12U;

bool ReadU32(std::ifstream& file, std::uint32_t& value) {
    return static_cast<bool>(file.read(reinterpret_cast<char*>(&value), sizeof(value)));
}
}

bool GLBLoader::Load(const std::string& path) {
    jsonChunk.clear();
    binaryChunk.clear();
    lastError_ = GLBLoadError::None;

    if (path.empty()) {
        lastError_ = GLBLoadError::EmptyPath;
        return false;
    }

    std::error_code ec;
    const auto fileSize = std::filesystem::file_size(path, ec);
    if (ec || fileSize < kHeaderBytes || fileSize > std::numeric_limits<std::uint32_t>::max()) {
        lastError_ = GLBLoadError::HeaderTruncated;
        return false;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        lastError_ = GLBLoadError::OpenFailed;
        return false;
    }

    std::uint32_t magic = 0, version = 0, length = 0;
    if (!ReadU32(file, magic) || !ReadU32(file, version) || !ReadU32(file, length)) {
        lastError_ = GLBLoadError::HeaderTruncated;
        return false;
    }
    if (magic != kGlbMagic) {
        lastError_ = GLBLoadError::InvalidMagic;
        return false;
    }
    if (version != kGlbVersion) {
        lastError_ = GLBLoadError::UnsupportedVersion;
        return false;
    }
    if (length < kHeaderBytes || length != fileSize) {
        lastError_ = GLBLoadError::InvalidLength;
        return false;
    }

    std::uint64_t offset = kHeaderBytes;
    bool seenJson = false;
    while (offset < length) {
        std::uint32_t chunkLength = 0, chunkType = 0;
        if (!ReadU32(file, chunkLength) || !ReadU32(file, chunkType)) {
            lastError_ = GLBLoadError::ChunkTruncated;
            return false;
        }
        offset += 8U;
        if (chunkLength > length - offset) {
            lastError_ = GLBLoadError::ChunkOutOfBounds;
            return false;
        }

        std::vector<std::uint8_t> data(chunkLength);
        if (chunkLength != 0U && !file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(chunkLength))) {
            lastError_ = GLBLoadError::ChunkTruncated;
            return false;
        }
        offset += chunkLength;

        if (chunkType == kJsonChunk) {
            if (seenJson) {
                lastError_ = GLBLoadError::DuplicateJSON;
                return false;
            }
            seenJson = true;
            jsonChunk.assign(reinterpret_cast<const char*>(data.data()), data.size());
            while (!jsonChunk.empty() && (jsonChunk.back() == '\0' || jsonChunk.back() == ' ' || jsonChunk.back() == '\t' || jsonChunk.back() == '\r' || jsonChunk.back() == '\n')) {
                jsonChunk.pop_back();
            }
        } else if (chunkType == kBinChunk) {
            if (!binaryChunk.empty()) {
                lastError_ = GLBLoadError::ChunkOutOfBounds;
                return false;
            }
            binaryChunk = std::move(data);
        }
    }

    if (!seenJson || jsonChunk.empty()) {
        lastError_ = GLBLoadError::MissingJSON;
        return false;
    }

    rapidjson::Document document;
    document.Parse(jsonChunk.data(), jsonChunk.size());
    if (document.HasParseError() || !document.IsObject()) {
        lastError_ = GLBLoadError::InvalidJSON;
        return false;
    }

    lastError_ = GLBLoadError::None;
    return true;
}