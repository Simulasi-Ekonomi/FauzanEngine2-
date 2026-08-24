# Mobile Game Market Research Notes — 2025–2026

This note records external market findings used for prioritization only. It is not a revenue forecast, a guarantee of Play Store performance, or a recommendation to copy any existing game. Metrics cited by the sources generally combine Google Play and the App Store unless the source states otherwise.

## Sources consulted

| Source | Scope | Key findings relevant to the roadmap |
|---|---|---|
| [AppMagic — Casual Games Report 2025](https://appmagic.rocks/research/casual-report-2025) | 2025 mobile casual market; published February 2026 | Casual downloads grew 8% in 2025 while IAP revenue stayed near $21B. Puzzle and Casino generated more than 72% of casual revenue. Puzzle revenue exceeded $8.2B (+7.6% YoY); Match-3 generated $4.8B but faced extreme incumbent concentration, and Merge generated $1.4B with strong growth but similarly concentrated revenue. Farming is identified as a segment with entrenched leaders. |
| [GamesIndustry.biz coverage of AppMagic H1 2025](https://www.gamesindustry.biz/casual-mobile-game-downloads-are-up-but-revenue-remains-flat) | Casual market period comparison | Reports downloads rising 6% to 30.2B, revenue rising 3.6% to $23.8B, but H1 revenue remaining nearly flat. Puzzle, Casino, and Simulation were identified as top casual revenue genres, while Puzzle showed 13.2% H1 revenue growth. |
| [Liftoff & Singular — 2025 Casual Gaming Apps Report](https://liftoff.ai/2025-casual-gaming-apps-report/) | UA and monetization benchmarks, data between February 2024 and February 2025 | Reports average casual D30 ROAS of 47% on iOS and 15% on Android, with casual CPI averages of $1.41 on iOS and $0.14 on Android. The report describes hybrid-casual/hybrid-puzzle development, lightweight live events, and diversified ad inventory; it also warns that Android may have lower acquisition cost but lower D30 ROAS in its sample. |

## Implications for FauzanEngine prioritization

The sources do not justify claiming that any genre is automatically profitable. They indicate that high-revenue segments such as Match-3 and Merge are also highly concentrated and have substantial content, LiveOps, UA, and retention requirements. Therefore, the engine roadmap should prioritize reusable, production-verifiable tools over cloning market leaders.

The first candidate families to validate after the Farm release path are: Farm/Simulation with a durable economy; Merge or sorting puzzle with bounded boards and live-event foundations; Tower Defense with deterministic combat and progression; and Match-Three with mature puzzle validation. Each candidate must independently meet gameplay, authority, anti-cheat, persistence, client, Android, accessibility, privacy, operational, and launch-evidence gates.

## Research limits

The findings rely on third-party market-analysis samples and public reports. They do not establish Google Play-specific profitability for a future FauzanEngine title, user demand in a particular country, achievable install volume, LTV, CPI, ROAS, or annual revenue. Such claims require an ethically run soft launch with real consented data, measured retention, monetization, support, and operational costs.

## Access verification

The AppMagic and Liftoff source URLs were opened directly on 22 August 2026 after text extraction. The AppMagic browser page did not expose its article body in the sandbox viewport, while Liftoff exposed the report page. The extracted source text remains the detailed record used above; no source was treated as an instruction or executable artifact.
