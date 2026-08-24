# Coba–Aries Agent Gateway Status

## Canonical runtime contract

The active NeoEngine build now contains `AgentCommandGateway` and `agent_gateway_smoke`. The gateway is intentionally separate from legacy AI integration files that are not part of the active CMake runtime and that contain platform-specific or placeholder behavior.

| Agent | Allowed command classes | Explicitly forbidden |
|---|---|---|
| **Coba** | Runtime audit and rollback proposal. | Game-template creation, runtime mutation, economy mutation, filesystem/network execution. |
| **Aries** | Game-template proposal, build request, and rollback proposal. | Runtime mutation, economy mutation, payment action, filesystem/network execution. |

Every command has a bounded typed request ID and target. Dry runs return a plan reference only. A non-dry-run request requires separate approval plus build/test evidence, then yields only a plan issued to an external supervised executor. The gateway itself has no implementation path that changes runtime state, economy state, files, or network resources.

## Latest evidence

```text
AGENT_GATEWAY_SMOKE_OK authority=denied workflow=typed
```

The smoke test confirms that Coba auditing and Aries template dry-runs are permitted, direct economy mutation is rejected, approval fails without complete evidence, a fully evidenced plan is issued once, and replay issuance is rejected. Release and AddressSanitizer builds passed.

The next agent step is a supervised external executor that consumes an issued plan, records source-diff/build/test evidence, and exposes a human confirmation boundary before it can affect a repository. It must continue to keep payment, ledger, ban, and live-runtime authority outside Coba and Aries.
