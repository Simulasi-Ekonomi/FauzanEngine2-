#pragma once
// =========================================================
// NeoEngine Autonomous - TelemetryBus
// Lock-free telemetry collection + socket to Python agents
// Thread-safe, high-performance, zero-copy where possible
// =========================================================

#include <atomic>
#include <array>
#include <string>
#include <chrono>
#include <thread>
#include <functional>
#include <cstring>
#include <cstdio>
#include <mutex>
#include <memory>
#include <vector>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using SocketHandle = SOCKET;
    #define NEO_INVALID_SOCKET INVALID_SOCKET
    #define NEO_SOCKET_ERROR SOCKET_ERROR
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    using SocketHandle = int;
    #define NEO_INVALID_SOCKET (-1)
    #define NEO_SOCKET_ERROR (-1)
#endif

namespace Neo {
namespace Autonomous {

// =========================================================
// Lock-Free SPSC (Single Producer Single Consumer) Queue
// =========================================================

template<typename T, size_t Capacity>
class LockFreeQueue {
    static_assert((Capacity & (Capacity - 1)) == 0,
                  "Capacity must be power of 2");
public:
    LockFreeQueue() : m_Head(0), m_Tail(0) {}

    bool TryPush(const T& item) {
        size_t head = m_Head.load(std::memory_order_relaxed);
        size_t next = (head + 1) & (Capacity - 1);
        if (next == m_Tail.load(std::memory_order_acquire)) {
            return false; // queue full
        }
        m_Buffer[head] = item;
        m_Head.store(next, std::memory_order_release);
        return true;
    }

    bool TryPop(T& item) {
        size_t tail = m_Tail.load(std::memory_order_relaxed);
        if (tail == m_Head.load(std::memory_order_acquire)) {
            return false; // queue empty
        }
        item = m_Buffer[tail];
        m_Tail.store((tail + 1) & (Capacity - 1), std::memory_order_release);
        return true;
    }

    size_t Size() const {
        size_t head = m_Head.load(std::memory_order_acquire);
        size_t tail = m_Tail.load(std::memory_order_acquire);
        return (head - tail + Capacity) & (Capacity - 1);
    }

    bool IsEmpty() const {
        return m_Head.load(std::memory_order_acquire) ==
               m_Tail.load(std::memory_order_acquire);
    }

    bool IsFull() const {
        size_t head = m_Head.load(std::memory_order_acquire);
        size_t next = (head + 1) & (Capacity - 1);
        return next == m_Tail.load(std::memory_order_acquire);
    }

private:
    alignas(64) std::atomic<size_t> m_Head;
    alignas(64) std::atomic<size_t> m_Tail;
    std::array<T, Capacity> m_Buffer;
};

// =========================================================
// Telemetry Sample - compact data struct for the queue
// =========================================================

struct TelemetrySample {
    uint64_t timestampUs = 0;   // microseconds since engine start
    float frameTimeMs = 0.0f;
    float cpuTemp = 0.0f;
    float gpuTemp = 0.0f;
    float cpuUsage = 0.0f;
    float gpuUsage = 0.0f;
    float memoryUsedMB = 0.0f;
    float memoryBudgetMB = 0.0f;
    float batteryPercent = 0.0f;
    int32_t entityCount = 0;
    int32_t drawCalls = 0;
    int32_t triangles = 0;
    int32_t throttleLevel = 0;
    float healthScore = 0.0f;
    int32_t activeFaults = 0;

    // Serialize to JSON for Python agent consumption
    int ToJSON(char* buf, size_t bufSize) const {
        return snprintf(buf, bufSize,
            "{\"ts\":%llu,\"ft\":%.2f,\"ct\":%.1f,\"gt\":%.1f,"
            "\"cu\":%.1f,\"gu\":%.1f,\"mu\":%.1f,\"mb\":%.1f,"
            "\"bat\":%.1f,\"ent\":%d,\"dc\":%d,\"tri\":%d,"
            "\"thr\":%d,\"hp\":%.3f,\"flt\":%d}\n",
            static_cast<unsigned long long>(timestampUs),
            frameTimeMs, cpuTemp, gpuTemp,
            cpuUsage, gpuUsage, memoryUsedMB, memoryBudgetMB,
            batteryPercent, entityCount, drawCalls, triangles,
            throttleLevel, healthScore, activeFaults);
    }
};

// =========================================================
// Aries Agent Command - received from Python
// =========================================================

struct AriesCommand {
    enum class Type : uint8_t {
        None = 0,
        SetThrottle,        // Set CPU throttle level
        SetRenderQuality,   // Adjust render quality
        SetLODBias,         // Adjust LOD bias
        SetDrawDistance,     // Adjust draw distance
        ToggleShadows,      // Enable/disable shadows
        RequestSnapshot,    // Request engine state snapshot
        ForceGC,            // Force garbage collection
        SetTargetFPS,       // Change target framerate
        CustomAction        // Freeform action string
    };

    Type type = Type::None;
    float floatValue = 0.0f;
    int32_t intValue = 0;
    char stringValue[128] = {0};
};

// =========================================================
// TelemetryBus - main telemetry hub
// =========================================================

class TelemetryBus {
public:
    static constexpr size_t QUEUE_CAPACITY = 1024; // must be power of 2
    static constexpr int DEFAULT_PORT = 9876;
    static constexpr size_t MAX_JSON_SIZE = 1024;

    using CommandCallback = std::function<void(const AriesCommand&)>;

    TelemetryBus()
        : m_Socket(NEO_INVALID_SOCKET)
        , m_Running(false)
        , m_Connected(false)
        , m_Port(DEFAULT_PORT)
        , m_SampleCount(0)
        , m_DroppedSamples(0)
        , m_StartTime(std::chrono::steady_clock::now())
        , m_ThrottleThresholdCelsius(75.0f)
    {}

    ~TelemetryBus() {
        Stop();
    }

    // Non-copyable
    TelemetryBus(const TelemetryBus&) = delete;
    TelemetryBus& operator=(const TelemetryBus&) = delete;

    // Start the telemetry bus with socket connection to Python agent
    bool Start(const std::string& host = "127.0.0.1", int port = DEFAULT_PORT) {
        if (m_Running.load()) return true;

        m_Host = host;
        m_Port = port;

        #ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            return false;
        }
        #endif

        m_Running.store(true);

        // Start sender thread
        m_SenderThread = std::make_unique<std::thread>(&TelemetryBus::SenderLoop, this);

        // Start receiver thread (for Aries commands)
        m_ReceiverThread = std::make_unique<std::thread>(&TelemetryBus::ReceiverLoop, this);

        return true;
    }

    // Stop the telemetry bus
    void Stop() {
        m_Running.store(false);

        if (m_SenderThread && m_SenderThread->joinable()) {
            m_SenderThread->join();
        }
        if (m_ReceiverThread && m_ReceiverThread->joinable()) {
            m_ReceiverThread->join();
        }

        CloseSocket();

        #ifdef _WIN32
        WSACleanup();
        #endif
    }

    // Push a telemetry sample (called from engine thread, lock-free)
    void PushSample(const TelemetrySample& sample) {
        if (!m_OutQueue.TryPush(sample)) {
            m_DroppedSamples.fetch_add(1, std::memory_order_relaxed);
        }
        m_SampleCount.fetch_add(1, std::memory_order_relaxed);
    }

    // Convenience: push metrics directly
    void PushMetrics(float frameTimeMs, float cpuTemp, float gpuTemp,
                     float cpuUsage, float gpuUsage,
                     float memUsedMB, float memBudgetMB,
                     float battery, int entities, int drawCalls,
                     int triangles, int throttle,
                     float health, int faults) {
        TelemetrySample sample;
        auto now = std::chrono::steady_clock::now();
        sample.timestampUs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                now - m_StartTime).count());
        sample.frameTimeMs = frameTimeMs;
        sample.cpuTemp = cpuTemp;
        sample.gpuTemp = gpuTemp;
        sample.cpuUsage = cpuUsage;
        sample.gpuUsage = gpuUsage;
        sample.memoryUsedMB = memUsedMB;
        sample.memoryBudgetMB = memBudgetMB;
        sample.batteryPercent = battery;
        sample.entityCount = entities;
        sample.drawCalls = drawCalls;
        sample.triangles = triangles;
        sample.throttleLevel = throttle;
        sample.healthScore = health;
        sample.activeFaults = faults;

        PushSample(sample);

        // Self-throttling based on CPU temperature
        if (cpuTemp > m_ThrottleThresholdCelsius.load(std::memory_order_relaxed)) {
            m_ShouldThrottle.store(true, std::memory_order_release);
        } else {
            m_ShouldThrottle.store(false, std::memory_order_release);
        }
    }

    // Check if Aries says we should throttle
    bool ShouldThrottle() const {
        return m_ShouldThrottle.load(std::memory_order_acquire);
    }

    // Set throttle threshold (can be updated by Aries agent)
    void SetThrottleThreshold(float tempCelsius) {
        m_ThrottleThresholdCelsius.store(tempCelsius, std::memory_order_release);
    }

    // Register callback for commands from Aries agent
    void SetCommandCallback(CommandCallback callback) {
        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        m_CommandCallback = std::move(callback);
    }

    // Pop a command from the incoming queue (called from engine thread)
    bool PopCommand(AriesCommand& cmd) {
        return m_InQueue.TryPop(cmd);
    }

    // Stats
    uint64_t GetSampleCount() const { return m_SampleCount.load(std::memory_order_relaxed); }
    uint64_t GetDroppedSamples() const { return m_DroppedSamples.load(std::memory_order_relaxed); }
    bool IsConnected() const { return m_Connected.load(std::memory_order_acquire); }
    bool IsRunning() const { return m_Running.load(std::memory_order_acquire); }
    size_t GetQueueSize() const { return m_OutQueue.Size(); }

private:
    bool ConnectSocket() {
        m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_Socket == NEO_INVALID_SOCKET) return false;

        // Set non-blocking for connection timeout
        #ifdef _WIN32
        unsigned long mode = 1;
        ioctlsocket(m_Socket, FIONBIO, &mode);
        #else
        int flags = fcntl(m_Socket, F_GETFL, 0);
        fcntl(m_Socket, F_SETFL, flags | O_NONBLOCK);
        #endif

        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(static_cast<uint16_t>(m_Port));
        inet_pton(AF_INET, m_Host.c_str(), &serverAddr.sin_addr);

        int result = connect(m_Socket,
                            reinterpret_cast<struct sockaddr*>(&serverAddr),
                            sizeof(serverAddr));

        if (result == NEO_SOCKET_ERROR) {
            #ifdef _WIN32
            if (WSAGetLastError() != WSAEWOULDBLOCK) {
            #else
            if (errno != EINPROGRESS) {
            #endif
                CloseSocket();
                return false;
            }

            // Wait for connection with timeout
            fd_set writeSet;
            FD_ZERO(&writeSet);
            FD_SET(m_Socket, &writeSet);
            struct timeval tv;
            tv.tv_sec = 2;
            tv.tv_usec = 0;

            result = select(static_cast<int>(m_Socket) + 1, nullptr, &writeSet, nullptr, &tv);
            if (result <= 0) {
                CloseSocket();
                return false;
            }
        }

        // Set back to blocking
        #ifdef _WIN32
        unsigned long blockMode = 0;
        ioctlsocket(m_Socket, FIONBIO, &blockMode);
        #else
        int bflags = fcntl(m_Socket, F_GETFL, 0);
        fcntl(m_Socket, F_SETFL, bflags & ~O_NONBLOCK);
        #endif

        m_Connected.store(true, std::memory_order_release);
        return true;
    }

    void CloseSocket() {
        if (m_Socket != NEO_INVALID_SOCKET) {
            #ifdef _WIN32
            closesocket(m_Socket);
            #else
            close(m_Socket);
            #endif
            m_Socket = NEO_INVALID_SOCKET;
        }
        m_Connected.store(false, std::memory_order_release);
    }

    void SenderLoop() {
        char jsonBuf[MAX_JSON_SIZE];

        while (m_Running.load(std::memory_order_acquire)) {
            // Try to connect if not connected
            if (!m_Connected.load(std::memory_order_acquire)) {
                if (!ConnectSocket()) {
                    // Wait before retry
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    continue;
                }
            }

            // Drain the queue and send
            TelemetrySample sample;
            bool sent = false;
            while (m_OutQueue.TryPop(sample)) {
                int len = sample.ToJSON(jsonBuf, MAX_JSON_SIZE);
                if (len > 0 && len < static_cast<int>(MAX_JSON_SIZE)) {
                    int result = send(m_Socket, jsonBuf, len, 0);
                    if (result == NEO_SOCKET_ERROR) {
                        CloseSocket();
                        break;
                    }
                    sent = true;
                }
            }

            if (!sent) {
                // No data to send, sleep briefly
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
        }
    }

    void ReceiverLoop() {
        char recvBuf[512];

        while (m_Running.load(std::memory_order_acquire)) {
            if (!m_Connected.load(std::memory_order_acquire)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            // Set receive timeout
            #ifdef _WIN32
            DWORD timeout = 100;
            setsockopt(m_Socket, SOL_SOCKET, SO_RCVTIMEO,
                       reinterpret_cast<const char*>(&timeout), sizeof(timeout));
            #else
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100000; // 100ms
            setsockopt(m_Socket, SOL_SOCKET, SO_RCVTIMEO,
                       reinterpret_cast<const char*>(&tv), sizeof(tv));
            #endif

            int bytesRead = recv(m_Socket, recvBuf, sizeof(recvBuf) - 1, 0);
            if (bytesRead > 0) {
                recvBuf[bytesRead] = '\0';
                ParseAndDispatchCommand(recvBuf, bytesRead);
            } else if (bytesRead == 0) {
                // Connection closed
                CloseSocket();
            }
            // bytesRead < 0 with timeout is normal (no data)
        }
    }

    void ParseAndDispatchCommand(const char* data, int len) {
        // Simple command parsing: "CMD:TYPE:VALUE"
        AriesCommand cmd;

        std::string input(data, static_cast<size_t>(len));

        if (input.find("THROTTLE:") == 0) {
            cmd.type = AriesCommand::Type::SetThrottle;
            cmd.intValue = std::atoi(input.c_str() + 9);
            m_ThrottleThresholdCelsius.store(
                static_cast<float>(cmd.intValue), std::memory_order_release);
        } else if (input.find("QUALITY:") == 0) {
            cmd.type = AriesCommand::Type::SetRenderQuality;
            cmd.floatValue = static_cast<float>(std::atof(input.c_str() + 8));
        } else if (input.find("LOD:") == 0) {
            cmd.type = AriesCommand::Type::SetLODBias;
            cmd.floatValue = static_cast<float>(std::atof(input.c_str() + 4));
        } else if (input.find("DRAWDIST:") == 0) {
            cmd.type = AriesCommand::Type::SetDrawDistance;
            cmd.floatValue = static_cast<float>(std::atof(input.c_str() + 9));
        } else if (input.find("SHADOWS:") == 0) {
            cmd.type = AriesCommand::Type::ToggleShadows;
            cmd.intValue = (input[8] == '1') ? 1 : 0;
        } else if (input.find("SNAPSHOT") == 0) {
            cmd.type = AriesCommand::Type::RequestSnapshot;
        } else if (input.find("GC") == 0) {
            cmd.type = AriesCommand::Type::ForceGC;
        } else if (input.find("FPS:") == 0) {
            cmd.type = AriesCommand::Type::SetTargetFPS;
            cmd.intValue = std::atoi(input.c_str() + 4);
        } else {
            cmd.type = AriesCommand::Type::CustomAction;
            size_t copyLen = std::min(input.size(), sizeof(cmd.stringValue) - 1);
            std::memcpy(cmd.stringValue, input.c_str(), copyLen);
            cmd.stringValue[copyLen] = '\0';
        }

        m_InQueue.TryPush(cmd);

        // Invoke callback if set
        std::lock_guard<std::mutex> lock(m_CallbackMutex);
        if (m_CommandCallback) {
            m_CommandCallback(cmd);
        }
    }

    // Lock-free queues
    LockFreeQueue<TelemetrySample, QUEUE_CAPACITY> m_OutQueue;
    LockFreeQueue<AriesCommand, 64> m_InQueue;

    // Socket
    SocketHandle m_Socket;
    std::string m_Host;
    int m_Port;

    // Threads
    std::unique_ptr<std::thread> m_SenderThread;
    std::unique_ptr<std::thread> m_ReceiverThread;

    // Atomic state
    std::atomic<bool> m_Running;
    std::atomic<bool> m_Connected;
    std::atomic<uint64_t> m_SampleCount;
    std::atomic<uint64_t> m_DroppedSamples;
    std::atomic<float> m_ThrottleThresholdCelsius;
    std::atomic<bool> m_ShouldThrottle{false};

    // Callback
    std::mutex m_CallbackMutex;
    CommandCallback m_CommandCallback;

    // Timing
    std::chrono::steady_clock::time_point m_StartTime;
};

} // namespace Autonomous
} // namespace Neo
