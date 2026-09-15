#include "RuntimeFrameContract.h"
#include "ECS/ECSCommandBuffer.h"
#include "AssetRuntimeState.h"
#include "Renderer/RenderFrameGraph.h"
#include "Physics/PhysicsStepBudget.h"
#include "Systems/CommandIdempotency.h"
#include "Systems/TransactionJournal.h"
#include "Systems/SecurityCommandValidator.h"
#include "Platform/AndroidLifecycleGate.h"

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

    NeoEngine::AssetRuntimeStateStore assets;
    assert(assets.Register("mesh/player", 0x1234U));
    assert(!assets.Register("mesh/player", 0x5678U));
    assert(assets.Transition("mesh/player", NeoEngine::AssetRuntimeState::Staged));
    assert(assets.Transition("mesh/player", NeoEngine::AssetRuntimeState::Loading));
    assert(assets.Transition("mesh/player", NeoEngine::AssetRuntimeState::Ready, 4096U));
    const auto* ready = assets.Find("mesh/player");
    assert(ready != nullptr && ready->residentBytes == 4096U);
    assert(!assets.Transition("mesh/player", NeoEngine::AssetRuntimeState::Staged));
    assert(assets.Transition("mesh/player", NeoEngine::AssetRuntimeState::Evicted));
    assert(assets.Find("mesh/player")->residentBytes == 0U);

    NeoEngine::RenderFrameGraph graph;
    assert(graph.AddPass(1U, "depth"));
    assert(graph.AddPass(2U, "opaque", {1U}));
    assert(graph.AddPass(3U, "post", {2U}));
    std::vector<uint32_t> order;
    assert(graph.BuildOrder(order));
    assert(order.size() == 3U);
    assert(order[0] == 1U && order[1] == 2U && order[2] == 3U);
    assert(!graph.AddPass(4U, "cycle", {3U, 4U}));
    assert(graph.Size() == 3U);

    NeoEngine::PhysicsStepBudget budget;
    assert(budget.Accepts(100000U, 200000U));
    assert(!budget.Accepts(100001U, 200000U));
    budget.Record(100000U, 200000U, 200000U, 4999U);
    assert(budget.MeetsTarget());
    budget.Record(100000U, 200000U, 200000U, 5001U);
    assert(!budget.MeetsTarget());

    NeoEngine::CommandIdempotency idempotency(2U);
    assert(idempotency.Accept(1U, 0xAAU));
    assert(idempotency.Accept(1U, 0xAAU));
    assert(!idempotency.Accept(1U, 0xBBU));
    assert(idempotency.Accept(2U, 0xCCU));
    assert(!idempotency.Accept(3U, 0xDDU));
    assert(idempotency.Matches(2U, 0xCCU));

    NeoEngine::TransactionJournal journal(1000);
    NeoEngine::TransactionRecord transaction{};
    assert(journal.Apply(1U, -250, transaction));
    assert(transaction.balanceAfter == 750);
    assert(journal.Apply(1U, -250, transaction));
    assert(transaction.balanceAfter == 750);
    assert(!journal.Apply(1U, -200, transaction));
    assert(journal.Apply(2U, 100, transaction));
    assert(journal.Balance() == 850);

    NeoEngine::AndroidLifecycleGate lifecycle;
    assert(!lifecycle.CanRender());
    assert(lifecycle.OnResume());
    assert(lifecycle.CanRender());
    assert(lifecycle.OnPause());
    assert(!lifecycle.CanRender() && lifecycle.CanSubmitAudio());
    assert(lifecycle.OnResume());
    assert(lifecycle.OnStop());
    assert(!lifecycle.CanRender());
    assert(lifecycle.OnDestroy());
    assert(!lifecycle.OnResume());

    NeoEngine::SecurityCommandValidator validator({8U, 2U, true});
    assert(!validator.Validate(1U, 9U));
    assert(validator.Validate(1U, 8U));
    assert(!validator.Validate(1U, 8U));
    assert(validator.Validate(2U, 8U));
    assert(!validator.Validate(3U, 8U));
    validator.BeginFrame();
    assert(validator.Validate(3U, 8U));
    return 0;
}
