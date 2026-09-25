#pragma once
#include <cstdint>
#include <string>
#include <vector>

enum class GLBLoadError : std::uint8_t {
    None,
    EmptyPath,
    OpenFailed,
    HeaderTruncated,
    InvalidMagic,
    UnsupportedVersion,
    InvalidLength,
    ChunkTruncated,
    ChunkOutOfBounds,
    DuplicateJSON,
    MissingJSON,
    InvalidJSON
};

struct GLBChunk {
    std::uint32_t length = 0;
    std::uint32_t type = 0;
    std::vector<std::uint8_t> data;
};

class GLBLoader {
public:
    [[nodiscard]] bool Load(const std::string& path);
    [[nodiscard]] const std::string& GetJSON() const noexcept { return jsonChunk; }
    [[nodiscard]] const std::vector<std::uint8_t>& GetBinary() const noexcept { return binaryChunk; }
    [[nodiscard]] GLBLoadError LastError() const noexcept { return lastError_; }

private:
    std::string jsonChunk;
    std::vector<std::uint8_t> binaryChunk;
    GLBLoadError lastError_ = GLBLoadError::EmptyPath;
};