#include "NeoRuntime.h"

#include "FarmRenderAdapter.h"
#include "FarmSpriteRenderAdapter.h"
#include "TextureStaging.h"
#include "Systems/AgricultureCurriculum.h"

#include <limits>

namespace NeoEngine {
namespace {
constexpr const char* kFarmProgressCheckpointKind = "neo-farm-progress";

void AppendU32(std::vector<uint8_t>& bytes, uint32_t value) {
    for (uint8_t shift = 0U; shift < 32U; shift += 8U) bytes.push_back(static_cast<uint8_t>((value >> shift) & 0xFFU));
}

bool ReadU32(const std::vector<uint8_t>& bytes, size_t& offset, uint32_t& value) {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(uint32_t)) return false;
    value = 0U;
    for (uint8_t shift = 0U; shift < 32U; shift += 8U) value |= static_cast<uint32_t>(bytes[offset + shift / 8U]) << shift;
    offset += sizeof(uint32_t);
    return true;
}

bool AppendBlob(std::vector<uint8_t>& bytes, const std::vector<uint8_t>& blob) {
    if (blob.size() > std::numeric_limits<uint32_t>::max() || bytes.size() > RuntimeSaveCodec::kMaxPayloadBytes - sizeof(uint32_t) ||
        blob.size() > RuntimeSaveCodec::kMaxPayloadBytes - sizeof(uint32_t) - bytes.size()) return false;
    AppendU32(bytes, static_cast<uint32_t>(blob.size()));
    bytes.insert(bytes.end(), blob.begin(), blob.end());
    return true;
}

bool ReadBlob(const std::vector<uint8_t>& bytes, size_t& offset, std::vector<uint8_t>& blob) {
    uint32_t length = 0U;
    if (!ReadU32(bytes, offset, length) || length == 0U || offset > bytes.size() || bytes.size() - offset < length) return false;
    blob.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset), bytes.begin() + static_cast<std::ptrdiff_t>(offset + length));
    offset += length;
    return true;
}
} // namespace

bool NeoRuntime::Initialize(const RuntimeConfig& config) {
    if (m_State != RuntimeState::Created || config.fixedTicksPerFrame == 0 || config.initialCoins < 0 || config.farmNpcCount == 0 || config.farmNpcCount > FarmWorldTool::kMaxNpcs || (config.enableFarmRuntimeHud && (config.renderWidth < 64U || config.renderHeight < 48U))) {
        m_LastError = RuntimeError::InvalidConfiguration;
        m_State = RuntimeState::Failed;
        return false;
    }
    auto farm = std::make_unique<FarmSystem>(config.farmWidth, config.farmHeight, config.initialCoins, config.farmBalance);
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
    auto assets = std::make_unique<AssetRegistry>();
    auto resources = std::make_unique<AssetResourceManager>(*assets);
    auto renderer = std::make_unique<SoftwareRenderer>();
    if (!renderer->Initialize(config.renderWidth, config.renderHeight)) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }
    auto surfacePresenter = std::unique_ptr<SoftwareSurfacePresenter>{};
    if (config.enableSoftwareSurfacePresentation) {
        surfacePresenter = std::make_unique<SoftwareSurfacePresenter>();
        if (!surfacePresenter->Initialize({config.renderWidth, config.renderHeight, config.softwareSurfaceHidden})) { m_LastError = RuntimeError::PresentationFailed; m_State = RuntimeState::Failed; return false; }
    }
    auto clock = std::make_unique<RuntimeClock>();
    if (!clock->Initialize()) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }
    auto gameTime = std::make_unique<RuntimeTimeSystem>();
    if (!gameTime->Initialize(config.timeConfig) || !clock->SetTimeScale(static_cast<float>(config.timeConfig.defaultTimeScalePermille) / 1000.0F)) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }
    auto scene = std::make_unique<SceneWorld>();
    if (!world->PopulateScene(*scene)) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }
    auto authoringWorld = std::make_unique<WorldAuthoring>();
    WorldAuthoringConfig authoringWorldConfig{};
    authoringWorldConfig.side = config.authoringWorldSide;
    authoringWorldConfig.seed = config.authoringWorldSeed;
    if (!authoringWorld->Generate(authoringWorldConfig) || !authoringWorld->BindScene(*scene)) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }
    auto actors = std::make_unique<ActorComponentWorld>(*scene);
    auto replication = std::make_unique<ReplicationWorld>(*scene, config.replicationRole, config.replicationLocalClientId);
    auto curriculum = std::unique_ptr<CurriculumSystem>{};
    if (config.enableFarmCurriculum) {
        CurriculumGraph graph;
        curriculum = std::make_unique<CurriculumSystem>();
        if (!BuildAgricultureCurriculum(graph) || !curriculum->Initialize(graph)) { m_LastError = RuntimeError::CurriculumFailed; m_State = RuntimeState::Failed; return false; }
    }

    auto input = std::unique_ptr<InputState>{};
    auto kinematicMotion = std::unique_ptr<KinematicMotionController>{};
    auto inputMotion = std::unique_ptr<InputMotionBridge>{};
    auto farmPlayerInput = std::unique_ptr<FarmPlayerInputBridge>{};
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
    if (config.enableFarmPlayerInput) {
        if (input == nullptr) input = std::make_unique<InputState>();
        farmPlayerInput = std::make_unique<FarmPlayerInputBridge>();
        const FarmPlayerInputBindings& bindings = config.farmPlayerInputBindings;
        if (!farmPlayerInput->Initialize(bindings) || !input->Bind(bindings.moveUp, MakeInputCode(InputDeviceType::Keyboard, 20U)) || !input->Bind(bindings.moveDown, MakeInputCode(InputDeviceType::Keyboard, 21U)) || !input->Bind(bindings.moveLeft, MakeInputCode(InputDeviceType::Keyboard, 22U)) || !input->Bind(bindings.moveRight, MakeInputCode(InputDeviceType::Keyboard, 23U)) || !input->Bind(bindings.interact, MakeInputCode(InputDeviceType::Keyboard, 24U))) { m_LastError = RuntimeError::InvalidConfiguration; m_State = RuntimeState::Failed; return false; }
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
    m_FarmWorldConfig = worldConfig;
    m_TrustSafety = std::move(trustSafety);
    m_Farm = std::move(farm);
    m_FarmWorld = std::move(world);
    m_FarmAuthority = std::move(authority);
    m_Assets = std::move(assets);
    m_Resources = std::move(resources);
    m_Actors = std::move(actors);
    m_Replication = std::move(replication);
    m_Authoring = std::make_unique<AuthoringCatalog>();
    m_Curriculum = std::move(curriculum);
    m_LastCurriculumEvents.clear();
    m_AuthoringWorld = std::move(authoringWorld);
    m_Clock = std::move(clock);
    m_Time = std::move(gameTime);
    m_Timers = std::make_unique<RuntimeTimerQueue>();
    m_Events = std::make_unique<EventSignalBus>();
    m_Input = std::move(input);
    m_KinematicMotion = std::move(kinematicMotion);
    m_InputMotion = std::move(inputMotion);
    m_FarmPlayerInput = std::move(farmPlayerInput);
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
    m_FarmRuntimeHud = config.enableFarmRuntimeHud ? std::make_unique<FarmRuntimeHud>() : nullptr;
    m_FarmRenderAssets.reset();
    m_FarmSpriteRenderer = std::make_unique<FarmSpriteRenderAdapter>();
    m_FarmSpriteTextures = std::make_unique<TextureStagingStore>();
    m_LastFrameReceipt = {};
    m_HasFrameReceipt = false;
    m_RenderedFarmFrames = 0U;
    m_LastFarmRenderReceipt = {};
    m_HasFarmRenderReceipt = false;
    m_SurfacePresenter = std::move(surfacePresenter);
    m_LastError = RuntimeError::None;
    m_State = RuntimeState::Initialized;
    return true;
}

bool NeoRuntime::Tick() {
    if (m_State != RuntimeState::Initialized || !m_Farm || !m_FarmWorld || !m_FarmAuthority || !m_Assets || !m_Resources || !m_Actors || !m_Replication || !m_Authoring || !m_AuthoringWorld || !m_Clock || !m_Timers || !m_Events || !m_Scene || !m_Clock->Advance(1.0F / 60.0F)) { m_LastError = RuntimeError::InvalidState; return false; }
    std::vector<RuntimeTimerFire> fires;
    if (!m_Timers->Advance(m_Clock->Snapshot().scaledDeltaSeconds, fires)) { m_LastError = RuntimeError::InvalidState; return false; }
    for (const RuntimeTimerFire& fire : fires) if (!m_Events->Queue({RuntimeEventKind::TimerFired, fire.userTag, static_cast<int32_t>(fire.fireCount), m_Clock->Snapshot().fixedStepCount})) { m_LastError = RuntimeError::InvalidState; return false; }
    if (m_Input != nullptr) m_Input->BeginFrame();
    std::vector<RuntimeTimeEvent> timeEvents;
    uint32_t simulatedTicks = 0U;
    if (m_Time == nullptr || !m_Time->AdvanceFixedTicks(m_FixedTicksPerFrame, timeEvents, simulatedTicks)) { m_LastError = RuntimeError::TimeFailed; m_State = RuntimeState::Failed; return false; }
    if (timeEvents.size() > EventSignalBus::kMaxEvents - m_Events->PendingCount()) { m_LastError = RuntimeError::TimeFailed; m_State = RuntimeState::Failed; return false; }
    for (const RuntimeTimeEvent& event : timeEvents) {
        const RuntimeEventKind kind = event.kind == RuntimeTimeEventKind::TimeChanged ? RuntimeEventKind::GameTimeChanged : event.kind == RuntimeTimeEventKind::DayChanged ? RuntimeEventKind::GameDayChanged : RuntimeEventKind::GamePhaseChanged;
        if (!m_Events->Queue({kind, 0U, static_cast<int32_t>(event.snapshot.minuteOfDay), event.snapshot.hostFixedStepCount})) { m_LastError = RuntimeError::TimeFailed; m_State = RuntimeState::Failed; return false; }
    }
    if (m_Clock->Snapshot().paused || simulatedTicks == 0U) {
        const uint32_t eventCount = m_Events->PendingCount();
        EventSignalDispatchReceipt dispatchReceipt{};
        CurriculumProgressReceipt curriculumReceipt{};
        if (m_Curriculum != nullptr && !m_Curriculum->Snapshot(curriculumReceipt)) { m_LastError = RuntimeError::CurriculumFailed; return false; }
        if (!m_Events->Dispatch(&dispatchReceipt)) { m_LastError = RuntimeError::InvalidState; return false; }
        m_LastFrameReceipt = {m_Clock->Snapshot(), m_Time->Snapshot(), {}, m_Farm->Snapshot(), m_FarmWorld->Snapshot(), eventCount};
        m_LastFrameReceipt.eventDispatch = dispatchReceipt; m_LastFrameReceipt.curriculum = curriculumReceipt; m_LastFrameReceipt.hasCurriculumReceipt = m_Curriculum != nullptr;
        m_LastFrameReceipt.input = m_Input == nullptr ? InputStateSummary{} : m_Input->Summary(); m_LastFrameReceipt.assets = m_Assets->Summary(); m_LastFrameReceipt.sceneAliveEntityCount = m_Scene->AliveCount();
        m_HasFrameReceipt = true;
        m_LastError = RuntimeError::None;
        return true;
    }
    if (m_MotionAuthority != nullptr) m_MotionAuthority->BeginFrame();
    if (m_InputMotion != nullptr && (m_Input == nullptr || m_KinematicMotion == nullptr || !m_InputMotion->Step(*m_Input, *m_KinematicMotion, *m_Scene, m_InputMotionEntity_, m_Clock->Snapshot().scaledDeltaSeconds))) { m_LastError = RuntimeError::InputMotionFailed; m_State = RuntimeState::Failed; return false; }
    if (m_RouteFollower != nullptr && !m_RouteFollower->ReachedGoal()) {
        const bool routeSucceeded = m_UsesSkeletalRouteMotion ? (m_RouteNavigation != nullptr && m_SkeletalRouteMotionController != nullptr && m_RouteRootMotionAdapter != nullptr && m_MotionAuthority != nullptr && m_RouteRootMotionAdapter->Advance(m_Clock->Snapshot().scaledDeltaSeconds, *m_RouteFollower, *m_SkeletalRouteMotionController, *m_Scene, m_RouteMotionEntity_, *m_RouteNavigation, *m_MotionAuthority, m_SkeletalRoutePalette)) : (m_RouteNavigation != nullptr && m_RouteMotionController != nullptr && m_MotionAuthority != nullptr && m_RouteFollower->StepGuarded(*m_Scene, m_RouteMotionEntity_, *m_RouteMotionController, *m_RouteNavigation, m_Clock->Snapshot().scaledDeltaSeconds, *m_MotionAuthority));
        if (!routeSucceeded) { m_LastError = RuntimeError::RouteMotionFailed; m_State = RuntimeState::Failed; return false; }
    }
    ActorComponentWorldReceipt actorReceipt{};
    if (m_Actors == nullptr || !m_Actors->TickFixed(simulatedTicks, actorReceipt)) { m_LastError = RuntimeError::ActorComponentTickFailed; m_State = RuntimeState::Failed; return false; }
    FarmPlayerInputReceipt farmPlayerInputReceipt{};
    const bool hasFarmPlayerInput = m_FarmPlayerInput != nullptr;
    if (hasFarmPlayerInput && (m_Input == nullptr || !m_FarmPlayerInput->Step(*m_Input, *m_FarmWorld))) { m_LastError = RuntimeError::FarmPlayerInputFailed; m_State = RuntimeState::Failed; return false; }
    if (hasFarmPlayerInput) farmPlayerInputReceipt = m_FarmPlayerInput->LastReceipt();
    if (!m_FarmWorld->Tick(simulatedTicks)) { m_LastError = RuntimeError::WorldTickFailed; m_State = RuntimeState::Failed; return false; }
    if (!m_FarmWorld->SyncScene()) { m_LastError = RuntimeError::WorldTickFailed; m_State = RuntimeState::Failed; return false; }
    if (m_Authoring->IsSceneBound() && !m_Authoring->Tick(simulatedTicks)) { m_LastError = RuntimeError::AuthoringTickFailed; m_State = RuntimeState::Failed; return false; }
    CurriculumProgressReceipt curriculumReceipt{};
    m_LastCurriculumEvents.clear();
    if (m_Curriculum != nullptr && (!m_Curriculum->Evaluate({m_Time->Snapshot(), m_Farm->Snapshot()}, m_LastCurriculumEvents) || !m_Curriculum->Snapshot(curriculumReceipt))) { m_LastError = RuntimeError::CurriculumFailed; m_State = RuntimeState::Failed; return false; }
    const uint32_t eventCount = m_Events->PendingCount();
    EventSignalDispatchReceipt dispatchReceipt{};
    if (!m_Events->Dispatch(&dispatchReceipt)) { m_LastError = RuntimeError::InvalidState; return false; }
    m_LastFrameReceipt = {m_Clock->Snapshot(), m_Time->Snapshot(), actorReceipt, m_Farm->Snapshot(), m_FarmWorld->Snapshot(), eventCount};
    m_LastFrameReceipt.eventDispatch = dispatchReceipt; m_LastFrameReceipt.curriculum = curriculumReceipt; m_LastFrameReceipt.hasCurriculumReceipt = m_Curriculum != nullptr;
    m_LastFrameReceipt.farmPlayerInput = farmPlayerInputReceipt; m_LastFrameReceipt.hasFarmPlayerInputReceipt = hasFarmPlayerInput; m_LastFrameReceipt.input = m_Input == nullptr ? InputStateSummary{} : m_Input->Summary(); m_LastFrameReceipt.assets = m_Assets->Summary(); m_LastFrameReceipt.sceneAliveEntityCount = m_Scene->AliveCount();
    m_HasFrameReceipt = true;
    m_LastError = RuntimeError::None;
    return true;
}

bool NeoRuntime::ReplanRouteMotion() {
    if (m_State != RuntimeState::Initialized || !m_Scene || !m_RouteNavigation || !m_RouteFollower || m_RouteMotionEntity_.index == 0xFFFFU) { m_LastError = RuntimeError::InvalidState; return false; }
    if (m_UsesSkeletalRouteMotion) { m_LastError = RuntimeError::RouteReplanFailed; return false; }
    if (!m_RouteFollower->Replan(*m_Scene, m_RouteMotionEntity_, *m_RouteNavigation)) { m_LastError = RuntimeError::RouteReplanFailed; return false; }
    m_LastError = RuntimeError::None;
    return true;
}

bool NeoRuntime::BindFarmSpriteAssets(const FarmSpriteAssetSet& assetSet) {
    if (m_State != RuntimeState::Initialized || !m_Assets || !m_Resources || m_FarmRenderAssets != nullptr) { m_LastError = RuntimeError::InvalidState; return false; }
    auto candidate = std::make_unique<FarmRenderAssetManifest>();
    if (!candidate->Bind(assetSet, *m_Assets, *m_Resources)) { m_LastError = RuntimeError::InvalidState; return false; }
    m_FarmRenderAssets = std::move(candidate);
    m_LastError = RuntimeError::None;
    return true;
}

bool NeoRuntime::RenderFarm() {
    if (m_State != RuntimeState::Initialized || !m_Farm || !m_FarmWorld || !m_Renderer) { m_LastError = RuntimeError::InvalidState; return false; }
    SoftwareRenderer candidate = *m_Renderer;
    if ((m_FarmRenderAssets != nullptr && (m_FarmSpriteRenderer == nullptr || m_FarmSpriteTextures == nullptr || !m_FarmRenderAssets->Validate(*m_Assets, *m_Resources) || !m_FarmSpriteRenderer->RenderWorld(*m_Farm, *m_FarmWorld, m_FarmRenderAssets->AssetSet(), *m_Assets, *m_FarmSpriteTextures, candidate))) || (m_FarmRenderAssets == nullptr && !FarmRenderAdapter::RenderWorld(*m_Farm, *m_FarmWorld, candidate))) { m_LastError = RuntimeError::RenderFailed; return false; }
    const uint64_t worldHash = candidate.FrameHash(); const FarmTelemetrySnapshot telemetry = m_Farm->Snapshot(); uint64_t hudHash = 0U;
    FarmOnboardingReceipt onboarding{};
    if (m_FarmRuntimeHud != nullptr) {
        const FarmCharacterState player = m_FarmWorld->Character(); const FarmTileState tile = m_Farm->TileStateAt(player.x, player.z);
        const FarmActionAvailability availability{tile == FarmTileState::Empty, tile == FarmTileState::Tilled && m_Farm->ItemCount(FarmItem::WheatSeed) != 0U, tile == FarmTileState::Growing && !m_Farm->IsWateredAt(player.x, player.z), tile == FarmTileState::Harvestable};
        const FarmPlayerInputReceipt inputReceipt = m_FarmPlayerInput == nullptr ? FarmPlayerInputReceipt{} : m_FarmPlayerInput->LastReceipt();
        const FarmPlayerAction selectedAction = m_FarmPlayerInput == nullptr ? FarmPlayerAction::Till : m_FarmPlayerInput->SelectedAction();
        onboarding.lastError = telemetry.lastError;
        if (telemetry.questCompleted) { onboarding.nextStep = FarmOnboardingStep::Complete; onboarding.complete = true; }
        else if (tile == FarmTileState::Empty) onboarding.nextStep = FarmOnboardingStep::Till;
        else if (tile == FarmTileState::Tilled) onboarding.nextStep = FarmOnboardingStep::Plant;
        else if (tile == FarmTileState::Growing && !m_Farm->IsWateredAt(player.x, player.z)) onboarding.nextStep = FarmOnboardingStep::Water;
        else if (tile == FarmTileState::Harvestable) onboarding.nextStep = FarmOnboardingStep::Harvest;
        else onboarding.nextStep = FarmOnboardingStep::Water;
        CurriculumProgressReceipt curriculumReceipt{};
        if (m_Curriculum != nullptr && !m_Curriculum->Snapshot(curriculumReceipt)) { m_LastError = RuntimeError::CurriculumFailed; return false; }
        FarmRuntimeFrameReceipt receipt{m_RenderedFarmFrames + 1U, worldHash, telemetry, {m_Farm->ItemCount(FarmItem::WheatSeed), m_Farm->ItemCount(FarmItem::WheatProduce)}, inputReceipt, availability, m_Time == nullptr ? RuntimeTimeSnapshot{} : m_Time->Snapshot(), curriculumReceipt};
        receipt.onboarding = onboarding;
        m_FarmRuntimeHud->SetActionAvailability(availability);
        if (!m_FarmRuntimeHud->Draw(receipt, selectedAction, candidate)) { m_LastError = RuntimeError::HudFailed; return false; }
        hudHash = candidate.FrameHash();
    }
    if (m_SurfacePresenter != nullptr && (!m_SurfacePresenter->PumpEvents() || !m_SurfacePresenter->Present(candidate))) { m_LastError = RuntimeError::PresentationFailed; return false; }
    const RuntimeFarmRenderReceipt receipt{m_RenderedFarmFrames + 1U, worldHash, hudHash, m_SurfacePresenter == nullptr ? 0U : m_SurfacePresenter->PresentedFrameCount(), telemetry}; *m_Renderer = std::move(candidate); ++m_RenderedFarmFrames; m_LastFarmRenderReceipt = receipt; m_HasFarmRenderReceipt = true; if (m_HasFrameReceipt) { NeoRuntimeFrameReceipt candidateReceipt = m_LastFrameReceipt; candidateReceipt.farmRender = receipt; candidateReceipt.onboarding = onboarding; candidateReceipt.hasFarmRenderReceipt = true;             if (m_FarmRenderAssets != nullptr) { candidateReceipt.farmSpriteAssets = m_FarmRenderAssets->Receipt(); candidateReceipt.hasFarmSpriteAssets = true; } m_LastFrameReceipt = candidateReceipt; } m_LastError = RuntimeError::None;

    return true;
}

bool NeoRuntime::RouteFarmHudPointer(float x, float y, UiPointerPhase phase, FarmActionPanelReceipt& receipt) {
    receipt = {};
    if (m_State != RuntimeState::Initialized || m_FarmRuntimeHud == nullptr || m_FarmPlayerInput == nullptr) { m_LastError = RuntimeError::InvalidState; return false; }
    if (!m_FarmRuntimeHud->RoutePointer(x, y, phase, *m_FarmPlayerInput, receipt)) { m_LastError = RuntimeError::HudInputFailed; return false; }
    m_LastError = RuntimeError::None;
    return true;
}

bool NeoRuntime::RouteFarmHudKeyboard(UiKeyboardKey key, FarmActionPanelReceipt& receipt) {
    receipt = {};
    if (m_State != RuntimeState::Initialized || m_FarmRuntimeHud == nullptr || m_FarmPlayerInput == nullptr) { m_LastError = RuntimeError::InvalidState; return false; }
    if (!m_FarmRuntimeHud->RouteKeyboard(key, *m_FarmPlayerInput, receipt)) { m_LastError = RuntimeError::HudInputFailed; return false; }
    m_LastError = RuntimeError::None;
    return true;
}

bool NeoRuntime::SetPaused(bool paused) {
    if (m_State != RuntimeState::Initialized || !m_Clock || !m_Time || !m_Events || m_Events->PendingCount() >= EventSignalBus::kMaxEvents) { m_LastError = RuntimeError::InvalidState; return false; }
    if (!m_Clock->SetPaused(paused) || !m_Time->SetPaused(paused) || !m_Events->Queue({paused ? RuntimeEventKind::RuntimePaused : RuntimeEventKind::RuntimeResumed, 0, 0, m_Clock->Snapshot().fixedStepCount})) { m_LastError = RuntimeError::TimeFailed; return false; }
    m_LastError = RuntimeError::None;
    return true;
}

bool NeoRuntime::SetTimeScalePermille(uint16_t scalePermille) {
    if (m_State != RuntimeState::Initialized || !m_Clock || !m_Time) { m_LastError = RuntimeError::InvalidState; return false; }
    const uint16_t previous = m_Time->Snapshot().timeScalePermille;
    if (!m_Time->SetTimeScalePermille(scalePermille)) { m_LastError = RuntimeError::TimeFailed; return false; }
    if (!m_Clock->SetTimeScale(static_cast<float>(scalePermille) / 1000.0F)) {
        m_Time->SetTimeScalePermille(previous);
        m_LastError = RuntimeError::TimeFailed;
        return false;
    }
    m_LastError = RuntimeError::None;
    return true;
}

bool NeoRuntime::SaveFarmProgressCheckpoint(uint64_t revision, std::vector<uint8_t>& bytes) {
    if (m_State != RuntimeState::Initialized || revision == 0U || !m_FarmWorld || !m_Time || !m_FarmAuthority || m_Events == nullptr || m_Events->PendingCount() != 0U) {
        m_LastError = RuntimeError::CheckpointEncodeFailed;
        return false;
    }
    const std::vector<uint8_t> worldBytes = m_FarmWorld->Serialize();
    std::vector<uint8_t> timeBytes;
    std::vector<uint8_t> curriculumBytes;
    const std::vector<uint8_t> authorityBytes = m_FarmAuthority->SerializeAuthorityLedger();
    if (worldBytes.empty() || !m_Time->Serialize(timeBytes) || timeBytes.empty() || authorityBytes.empty() ||
        (m_Curriculum != nullptr && (!m_Curriculum->Serialize(curriculumBytes) || curriculumBytes.empty()))) {
        m_LastError = RuntimeError::CheckpointEncodeFailed;
        return false;
    }
    std::vector<uint8_t> payload;
    if (!AppendBlob(payload, worldBytes) || !AppendBlob(payload, timeBytes) || !AppendBlob(payload, authorityBytes) ||
        (m_Curriculum != nullptr && !AppendBlob(payload, curriculumBytes))) {
        m_LastError = RuntimeError::CheckpointEncodeFailed;
        return false;
    }
    RuntimePersistenceError error = RuntimePersistenceError::None;
    std::vector<uint8_t> encoded;
    if (!RuntimeSaveCodec::Serialize({kFarmProgressCheckpointKind, revision, std::move(payload)}, encoded, error)) {
        m_LastError = RuntimeError::CheckpointEncodeFailed;
        return false;
    }
    bytes = std::move(encoded);
    m_LastError = RuntimeError::None;
    return true;
}

bool NeoRuntime::RestoreFarmProgressCheckpoint(const std::vector<uint8_t>& bytes, uint64_t& revision) {
    if (m_State != RuntimeState::Initialized || !m_Farm || !m_FarmWorld || !m_Time || !m_FarmAuthority || !m_TrustSafety || !m_Scene || !m_Clock || m_Events == nullptr || m_Events->PendingCount() != 0U) {
        m_LastError = RuntimeError::CheckpointDecodeFailed;
        return false;
    }
    RuntimeSaveEnvelope envelope{};
    RuntimePersistenceError error = RuntimePersistenceError::None;
    if (!RuntimeSaveCodec::Deserialize(bytes, envelope, error) || envelope.kind != kFarmProgressCheckpointKind || envelope.revision == 0U || envelope.payload.size() > RuntimeSaveCodec::kMaxPayloadBytes) {
        m_LastError = RuntimeError::CheckpointDecodeFailed;
        return false;
    }
    size_t offset = 0U;
    std::vector<uint8_t> worldBytes, timeBytes, authorityBytes, curriculumBytes;
    if (!ReadBlob(envelope.payload, offset, worldBytes) || !ReadBlob(envelope.payload, offset, timeBytes) || !ReadBlob(envelope.payload, offset, authorityBytes) ||
        (offset < envelope.payload.size() && !ReadBlob(envelope.payload, offset, curriculumBytes)) || offset != envelope.payload.size() ||
        (m_Curriculum == nullptr && !curriculumBytes.empty())) {
        m_LastError = RuntimeError::CheckpointDecodeFailed;
        return false;
    }
    auto candidateFarm = std::make_unique<FarmSystem>(*m_Farm);
    candidateFarm->SetTrustSafety(m_TrustSafety.get(), "runtime-farm-player");
    auto candidateWorld = std::make_unique<FarmWorldTool>();
    auto candidateTime = std::make_unique<RuntimeTimeSystem>(*m_Time);
    auto candidateAuthority = std::make_unique<FarmAuthoritativeService>();
    auto candidateCurriculum = m_Curriculum == nullptr ? std::unique_ptr<CurriculumSystem>{} : std::make_unique<CurriculumSystem>(*m_Curriculum);
    if (!candidateWorld->Initialize(*candidateFarm, *m_TrustSafety, "runtime-farm-player", m_FarmWorldConfig) || !candidateWorld->Deserialize(worldBytes) ||
        !candidateTime->Deserialize(timeBytes) || (candidateCurriculum != nullptr && !curriculumBytes.empty() && !candidateCurriculum->Deserialize(curriculumBytes)) ||
        !candidateAuthority->Initialize(*candidateWorld, *m_TrustSafety, "runtime-farm-player", "runtime-farm-session") ||
        !candidateAuthority->RestoreAuthorityLedger(authorityBytes) || !candidateAuthority->BindSession("runtime-farm-player", "runtime-farm-session") ||
        !candidateWorld->AdoptTopologyPreservingSceneBinding(*m_FarmWorld)) {
        m_LastError = RuntimeError::CheckpointDecodeFailed;
        return false;
    }
    const RuntimeTimeSnapshot restoredTime = candidateTime->Snapshot();
    RuntimeClock candidateClock = *m_Clock;
    if (restoredTime.timeScalePermille > RuntimeTimeSystem::kMaxTimeScalePermille || !candidateClock.SetPaused(restoredTime.paused) ||
        !candidateClock.SetTimeScale(static_cast<float>(restoredTime.timeScalePermille) / 1000.0F) || !candidateWorld->SyncScene()) {
        m_LastError = RuntimeError::CheckpointDecodeFailed;
        return false;
    }
    m_Farm = std::move(candidateFarm);
    m_FarmWorld = std::move(candidateWorld);
    m_Time = std::move(candidateTime);
    m_FarmAuthority = std::move(candidateAuthority);
    m_Curriculum = std::move(candidateCurriculum);
    m_LastCurriculumEvents.clear();
    *m_Clock = std::move(candidateClock);
    m_LastFrameReceipt = {};
    m_HasFrameReceipt = false;
    m_LastFarmRenderReceipt = {};
    m_HasFarmRenderReceipt = false;
    revision = envelope.revision;
    m_LastError = RuntimeError::None;
    return true;
}

bool NeoRuntime::Shutdown() {
    if (m_State != RuntimeState::Initialized && m_State != RuntimeState::Failed) { m_LastError = RuntimeError::InvalidState; return false; }
    m_SurfacePresenter.reset();
    m_FarmRuntimeHud.reset();
    m_FarmRenderAssets.reset();
    m_FarmSpriteRenderer.reset();
    m_FarmSpriteTextures.reset();
    m_LastFrameReceipt = {};
    m_HasFrameReceipt = false;
    m_RenderedFarmFrames = 0U;
    m_LastFarmRenderReceipt = {};
    m_HasFarmRenderReceipt = false;
    m_Renderer.reset();
    m_FarmWorldConfig = {};
    m_Clock.reset();
    m_Time.reset();
    m_Timers.reset();
    m_Events.reset();
    m_InputMotion.reset();
    m_FarmPlayerInput.reset();
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
    m_Replication.reset();
    m_Actors.reset();
    m_Resources.reset();
    m_Authoring.reset();
    m_Curriculum.reset();
    m_LastCurriculumEvents.clear();
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
