#include "Runtime/StreamManager.h"

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace {
bool WriteFile(const std::filesystem::path& path, const std::string& bytes) {
    std::ofstream file(path, std::ios::binary);
    file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    return file.good();
}
}

int main() {
    namespace fs = std::filesystem;
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path root = fs::temp_directory_path() / ("neo-stream-manager-" + std::to_string(nonce));
    std::error_code ec;
    fs::create_directories(root, ec);
    if (ec) return 1;

    const fs::path highPath = root / "high.bin";
    const fs::path lowPath = root / "low.bin";
    const fs::path tooLargePath = root / "too-large.bin";
    if (!WriteFile(highPath, "12345678") || !WriteFile(lowPath, "abcd") || !WriteFile(tooLargePath, "0123456789")) {
        fs::remove_all(root, ec);
        return 2;
    }

    NeoEngine::StreamManager manager(1, 2, 8, 8);
    std::mutex callbackMutex;
    std::condition_variable callbackCondition;
    std::vector<std::pair<std::string, bool>> completions;
    auto completion = [&callbackMutex, &callbackCondition, &completions](std::string name) {
        return [&, name = std::move(name)](bool success) {
            {
                std::lock_guard<std::mutex> lock(callbackMutex);
                completions.emplace_back(name, success);
            }
            callbackCondition.notify_one();
        };
    };
    auto loaded = [](const std::vector<uint8_t>& bytes) { return !bytes.empty(); };

    if (!manager.RequestLoad(lowPath.string(), 1, loaded, completion("cancelled")) ||
        !manager.RequestLoad(highPath.string(), 10, loaded, completion("high")) ||
        manager.RequestLoad(tooLargePath.string(), 0, {}, completion("unexpected")) || manager.GetQueueSize() != 2U) {
        fs::remove_all(root, ec);
        return 3;
    }
    if (!manager.Cancel(lowPath.string()) || manager.Cancel(lowPath.string()) ||
        manager.GetQueueSize() != 1U || !manager.RequestLoad(lowPath.string(), 1, loaded, completion("low"))) {
        fs::remove_all(root, ec);
        return 4;
    }

    manager.Start();
    {
        std::unique_lock<std::mutex> lock(callbackMutex);
        if (!callbackCondition.wait_for(lock, std::chrono::seconds(5), [&] { return completions.size() == 2U; })) {
            manager.Stop(); fs::remove_all(root, ec); return 5;
        }
        if (completions[0] != std::pair<std::string, bool>{"high", true} ||
            completions[1] != std::pair<std::string, bool>{"low", false}) {
            manager.Stop(); fs::remove_all(root, ec); return 6;
        }
    }
    if (!manager.IsLoaded(highPath.string()) || manager.IsLoaded(lowPath.string()) ||
        manager.GetResidentBytes() != 8U || manager.GetLoadedCount() != 1U) {
        manager.Stop(); fs::remove_all(root, ec); return 7;
    }
    std::vector<uint8_t> snapshot;
    if (!manager.GetAssetCopy(highPath.string(), snapshot) || snapshot.size() != 8U ||
        manager.GetAssetCopy("missing", snapshot)) {
        manager.Stop(); fs::remove_all(root, ec); return 8;
    }

    manager.UnloadAsset(highPath.string());
    if (manager.GetResidentBytes() != 0U ||
        !manager.RequestLoad(lowPath.string(), 3, loaded, completion("low"))) {
        manager.Stop(); fs::remove_all(root, ec); return 9;
    }
    {
        std::unique_lock<std::mutex> lock(callbackMutex);
        if (!callbackCondition.wait_for(lock, std::chrono::seconds(5), [&] { return completions.size() == 3U; }) ||
            completions.back() != std::pair<std::string, bool>{"low", true}) {
            manager.Stop(); fs::remove_all(root, ec); return 10;
        }
    }
    if (!manager.IsLoaded(lowPath.string()) || manager.GetResidentBytes() != 4U) {
        manager.Stop(); fs::remove_all(root, ec); return 11;
    }

    manager.UnloadAsset(lowPath.string());
    if (!manager.RequestLoad(tooLargePath.string(), 1, {}, completion("oversize"))) {
        manager.Stop(); fs::remove_all(root, ec); return 12;
    }
    {
        std::unique_lock<std::mutex> lock(callbackMutex);
        if (!callbackCondition.wait_for(lock, std::chrono::seconds(5), [&] { return completions.size() == 4U; }) ||
            completions.back() != std::pair<std::string, bool>{"oversize", false}) {
            manager.Stop(); fs::remove_all(root, ec); return 13;
        }
    }

    manager.Stop();
    if (manager.GetQueueSize() != 0U || manager.GetResidentBytes() != 0U) {
        fs::remove_all(root, ec); return 14;
    }

    // Shutdown must complete requests that never reached a worker as cancelled.
    if (!manager.RequestLoad(lowPath.string(), 1, {}, completion("stop-cancel"))) {
        fs::remove_all(root, ec); return 15;
    }
    manager.Stop();
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        if (completions.size() != 5U ||
            completions.back() != std::pair<std::string, bool>{"stop-cancel", false}) {
            fs::remove_all(root, ec); return 16;
        }
    }

    // Stop/Start must recreate workers and continue accepting requests.
    if (!manager.RequestLoad(highPath.string(), 1, {}, completion("restart"))) {
        fs::remove_all(root, ec); return 17;
    }
    manager.Start();
    {
        std::unique_lock<std::mutex> lock(callbackMutex);
        if (!callbackCondition.wait_for(lock, std::chrono::seconds(5), [&] { return completions.size() == 6U; }) ||
            completions.back() != std::pair<std::string, bool>{"restart", true}) {
            manager.Stop(); fs::remove_all(root, ec); return 18;
        }
    }
    manager.Stop();
    fs::remove_all(root, ec);
    std::cout << "STREAM_MANAGER_ASYNC_FILE_SMOKE_OK\n";
    return 0;
}

