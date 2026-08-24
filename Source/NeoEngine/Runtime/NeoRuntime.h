#pragma once
#include "AssetRegistry.h"
#include "SceneWorld.h"
#include "SoftwareRenderer.h"
#include "RuntimeClock.h"
#include "RuntimeTimerQueue.h"
#include "EventSignalBus.h"
#include "InputMotionBridge.h"
#include "GridRouteFollower.h"
#include "MovementAuthority.h"
#include "RouteRootMotionAdapter.h"
#include "Systems/FarmSystem.h"
#include "Systems/FarmAuthoritativeService.h"
#include "Systems/FarmWorldTool.h"
#include "Systems/AuthoringCatalog.h"
#include "Systems/WorldAuthoring.h"
#include "Systems/TrustSafetySystem.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace NeoEngine {
enum class RuntimeState : uint8_t { Created, Initialized, Shutdown, Failed };
enum class RuntimeError : uint8_t { None, InvalidConfiguration, InvalidState, FarmTickFailed, WorldTickFailed, AuthoringTickFailed, AuthorityFailed, InputMotionFailed, RouteMotionFailed, RouteReplanFailed, RenderFailed };
enum class SkeletalRouteDirection : uint8_t { PositiveX, NegativeX, PositiveZ, NegativeZ };
struct RuntimeConfig { uint16_t farmWidth = 8; uint16_t farmHeight = 8; uint32_t fixedTicksPerFrame = 1; int64_t initialCoins = 100; uint16_t renderWidth=256; uint16_t renderHeight=256; uint16_t farmNpcCount=8; uint16_t authoringWorldSide=32; uint64_t authoringWorldSeed=0x4E454F574F524C44ULL; bool enableInputMotion=false; float inputMotionUnitsPerSecond=5.0F; bool inputMotionFaceMovementDirection=false; bool enableRouteMotion=false; float routeMotionUnitsPerSecond=5.0F; bool routeMotionFaceMovementDirection=false; bool enableSkeletalRouteMotion=false; SkeletalRouteDirection skeletalRouteDirection=SkeletalRouteDirection::PositiveX; SkeletalPosePlaybackMode skeletalRoutePlaybackMode=SkeletalPosePlaybackMode::Clamp; Skeleton skeletalRouteSkeleton{}; SkeletalPoseClip skeletalRouteClip{}; uint16_t routeMotionNavigationSide=GridNavigation::kMinSide; std::vector<GridCell> routeMotionRoute{}; };
class NeoRuntime {
public:
    bool Initialize(const RuntimeConfig& config);
    bool Tick();
    bool SetPaused(bool paused);
    bool ReplanRouteMotion();
    bool RenderFarm();
    bool Shutdown();
    RuntimeState State() const { return m_State; }
    RuntimeError LastError() const { return m_LastError; }
    FarmSystem* Farm() { return m_Farm.get(); }
    const FarmSystem* Farm() const { return m_Farm.get(); }
    FarmWorldTool* FarmWorld() { return m_FarmWorld.get(); }
    const FarmWorldTool* FarmWorld() const { return m_FarmWorld.get(); }
    FarmAuthoritativeService* FarmAuthority() { return m_FarmAuthority.get(); }
    const FarmAuthoritativeService* FarmAuthority() const { return m_FarmAuthority.get(); }
    TrustSafetySystem* TrustSafety() { return m_TrustSafety.get(); }
    const TrustSafetySystem* TrustSafety() const { return m_TrustSafety.get(); }
    AssetRegistry* Assets() { return m_Assets.get(); }
    const AssetRegistry* Assets() const { return m_Assets.get(); }
    AuthoringCatalog* Authoring() { return m_Authoring.get(); }
    const AuthoringCatalog* Authoring() const { return m_Authoring.get(); }
    WorldAuthoring* AuthoringWorld() { return m_AuthoringWorld.get(); }
    const WorldAuthoring* AuthoringWorld() const { return m_AuthoringWorld.get(); }
    SceneWorld* Scene() { return m_Scene.get(); }
    const SceneWorld* Scene() const { return m_Scene.get(); }
    SoftwareRenderer* Renderer() { return m_Renderer.get(); }
    const SoftwareRenderer* Renderer() const { return m_Renderer.get(); }
    RuntimeClock* Clock() { return m_Clock.get(); }
    const RuntimeClock* Clock() const { return m_Clock.get(); }
    RuntimeTimerQueue* Timers() { return m_Timers.get(); }
    const RuntimeTimerQueue* Timers() const { return m_Timers.get(); }
    EventSignalBus* Events() { return m_Events.get(); }
    const EventSignalBus* Events() const { return m_Events.get(); }
    InputState* Input() { return m_Input.get(); }
    const InputState* Input() const { return m_Input.get(); }
    const SceneEntity* InputMotionEntity() const { return m_InputMotionEntity_.index==0xFFFFU ? nullptr : &m_InputMotionEntity_; }
    GridNavigation* RouteNavigation() { return m_RouteNavigation.get(); }
    const GridNavigation* RouteNavigation() const { return m_RouteNavigation.get(); }
    const SceneEntity* RouteMotionEntity() const { return m_RouteMotionEntity_.index==0xFFFFU ? nullptr : &m_RouteMotionEntity_; }
    const SkeletalAnimationController* SkeletalRouteMotionController() const { return m_SkeletalRouteMotionController.get(); }
    MovementAuthorityGate* MotionAuthority() { return m_MotionAuthority.get(); }
    const MovementAuthorityGate* MotionAuthority() const { return m_MotionAuthority.get(); }
private:
    RuntimeState m_State = RuntimeState::Created;
    RuntimeError m_LastError = RuntimeError::None;
    uint32_t m_FixedTicksPerFrame = 0;
    std::unique_ptr<TrustSafetySystem> m_TrustSafety;
    std::unique_ptr<FarmSystem> m_Farm;
    std::unique_ptr<FarmWorldTool> m_FarmWorld;
    std::unique_ptr<FarmAuthoritativeService> m_FarmAuthority;
    std::unique_ptr<AssetRegistry> m_Assets;
    std::unique_ptr<AuthoringCatalog> m_Authoring;
    std::unique_ptr<WorldAuthoring> m_AuthoringWorld;
    std::unique_ptr<RuntimeClock> m_Clock;
    std::unique_ptr<RuntimeTimerQueue> m_Timers;
    std::unique_ptr<EventSignalBus> m_Events;
    std::unique_ptr<InputState> m_Input;
    std::unique_ptr<KinematicMotionController> m_KinematicMotion;
    std::unique_ptr<InputMotionBridge> m_InputMotion;
    SceneEntity m_InputMotionEntity_{0xFFFFU,0U};
    std::unique_ptr<GridNavigation> m_RouteNavigation;
    std::unique_ptr<KinematicMotionController> m_RouteMotionController;
    std::unique_ptr<GridRouteFollower> m_RouteFollower;
    std::unique_ptr<SkeletalAnimationController> m_SkeletalRouteMotionController;
    std::unique_ptr<RouteRootMotionAdapter> m_RouteRootMotionAdapter;
    std::vector<Mat4> m_SkeletalRoutePalette;
    bool m_UsesSkeletalRouteMotion = false;
    SceneEntity m_RouteMotionEntity_{0xFFFFU,0U};
    std::unique_ptr<MovementAuthorityGate> m_MotionAuthority;
    std::unique_ptr<SceneWorld> m_Scene;
    std::unique_ptr<SoftwareRenderer> m_Renderer;
};
} // namespace NeoEngine
