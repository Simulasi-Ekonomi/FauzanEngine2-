# FauzanEngine Mobile Game Priority Matrix

## Decision rule

This matrix is a roadmap hypothesis, not a revenue forecast. Priority balances three criteria: alignment with the engine’s canonical tools, opportunity indicated by public market research, and the production burden required to compete ethically. A high market-revenue category is not automatically a high-priority first release if incumbent concentration or required LiveOps depth exceeds current capability.

## Recommended validation order

| Priority | Game family | Why it is relevant | Shared tools required before a release claim | Competitive / delivery risk |
|---|---|---|---|---|
| 1 | Farm / cozy simulation | It is the user’s principal product and already has the strongest domain foundation: economy, anti-inject rules, TrustSafety, telemetry, FarmWorldTool, NPCs, governance, and a headless frame. Public research lists Simulation among leading casual-revenue genres while noting strong incumbents. | Authoritative timed simulation; inventory/building/economy service; social/co-op rules if enabled; client presentation; save/recovery; moderation; operational tools. | High. Existing simulation leaders and long-term content/LiveOps expectations make local sandbox evidence insufficient. |
| 2 | Merge / sorting puzzle | Public reports identify Merge growth and hybrid puzzle experimentation, while the existing AssetRegistry, SceneWorld, telemetry, economy, and event primitives can be generalized. A bounded board tool is feasible without claiming parity with leading products. | Board/rule engine; deterministic replay; level-data importer; progression; session recovery; client touch UX; IAA/IAP policy; experiments and live-event toolchain. | High. Top revenue is concentrated among incumbents; product needs differentiated mechanics and measured soft-launch evidence. |
| 3 | Tower Defense | The engine already has an executable Tower Defense template and deterministic/physics foundations. It is a practical production-tool candidate that can stress combat, AI, maps, saving, and co-op authority. | Data-driven towers/enemies/waves; pathfinding; authoritative combat; balance pipeline; maps/assets; replay and desync test; co-op if advertised. | Medium to high. Progression and content velocity, not only combat simulation, determine release quality. |
| 4 | Match-Three / Match-2 | An executable template exists and public data confirms major market size, but this is not a low-risk first commercial release due to saturation and incumbent concentration. It is a useful tool-validation target after generic board/live-event systems exist. | Level authoring; solver/validation; boosters; economy; event calendar; client UX; anti-cheat; segmentation and operational evidence. | Very high. AppMagic reports only a small fraction of new Match-3/merge titles reaching even a low revenue threshold in its observed data. |
| 5 | Sudoku and premium offline puzzles | A small-scope candidate for validating offline safety, accessibility, localization, ads/consent boundaries, packaging, and store workflow. It should not be treated as a high-revenue guarantee. | Puzzle generator/validator; daily challenge policy; offline saves; accessible UI; Android packaging; privacy/consent evidence. | Lower technical risk, uncertain monetization potential. |
| 6 | Dungeon crawler / light RPG | The new sandbox proves large deterministic state, level/dungeon capacity, drops, upgrades, and respawn rules. It is appropriate after authoritative inventory/combat/persistence exist. | Authoritative inventory; combat/pathing; skill/equipment data; anti-cheat; accounts; social/guild systems only if supported; renderer/client. | High. Scope growth and content production are significant. |

## Explicit non-priorities before the shared platform passes

Casino, gambling-like mechanics, and real-money features are not a default priority. They introduce regulatory, age, geographic, payment, fraud, and responsible-use requirements beyond a generic casual-game roadmap. Likewise, copying existing top titles, fabricating social competition, or manufacturing ratings/reviews is prohibited.

## Shared platform sequence

1. Finish release gates R3–R11 that affect every mobile title: assets/renderer, authoritative service, persistence/recovery, anti-cheat, Android delivery, and operational evidence.
2. Convert Farm into the first authoritative vertical slice because it exercises the broadest reusable tools.
3. Extract reusable board, event, progression, and content-authoring components from Farm without coupling games to Farm-specific economy rules.
4. Upgrade Tower Defense and Match-Three one at a time through their own release matrices. Do not advertise all thirty catalog entries as production-ready by association.

## Market evidence and limits

The categorization is grounded in the public research notes in [mobile_game_market_research_2025_2026.md](mobile_game_market_research_2025_2026.md). The figures come from third-party data samples, may combine app stores, and do not predict FauzanEngine revenue, installs, ROAS, or Play Store ranking. Final genre selection requires a real, consented, policy-compliant soft launch after the release gates are technically passed.
