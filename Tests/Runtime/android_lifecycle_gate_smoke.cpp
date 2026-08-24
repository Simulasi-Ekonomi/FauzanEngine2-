#include "Runtime/AndroidLifecycleGate.h"

#include <cstdio>

int main(){using namespace NeoEngine;AndroidLifecycleGate gate;if(gate.Tick(1.0F/60.0F)||gate.LastError()!=AndroidLifecycleError::InvalidTransition||!gate.Initialize()||!gate.Resume()||!gate.Tick(1.0F/60.0F)||gate.Tick(0.5F)||gate.LastError()!=AndroidLifecycleError::InvalidDelta||!gate.Pause()||gate.Tick(1.0F/60.0F)||gate.LastError()!=AndroidLifecycleError::InvalidTransition||!gate.Resume()||!gate.Tick(1.0F/60.0F)||!gate.Shutdown()||gate.Resume()||gate.LastError()!=AndroidLifecycleError::InvalidTransition||gate.TickCount()!=2U)return 1;std::printf("ANDROID_LIFECYCLE_GATE_SMOKE_OK init=1 pauseResume=1 tick=2 shutdown=1 failClosed=1\n");return 0;}
