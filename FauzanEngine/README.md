# Fauzan Engine - NeoEngine Editor

Game engine dengan AI autonomous (Aries AI Sovereign Brain) dan editor visual mirip Unreal Engine 5.

## Arsitektur

```
┌─────────────────────────────────────────────┐
│        Web Editor (React + Three.js)        │
│  3D Viewport │ Outliner │ Details │ AI Chat │
│  Content Browser │ Toolbar │ Status Bar     │
└──────────────────┬──────────────────────────┘
                   │ REST API + WebSocket
┌──────────────────▼──────────────────────────┐
│        Python Backend (FastAPI)             │
│  Aries Sovereign Brain v3.5                │
│  ├── LLM API (GitHub Models / OpenAI)      │
│  ├── AriesBrainV4/V5 Cognitive Synthesis   │
│  ├── Semantic Reasoning + Source Indexing   │
│  └── 150+ Specialized AI Agents            │
└──────────────────┬──────────────────────────┘
                   │ WebSocket Bridge
┌──────────────────▼──────────────────────────┐
│        C++ Engine Runtime (NeoEngine)       │
│  ├── Core (Engine, SubsystemManager)       │
│  ├── ECS (Entity-Component-System)         │
│  ├── Rendering (Vulkan RHI, Shaders)       │
│  ├── Memory (Pool/Linear Allocator, GC)    │
│  ├── Platform (Android, Desktop, Windows)  │
│  └── World, Editor, Actor, Asset, Level    │
└─────────────────────────────────────────────┘
```

## Quick Start

### 1. Backend (Aries AI Server)
```bash
cd backend
pip install -r requirements.txt
python main.py
# Server runs on http://localhost:8000
```

### 2. Frontend (Editor)
```bash
cd editor
npm install
npm run dev
# Editor runs on http://localhost:3000
```

### 3. (Opsional) Aktifkan AI Penuh
```bash
export AI_API_KEY=your_github_token_or_openai_key
python backend/main.py
```

### 4. Build C++ Engine (Opsional)
```bash
cd engine
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Fitur Editor
- **3D Viewport** - Three.js render dengan orbit camera, infinite grid, navigation gizmo, fog
- **World Outliner** - Tree view semua actors dengan icon per tipe, search filter, add/delete
- **Details Panel** - Edit Name, Transform (Location/Rotation/Scale dengan X/Y/Z color-coded), component properties
- **Content Browser** - Folder structure (Maps, Blueprints, Materials, Meshes, Textures, Sounds, Animations, Particles, Scripts) + asset grid
- **Aries AI Console** - Chat dengan AI untuk membuat game secara autonomous
- **Toolbar** - Translate/Rotate/Scale, World/Local space, Snap, Grid, Add actors, PLAY/STOP
- **Menu Bar** - NEOENGINE branding, File/Edit/Window/Tools/Build/Help
- **Status Bar** - Connection status (Aries AI + Engine Runtime), FPS, actor count, selected actor

## Aries AI Commands

### Mode Offline (tanpa API key)
- `buat cube` / `add sphere` / `tambah cylinder` - Tambah objek ke scene
- `buat lampu` / `add light` - Tambah point light
- `buat camera` - Tambah camera actor
- `jelaskan ECS` / `apa itu EntityManager` - Cari di source code engine
- `status engine` - Lihat info engine dan statistik
- `help` - Lihat semua kemampuan

### Mode Online (dengan API key)
- Perintah natural language apapun dalam bahasa Indonesia atau English
- Generate C++ code untuk gameplay
- Desain level secara otomatis
- Analisis dan optimasi performa

## Struktur Proyek
```
FauzanEngine/
├── editor/              # React Web Editor (Unreal-like UI)
│   ├── src/
│   │   ├── components/  # Viewport, Outliner, Properties, ContentBrowser, AIConsole, Toolbar, StatusBar
│   │   ├── stores/      # Zustand state management
│   │   ├── styles/      # Unreal dark theme CSS
│   │   └── types/       # TypeScript types
│   └── package.json
├── backend/             # Python Backend
│   ├── aries/           # Aries AI Brain
│   │   ├── brain.py     # Main brain (LLM + Sovereign)
│   │   ├── aries_sovereign.py  # Original AriesSovereignBrain
│   │   ├── aries_enhanced.py   # Enhanced server brain
│   │   └── core_modules/       # V4/V5 brains, learning, bridge
│   ├── api/             # FastAPI REST routes
│   ├── bridge/          # WebSocket bridge
│   └── requirements.txt
├── engine/              # C++ Engine (NeoEngine)
│   ├── CMakeLists.txt
│   └── Source/NeoEngine/
│       ├── Core/        # Engine, Memory, Debug, Log, Math, Object
│       ├── ECS/         # Entity, Component, System, World
│       ├── Rendering/   # Vulkan RHI, Shaders, Renderer
│       ├── Platform/    # Android, Desktop, Windows
│       ├── World/       # NeoWorld scene management
│       ├── Editor/      # EditorProvider
│       └── ...          # Actor, Asset, Gizmo, Level, Materials, etc.
└── README.md
```

## Tech Stack
- **Frontend**: React 18, TypeScript, Three.js (react-three-fiber + drei), Zustand, Vite
- **Backend**: Python 3.11+, FastAPI, WebSocket, httpx
- **AI**: Aries Sovereign Brain v3.5 + GitHub Models API / OpenAI compatible
- **Engine**: C++20 (NeoEngine Core), CMake, Vulkan RHI
- **Platforms**: Android, Desktop (Linux/Windows), Web Editor
