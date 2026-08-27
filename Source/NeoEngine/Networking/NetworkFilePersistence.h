#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace NeoEngine::Networking {

class NetworkFilePersistence {
public:
    static constexpr size_t MaxStateBytes = 4 * 1024 * 1024;

    explicit NetworkFilePersistence(std::filesystem::path root) : root_(std::move(root)) {}

    bool save(uint64_t sessionId, uint64_t revision, const std::byte* data, size_t size) const {
        if (!sessionId || !revision || !data || !size || size > MaxStateBytes) return false;
        std::error_code ec;
        std::filesystem::create_directories(root_, ec);
        if (ec) return false;

        const auto finalPath = root_ / (std::to_string(sessionId) + ".state");
        const auto tempPath = root_ / (std::to_string(sessionId) + ".state.tmp");
        std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out.write(reinterpret_cast<const char*>(&revision), sizeof(revision));
        out.write(reinterpret_cast<const char*>(&size), sizeof(size));
        out.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
        out.flush();
        if (!out.good()) return false;
        out.close();

        std::filesystem::rename(tempPath, finalPath, ec);
        if (ec) {
            std::filesystem::remove(finalPath, ec);
            ec.clear();
            std::filesystem::rename(tempPath, finalPath, ec);
        }
        if (ec) {
            std::filesystem::remove(tempPath, ec);
            return false;
        }
        return true;
    }

    bool load(uint64_t sessionId, uint64_t revision, std::vector<std::byte>& out) const {
        if (!sessionId || !revision) return false;
        const auto path = root_ / (std::to_string(sessionId) + ".state");
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;

        uint64_t storedRevision{};
        uint64_t storedSize{};
        in.read(reinterpret_cast<char*>(&storedRevision), sizeof(storedRevision));
        in.read(reinterpret_cast<char*>(&storedSize), sizeof(storedSize));
        if (!in || storedRevision != revision || !storedSize || storedSize > MaxStateBytes) return false;

        out.resize(static_cast<size_t>(storedSize));
        in.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(storedSize));
        return in.good() || in.eof();
    }

private:
    std::filesystem::path root_;
};

} // namespace NeoEngine::Networking
