#include "StreamManager.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <utility>

namespace NeoEngine {
namespace {
constexpr size_t kReadChunkBytes = 64U * 1024U;
}

StreamManager::StreamManager(int maxWorkers, size_t maxQueuedRequests,
                             size_t maxAssetBytes, size_t maxResidentBytes) noexcept
    : m_WorkerCount(static_cast<size_t>(std::clamp(maxWorkers, 1, 32))),
      m_QueuedLimit(maxQueuedRequests), m_MaxAssetBytes(maxAssetBytes),
      m_MaxResidentBytes(maxResidentBytes) {
    try { m_Workers.reserve(m_WorkerCount); }
    catch (...) { m_QueuedLimit = 0U; m_WorkerCount = 0U; }
}

StreamManager::~StreamManager() noexcept { Stop(); }

void StreamManager::Start() noexcept {
    std::unique_lock<std::mutex> lock(m_Mutex);
    if (m_Running || m_QueuedLimit == 0U || m_MaxAssetBytes == 0U || m_MaxResidentBytes == 0U) return;
    m_Running = true;
    const size_t workerCount = m_WorkerCount;
    try {
        for (size_t index = 0U; index < workerCount; ++index) {
            m_Workers.emplace_back(&StreamManager::WorkerLoop, this);
        }
    } catch (...) {
        m_Running = false;
        auto queuedRequests = std::move(m_Queue);
        for (const QueuedRequest& queued : queuedRequests) {
            const auto it = m_Requests.find(queued.request.assetPath);
            if (it != m_Requests.end() && it->second == queued.cancellation) {
                queued.cancellation->cancelled.store(true, std::memory_order_release);
                m_Requests.erase(it);
            }
        }
        for (auto& [path, cancellation] : m_Requests)
            cancellation->cancelled.store(true, std::memory_order_release);
        auto workers = std::move(m_Workers);
        lock.unlock();
        m_Condition.notify_all();
        for (const QueuedRequest& queued : queuedRequests) {
            if (queued.request.onComplete) {
                try { queued.request.onComplete(false); } catch (...) {}
            }
        }
        for (std::thread& worker : workers) if (worker.joinable()) worker.join();
        return;
    }
    lock.unlock();
    m_Condition.notify_all();
}

void StreamManager::Stop() noexcept {
    std::vector<std::thread> workers;
    std::vector<QueuedRequest> queuedRequests;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Running = false;

        // Move queued requests out without allocation. Their callbacks can be
        // invoked after releasing the manager lock; active requests remain in
        // m_Requests so their worker owns the single completion callback.
        queuedRequests = std::move(m_Queue);
        for (const QueuedRequest& queued : queuedRequests) {
            const auto it = m_Requests.find(queued.request.assetPath);
            if (it != m_Requests.end() && it->second == queued.cancellation) {
                queued.cancellation->cancelled.store(true, std::memory_order_release);
                m_Requests.erase(it);
            }
        }
        for (auto& [path, cancellation] : m_Requests)
            cancellation->cancelled.store(true, std::memory_order_release);
        workers = std::move(m_Workers);
    }
    m_Condition.notify_all();
    for (const QueuedRequest& queued : queuedRequests) {
        if (queued.request.onComplete) {
            try { queued.request.onComplete(false); } catch (...) {}
        }
    }
    for (std::thread& worker : workers) if (worker.joinable()) worker.join();
}

bool StreamManager::RequestLoad(const std::string& path, int priority,
                                std::function<void(const std::vector<uint8_t>&)> callback,
                                std::function<void(bool)> onComplete) noexcept {
    if (path.empty()) return false;
    try {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (m_Queue.size() >= m_QueuedLimit || m_Requests.contains(path) || m_NextSequence == std::numeric_limits<uint64_t>::max()) return false;
        auto cancellation = std::make_shared<CancellationState>();
        AsyncFileStreamRequest request{path, priority, std::move(callback), std::move(onComplete)};
        QueuedRequest queued{std::move(request), m_NextSequence++, cancellation};
        const auto [requestIt, inserted] = m_Requests.emplace(path, cancellation);
        if (!inserted) return false;
        try { m_Queue.push_back(std::move(queued)); }
        catch (...) { m_Requests.erase(requestIt); return false; }
    } catch (...) { return false; }
    m_Condition.notify_one();
    return true;
}

bool StreamManager::Cancel(const std::string& path) noexcept {
    if (path.empty()) return false;

    std::function<void(bool)> cancelledCallback;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        const auto it = m_Requests.find(path);
        if (it == m_Requests.end()) return false;
        it->second->cancelled.store(true, std::memory_order_release);

        // A queued request can be removed immediately and must complete here.
        // An active request remains in m_Requests so its worker owns the single
        // completion callback and can report cancellation after bounded I/O.
        const auto queued = std::find_if(m_Queue.begin(), m_Queue.end(),
            [&path](const QueuedRequest& request) { return request.request.assetPath == path; });
        if (queued != m_Queue.end()) {
            if (queued->request.onComplete) cancelledCallback = queued->request.onComplete;
            m_Queue.erase(queued);
            m_Requests.erase(it);
        }
    }

    m_Condition.notify_all();
    if (cancelledCallback) {
        try { cancelledCallback(false); } catch (...) {}
    }
    return true;
}

const std::vector<uint8_t>* StreamManager::GetAsset(const std::string& path) const noexcept {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_LoadedAssets.find(path);
    return it != m_LoadedAssets.end() && it->second.loaded ? &it->second.data : nullptr;
}

bool StreamManager::GetAssetCopy(const std::string& path, std::vector<uint8_t>& out) const noexcept {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_LoadedAssets.find(path);
    if (it == m_LoadedAssets.end() || !it->second.loaded) return false;
    try { out = it->second.data; }
    catch (...) { return false; }
    return true;
}

bool StreamManager::IsLoaded(const std::string& path) const noexcept {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_LoadedAssets.find(path);
    return it != m_LoadedAssets.end() && it->second.loaded;
}

void StreamManager::UnloadAsset(const std::string& path) noexcept {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_LoadedAssets.find(path);
    if (it == m_LoadedAssets.end()) return;
    m_ResidentBytes -= it->second.data.size();
    m_LoadedAssets.erase(it);
}

size_t StreamManager::GetQueueSize() const noexcept {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Queue.size();
}
size_t StreamManager::GetLoadedCount() const noexcept {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_LoadedAssets.size();
}
size_t StreamManager::GetResidentBytes() const noexcept {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_ResidentBytes;
}

bool StreamManager::LoadAssetFile(const std::string& path,
                                 const std::shared_ptr<CancellationState>& cancellation,
                                 std::vector<uint8_t>& data) const noexcept {
    try {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;
        const std::streampos end = file.tellg();
        if (end <= std::streampos(0)) return false;
        const auto fileBytes = static_cast<uintmax_t>(static_cast<std::streamoff>(end));
        if (fileBytes > m_MaxAssetBytes || fileBytes > std::numeric_limits<size_t>::max()) return false;
        const size_t totalBytes = static_cast<size_t>(fileBytes);
        data.resize(totalBytes);
        file.seekg(0, std::ios::beg);
        size_t offset = 0U;
        while (offset < totalBytes) {
            if (cancellation->cancelled.load(std::memory_order_acquire)) { data.clear(); return false; }
            const size_t chunk = std::min(kReadChunkBytes, totalBytes - offset);
            file.read(reinterpret_cast<char*>(data.data() + offset), static_cast<std::streamsize>(chunk));
            if (file.gcount() != static_cast<std::streamsize>(chunk)) { data.clear(); return false; }
            offset += chunk;
        }
        return !cancellation->cancelled.load(std::memory_order_acquire);
    } catch (...) { data.clear(); return false; }
}

void StreamManager::WorkerLoop() noexcept {
    for (;;) {
        QueuedRequest queued;
        {
            std::unique_lock<std::mutex> lock(m_Mutex);
            m_Condition.wait(lock, [this] { return !m_Running || !m_Queue.empty(); });
            if (!m_Running) return;
            const auto next = std::max_element(m_Queue.begin(), m_Queue.end(), [](const QueuedRequest& lhs, const QueuedRequest& rhs) {
                if (lhs.request.priority != rhs.request.priority) return lhs.request.priority < rhs.request.priority;
                return lhs.sequence > rhs.sequence;
            });
            if (next == m_Queue.end()) continue;
            queued = std::move(*next);
            m_Queue.erase(next);
        }

        std::vector<uint8_t> fileData;
        const bool loaded = LoadAssetFile(queued.request.assetPath, queued.cancellation, fileData);
        bool publish = false;
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            const auto pending = m_Requests.find(queued.request.assetPath);
            const bool isCurrent = pending != m_Requests.end() && pending->second == queued.cancellation;
            if (loaded && m_Running && isCurrent && !queued.cancellation->cancelled.load(std::memory_order_acquire)) {
                const auto old = m_LoadedAssets.find(queued.request.assetPath);
                const size_t oldBytes = old == m_LoadedAssets.end() ? 0U : old->second.data.size();
                if (oldBytes <= m_ResidentBytes) {
                    const size_t residentWithoutOld = m_ResidentBytes - oldBytes;
                    if (fileData.size() <= m_MaxResidentBytes && residentWithoutOld <= m_MaxResidentBytes - fileData.size()) {
                        try {
                            StreamedAsset replacement{queued.request.assetPath, fileData, fileData.size(), true};
                            if (old == m_LoadedAssets.end()) {
                                const auto [asset, inserted] = m_LoadedAssets.emplace(queued.request.assetPath, std::move(replacement));
                                if (inserted) { m_ResidentBytes = residentWithoutOld + asset->second.data.size(); publish = true; }
                            } else {
                                old->second.data.swap(replacement.data);
                                old->second.size = old->second.data.size();
                                old->second.loaded = true;
                                m_ResidentBytes = residentWithoutOld + old->second.data.size();
                                publish = true;
                            }
                        } catch (...) { publish = false; }
                    }
                }
            }
            if (isCurrent) m_Requests.erase(pending);
        }
        // Completion is authoritative for every request that reached a worker:
        // success, I/O failure, resident-budget rejection, shutdown, or active
        // cancellation. Queued cancellation is completed by Cancel/Stop above.
        if (publish && !queued.cancellation->cancelled.load(std::memory_order_acquire) &&
            queued.request.onLoaded) {
            try { queued.request.onLoaded(fileData); } catch (...) {}
        }
        if (queued.request.onComplete) {
            try { queued.request.onComplete(
                publish && !queued.cancellation->cancelled.load(std::memory_order_acquire)); }
            catch (...) {}
        }
    }
}

} // namespace NeoEngine
