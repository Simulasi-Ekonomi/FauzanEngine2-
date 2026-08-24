#include "NeoRuntime.h"

#include "FarmRenderAdapter.h"

namespace NeoEngine {

bool NeoRuntime::Initialize(const RuntimeConfig& config) {
    if (m_State != RuntimeState::Created || config.fixedTicksPerFrame == 0 || config.initialCoins < 0 || config.farmNpcCount == 0 || config.farmNpcCount > FarmWorldTool::kMaxNpcs) {
        m_LastError = RuntimeError::InvalidConfiguration;
        m_State = RuntimeState::Failed;
        return false;
    }
    auto farm = std::make_unique<FarmSystem>(config.farmWidth, config.farmHeight, config.initialCoins);
    if (!farm->IsReady()) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }
    auto trustSafety = std::make_unique<TrustSafetySystem>();
    farm->SetTrustSafety(trustSafety.get(), "runtime-farm-player");
    auto world = std::make_unique<FarmWorldTool>();
    FarmWorldConfig worldConfig{};
    worldConfig.worldWidth = config.farmWidth;
    worldConfig.worldHeight = config.farmHeight;
    worldConfig.npcCount = config.farmNpcCount;
    if (!world->Initialize(*farm, *trustSafety, "runtime-farm-player", worldConfig)) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }
    auto authority = std::make_unique<FarmAuthoritativeService>();
    if (!authority->Initialize(*world, *trustSafety, "runtime-farm-player", "runtime-farm-session")) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }
    auto renderer = std::make_unique<SoftwareRenderer>();
    if (!renderer->Initialize(config.renderWidth, config.renderHeight)) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }
    auto clock = std::make_unique<RuntimeClock>();
    if (!clock->Initialize()) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }
    auto scene = std::make_unique<SceneWorld>();
    if (!world->PopulateScene(*scene)) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }
    auto authoringWorld = std::make_unique<WorldAuthoring>();
    WorldAuthoringConfig authoringWorldConfig{};
    authoringWorldConfig.side = config.authoringWorldSide;
    authoringWorldConfig.seed = config.authoringWorldSeed;
    if (!authoringWorld->Generate(authoringWorldConfig) || !authoringWorld->BindScene(*scene)) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }

    auto input = std::unique_ptr<InputState>{};
    auto kinematicMotion = std::unique_ptr<KinematicMotionController>{};
    auto inputMotion = std::unique_ptr<InputMotionBridge>{};
    SceneEntity inputMotionEntity{0xFFFFU, 0U};
    if (config.enableInputMotion) {
        input = std::make_unique<InputState>();
        if (!input->Bind("move_forward", MakeInputCode(InputDeviceType::Keyboard, 1U)) || !input->Bind("move_backward", MakeInputCode(InputDeviceType::Keyboard, 2U)) || !input->Bind("move_left", MakeInputCode(InputDeviceType::Keyboard, 3U)) || !input->Bind("move_right", MakeInputCode(InputDeviceType::Keyboard, 4U))) {
            m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false;
        }
        kinematicMotion = std::make_unique<KinematicMotionController>();
        inputMotion = std::make_unique<InputMotionBridge>();
        if (!kinematicMotion->Initialize({config.inputMotionUnitsPerSecond, 0.25F, config.inputMotionFaceMovementDirection}) || !inputMotion->Initialize() || !scene->Create(inputMotionEntity) || !scene->SetTransform(inputMotionEntity, {0.0F, 0.0F, 0.0F, 0, 0, 0, 1, 1, 1})) {
            m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false;
        }
    }
    if (config.enableRouteMotion && config.enableSkeletalRouteMotion) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }
    auto routeNavigation = std::unique_ptr<GridNavigation>{};
    auto routeMotionController = std::unique_ptr<KinematicMotionController>{};
    auto routeFollower = std::unique_ptr<GridRouteFollower>{};
    auto skeletalRouteMotionController = std::unique_ptr<SkeletalAnimationController>{};
    auto routeRootMotionAdapter = std::unique_ptr<RouteRootMotionAdapter>{};
    std::vector<Mat4> skeletalRoutePalette{};
    bool usesSkeletalRouteMotion = false;
    auto motionAuthority = std::make_unique<MovementAuthorityGate>();
    SceneEntity routeMotionEntity{0xFFFFU, 0U};
    if (config.enableRouteMotion || config.enableSkeletalRouteMotion) {
        const size_t expectedRouteCells = config.enableSkeletalRouteMotion ? 2U : 0U;
        if (config.routeMotionRoute.size() < 2U || config.routeMotionRoute.size() > GridRouteFollower::kMaxRouteCells || (expectedRouteCells != 0U && config.routeMotionRoute.size() != expectedRouteCells)) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }
        for (const GridCell cell : config.routeMotionRoute) if (cell.x >= config.routeMotionNavigationSide || cell.z >= config.routeMotionNavigationSide) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }
        routeNavigation = std::make_unique<GridNavigation>();
        routeFollower = std::make_unique<GridRouteFollower>();
        if (!routeNavigation->Initialize(config.routeMotionNavigationSide) || !routeFollower->SetRoute(config.routeMotionRoute) || !scene->Create(routeMotionEntity) || !scene->SetTransform(routeMotionEntity, {static_cast<float>(config.routeMotionRoute.front().x), 0.0F, static_cast<float>(config.routeMotionRoute.front().z), 0, 0, 0, 1, 1, 1})) {
            m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false;
        }
        if (config.enableRouteMotion) {
            routeMotionController = std::make_unique<KinematicMotionController>();
            if (!routeMotionController->Initialize({config.routeMotionUnitsPerSecond, 0.25F, config.routeMotionFaceMovementDirection})) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }
        } else {
            const GridCell first = config.routeMotionRoute.front(), second = config.routeMotionRoute.back();
            const int32_t dx = static_cast<int32_t>(second.x) - first.x, dz = static_cast<int32_t>(second.z) - first.z;
            const bool directionMatches = (config.skeletalRouteDirection == SkeletalRouteDirection::PositiveX && dx == 1 && dz == 0) || (config.skeletalRouteDirection == SkeletalRouteDirection::NegativeX && dx == -1 && dz == 0) || (config.skeletalRouteDirection == SkeletalRouteDirection::PositiveZ && dx == 0 && dz == 1) || (config.skeletalRouteDirection == SkeletalRouteDirection::NegativeZ && dx == 0 && dz == -1);
            skeletalRouteMotionController = std::make_unique<SkeletalAnimationController>();
            routeRootMotionAdapter = std::make_unique<RouteRootMotionAdapter>();
            if (!directionMatches || config.skeletalRoutePlaybackMode != SkeletalPosePlaybackMode::Clamp || !skeletalRouteMotionController->Initialize(config.skeletalRouteSkeleton, config.skeletalRouteClip, config.skeletalRoutePlaybackMode)) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }
            usesSkeletalRouteMotion = true;
        }
    }

    m_FixedTicksPerFrame = config.fixedTicksPerFrame;
    m_TrustSafety = std::move(trustSafety);
    m_Farm = std::move(farm);
    m_FarmWorld = std::move(world);
    m_FarmAuthority = std::move(authority);
    m_Assets = std::make_unique<AssetRegistry>();
    m_Authoring = std::make_unique<AuthoringCatalog>();
    m_AuthoringWorld = std::move(authoringWorld);
    m_Clock = std::move(clock);
    m_Timers = std::make_unique<RuntimeTimerQueue>();
    m_Events = std::make_unique<EventSignalBus>();
    m_Input = std::move(input);
    m_KinematicMotion = std::move(kinematicMotion);
    m_InputMotion = std::move(inputMotion);
    m_InputMotionEntity_ = inputMotionEntity;
    m_RouteNavigation = std::move(routeNavigation);
    m_RouteMotionController = std::move(routeMotionController);
    m_RouteFollower = std::move(routeFollower);
    m_SkeletalRouteMotionController = std::move(skeletalRouteMotionController);
    m_RouteRootMotionAdapter = std::move(routeRootMotionAdapter);
    m_SkeletalRoutePalette = std::move(skeletalRoutePalette);
    m_UsesSkeletalRouteMotion = usesSkeletalRouteMotion;
    m_RouteMotionEntity_ = routeMotionEntity;
    m_MotionAuthority = std::move(motionAuthority);
    m_Scene = std::move(scene);
    m_Renderer = std::move(renderer);
    m_LastError = RuntimeError::None;
    m_State = RuntimeState::Initialized;
    return true;
}

bool NeoRuntime::Tick() {
    if (m_State != RuntimeState::Initialized || !m_Farm || !m_FarmWorld || !m_FarmAuthority || !m_Authoring || !m_AuthoringWorld || !m_Clock || !m_Timers || !m_Events || !m_Scene || !m_Clock->Advance(1.0F / 60.0F)) { m_LastError = RuntimeError::InvalidState; return false; }
    std::vector<RuntimeTimerFire> fires;
    if (!m_Timers->Advance(m_Clock->Snapshot().scaledDeltaSeconds, fires)) { m_LastError = RuntimeError::InvalidState; return false; }
    for (const RuntimeTimerFire& fire : fires) if (!m_Events->Queue({RuntimeEventKind::TimerFired, fire.userTag, static_cast<int32_t>(fire.fireCount), m_Clock->Snapshot().fixedStepCount})) { m_LastError = RuntimeError::InvalidState; return false; }
    if (m_Input != nullptr) m_Input->BeginFrame();
    if (m_Clock->Snapshot().paused) return m_Events->Dispatch();
    if (m_MotionAuthority != nullptr) m_MotionAuthority->BeginFrame();
    if (m_InputMotion != nullptr && (m_Input == nullptr || m_KinematicMotion == nullptr || !m_InputMotion->Step(*m_Input, *m_KinematicMotion, *m_Scene, m_InputMotionEntity_, m_Clock->Snapshot().scaledDeltaSeconds))) { m_LastError = RuntimeError::InputMotionFailed; m_State = RuntimeState::Failed; return false; }
    if (m_RouteFollower != nullptr && !m_RouteFollower->ReachedGoal()) {
        const bool routeSucceeded = m_UsesSkeletalRouteMotion ? (m_RouteNavigation != nullptr && m_SkeletalRouteMotionController != nullptr && m_RouteRootMotionAdapter != nullptr && m_MotionAuthority != nullptr && m_RouteRootMotionAdapter->Advance(m_Clock->Snapshot().scaledDeltaSeconds, *m_RouteFollower, *m_SkeletalRouteMotionController, *m_Scene, m_RouteMotionEntity_, *m_RouteNavigation, *m_MotionAuthority, m_SkeletalRoutePalette)) : (m_RouteNavigation != nullptr && m_RouteMotionController != nullptr && m_MotionAuthority != nullptr && m_RouteFollower->StepGuarded(*m_Scene, m_RouteMotionEntity_, *m_RouteMotionController, *m_RouteNavigation, m_Clock->Snapshot().scaledDeltaSeconds, *m_MotionAuthority));
        if (!routeSucceeded) { m_LastError = RuntimeError::RouteMotionFailed; m_State = RuntimeState::Failed; return false; }
    }
    if (!m_FarmWorld->Tick(m_FixedTicksPerFrame)) { m_LastError = RuntimeError::WorldTickFailed; m_State = RuntimeState::Failed; return false; }
    if (!m_FarmWorld->SyncScene()) { m_LastError = RuntimeError::WorldTickFailed; m_State = RuntimeState::Failed; return false; }
    if (m_Authoring->IsSceneBound() && !m_Authoring->Tick(m_FixedTicksPerFrame)) { m_LastError = RuntimeError::AuthoringTickFailed; m_State = RuntimeState::Failed; return false; }
    return m_Events->Dispatch();
}

bool NeoRuntime::ReplanRouteMotion() {
    if (m_State != RuntimeState::Initialized || !m_Scene || !m_RouteNavigation || !m_RouteFollower || m_RouteMotionEntity_.index == 0xFFFFU) { m_LastError = RuntimeError::InvalidState; return false; }
    if (m_UsesSkeletalRouteMotion) { m_LastError = RuntimeError::RouteReplanFailed; return false; }
    if (!m_RouteFollower->Replan(*m_Scene, m_RouteMotionEntity_, *m_RouteNavigation)) { m_LastError = RuntimeError::RouteReplanFailed; return false; }
    m_LastError = RuntimeError::None;
    return true;
}

bool NeoRuntime::RenderFarm() {
    if (m_State != RuntimeState::Initialized || !m_Farm || !m_FarmWorld || !m_Renderer) { m_LastError = RuntimeError::InvalidState; return false; }
    if (!FarmRenderAdapter::RenderWorld(*m_Farm, *m_FarmWorld, *m_Renderer)) { m_LastError = RuntimeError::RenderFailed; return false; }
    return true;
}

bool NeoRuntime::SetPaused(bool paused) {
    if (m_State != RuntimeState::Initialized || !m_Clock || !m_Clock->SetPaused(paused)) { m_LastError = RuntimeError::InvalidState; return false; }
    return m_Events != nullptr && m_Events->Queue({paused ? RuntimeEventKind::RuntimePaused : RuntimeEventKind::RuntimeResumed, 0, 0, m_Clock->Snapshot().fixedStepCount});
}

bool NeoRuntime::Shutdown() {
    if (m_State != RuntimeState::Initialized && m_State != RuntimeState::Failed) { m_LastError = RuntimeError::InvalidState; return false; }
    m_Renderer.reset();
    m_Clock.reset();
    m_Timers.reset();
    m_Events.reset();
    m_InputMotion.reset();
    m_KinematicMotion.reset();
    m_Input.reset();
    m_InputMotionEntity_ = {0xFFFFU, 0U};
    m_RouteFollower.reset();
    m_RouteMotionController.reset();
    m_RouteNavigation.reset();
    m_RouteRootMotionAdapter.reset();
    m_SkeletalRouteMotionController.reset();
    m_SkeletalRoutePalette.clear();
    m_UsesSkeletalRouteMotion = false;
    m_RouteMotionEntity_ = {0xFFFFU, 0U};
    m_MotionAuthority.reset();
    m_Authoring.reset();
    m_AuthoringWorld.reset();
    m_Scene.reset();
    m_Assets.reset();
    m_FarmAuthority.reset();
    m_FarmWorld.reset();
    m_Farm.reset();
    m_TrustSafety.reset();
    m_State = RuntimeState::Shutdown;
    m_LastError = RuntimeError::None;
    return true;
}

} // namespace NeoEngine
