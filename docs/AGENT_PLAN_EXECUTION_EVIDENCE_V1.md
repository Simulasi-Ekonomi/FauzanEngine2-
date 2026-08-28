# Agent Plan Execution Evidence V1

**Increment:** P1.4a  
**Scope:** bounded in-process consumption boundary between an approved PromptToolGraph plan and a supervised executor. The implementation emits an execution receipt only; it does not call an LLM, invoke an external API, mutate runtime/economy/ban authority, deploy, or perform other external side effects.

## Result

`prompt_tool_graph_smoke` passed in both configurations.

| Configuration | Build | Sanitizer | Result |
|---|---|---|---|
| Release | `CMAKE_BUILD_TYPE=Release` | None | PASS |
| ASAN | `CMAKE_BUILD_TYPE=Debug` | AddressSanitizer with `detect_leaks=1` | PASS |

Smoke output:

```text
PROMPT_TOOL_GRAPH_SMOKE_OK nodes=3 dryRun=1 approval=typed execution=once authority=denied side_effects=0
```

## Verified behavior

The test first evaluates a three-node dry-run graph and preserves the existing dependency-cycle and authority rejection checks. It then issues the same plan through the gateway with build/test evidence, verifies that a tampered node receipt is rejected without modifying the output receipt, rejects the same receipt set against a gateway that never issued the plan, consumes the correctly issued plan once in topological order, and rejects a replay as `PlanAlreadyConsumed` while preserving the caller's prior output.

`AgentCommandGateway::IsPlanIssued` binds the receipt to a non-dry-run command, its request identifier, the derived `plan-<requestId>` reference, and the gateway's issued-request set. `PromptToolGraph::ExecuteIssued` validates the plan and all receipts before inserting the prompt identifier into a bounded consumed-plan ledger. Authority commands remain rejected by the gateway, and the execution receipt explicitly reports `externalSideEffectsApplied=false`.

## Changed paths

| Path | Change |
|---|---|
| `Source/NeoEngine/Agents/AgentCommandGateway.h` | Added read-only issued-plan verification. |
| `Source/NeoEngine/Agents/AgentCommandGateway.cpp` | Implemented request/plan binding against the gateway issue ledger. |
| `Source/NeoEngine/Agents/PromptToolGraph.h` | Added execution receipt, bounded ledger, and fail-closed error states. |
| `Source/NeoEngine/Agents/PromptToolGraph.cpp` | Added atomic preflight and one-time consumption of issued plans. |
| `Tests/Agents/prompt_tool_graph_smoke.cpp` | Added malformed, unissued, one-shot, and output-preservation assertions. |

## Boundary

This is an in-process contract test, not an agent service. It does not prove prompt parsing, document ingestion, LLM reasoning, external tool execution, persistence across process restarts, distributed replay protection, secret management, autonomous deployment, or production operations.
