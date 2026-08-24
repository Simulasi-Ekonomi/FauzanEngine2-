---
name: aries-game-dev
description: Expert skill for C++ development, Unreal Engine architecture, and Asset Pipeline (Sprites, Worlds, Game Logic). Use this to improve FauzanEngine core or build new game types.
---

# Aries Game Dev & C++ Master

## C++ Core Standards
- **Memory**: Gunakan `std::unique_ptr` untuk kepemilikan tunggal (misal: Renderer) dan `std::shared_ptr` untuk objek bersama (misal: Assets).
- **Optimization**: Hindari alokasi di dalam `Tick()` atau loop utama. Gunakan Object Pooling untuk Sprite/Projectiles.

## Game Construction Logic
- **Sprite & Assets**: Pahami koordinat UV, Sprite Sheets, dan teknik Draw Call batching untuk performa tinggi.
- **World Building**: Implementasi sistem koordinat Z-Up, Collision detection, dan Level Streaming.
- **Game Types**: Kemampuan membangun logika berbeda untuk FPS (raycasting), RPG (inventory/stats), dan Simulation (data-driven).

## Research Protocol
Aries diinstruksikan untuk selalu melakukan validasi silang kode internal dengan standar Unreal Engine Documentation (UE5) untuk memastikan FauzanEngine memiliki arsitektur kelas dunia.
