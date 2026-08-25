# Animation Sprite Tint Binding v1

## Tujuan

Fase C.3a membuat sample scalar dari `AnimationStateMachine` terlihat pada output frame tanpa memberi animation system kepemilikan transform. `AnimationSpriteTintBinding` memetakan sample finite `[0, 1]` secara linear ke RGBA idle dan locomotion. `SceneSpriteAdapter::QueueTinted` mengalikan tint tersebut dengan tint instance hanya ketika sprite sedang di-queue ke `SpriteBatch` untuk frame itu.

> Binding ini bersifat **frame-local**. Ia tidak menulis `SceneWorld`, transform actor, RouteIntent, MovementAuthorityGate, motion controller, asset registry, atau state machine.

## Kontrak

| Area | Perilaku bounded |
|---|---|
| Input | Sample harus finite pada interval inklusif `[0, 1]`. |
| Output | RGBA idle→locomotion diblend per channel deterministik; output caller tidak diganti bila sample invalid. |
| Queue sprite | Tint frame dikalikan per-channel dengan `rgba` instance saat `SpriteBatch` dibentuk. |
| World state | `QueueTinted` hanya membaca transform dari `SceneWorld`, sama seperti `Queue`. |
| Authority | Tidak ada acquisition/step/transform write selain kinematic path existing di demo. |

Demo motion-animation memakai idle putih `0xFFFFFFFF` dan locomotion tint `0xFF80D0FF`. Empat frame finite mempertahankan script gerak sebelumnya: dua frame input menghasilkan sample locomotion/tint, lalu dua frame idle. Receipt mencatat jumlah frame bertint locomotion dan hash urutan tint sebagai observable tambahan.

## Evidence executable

`animation_sprite_tint_binding_smoke` membuktikan guard sebelum initialization, endpoint RGBA, midpoint `0xFF808080`, penolakan NaN tanpa mengganti output, serta boundary `noTransformWrite`. `motion_animation_surface_demo_smoke` tetap membuktikan 4 frame SDL/PPM finite, satu-satunya writer gerak kinematic, final idle sample, dan kini dua frame tint locomotion dengan tint sequence hash nonzero.

| Gate | Hasil final |
|---|---|
| `animation_sprite_tint_binding_smoke` Release + ASAN | Lulus dengan `detect_leaks=1`. |
| `motion_animation_surface_demo_smoke` Release + ASAN | Lulus; 4 frame, 4 present, 42 pixel terlihat, `x=0.067`, tint locomotion 2 frame. |
| Broad non-Vulkan Release | 117/117 smoke lulus. |
| Broad non-Vulkan AddressSanitizer | 117/117 smoke lulus dengan `detect_leaks=1`. |

## Batas terbuka

Binding ini bukan skeletal animation, flipbook/atlas, material animation, animation events, blend tree multi-channel, GPU skinning, GPU renderer, character controller baru, NPC AI, asset animation importer, desktop editor UI, APK/AAB, atau release readiness. Ia hanya menambah output visual bounded di atas state machine scalar existing.
