#include "AndroidLifecycleGate.h"

#include <cmath>

namespace NeoEngine {
bool AndroidLifecycleGate::Initialize(){if(state_!=AndroidLifecycleState::Fresh){lastError_=AndroidLifecycleError::InvalidTransition;return false;}state_=AndroidLifecycleState::Initialized;lastError_=AndroidLifecycleError::None;return true;}
bool AndroidLifecycleGate::Resume(){if(state_!=AndroidLifecycleState::Initialized&&state_!=AndroidLifecycleState::Paused){lastError_=AndroidLifecycleError::InvalidTransition;return false;}state_=AndroidLifecycleState::Active;lastError_=AndroidLifecycleError::None;return true;}
bool AndroidLifecycleGate::Pause(){if(state_!=AndroidLifecycleState::Active){lastError_=AndroidLifecycleError::InvalidTransition;return false;}state_=AndroidLifecycleState::Paused;lastError_=AndroidLifecycleError::None;return true;}
bool AndroidLifecycleGate::Tick(float deltaSeconds){if(state_!=AndroidLifecycleState::Active){lastError_=AndroidLifecycleError::InvalidTransition;return false;}if(!std::isfinite(deltaSeconds)||deltaSeconds<=0.0F||deltaSeconds>0.25F){lastError_=AndroidLifecycleError::InvalidDelta;return false;}++tickCount_;lastError_=AndroidLifecycleError::None;return true;}
bool AndroidLifecycleGate::Shutdown(){if(state_!=AndroidLifecycleState::Initialized&&state_!=AndroidLifecycleState::Active&&state_!=AndroidLifecycleState::Paused){lastError_=AndroidLifecycleError::InvalidTransition;return false;}state_=AndroidLifecycleState::Stopped;lastError_=AndroidLifecycleError::None;return true;}
} // namespace NeoEngine
