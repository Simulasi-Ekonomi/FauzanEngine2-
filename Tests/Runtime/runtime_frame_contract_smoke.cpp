#include "RuntimeFrameContract.h"
#include "ECS/ECSCommandBuffer.h"

#include <array>
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

    NeoEngine::ECSCommandBuffer commands;
    assert(commands.CreateEntity(42U));
    const std::array<uint8_t, 4> component{1U, 2U, 3U, 4U};
    assert(commands.SetComponent(42U, 7U, component));
    assert(commands.RemoveComponent(42U, 9U));
    assert(commands.DestroyEntity(42U));
    assert(commands.Commands().size() == 4U);
    assert(commands.Payload(commands.Commands()[1]).size() == component.size());
    assert(commands.Payload(commands.Commands()[1])[2] == 3U);
    commands.Clear();
    assert(commands.Empty());
    return 0;
}
