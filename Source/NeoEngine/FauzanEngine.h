#pragma once

// Core
#include "Core/ActorBase.h"
#include "Core/EngineLoop.h"
#include "Core/Config.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Quaternion.h"
#include "Core/Math/Matrix4.h"
#include "Core/Math/NeoMath.h"
#include "Core/Math/SIMDMath.h"

// ECS
#include "Core/ECS/ECSCore.h"
#include "Core/ECS/EntityManager.h"
#include "ECS/V4/FauzanECSSystems.h"

// World
#include "World/NeoWorld.h"
#include "World/SpatialGrid.h"
#include "World/V4/Octree.h"
#include "World/V4/BVH.h"
#include "World/V4/HybridSpatial.h"

// AI V4
#include "AI/V4/AIAgent.h"
#include "AI/V4/AILearning.h"
#include "AI/V4/RL/RLAgent.h"
#include "AI/V4/RL/RLSimulator.h"
#include "AI/V4/RL/RLTrainingPipeline.h"
#include "AI/V4/Agents/AgentOrchestrator.h"

// Physics
#include "Physics/CollisionSAT.h"

// Rendering
#include "Rendering/Renderer/Renderer.h"

// Systems
#include "Systems/InventorySystem.h"
#include "Systems/CombatSystem.h"
#include "Systems/ItemSerialTracker.h"
#include "Systems/MarketplaceSystem.h"

namespace NeoEngine {
    void InitializeEngine();
    void ShutdownEngine();
}
