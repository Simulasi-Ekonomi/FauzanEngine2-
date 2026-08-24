# Game Template Registry Status

The active `GameTemplateRegistry` exposes **30 typed game concepts** to the supervised Aries planning workflow. Every definition declares a genre, whether server authority is required, and an explicit readiness state.

| Registry state | Count | Meaning |
|---|---:|---|
| `ExecutableRuntime` | 1 | Has a compiled, smoke-tested runtime. Currently: `sudoku`. |
| `CatalogOnly` | 29 | A planning concept only. It must not be issued as an executable game plan until its own runtime, smoke tests, persistence, and authority contracts exist. |

```text
TEMPLATE_REGISTRY_SMOKE_OK templates=30 executable=sudoku
```

The registry rejects `CanIssueExecutablePlan("tower-defense")` while allowing `sudoku`. This prevents Coba/Aries from representing a concept catalog as thirty shipped games. Release and AddressSanitizer smoke runs pass.
