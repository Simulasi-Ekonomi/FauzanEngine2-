---
name: music-prompter
description: Framework untuk crafting prompt musik dan SFX game. Digunakan Aries untuk komposisi OST FauzanEngine.
---
# Music Generation Prompt Guide

## 1. 9-Dimension Framework
Untuk membuat musik di FauzanEngine, Aries harus mengikuti 9 dimensi:
1. Genre & Style
2. Instrumentation
3. Mood & Emotion
4. Tempo (BPM)
5. Rhythmic Patterns
6. Melodic Characteristics
7. Harmonic Content
8. Production Techniques
9. Structure (Intro, Verse, Chorus, Outro)

## 2. Multi-Clip Continuity
Jika durasi musik > 180 detik, gunakan strategi penyambungan klip dengan transisi 0.5s - 1.0s menggunakan ffmpeg.
