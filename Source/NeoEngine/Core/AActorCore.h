#pragma once

#include "Core/ActorBase.h"

namespace NeoEngine {

// AActorCore adalah alias langsung dari ActorBase untuk
// kompatibilitas dengan kode yang sudah ada.
// Semua fungsionalitas diwarisi dari ActorBase.
class AActorCore : public ActorBase {
public:
    AActorCore() = default;
    virtual ~AActorCore() = default;

    // Method-method ini sudah diwarisi dari ActorBase,
    // tetapi kita perlu mendeklarasikan ulang agar out-of-line
    // definition di file .cpp bisa dikenali.
    virtual void BeginPlay() override {}
    virtual void Tick(float deltaTime) override {}
    virtual void EndPlay() override {}

    // Forward method dari ActorBase untuk kemudahan akses
    void SetActorLocation(const Vector3& loc) { ActorBase::SetActorLocation(loc); }
    Vector3 GetActorLocation() const { return ActorBase::GetActorLocation(); }
    void SetActorRotation(const Quaternion& rot) { ActorBase::SetActorRotation(rot); }
    Quaternion GetActorRotation() const { return ActorBase::GetActorRotation(); }
    void AddActorWorldRotation(const Quaternion& delta) { ActorBase::AddActorWorldRotation(delta); }
    void SetActorName(const std::string& name) { ActorBase::SetActorName(name); }
    std::string GetActorName() const { return ActorBase::GetActorName(); }
    std::string GetName() const { return ActorBase::GetName(); }
};

} // namespace NeoEngine
