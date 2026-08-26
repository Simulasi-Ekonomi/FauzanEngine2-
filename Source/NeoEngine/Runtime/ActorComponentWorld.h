#pragma once

#include "SceneWorld.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace NeoEngine {

enum class ActorComponentError : uint8_t {
    None,
    NotInitialized,
    InvalidActor,
    InvalidComponent,
    DuplicateComponent,
    Capacity,
    InvalidName,
    AttachRejected,
    DetachRejected,
    TickRejected,
};

class IActorComponent {
public:
    virtual ~IActorComponent() = default;
    [[nodiscard]] virtual uint16_t TypeId() const = 0;
    [[nodiscard]] virtual bool OnAttach(SceneWorld& world, SceneEntity actor) = 0;
    [[nodiscard]] virtual bool OnDetach(SceneWorld& world, SceneEntity actor) = 0;
    [[nodiscard]] virtual bool OnFixedTick(SceneWorld& world, SceneEntity actor, uint32_t fixedTicks) = 0;
};

struct ActorComponentWorldReceipt {
    uint32_t actorCount = 0U;
    uint32_t componentCount = 0U;
    uint32_t tickedComponents = 0U;
    uint64_t registrationRevision = 0U;
};

class ActorComponentWorld {
public:
    static constexpr uint16_t kCapacity = SceneWorld::kCapacity;
    static constexpr uint8_t kMaxComponentsPerActor = 16U;
    static constexpr uint8_t kMaxNameBytes = 64U;

    explicit ActorComponentWorld(SceneWorld& sceneWorld);
    ActorComponentWorld(const ActorComponentWorld&) = delete;
    ActorComponentWorld& operator=(const ActorComponentWorld&) = delete;
    ~ActorComponentWorld();

    bool CreateActor(SceneEntity& output, std::string name = {});
    bool DestroyActor(SceneEntity actor);
    bool AttachComponent(SceneEntity actor, std::unique_ptr<IActorComponent> component);
    bool DetachComponent(SceneEntity actor, uint16_t typeId);
    bool SetComponentEnabled(SceneEntity actor, uint16_t typeId, bool enabled);
    bool TickFixed(uint32_t fixedTicks, ActorComponentWorldReceipt& receipt);

    [[nodiscard]] bool IsActorAlive(SceneEntity actor) const;
    [[nodiscard]] const std::string* ActorName(SceneEntity actor) const;
    [[nodiscard]] IActorComponent* FindComponent(SceneEntity actor, uint16_t typeId);
    [[nodiscard]] const IActorComponent* FindComponent(SceneEntity actor, uint16_t typeId) const;
    [[nodiscard]] bool IsComponentEnabled(SceneEntity actor, uint16_t typeId) const;
    [[nodiscard]] uint8_t ComponentCount(SceneEntity actor) const;
    [[nodiscard]] uint32_t ActorCount() const { return actorCount_; }
    [[nodiscard]] uint32_t ComponentCount() const { return componentCount_; }
    [[nodiscard]] uint64_t RegistrationRevision() const { return registrationRevision_; }
    [[nodiscard]] ActorComponentError LastError() const { return lastError_; }
    [[nodiscard]] const ActorComponentWorldReceipt& LastReceipt() const { return lastReceipt_; }
    [[nodiscard]] SceneWorld& Scene() { return sceneWorld_; }
    [[nodiscard]] const SceneWorld& Scene() const { return sceneWorld_; }

private:
    struct ComponentSlot {
        std::unique_ptr<IActorComponent> component;
        bool enabled = true;
    };
    struct ActorSlot {
        bool registered = false;
        SceneEntity scene{};
        std::string name;
        std::array<ComponentSlot, kMaxComponentsPerActor> components{};
    };

    bool Fail(ActorComponentError error);
    bool ValidActor(SceneEntity actor) const;
    ComponentSlot* FindSlot(SceneEntity actor, uint16_t typeId);
    const ComponentSlot* FindSlot(SceneEntity actor, uint16_t typeId) const;
    ActorSlot* FindActorSlot(SceneEntity actor);
    const ActorSlot* FindActorSlot(SceneEntity actor) const;

    SceneWorld& sceneWorld_;
    std::array<ActorSlot, kCapacity> actors_{};
    uint32_t actorCount_ = 0U;
    uint32_t componentCount_ = 0U;
    uint64_t registrationRevision_ = 0U;
    ActorComponentWorldReceipt lastReceipt_{};
    ActorComponentError lastError_ = ActorComponentError::NotInitialized;
};

} // namespace NeoEngine
