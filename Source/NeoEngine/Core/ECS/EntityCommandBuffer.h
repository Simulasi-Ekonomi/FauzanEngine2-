#pragma once
#include <vector>
#include <queue>
#include <mutex>

namespace NeoEngine {
    using EntityID = uint32_t;

    enum class ECBCommand : uint8_t { Create, Destroy, SetComponent, RemoveComponent };

    struct ECBEntry {
        ECBCommand cmd;
        EntityID id;
        uint32_t componentMask = 0;
    };

    class EntityCommandBuffer {
    public:
        void Create(EntityID id) {
            std::lock_guard<std::mutex> lock(mtx_);
            commands_.push({ECBCommand::Create, id, 0});
        }
        void Destroy(EntityID id) {
            std::lock_guard<std::mutex> lock(mtx_);
            commands_.push({ECBCommand::Destroy, id, 0});
        }
        void SetComponent(EntityID id, uint32_t mask) {
            std::lock_guard<std::mutex> lock(mtx_);
            commands_.push({ECBCommand::SetComponent, id, mask});
        }
        void Playback(class EntityManager& em) {
            std::lock_guard<std::mutex> lock(mtx_);
            while (!commands_.empty()) {
                auto& e = commands_.front();
                switch (e.cmd) {
                    case ECBCommand::Create: em.CreateEntity(); break;
                    case ECBCommand::Destroy: em.DestroyEntity(e.id); break;
                    case ECBCommand::SetComponent: em.AddComponent(e.id, e.componentMask); break;
                    default: break;
                }
                commands_.pop();
            }
        }
    private:
        std::queue<ECBEntry> commands_;
        std::mutex mtx_;
    };
} // namespace
