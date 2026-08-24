#pragma once

#include "KinematicMotionController.h"
#include "MovementAuthority.h"
#include "Systems/GridNavigation.h"

#include <cstdint>
#include <span>
#include <vector>

namespace NeoEngine {
struct RouteIntent {
    SceneEntity entity{};
    uint32_t routeRevision = 0U;
    uint16_t routeIndex = 0U;
    GridCell from{};
    GridCell target{};
    float targetX = 0.0F;
    float targetZ = 0.0F;
    float remainingPlanarDistance = 0.0F;
};
struct RouteIntentReceipt { RouteIntent intent{}; bool motionApplied = false; };
enum class GridRouteFollowerError : uint8_t { None, InvalidRoute, NotReady, MissingEntity, StartMismatch, BlockedTarget, ReplanFailed, ControllerFailed, TransformFailed, AuthorityConflict, IntentReceiptInvalid, IntentStale };
class GridRouteFollower {
public:
    static constexpr uint16_t kMaxRouteCells = GridNavigation::kMaxRouteCells;
    bool SetRoute(std::span<const GridCell> route);
    bool Step(SceneWorld& world,SceneEntity entity,KinematicMotionController& controller,const GridNavigation& navigation,float seconds);
    bool StepGuarded(SceneWorld& world,SceneEntity entity,KinematicMotionController& controller,const GridNavigation& navigation,float seconds,MovementAuthorityGate& authority);
    // Reads one validated next-cell intent without changing SceneWorld, controller, route cursor, or started state.
    bool PeekIntent(const SceneWorld& world,SceneEntity entity,const GridNavigation& navigation,RouteIntent& out);
    // Accepts a post-writer receipt, advances at most one cell on arrival, and never writes SceneWorld.
    bool CommitIntent(const SceneWorld& world,const GridNavigation& navigation,const RouteIntentReceipt& receipt);
    bool Replan(SceneWorld& world,SceneEntity entity,const GridNavigation& navigation);
    [[nodiscard]] bool ReachedGoal() const { return !route_.empty() && routeIndex_ >= route_.size(); }
    [[nodiscard]] GridRouteFollowerError LastError() const { return lastError_; }
private:
    bool StepInPlace(SceneWorld& world,SceneEntity entity,KinematicMotionController& controller,const GridNavigation& navigation,float seconds);
    std::vector<GridCell> route_;
    size_t routeIndex_ = 0;
    uint32_t routeRevision_ = 0U;
    bool started_ = false;
    GridRouteFollowerError lastError_ = GridRouteFollowerError::NotReady;
};
} // namespace NeoEngine
