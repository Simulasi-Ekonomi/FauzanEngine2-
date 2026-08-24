#include "KinematicMotionController.h"

#include <cmath>

namespace NeoEngine {
bool KinematicMotionController::Initialize(KinematicMotionConfig config){
    if(!std::isfinite(config.unitsPerSecond)||!std::isfinite(config.maxStepSeconds)||config.unitsPerSecond<=0.0F||config.unitsPerSecond>1000.0F||config.maxStepSeconds<=0.0F||config.maxStepSeconds>1.0F){lastError_=KinematicMotionError::InvalidConfiguration;initialized_=false;return false;}
    config_=config;lastError_=KinematicMotionError::None;initialized_=true;return true;
}
bool KinematicMotionController::Step(SceneWorld& world,SceneEntity entity,KinematicPlanarInput input,float seconds){
    if(!initialized_){lastError_=KinematicMotionError::NotInitialized;return false;}
    if(!std::isfinite(input.x)||!std::isfinite(input.z)||!std::isfinite(seconds)||seconds<0.0F||seconds>config_.maxStepSeconds){lastError_=KinematicMotionError::InvalidInput;return false;}
    const Transform3* current=world.GetLocalTransform(entity);if(current==nullptr){lastError_=KinematicMotionError::MissingEntity;return false;}
    const float magnitude=std::sqrt(input.x*input.x+input.z*input.z);if(!std::isfinite(magnitude)){lastError_=KinematicMotionError::InvalidInput;return false;}
    if(magnitude<=1e-6F){lastError_=KinematicMotionError::None;return true;}
    const float normalizedX=input.x/magnitude,normalizedZ=input.z/magnitude,distance=config_.unitsPerSecond*seconds;Transform3 next=*current;next.x+=normalizedX*distance;next.z+=normalizedZ*distance;if(config_.alignYawToPlanarInput)next.ry=std::atan2(normalizedX,normalizedZ);
    if(!world.SetTransform(entity,next)){lastError_=KinematicMotionError::TransformFailed;return false;}
    lastError_=KinematicMotionError::None;return true;
}
} // namespace NeoEngine
