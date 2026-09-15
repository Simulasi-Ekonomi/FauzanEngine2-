#include "RuntimeFrameContract.h"

#include <cassert>

int main() {
    NeoEngine::RuntimeFrameContract contract;
    assert(contract.Begin(1U, 0U));
    assert(contract.Stage() == NeoEngine::RuntimeFrameStage::InputSnapshot);
    assert(contract.Advance(NeoEngine::RuntimeFrameStage::Simulation));
    assert(contract.Advance(NeoEngine::RuntimeFrameStage::SceneSnapshot));
    assert(contract.Advance(NeoEngine::RuntimeFrameStage::RenderCommands));
    assert(contract.Advance(NeoEngine::RuntimeFrameStage::AudioEvents));
    assert(contract.Advance(NeoEngine::RuntimeFrameStage::Completed));
    assert(contract.IsComplete());

    NeoEngine::RuntimeFrameContract rejected;
    assert(rejected.Begin(2U, 7U));
    assert(!rejected.Advance(NeoEngine::RuntimeFrameStage::RenderCommands));
    rejected.Fail();
    assert(!rejected.IsActive());
    assert(!rejected.IsComplete());
    return 0;
}
