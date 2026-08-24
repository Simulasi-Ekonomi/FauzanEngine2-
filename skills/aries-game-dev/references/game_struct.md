# Game Structure Reference

## 1. Sprite & Assets
- **Format**: PNG/TGA dengan Alpha Channel.
- **Atlas**: Gunakan Sprite Sheets untuk efisiensi render (Draw Calls).

## 2. World & Physics
- **Collision**: Box, Sphere, dan Mesh-based collision.
- **Coordinate System**: Z-Up (Standar Unreal) vs Y-Up.

## 3. Game Types Logic
- **Simulation**: State-machine based, fokus pada data persistence.
- **Action**: Physics-based, fokus pada frame-rate independent movement.
