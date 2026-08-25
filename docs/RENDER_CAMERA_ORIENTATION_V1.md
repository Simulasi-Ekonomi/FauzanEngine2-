# Render Camera Orientation v1 — Validated 2D and 3D Projection Pose

## Ruang Lingkup

`RenderCamera` sebelumnya hanya melihat sepanjang sumbu +Z dunia. Milestone ini menambahkan pose orientasi eksplisit berupa `forward` dan `up` pada `RenderCameraConfig`, lalu membangun basis kamera yang ternormalisasi untuk proyeksi ortografis maupun perspektif.

| Mode | Kontrak yang dipertahankan | Tambahan orientasi |
|---|---|---|
| Orthographic | half-height, aspect, near/far clip, dan outside-clip fail-closed. | Koordinat dunia diproyeksikan melalui right/up/forward basis. |
| Perspective | vertical FOV, aspect, near/far clip, dan behind-camera fail-closed. | Arah view tidak lagi terkunci pada +Z dunia. |

## Validasi

`Initialize` menolak vector `forward`/`up` non-finite, forward nol, atau up yang kolinear terhadap forward. Basis dihitung sebagai `right = normalize(cross(up, forward))` dan `correctedUp = normalize(cross(forward, right))`. Tidak ada pembacaan atau penulisan `SceneWorld`, entity, RouteIntent, MovementAuthorityGate, atau skeletal route pada komponen ini.

> Orientasi kamera adalah state render lokal. Ia bukan writer transform gameplay dan tidak memperluas authority karakter/NPC.

## Bukti

`render_camera_smoke` tetap membuktikan proyeksi ortografis, perspektif, clipping, dan konfigurasi invalid lama; kini ia juga membuktikan camera perspective yang menghadap +X, proyeksi sumbu up yang benar, serta rejection forward nol dan up kolinear. Smoke lulus pada Release dan ASAN dengan `detect_leaks=1`. Suite non-Vulkan penuh lulus **95/95 Release** dan **95/95 ASAN**.

## Batas

Tidak ada camera controller, input pan/zoom/orbit, smoothing, follow target, frustum culling, matrices publik, temporal jitter, reverse-Z, stereo, editor gizmo, atau window interaction. Ini adalah fondasi proyeksi berorientasi yang terbatas untuk renderer 2D/3D, bukan sistem kamera game lengkap.
