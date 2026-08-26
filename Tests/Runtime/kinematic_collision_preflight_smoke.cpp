#include "Core/ECS/ArchetypeManager.h"
#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Runtime/KinematicCollisionPreflight.h"
#include "Runtime/MovementAuthority.h"
#include "Threading/JobSystem.h"

#include <cmath>
#include <cstdio>
#include <limits>

int main() {
    using namespace NeoEngine; JobSystem::Get().Initialize(2); const auto fail=[](const char* stage){std::fprintf(stderr,"KINEMATIC_COLLISION_PREFLIGHT_FAIL %s\n",stage);JobSystem::Get().Shutdown();return 1;}; ArchetypeManager entities; const EntityID obstacle=entities.CreateEntity(COMP_POSITION|COMP_VELOCITY|COMP_COLLIDER); entities.SetPosX(obstacle,0.40F);entities.SetRadius(obstacle,0.25F);entities.SetInvMass(obstacle,0.0F);XPBDPhysicsSystem physics;physics.Step(entities,1.0F/60.0F);physics.SetEntityLayer(0,COLLISION_LAYER_STATIC);
    SceneWorld world;SceneEntity actor{};if(!world.Create(actor)||!world.SetTransform(actor,{0,0,0,0,0,0,1,1,1})||!world.UpdateTransforms())return fail("world");KinematicMotionController motion;if(!motion.Initialize({2.0F,0.25F,false}))return fail("motion");KinematicCollisionPreflight guard;if(!guard.Initialize({COLLISION_LAYER_STATIC,0.0F}))return fail("guard");MovementAuthorityGate authority;
    KinematicCollisionPreflightProbe probe{false,{999U,9.0F,9.0F,9.0F}};if(!guard.Probe(physics,world,actor,{1,0},0.25F,motion,probe)||probe.blocked!=true||probe.blocker.entity!=obstacle||world.GetLocalTransform(actor)->x!=0.0F)return fail("probe_blocked");
    const KinematicCollisionPreflightProbe blocked=probe;if(guard.Probe(physics,world,actor,{std::numeric_limits<float>::quiet_NaN(),0},0.25F,motion,probe)||guard.LastError()!=KinematicCollisionPreflightError::InvalidInput||probe.blocked!=blocked.blocked||probe.blocker.entity!=blocked.blocker.entity)return fail("probe_atomic");
    authority.BeginFrame();if(!authority.Acquire(actor,MovementAuthority::KinematicRoute)||guard.Step(physics,world,actor,{1,0},0.25F,motion)||guard.LastError()!=KinematicCollisionPreflightError::Blocked||world.GetLocalTransform(actor)->x!=0.0F)return fail("blocked");
    XPBDPhysicsSystem emptyPhysics;authority.BeginFrame();if(!authority.Acquire(actor,MovementAuthority::KinematicRoute)||!guard.Step(emptyPhysics,world,actor,{1,0},0.25F,motion)||guard.LastError()!=KinematicCollisionPreflightError::None||std::fabs(world.GetLocalTransform(actor)->x-0.5F)>0.001F)return fail("pass");
    const float preserved=world.GetLocalTransform(actor)->x;if(guard.Step(physics,world,actor,{std::numeric_limits<float>::quiet_NaN(),0},0.25F,motion)||guard.LastError()!=KinematicCollisionPreflightError::InvalidInput||world.GetLocalTransform(actor)->x!=preserved)return fail("invalid");
    JobSystem::Get().Shutdown();std::printf("KINEMATIC_COLLISION_PREFLIGHT_SMOKE_OK probe=1 blocked=1 pass=1 singleWriter=1 atomic=1\n");return 0;
}
