#include "Runtime/TransformAnimationBinding.h"

#include <cmath>
#include <cstdio>
#include <limits>
#include <string>

int main() {
    using namespace NeoEngine; SceneWorld world; SceneEntity entity{}; AnimationTimeline timeline; TransformAnimationBinding binding;
    if(!world.Create(entity)||!world.SetTransform(entity,{1,2,3,0,0,0,1,1,1})||!timeline.AddTrack("walk-x",{{0,0},{1,4}})||!binding.Add({entity,"walk-x",TransformChannel::PositionX,2,1})||!binding.Add({entity,"walk-x",TransformChannel::RotationY,1,0})||!binding.Apply(world,timeline,0.5F,AnimationPlayback::Clamp)||!world.UpdateTransforms()) { std::printf("TRANSFORM_ANIMATION_BINDING_FAIL stage=setup error=%u\n",static_cast<unsigned>(binding.LastError())); return 1; }
    const Transform3* local=world.GetLocalTransform(entity); const Transform3* transformed=world.GetTransform(entity); if(!local||!transformed||std::fabs(local->x-5.0F)>0.0001F||std::fabs(local->ry-2.0F)>0.0001F||std::fabs(transformed->x-5.0F)>0.0001F) { std::printf("TRANSFORM_ANIMATION_BINDING_FAIL stage=result local=%f,%f world=%f\n",local?local->x:-1.0F,local?local->ry:-1.0F,transformed?transformed->x:-1.0F); return 1; }
    TransformAnimationBinding invalid; TransformAnimationBinding missing;
    if(invalid.Add({{0xFFFF,0},"walk-x",TransformChannel::PositionX,1,0})||invalid.LastError()!=TransformAnimationError::InvalidBinding||invalid.Add({entity,std::string("nul\0track",9U),TransformChannel::PositionX,1,0})||invalid.LastError()!=TransformAnimationError::InvalidBinding||invalid.Add({entity,"walk-x",static_cast<TransformChannel>(255U),1,0})||invalid.LastError()!=TransformAnimationError::InvalidBinding||!missing.Add({{999,1},"walk-x",TransformChannel::PositionX,1,0})||missing.Apply(world,timeline,0,AnimationPlayback::Clamp)||missing.LastError()!=TransformAnimationError::MissingEntity) return 1;
    TransformAnimationBinding atomic;
    if(!atomic.Add({entity,"walk-x",TransformChannel::PositionX,1,0})||!atomic.Add({entity,"missing",TransformChannel::PositionY,1,0})) return 1;
    if(atomic.Apply(world,timeline,0.5F,AnimationPlayback::Clamp)||atomic.LastError()!=TransformAnimationError::MissingTrack||!local||std::fabs(local->x-5.0F)>0.0001F||std::fabs(local->ry-2.0F)>0.0001F) return 1;
    TransformAnimationBinding overflow;
    if(!overflow.Add({entity,"walk-x",TransformChannel::PositionX,std::numeric_limits<float>::max(),0.0F})||overflow.Apply(world,timeline,0.5F,AnimationPlayback::Clamp)||overflow.LastError()!=TransformAnimationError::SampleFailed||std::fabs(local->x-5.0F)>0.0001F) return 1;
    std::printf("TRANSFORM_ANIMATION_BINDING_SMOKE_OK bindings=2 interpolation=1 sceneWrite=1 validation=1\n"); return 0;
}
