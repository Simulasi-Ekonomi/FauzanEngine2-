# Tower Defense Template Status

`TowerDefenseGame` is the second executable template runtime. It has bounded three-lane placement, 16-tower capacity, wave admission, deterministic target ordering, enemy-path progression, base-life loss, reward accounting, and a deterministic state hash.

```text
TOWER_DEFENSE_SMOKE_OK wave=1 gold=70 lives=8
```

The smoke scenario verifies duplicate wave rejection, lane validation, deterministic replay against an identical simulation, killed enemy reward, and the expected base damage when no tower covers lane two. Release and AddressSanitizer pass. This is a game-state core only; graphical map/mesh presentation, progression balance, persistence, and backend economy authority remain separate work.
