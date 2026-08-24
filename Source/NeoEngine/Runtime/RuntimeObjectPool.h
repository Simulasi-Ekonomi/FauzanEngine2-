#pragma once

#include <array>
#include <cstdint>
#include <new>
#include <utility>

namespace NeoEngine {

struct PooledHandle { uint16_t index = UINT16_MAX; uint16_t generation = 0; friend bool operator==(const PooledHandle&, const PooledHandle&) = default; };
enum class RuntimeObjectPoolError : uint8_t { None, Capacity, InvalidHandle, AlreadyReleased };

template <typename T, uint16_t Capacity>
class RuntimeObjectPool {
    static_assert(Capacity > 0 && Capacity < UINT16_MAX);
public:
    template <typename... Args>
    bool Acquire(PooledHandle& handle, Args&&... args) {
        for (uint16_t index = 0; index < Capacity; ++index) if (!slots_[index].alive) {
            Slot& slot = slots_[index]; new (slot.bytes) T(std::forward<Args>(args)...); slot.alive = true; handle = {index, slot.generation}; lastError_ = RuntimeObjectPoolError::None; return true;
        }
        lastError_ = RuntimeObjectPoolError::Capacity; return false;
    }
    bool Release(PooledHandle handle) {
        if (handle.index >= Capacity) { lastError_ = RuntimeObjectPoolError::InvalidHandle; return false; }
        Slot& slot = slots_[handle.index]; if (slot.generation != handle.generation) { lastError_ = RuntimeObjectPoolError::InvalidHandle; return false; }
        if (!slot.alive) { lastError_ = RuntimeObjectPoolError::AlreadyReleased; return false; }
        reinterpret_cast<T*>(slot.bytes)->~T(); slot.alive = false; ++slot.generation; if (slot.generation == 0) ++slot.generation; lastError_ = RuntimeObjectPoolError::None; return true;
    }
    T* Get(PooledHandle handle) { if (handle.index >= Capacity) return nullptr; Slot& slot = slots_[handle.index]; return slot.alive && slot.generation == handle.generation ? reinterpret_cast<T*>(slot.bytes) : nullptr; }
    const T* Get(PooledHandle handle) const { return const_cast<RuntimeObjectPool*>(this)->Get(handle); }
    [[nodiscard]] uint16_t AliveCount() const { uint16_t count = 0; for (const auto& slot : slots_) count += slot.alive ? 1U : 0U; return count; }
    [[nodiscard]] RuntimeObjectPoolError LastError() const { return lastError_; }
    ~RuntimeObjectPool() { for (auto& slot : slots_) if (slot.alive) reinterpret_cast<T*>(slot.bytes)->~T(); }
    RuntimeObjectPool() = default; RuntimeObjectPool(const RuntimeObjectPool&) = delete; RuntimeObjectPool& operator=(const RuntimeObjectPool&) = delete;
private:
    struct Slot { alignas(T) std::byte bytes[sizeof(T)]; uint16_t generation = 1; bool alive = false; };
    std::array<Slot, Capacity> slots_{}; RuntimeObjectPoolError lastError_ = RuntimeObjectPoolError::None;
};

} // namespace NeoEngine
