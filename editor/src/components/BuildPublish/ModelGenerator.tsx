import React, { useState } from 'react';

interface ModelGeneratorProps {
  isOpen: boolean;
  onClose: () => void;
  onGenerate: (modelData: GeneratedModel) => void;
  onLog: (msg: string) => void;
}

export interface GeneratedModel {
  name: string;
  type: 'mesh' | 'character' | 'environment' | 'prop' | 'vehicle';
  vertices: number;
  faces: number;
  actors: Array<{
    name: string;
    type: string;
    position: { x: number; y: number; z: number };
    scale: { x: number; y: number; z: number };
    color?: string;
  }>;
}

// Procedural 3D model presets
const MODEL_PRESETS: Record<string, { category: string; models: Array<{ name: string; prompt: string }> }> = {
  buildings: {
    category: 'Buildings & Architecture',
    models: [
      { name: 'Simple House', prompt: 'buat rumah sederhana' },
      { name: 'Medieval Castle', prompt: 'buat kastil medieval' },
      { name: 'Modern Skyscraper', prompt: 'buat gedung pencakar langit' },
      { name: 'Japanese Temple', prompt: 'buat kuil jepang' },
      { name: 'Fantasy Tower', prompt: 'buat menara fantasy' },
      { name: 'Wooden Cabin', prompt: 'buat kabin kayu' },
    ],
  },
  characters: {
    category: 'Characters',
    models: [
      { name: 'Knight', prompt: 'buat karakter knight' },
      { name: 'Wizard', prompt: 'buat karakter wizard' },
      { name: 'Robot', prompt: 'buat karakter robot' },
      { name: 'Zombie', prompt: 'buat karakter zombie' },
      { name: 'Dragon', prompt: 'buat naga' },
      { name: 'Villager NPC', prompt: 'buat npc penduduk' },
    ],
  },
  environment: {
    category: 'Environment & Nature',
    models: [
      { name: 'Tree (Oak)', prompt: 'buat pohon oak' },
      { name: 'Pine Tree', prompt: 'buat pohon pinus' },
      { name: 'Rock Formation', prompt: 'buat formasi batu' },
      { name: 'Mountain', prompt: 'buat gunung' },
      { name: 'River with Bridge', prompt: 'buat sungai dengan jembatan' },
      { name: 'Waterfall', prompt: 'buat air terjun' },
    ],
  },
  props: {
    category: 'Props & Items',
    models: [
      { name: 'Treasure Chest', prompt: 'buat peti harta' },
      { name: 'Sword', prompt: 'buat pedang' },
      { name: 'Shield', prompt: 'buat perisai' },
      { name: 'Campfire', prompt: 'buat api unggun' },
      { name: 'Barrel', prompt: 'buat tong' },
      { name: 'Torch', prompt: 'buat obor' },
    ],
  },
  vehicles: {
    category: 'Vehicles',
    models: [
      { name: 'Car', prompt: 'buat mobil' },
      { name: 'Spaceship', prompt: 'buat pesawat luar angkasa' },
      { name: 'Boat', prompt: 'buat perahu' },
      { name: 'Tank', prompt: 'buat tank' },
      { name: 'Horse', prompt: 'buat kuda' },
      { name: 'Helicopter', prompt: 'buat helikopter' },
    ],
  },
};

// Generate procedural 3D model from prompt
export function generateModelFromPrompt(prompt: string): GeneratedModel | null {
  const p = prompt.toLowerCase();

  // House / Rumah
  if (p.includes('rumah') || p.includes('house') || p.includes('home')) {
    return {
      name: 'House', type: 'environment', vertices: 248, faces: 124,
      actors: [
        { name: 'House_Foundation', type: 'cube', position: { x: 0, y: 0.25, z: 0 }, scale: { x: 4, y: 0.5, z: 3 }, color: '#8B7355' },
        { name: 'House_Walls', type: 'cube', position: { x: 0, y: 1.75, z: 0 }, scale: { x: 3.8, y: 2.5, z: 2.8 }, color: '#D2B48C' },
        { name: 'House_Roof', type: 'cube', position: { x: 0, y: 3.5, z: 0 }, scale: { x: 4.2, y: 0.8, z: 3.2 }, color: '#8B0000' },
        { name: 'House_Door', type: 'cube', position: { x: 0, y: 1.2, z: 1.45 }, scale: { x: 0.8, y: 1.8, z: 0.1 }, color: '#5C4033' },
        { name: 'House_Window_L', type: 'cube', position: { x: -1, y: 2, z: 1.45 }, scale: { x: 0.6, y: 0.6, z: 0.05 }, color: '#87CEEB' },
        { name: 'House_Window_R', type: 'cube', position: { x: 1, y: 2, z: 1.45 }, scale: { x: 0.6, y: 0.6, z: 0.05 }, color: '#87CEEB' },
        { name: 'House_Chimney', type: 'cube', position: { x: 1.2, y: 4.2, z: -0.5 }, scale: { x: 0.5, y: 1.2, z: 0.5 }, color: '#696969' },
      ],
    };
  }

  // Castle / Kastil
  if (p.includes('kastil') || p.includes('castle') || p.includes('fortress')) {
    return {
      name: 'Castle', type: 'environment', vertices: 512, faces: 256,
      actors: [
        { name: 'Castle_Base', type: 'cube', position: { x: 0, y: 1, z: 0 }, scale: { x: 8, y: 2, z: 8 }, color: '#808080' },
        { name: 'Castle_Tower_FL', type: 'cylinder', position: { x: -3.5, y: 3, z: 3.5 }, scale: { x: 1.2, y: 4, z: 1.2 }, color: '#696969' },
        { name: 'Castle_Tower_FR', type: 'cylinder', position: { x: 3.5, y: 3, z: 3.5 }, scale: { x: 1.2, y: 4, z: 1.2 }, color: '#696969' },
        { name: 'Castle_Tower_BL', type: 'cylinder', position: { x: -3.5, y: 3, z: -3.5 }, scale: { x: 1.2, y: 4, z: 1.2 }, color: '#696969' },
        { name: 'Castle_Tower_BR', type: 'cylinder', position: { x: 3.5, y: 3, z: -3.5 }, scale: { x: 1.2, y: 4, z: 1.2 }, color: '#696969' },
        { name: 'Castle_MainTower', type: 'cylinder', position: { x: 0, y: 4.5, z: 0 }, scale: { x: 2, y: 7, z: 2 }, color: '#778899' },
        { name: 'Castle_Gate', type: 'cube', position: { x: 0, y: 1.2, z: 4.1 }, scale: { x: 2, y: 2.4, z: 0.3 }, color: '#5C4033' },
        { name: 'Castle_Wall_L', type: 'cube', position: { x: -3.5, y: 2, z: 0 }, scale: { x: 0.5, y: 3, z: 7 }, color: '#808080' },
        { name: 'Castle_Wall_R', type: 'cube', position: { x: 3.5, y: 2, z: 0 }, scale: { x: 0.5, y: 3, z: 7 }, color: '#808080' },
        { name: 'Castle_Flag', type: 'cube', position: { x: 0, y: 8.5, z: 0 }, scale: { x: 1, y: 0.6, z: 0.05 }, color: '#FF0000' },
      ],
    };
  }

  // Knight
  if (p.includes('knight') || p.includes('ksatria')) {
    return {
      name: 'Knight', type: 'character', vertices: 180, faces: 90,
      actors: [
        { name: 'Knight_Body', type: 'cube', position: { x: 0, y: 1.2, z: 0 }, scale: { x: 0.6, y: 0.8, z: 0.4 }, color: '#C0C0C0' },
        { name: 'Knight_Head', type: 'sphere', position: { x: 0, y: 1.9, z: 0 }, scale: { x: 0.35, y: 0.4, z: 0.35 }, color: '#C0C0C0' },
        { name: 'Knight_Helmet', type: 'cube', position: { x: 0, y: 2.1, z: 0 }, scale: { x: 0.4, y: 0.15, z: 0.4 }, color: '#808080' },
        { name: 'Knight_Legs', type: 'cube', position: { x: 0, y: 0.4, z: 0 }, scale: { x: 0.5, y: 0.8, z: 0.35 }, color: '#696969' },
        { name: 'Knight_Sword', type: 'cube', position: { x: 0.5, y: 1.2, z: 0 }, scale: { x: 0.06, y: 1.2, z: 0.06 }, color: '#E8E8E8' },
        { name: 'Knight_Shield', type: 'cube', position: { x: -0.5, y: 1.2, z: 0.1 }, scale: { x: 0.05, y: 0.6, z: 0.5 }, color: '#8B0000' },
      ],
    };
  }

  // Robot
  if (p.includes('robot') || p.includes('mech')) {
    return {
      name: 'Robot', type: 'character', vertices: 200, faces: 100,
      actors: [
        { name: 'Robot_Body', type: 'cube', position: { x: 0, y: 1.5, z: 0 }, scale: { x: 0.8, y: 1, z: 0.5 }, color: '#4A90D9' },
        { name: 'Robot_Head', type: 'cube', position: { x: 0, y: 2.3, z: 0 }, scale: { x: 0.5, y: 0.5, z: 0.5 }, color: '#5AA0E9' },
        { name: 'Robot_Eye_L', type: 'sphere', position: { x: -0.12, y: 2.35, z: 0.26 }, scale: { x: 0.08, y: 0.08, z: 0.08 }, color: '#FF0000' },
        { name: 'Robot_Eye_R', type: 'sphere', position: { x: 0.12, y: 2.35, z: 0.26 }, scale: { x: 0.08, y: 0.08, z: 0.08 }, color: '#FF0000' },
        { name: 'Robot_Arm_L', type: 'cube', position: { x: -0.6, y: 1.5, z: 0 }, scale: { x: 0.2, y: 0.8, z: 0.2 }, color: '#3A80C9' },
        { name: 'Robot_Arm_R', type: 'cube', position: { x: 0.6, y: 1.5, z: 0 }, scale: { x: 0.2, y: 0.8, z: 0.2 }, color: '#3A80C9' },
        { name: 'Robot_Legs', type: 'cube', position: { x: 0, y: 0.5, z: 0 }, scale: { x: 0.6, y: 1, z: 0.4 }, color: '#2A70B9' },
        { name: 'Robot_Antenna', type: 'cube', position: { x: 0, y: 2.8, z: 0 }, scale: { x: 0.04, y: 0.4, z: 0.04 }, color: '#888' },
      ],
    };
  }

  // Tree / Pohon
  if (p.includes('pohon') || p.includes('tree')) {
    const isPine = p.includes('pinus') || p.includes('pine');
    return {
      name: isPine ? 'Pine Tree' : 'Oak Tree', type: 'environment', vertices: 96, faces: 48,
      actors: [
        { name: 'Tree_Trunk', type: 'cylinder', position: { x: 0, y: 1.5, z: 0 }, scale: { x: 0.3, y: 3, z: 0.3 }, color: '#8B4513' },
        ...(isPine ? [
          { name: 'Tree_Leaves_1', type: 'sphere', position: { x: 0, y: 4, z: 0 }, scale: { x: 1.5, y: 1.5, z: 1.5 }, color: '#006400' },
          { name: 'Tree_Leaves_2', type: 'sphere', position: { x: 0, y: 5, z: 0 }, scale: { x: 1.1, y: 1.2, z: 1.1 }, color: '#228B22' },
          { name: 'Tree_Leaves_3', type: 'sphere', position: { x: 0, y: 5.8, z: 0 }, scale: { x: 0.7, y: 1, z: 0.7 }, color: '#2E8B57' },
        ] : [
          { name: 'Tree_Canopy', type: 'sphere', position: { x: 0, y: 4.5, z: 0 }, scale: { x: 2.5, y: 2, z: 2.5 }, color: '#228B22' },
          { name: 'Tree_Canopy_2', type: 'sphere', position: { x: 0.5, y: 4, z: 0.5 }, scale: { x: 1.5, y: 1.5, z: 1.5 }, color: '#2E8B57' },
        ]),
      ],
    };
  }

  // Dragon / Naga
  if (p.includes('dragon') || p.includes('naga')) {
    return {
      name: 'Dragon', type: 'character', vertices: 350, faces: 175,
      actors: [
        { name: 'Dragon_Body', type: 'sphere', position: { x: 0, y: 2, z: 0 }, scale: { x: 1.5, y: 1, z: 2 }, color: '#8B0000' },
        { name: 'Dragon_Head', type: 'sphere', position: { x: 0, y: 2.5, z: 2 }, scale: { x: 0.6, y: 0.5, z: 0.8 }, color: '#8B0000' },
        { name: 'Dragon_Jaw', type: 'cube', position: { x: 0, y: 2.2, z: 2.5 }, scale: { x: 0.4, y: 0.15, z: 0.5 }, color: '#6B0000' },
        { name: 'Dragon_Wing_L', type: 'cube', position: { x: -2, y: 3, z: 0 }, scale: { x: 2, y: 0.05, z: 1.5 }, color: '#660000' },
        { name: 'Dragon_Wing_R', type: 'cube', position: { x: 2, y: 3, z: 0 }, scale: { x: 2, y: 0.05, z: 1.5 }, color: '#660000' },
        { name: 'Dragon_Tail', type: 'cube', position: { x: 0, y: 1.8, z: -2 }, scale: { x: 0.3, y: 0.3, z: 2 }, color: '#8B0000' },
        { name: 'Dragon_Tail_Tip', type: 'sphere', position: { x: 0, y: 1.8, z: -3.2 }, scale: { x: 0.4, y: 0.4, z: 0.4 }, color: '#6B0000' },
        { name: 'Dragon_Leg_FL', type: 'cube', position: { x: -0.6, y: 0.6, z: 1 }, scale: { x: 0.3, y: 1.2, z: 0.3 }, color: '#7B0000' },
        { name: 'Dragon_Leg_FR', type: 'cube', position: { x: 0.6, y: 0.6, z: 1 }, scale: { x: 0.3, y: 1.2, z: 0.3 }, color: '#7B0000' },
        { name: 'Dragon_Eye_L', type: 'sphere', position: { x: -0.25, y: 2.7, z: 2.3 }, scale: { x: 0.1, y: 0.1, z: 0.1 }, color: '#FFD700' },
        { name: 'Dragon_Eye_R', type: 'sphere', position: { x: 0.25, y: 2.7, z: 2.3 }, scale: { x: 0.1, y: 0.1, z: 0.1 }, color: '#FFD700' },
      ],
    };
  }

  // Car / Mobil
  if (p.includes('mobil') || p.includes('car') || p.includes('vehicle')) {
    return {
      name: 'Car', type: 'vehicle', vertices: 160, faces: 80,
      actors: [
        { name: 'Car_Body', type: 'cube', position: { x: 0, y: 0.5, z: 0 }, scale: { x: 1.8, y: 0.5, z: 4 }, color: '#E04040' },
        { name: 'Car_Cabin', type: 'cube', position: { x: 0, y: 1.05, z: -0.3 }, scale: { x: 1.5, y: 0.6, z: 2 }, color: '#CC3030' },
        { name: 'Car_Windshield', type: 'cube', position: { x: 0, y: 1.05, z: 0.75 }, scale: { x: 1.4, y: 0.55, z: 0.05 }, color: '#87CEEB' },
        { name: 'Car_Wheel_FL', type: 'cylinder', position: { x: -0.9, y: 0.2, z: 1.2 }, scale: { x: 0.35, y: 0.2, z: 0.35 }, color: '#1a1a1a' },
        { name: 'Car_Wheel_FR', type: 'cylinder', position: { x: 0.9, y: 0.2, z: 1.2 }, scale: { x: 0.35, y: 0.2, z: 0.35 }, color: '#1a1a1a' },
        { name: 'Car_Wheel_BL', type: 'cylinder', position: { x: -0.9, y: 0.2, z: -1.2 }, scale: { x: 0.35, y: 0.2, z: 0.35 }, color: '#1a1a1a' },
        { name: 'Car_Wheel_BR', type: 'cylinder', position: { x: 0.9, y: 0.2, z: -1.2 }, scale: { x: 0.35, y: 0.2, z: 0.35 }, color: '#1a1a1a' },
        { name: 'Car_Headlight_L', type: 'sphere', position: { x: -0.6, y: 0.5, z: 2.05 }, scale: { x: 0.15, y: 0.1, z: 0.05 }, color: '#FFFACD' },
        { name: 'Car_Headlight_R', type: 'sphere', position: { x: 0.6, y: 0.5, z: 2.05 }, scale: { x: 0.15, y: 0.1, z: 0.05 }, color: '#FFFACD' },
      ],
    };
  }

  // Spaceship / Pesawat
  if (p.includes('pesawat') || p.includes('spaceship') || p.includes('spacecraft')) {
    return {
      name: 'Spaceship', type: 'vehicle', vertices: 200, faces: 100,
      actors: [
        { name: 'Ship_Hull', type: 'sphere', position: { x: 0, y: 1, z: 0 }, scale: { x: 1, y: 0.4, z: 2.5 }, color: '#778899' },
        { name: 'Ship_Cockpit', type: 'sphere', position: { x: 0, y: 1.3, z: 1 }, scale: { x: 0.5, y: 0.3, z: 0.8 }, color: '#87CEEB' },
        { name: 'Ship_Wing_L', type: 'cube', position: { x: -1.8, y: 0.9, z: -0.3 }, scale: { x: 2, y: 0.08, z: 1.2 }, color: '#696969' },
        { name: 'Ship_Wing_R', type: 'cube', position: { x: 1.8, y: 0.9, z: -0.3 }, scale: { x: 2, y: 0.08, z: 1.2 }, color: '#696969' },
        { name: 'Ship_Engine_L', type: 'cylinder', position: { x: -1, y: 0.8, z: -1.5 }, scale: { x: 0.25, y: 0.6, z: 0.25 }, color: '#555' },
        { name: 'Ship_Engine_R', type: 'cylinder', position: { x: 1, y: 0.8, z: -1.5 }, scale: { x: 0.25, y: 0.6, z: 0.25 }, color: '#555' },
        { name: 'Ship_Thruster_L', type: 'sphere', position: { x: -1, y: 0.8, z: -1.9 }, scale: { x: 0.2, y: 0.2, z: 0.2 }, color: '#00BFFF' },
        { name: 'Ship_Thruster_R', type: 'sphere', position: { x: 1, y: 0.8, z: -1.9 }, scale: { x: 0.2, y: 0.2, z: 0.2 }, color: '#00BFFF' },
      ],
    };
  }

  // Sword / Pedang
  if (p.includes('pedang') || p.includes('sword')) {
    return {
      name: 'Sword', type: 'prop', vertices: 64, faces: 32,
      actors: [
        { name: 'Sword_Blade', type: 'cube', position: { x: 0, y: 1.5, z: 0 }, scale: { x: 0.08, y: 1.5, z: 0.3 }, color: '#E8E8E8' },
        { name: 'Sword_Guard', type: 'cube', position: { x: 0, y: 0.6, z: 0 }, scale: { x: 0.6, y: 0.08, z: 0.1 }, color: '#FFD700' },
        { name: 'Sword_Handle', type: 'cylinder', position: { x: 0, y: 0.25, z: 0 }, scale: { x: 0.06, y: 0.5, z: 0.06 }, color: '#8B4513' },
        { name: 'Sword_Pommel', type: 'sphere', position: { x: 0, y: 0, z: 0 }, scale: { x: 0.1, y: 0.1, z: 0.1 }, color: '#FFD700' },
      ],
    };
  }

  // Campfire / Api unggun
  if (p.includes('api unggun') || p.includes('campfire') || p.includes('bonfire')) {
    return {
      name: 'Campfire', type: 'prop', vertices: 96, faces: 48,
      actors: [
        { name: 'Fire_Log_1', type: 'cylinder', position: { x: -0.2, y: 0.1, z: 0 }, scale: { x: 0.1, y: 0.8, z: 0.1 }, color: '#8B4513' },
        { name: 'Fire_Log_2', type: 'cylinder', position: { x: 0.2, y: 0.1, z: 0.1 }, scale: { x: 0.1, y: 0.8, z: 0.1 }, color: '#A0522D' },
        { name: 'Fire_Log_3', type: 'cylinder', position: { x: 0, y: 0.1, z: -0.2 }, scale: { x: 0.1, y: 0.7, z: 0.1 }, color: '#8B4513' },
        { name: 'Fire_Flame_Core', type: 'sphere', position: { x: 0, y: 0.6, z: 0 }, scale: { x: 0.3, y: 0.5, z: 0.3 }, color: '#FF4500' },
        { name: 'Fire_Flame_Outer', type: 'sphere', position: { x: 0, y: 0.8, z: 0 }, scale: { x: 0.2, y: 0.4, z: 0.2 }, color: '#FFD700' },
        { name: 'Fire_Light', type: 'light_point', position: { x: 0, y: 1, z: 0 }, scale: { x: 1, y: 1, z: 1 } },
        { name: 'Fire_Stone_1', type: 'sphere', position: { x: -0.4, y: 0.08, z: 0 }, scale: { x: 0.12, y: 0.1, z: 0.12 }, color: '#696969' },
        { name: 'Fire_Stone_2', type: 'sphere', position: { x: 0.4, y: 0.08, z: 0 }, scale: { x: 0.1, y: 0.08, z: 0.1 }, color: '#808080' },
        { name: 'Fire_Stone_3', type: 'sphere', position: { x: 0, y: 0.08, z: 0.35 }, scale: { x: 0.11, y: 0.09, z: 0.11 }, color: '#696969' },
      ],
    };
  }

  // Wizard
  if (p.includes('wizard') || p.includes('penyihir') || p.includes('mage')) {
    return {
      name: 'Wizard', type: 'character', vertices: 150, faces: 75,
      actors: [
        { name: 'Wizard_Body', type: 'cube', position: { x: 0, y: 1.2, z: 0 }, scale: { x: 0.5, y: 1, z: 0.4 }, color: '#4B0082' },
        { name: 'Wizard_Head', type: 'sphere', position: { x: 0, y: 2, z: 0 }, scale: { x: 0.3, y: 0.35, z: 0.3 }, color: '#FFE4C4' },
        { name: 'Wizard_Hat', type: 'cylinder', position: { x: 0, y: 2.6, z: 0 }, scale: { x: 0.35, y: 0.8, z: 0.35 }, color: '#4B0082' },
        { name: 'Wizard_Hat_Brim', type: 'cylinder', position: { x: 0, y: 2.2, z: 0 }, scale: { x: 0.5, y: 0.05, z: 0.5 }, color: '#3A0072' },
        { name: 'Wizard_Staff', type: 'cylinder', position: { x: 0.5, y: 1.5, z: 0 }, scale: { x: 0.04, y: 2.5, z: 0.04 }, color: '#8B4513' },
        { name: 'Wizard_Orb', type: 'sphere', position: { x: 0.5, y: 2.8, z: 0 }, scale: { x: 0.12, y: 0.12, z: 0.12 }, color: '#00BFFF' },
        { name: 'Wizard_Robe', type: 'cube', position: { x: 0, y: 0.4, z: 0 }, scale: { x: 0.7, y: 0.8, z: 0.5 }, color: '#3A0072' },
      ],
    };
  }

  // Chest / Peti
  if (p.includes('peti') || p.includes('chest') || p.includes('treasure')) {
    return {
      name: 'Treasure Chest', type: 'prop', vertices: 48, faces: 24,
      actors: [
        { name: 'Chest_Base', type: 'cube', position: { x: 0, y: 0.25, z: 0 }, scale: { x: 0.8, y: 0.5, z: 0.5 }, color: '#8B4513' },
        { name: 'Chest_Lid', type: 'cube', position: { x: 0, y: 0.55, z: 0 }, scale: { x: 0.82, y: 0.1, z: 0.52 }, color: '#A0522D' },
        { name: 'Chest_Lock', type: 'cube', position: { x: 0, y: 0.35, z: 0.26 }, scale: { x: 0.12, y: 0.15, z: 0.02 }, color: '#FFD700' },
        { name: 'Chest_Hinge_L', type: 'cube', position: { x: -0.3, y: 0.5, z: -0.24 }, scale: { x: 0.05, y: 0.08, z: 0.04 }, color: '#FFD700' },
        { name: 'Chest_Hinge_R', type: 'cube', position: { x: 0.3, y: 0.5, z: -0.24 }, scale: { x: 0.05, y: 0.08, z: 0.04 }, color: '#FFD700' },
      ],
    };
  }

  // Mountain / Gunung
  if (p.includes('gunung') || p.includes('mountain')) {
    return {
      name: 'Mountain', type: 'environment', vertices: 120, faces: 60,
      actors: [
        { name: 'Mountain_Base', type: 'sphere', position: { x: 0, y: 0, z: 0 }, scale: { x: 8, y: 3, z: 8 }, color: '#556B2F' },
        { name: 'Mountain_Peak', type: 'sphere', position: { x: 0, y: 4, z: 0 }, scale: { x: 4, y: 5, z: 4 }, color: '#696969' },
        { name: 'Mountain_Snow', type: 'sphere', position: { x: 0, y: 7, z: 0 }, scale: { x: 2, y: 2, z: 2 }, color: '#FFFAFA' },
        { name: 'Mountain_Rock_1', type: 'sphere', position: { x: 3, y: 1, z: 2 }, scale: { x: 1, y: 0.8, z: 1 }, color: '#808080' },
        { name: 'Mountain_Rock_2', type: 'sphere', position: { x: -2, y: 0.5, z: 3 }, scale: { x: 0.8, y: 0.6, z: 0.8 }, color: '#708090' },
      ],
    };
  }

  // Tank
  if (p.includes('tank')) {
    return {
      name: 'Tank', type: 'vehicle', vertices: 180, faces: 90,
      actors: [
        { name: 'Tank_Hull', type: 'cube', position: { x: 0, y: 0.5, z: 0 }, scale: { x: 2, y: 0.6, z: 3 }, color: '#556B2F' },
        { name: 'Tank_Turret', type: 'cube', position: { x: 0, y: 1.1, z: -0.2 }, scale: { x: 1.2, y: 0.5, z: 1.5 }, color: '#4B5320' },
        { name: 'Tank_Barrel', type: 'cylinder', position: { x: 0, y: 1.2, z: 1.8 }, scale: { x: 0.1, y: 1.5, z: 0.1 }, color: '#333' },
        { name: 'Tank_Track_L', type: 'cube', position: { x: -1.1, y: 0.3, z: 0 }, scale: { x: 0.3, y: 0.5, z: 3.2 }, color: '#333' },
        { name: 'Tank_Track_R', type: 'cube', position: { x: 1.1, y: 0.3, z: 0 }, scale: { x: 0.3, y: 0.5, z: 3.2 }, color: '#333' },
      ],
    };
  }

  // Barrel / Tong
  if (p.includes('tong') || p.includes('barrel')) {
    return {
      name: 'Barrel', type: 'prop', vertices: 32, faces: 16,
      actors: [
        { name: 'Barrel_Body', type: 'cylinder', position: { x: 0, y: 0.5, z: 0 }, scale: { x: 0.4, y: 1, z: 0.4 }, color: '#8B4513' },
        { name: 'Barrel_Ring_Top', type: 'cylinder', position: { x: 0, y: 0.9, z: 0 }, scale: { x: 0.42, y: 0.05, z: 0.42 }, color: '#696969' },
        { name: 'Barrel_Ring_Bottom', type: 'cylinder', position: { x: 0, y: 0.1, z: 0 }, scale: { x: 0.42, y: 0.05, z: 0.42 }, color: '#696969' },
      ],
    };
  }

  // === NEW HIGH-DETAIL MODELS ===

  // Japanese Temple / Kuil Jepang
  if (p.includes('kuil') || p.includes('temple') || p.includes('japanese temple')) {
    return {
      name: 'Japanese Temple', type: 'environment', vertices: 480, faces: 240,
      actors: [
        { name: 'Temple_Base', type: 'cube', position: { x: 0, y: 0.3, z: 0 }, scale: { x: 6, y: 0.6, z: 5 }, color: '#8B7355' },
        { name: 'Temple_Walls', type: 'cube', position: { x: 0, y: 1.3, z: 0 }, scale: { x: 5.5, y: 1.4, z: 4.5 }, color: '#F5F5DC' },
        { name: 'Temple_Roof_Base', type: 'cube', position: { x: 0, y: 2.3, z: 0 }, scale: { x: 6.5, y: 0.3, z: 5.5 }, color: '#2F1B14' },
        { name: 'Temple_Roof_Top', type: 'cone', position: { x: 0, y: 3, z: 0 }, scale: { x: 3.5, y: 1.5, z: 3.5 }, color: '#3C2415' },
        { name: 'Temple_Pillar_FL', type: 'cylinder', position: { x: -2.2, y: 1.3, z: 1.8 }, scale: { x: 0.2, y: 2.6, z: 0.2 }, color: '#C41E3A' },
        { name: 'Temple_Pillar_FR', type: 'cylinder', position: { x: 2.2, y: 1.3, z: 1.8 }, scale: { x: 0.2, y: 2.6, z: 0.2 }, color: '#C41E3A' },
        { name: 'Temple_Pillar_BL', type: 'cylinder', position: { x: -2.2, y: 1.3, z: -1.8 }, scale: { x: 0.2, y: 2.6, z: 0.2 }, color: '#C41E3A' },
        { name: 'Temple_Pillar_BR', type: 'cylinder', position: { x: 2.2, y: 1.3, z: -1.8 }, scale: { x: 0.2, y: 2.6, z: 0.2 }, color: '#C41E3A' },
        { name: 'Temple_Steps', type: 'cube', position: { x: 0, y: 0.1, z: 2.8 }, scale: { x: 2, y: 0.2, z: 0.6 }, color: '#808080' },
        { name: 'Temple_Lantern_L', type: 'cube', position: { x: -3.5, y: 1, z: 2.5 }, scale: { x: 0.4, y: 0.8, z: 0.4 }, color: '#A0A0A0' },
        { name: 'Temple_Lantern_R', type: 'cube', position: { x: 3.5, y: 1, z: 2.5 }, scale: { x: 0.4, y: 0.8, z: 0.4 }, color: '#A0A0A0' },
        { name: 'Temple_Torii', type: 'cube', position: { x: 0, y: 2.5, z: 4.5 }, scale: { x: 4, y: 0.3, z: 0.3 }, color: '#C41E3A' },
        { name: 'Temple_Torii_Pillar_L', type: 'cylinder', position: { x: -1.8, y: 1.5, z: 4.5 }, scale: { x: 0.15, y: 5, z: 0.15 }, color: '#C41E3A' },
        { name: 'Temple_Torii_Pillar_R', type: 'cylinder', position: { x: 1.8, y: 1.5, z: 4.5 }, scale: { x: 0.15, y: 5, z: 0.15 }, color: '#C41E3A' },
      ],
    };
  }

  // Modern Skyscraper
  if (p.includes('gedung') || p.includes('skyscraper') || p.includes('pencakar langit')) {
    return {
      name: 'Modern Skyscraper', type: 'environment', vertices: 380, faces: 190,
      actors: [
        { name: 'Building_Base', type: 'cube', position: { x: 0, y: 3, z: 0 }, scale: { x: 3, y: 6, z: 3 }, color: '#4682B4' },
        { name: 'Building_Glass_L', type: 'cube', position: { x: -0.2, y: 3, z: 1.55 }, scale: { x: 2.5, y: 5.5, z: 0.05 }, color: '#87CEEB' },
        { name: 'Building_Glass_R', type: 'cube', position: { x: 0.2, y: 3, z: -1.55 }, scale: { x: 2.5, y: 5.5, z: 0.05 }, color: '#87CEEB' },
        { name: 'Building_Roof', type: 'cube', position: { x: 0, y: 6.2, z: 0 }, scale: { x: 3.2, y: 0.3, z: 3.2 }, color: '#36648B' },
        { name: 'Building_Antenna', type: 'cylinder', position: { x: 0, y: 7.5, z: 0 }, scale: { x: 0.05, y: 2.5, z: 0.05 }, color: '#888888' },
        { name: 'Building_Light', type: 'cylinder', position: { x: 0, y: 8.8, z: 0 }, scale: { x: 0.08, y: 0.15, z: 0.08 }, color: '#FF0000' },
        { name: 'Building_Lobby', type: 'cube', position: { x: 0, y: 0.5, z: 1.55 }, scale: { x: 1.5, y: 1, z: 0.1 }, color: '#87CEEB' },
        { name: 'Building_Windows_L', type: 'cube', position: { x: -1.56, y: 3, z: 0 }, scale: { x: 0.05, y: 5, z: 2.5 }, color: '#4682B4' },
        { name: 'Building_Windows_R', type: 'cube', position: { x: 1.56, y: 3, z: 0 }, scale: { x: 0.05, y: 5, z: 2.5 }, color: '#4682B4' },
      ],
    };
  }

  // Fantasy Tower / Menara Fantasy
  if (p.includes('menara') || p.includes('tower')) {
    return {
      name: 'Fantasy Tower', type: 'environment', vertices: 320, faces: 160,
      actors: [
        { name: 'Tower_Base', type: 'cylinder', position: { x: 0, y: 1.5, z: 0 }, scale: { x: 1.5, y: 3, z: 1.5 }, color: '#8B8682' },
        { name: 'Tower_Mid', type: 'cylinder', position: { x: 0, y: 4, z: 0 }, scale: { x: 1.2, y: 2, z: 1.2 }, color: '#808080' },
        { name: 'Tower_Top', type: 'cylinder', position: { x: 0, y: 6.5, z: 0 }, scale: { x: 1.4, y: 2, z: 1.4 }, color: '#696969' },
        { name: 'Tower_Cone', type: 'cone', position: { x: 0, y: 8.5, z: 0 }, scale: { x: 1.6, y: 2.5, z: 1.6 }, color: '#8B0000' },
        { name: 'Tower_Flag', type: 'cube', position: { x: 0.3, y: 10.5, z: 0 }, scale: { x: 0.8, y: 0.5, z: 0.05 }, color: '#FFD700' },
        { name: 'Tower_Door', type: 'cube', position: { x: 0, y: 0.8, z: 1.5 }, scale: { x: 0.8, y: 1.4, z: 0.1 }, color: '#5C4033' },
        { name: 'Tower_Window_1', type: 'cube', position: { x: 0, y: 5, z: 1.25 }, scale: { x: 0.4, y: 0.5, z: 0.05 }, color: '#87CEEB' },
        { name: 'Tower_Window_2', type: 'cube', position: { x: 0, y: 6.5, z: 1.45 }, scale: { x: 0.5, y: 0.6, z: 0.05 }, color: '#87CEEB' },
      ],
    };
  }

  // Wooden Cabin / Kabin Kayu
  if (p.includes('kabin') || p.includes('cabin') || p.includes('wooden')) {
    return {
      name: 'Wooden Cabin', type: 'environment', vertices: 280, faces: 140,
      actors: [
        { name: 'Cabin_Floor', type: 'cube', position: { x: 0, y: 0.15, z: 0 }, scale: { x: 3.5, y: 0.3, z: 3 }, color: '#8B7355' },
        { name: 'Cabin_Wall_Back', type: 'cube', position: { x: 0, y: 1.2, z: -1.45 }, scale: { x: 3.5, y: 1.8, z: 0.15 }, color: '#A0522D' },
        { name: 'Cabin_Wall_Left', type: 'cube', position: { x: -1.7, y: 1.2, z: 0 }, scale: { x: 0.15, y: 1.8, z: 3 }, color: '#8B4513' },
        { name: 'Cabin_Wall_Right', type: 'cube', position: { x: 1.7, y: 1.2, z: 0 }, scale: { x: 0.15, y: 1.8, z: 3 }, color: '#8B4513' },
        { name: 'Cabin_Roof', type: 'cone', position: { x: 0, y: 2.8, z: 0 }, scale: { x: 2.5, y: 1.2, z: 2.5 }, color: '#6B4226' },
        { name: 'Cabin_Door', type: 'cube', position: { x: 0, y: 0.9, z: 1.5 }, scale: { x: 0.8, y: 1.5, z: 0.1 }, color: '#5C4033' },
        { name: 'Cabin_Chimney', type: 'cube', position: { x: 1, y: 3, z: -0.5 }, scale: { x: 0.4, y: 1.2, z: 0.4 }, color: '#696969' },
        { name: 'Cabin_Porch', type: 'cube', position: { x: 0, y: 0.05, z: 2 }, scale: { x: 2.5, y: 0.1, z: 1 }, color: '#8B7355' },
        { name: 'Cabin_Window', type: 'cube', position: { x: -0.8, y: 1.4, z: 1.55 }, scale: { x: 0.5, y: 0.5, z: 0.05 }, color: '#FFD700' },
      ],
    };
  }

  // Boat / Perahu
  if (p.includes('perahu') || p.includes('boat')) {
    return {
      name: 'Boat', type: 'vehicle', vertices: 160, faces: 80,
      actors: [
        { name: 'Boat_Hull', type: 'cube', position: { x: 0, y: 0.4, z: 0 }, scale: { x: 1.5, y: 0.4, z: 4 }, color: '#8B4513' },
        { name: 'Boat_Hull_Front', type: 'cone', position: { x: 0, y: 0.4, z: 2.5 }, scale: { x: 0.6, y: 0.3, z: 1 }, color: '#A0522D' },
        { name: 'Boat_Deck', type: 'cube', position: { x: 0, y: 0.65, z: 0 }, scale: { x: 1.2, y: 0.1, z: 3.5 }, color: '#DEB887' },
        { name: 'Boat_Mast', type: 'cylinder', position: { x: 0, y: 2, z: 0 }, scale: { x: 0.06, y: 2.5, z: 0.06 }, color: '#8B4513' },
        { name: 'Boat_Sail', type: 'cube', position: { x: 0.6, y: 2.2, z: 0.3 }, scale: { x: 0.05, y: 1.5, z: 1.8 }, color: '#FFFAF0' },
        { name: 'Boat_Sail2', type: 'cube', position: { x: -0.6, y: 2, z: -0.2 }, scale: { x: 0.05, y: 1.2, z: 1.5 }, color: '#FFFAF0' },
      ],
    };
  }

  // Helicopter / Helikopter
  if (p.includes('helikopter') || p.includes('helicopter')) {
    return {
      name: 'Helicopter', type: 'vehicle', vertices: 220, faces: 110,
      actors: [
        { name: 'Heli_Body', type: 'sphere', position: { x: 0, y: 1.5, z: 0 }, scale: { x: 1, y: 0.8, z: 2 }, color: '#556B2F' },
        { name: 'Heli_Cockpit', type: 'sphere', position: { x: 0, y: 1.4, z: 1.5 }, scale: { x: 0.7, y: 0.6, z: 0.8 }, color: '#87CEEB' },
        { name: 'Heli_Tail', type: 'cube', position: { x: 0, y: 1.8, z: -2 }, scale: { x: 0.2, y: 0.2, z: 2 }, color: '#556B2F' },
        { name: 'Heli_Tail_Rotor', type: 'cube', position: { x: 0, y: 1.8, z: -3 }, scale: { x: 1, y: 0.05, z: 0.1 }, color: '#444' },
        { name: 'Heli_Main_Rotor', type: 'cube', position: { x: 0, y: 2.5, z: 0 }, scale: { x: 5, y: 0.05, z: 0.3 }, color: '#444' },
        { name: 'Heli_Rotor_Mast', type: 'cylinder', position: { x: 0, y: 2.1, z: 0 }, scale: { x: 0.08, y: 0.8, z: 0.08 }, color: '#333' },
        { name: 'Heli_Skid_L', type: 'cube', position: { x: -0.6, y: 0.7, z: 0 }, scale: { x: 0.08, y: 0.15, z: 2.5 }, color: '#333' },
        { name: 'Heli_Skid_R', type: 'cube', position: { x: 0.6, y: 0.7, z: 0 }, scale: { x: 0.08, y: 0.15, z: 2.5 }, color: '#333' },
      ],
    };
  }

  // Horse / Kuda
  if (p.includes('kuda') || p.includes('horse')) {
    return {
      name: 'Horse', type: 'vehicle', vertices: 180, faces: 90,
      actors: [
        { name: 'Horse_Body', type: 'cube', position: { x: 0, y: 1.3, z: 0 }, scale: { x: 0.8, y: 0.8, z: 2 }, color: '#8B4513' },
        { name: 'Horse_Head', type: 'cube', position: { x: 0, y: 2, z: 1.3 }, scale: { x: 0.4, y: 0.6, z: 0.6 }, color: '#A0522D' },
        { name: 'Horse_Neck', type: 'cube', position: { x: 0, y: 1.9, z: 0.9 }, scale: { x: 0.35, y: 0.7, z: 0.4 }, color: '#8B4513' },
        { name: 'Horse_Leg_FL', type: 'cylinder', position: { x: -0.25, y: 0.5, z: 0.6 }, scale: { x: 0.1, y: 1, z: 0.1 }, color: '#7B3F00' },
        { name: 'Horse_Leg_FR', type: 'cylinder', position: { x: 0.25, y: 0.5, z: 0.6 }, scale: { x: 0.1, y: 1, z: 0.1 }, color: '#7B3F00' },
        { name: 'Horse_Leg_BL', type: 'cylinder', position: { x: -0.25, y: 0.5, z: -0.6 }, scale: { x: 0.1, y: 1, z: 0.1 }, color: '#7B3F00' },
        { name: 'Horse_Leg_BR', type: 'cylinder', position: { x: 0.25, y: 0.5, z: -0.6 }, scale: { x: 0.1, y: 1, z: 0.1 }, color: '#7B3F00' },
        { name: 'Horse_Tail', type: 'cube', position: { x: 0, y: 1.5, z: -1.3 }, scale: { x: 0.1, y: 0.8, z: 0.5 }, color: '#1a1a1a' },
        { name: 'Horse_Mane', type: 'cube', position: { x: 0, y: 2.3, z: 0.5 }, scale: { x: 0.15, y: 0.5, z: 0.6 }, color: '#1a1a1a' },
      ],
    };
  }

  // Zombie
  if (p.includes('zombie')) {
    return {
      name: 'Zombie', type: 'character', vertices: 160, faces: 80,
      actors: [
        { name: 'Zombie_Body', type: 'cube', position: { x: 0, y: 1.2, z: 0 }, scale: { x: 0.6, y: 0.9, z: 0.35 }, color: '#4A5D23' },
        { name: 'Zombie_Head', type: 'sphere', position: { x: 0.1, y: 2, z: 0 }, scale: { x: 0.3, y: 0.35, z: 0.3 }, color: '#7B8B3A' },
        { name: 'Zombie_Eye_L', type: 'sphere', position: { x: -0.05, y: 2.05, z: 0.25 }, scale: { x: 0.06, y: 0.06, z: 0.06 }, color: '#FF0000' },
        { name: 'Zombie_Eye_R', type: 'sphere', position: { x: 0.15, y: 2.05, z: 0.25 }, scale: { x: 0.06, y: 0.06, z: 0.06 }, color: '#FF0000' },
        { name: 'Zombie_Arm_L', type: 'cube', position: { x: -0.55, y: 1.6, z: 0.2 }, scale: { x: 0.15, y: 0.7, z: 0.15 }, color: '#4A5D23' },
        { name: 'Zombie_Arm_R', type: 'cube', position: { x: 0.55, y: 1.6, z: 0.2 }, scale: { x: 0.15, y: 0.7, z: 0.15 }, color: '#4A5D23' },
        { name: 'Zombie_Legs', type: 'cube', position: { x: 0, y: 0.35, z: 0 }, scale: { x: 0.5, y: 0.7, z: 0.3 }, color: '#3A4D13' },
        { name: 'Zombie_Wound', type: 'sphere', position: { x: -0.2, y: 1.1, z: 0.18 }, scale: { x: 0.08, y: 0.08, z: 0.05 }, color: '#8B0000' },
      ],
    };
  }

  // Villager NPC
  if (p.includes('npc') || p.includes('villager') || p.includes('penduduk')) {
    return {
      name: 'Villager NPC', type: 'character', vertices: 140, faces: 70,
      actors: [
        { name: 'NPC_Body', type: 'cube', position: { x: 0, y: 1.1, z: 0 }, scale: { x: 0.5, y: 0.8, z: 0.35 }, color: '#4169E1' },
        { name: 'NPC_Head', type: 'sphere', position: { x: 0, y: 1.8, z: 0 }, scale: { x: 0.25, y: 0.3, z: 0.25 }, color: '#FFE4C4' },
        { name: 'NPC_Hat', type: 'cube', position: { x: 0, y: 2.1, z: 0 }, scale: { x: 0.35, y: 0.15, z: 0.35 }, color: '#8B4513' },
        { name: 'NPC_Arm_L', type: 'cube', position: { x: -0.4, y: 1.1, z: 0 }, scale: { x: 0.15, y: 0.6, z: 0.15 }, color: '#4169E1' },
        { name: 'NPC_Arm_R', type: 'cube', position: { x: 0.4, y: 1.1, z: 0 }, scale: { x: 0.15, y: 0.6, z: 0.15 }, color: '#4169E1' },
        { name: 'NPC_Legs', type: 'cube', position: { x: 0, y: 0.35, z: 0 }, scale: { x: 0.4, y: 0.7, z: 0.3 }, color: '#654321' },
        { name: 'NPC_Exclamation', type: 'cube', position: { x: 0, y: 2.5, z: 0 }, scale: { x: 0.15, y: 0.2, z: 0.05 }, color: '#FFD700' },
      ],
    };
  }

  // Shield / Perisai
  if (p.includes('perisai') || p.includes('shield')) {
    return {
      name: 'Shield', type: 'prop', vertices: 48, faces: 24,
      actors: [
        { name: 'Shield_Body', type: 'cube', position: { x: 0, y: 1, z: 0 }, scale: { x: 0.7, y: 1.2, z: 0.08 }, color: '#8B0000' },
        { name: 'Shield_Border', type: 'cube', position: { x: 0, y: 1, z: 0 }, scale: { x: 0.75, y: 1.25, z: 0.05 }, color: '#FFD700' },
        { name: 'Shield_Emblem', type: 'sphere', position: { x: 0, y: 1, z: 0.05 }, scale: { x: 0.2, y: 0.2, z: 0.1 }, color: '#FFD700' },
      ],
    };
  }

  // Torch / Obor
  if (p.includes('obor') || p.includes('torch')) {
    return {
      name: 'Torch', type: 'prop', vertices: 40, faces: 20,
      actors: [
        { name: 'Torch_Handle', type: 'cylinder', position: { x: 0, y: 1, z: 0 }, scale: { x: 0.06, y: 1.5, z: 0.06 }, color: '#8B4513' },
        { name: 'Torch_Wrap', type: 'cylinder', position: { x: 0, y: 1.7, z: 0 }, scale: { x: 0.1, y: 0.3, z: 0.1 }, color: '#696969' },
        { name: 'Torch_Flame', type: 'sphere', position: { x: 0, y: 1.95, z: 0 }, scale: { x: 0.15, y: 0.3, z: 0.15 }, color: '#FF4500' },
        { name: 'Torch_Light', type: 'light_point', position: { x: 0, y: 2.2, z: 0 }, scale: { x: 1, y: 1, z: 1 } },
      ],
    };
  }

  // Rock Formation / Formasi Batu
  if (p.includes('batu') || p.includes('rock')) {
    return {
      name: 'Rock Formation', type: 'environment', vertices: 120, faces: 60,
      actors: [
        { name: 'Rock_Main', type: 'sphere', position: { x: 0, y: 0.8, z: 0 }, scale: { x: 1.2, y: 0.8, z: 1 }, color: '#808080' },
        { name: 'Rock_Side_L', type: 'sphere', position: { x: -0.8, y: 0.5, z: 0.3 }, scale: { x: 0.7, y: 0.5, z: 0.6 }, color: '#707070' },
        { name: 'Rock_Side_R', type: 'sphere', position: { x: 0.9, y: 0.4, z: -0.2 }, scale: { x: 0.6, y: 0.4, z: 0.5 }, color: '#909090' },
        { name: 'Rock_Top', type: 'sphere', position: { x: 0.2, y: 1.3, z: -0.1 }, scale: { x: 0.5, y: 0.4, z: 0.4 }, color: '#708090' },
      ],
    };
  }

  // Waterfall / Air Terjun
  if (p.includes('air terjun') || p.includes('waterfall')) {
    return {
      name: 'Waterfall', type: 'environment', vertices: 200, faces: 100,
      actors: [
        { name: 'Waterfall_Cliff', type: 'cube', position: { x: -1.5, y: 2.5, z: 0 }, scale: { x: 2, y: 5, z: 3 }, color: '#696969' },
        { name: 'Waterfall_Cliff2', type: 'cube', position: { x: -2, y: 1.5, z: -1 }, scale: { x: 1.5, y: 3, z: 2 }, color: '#808080' },
        { name: 'Waterfall_Pool', type: 'cube', position: { x: 0.5, y: 0.1, z: 0 }, scale: { x: 3, y: 0.2, z: 3 }, color: '#1a6eb5' },
        { name: 'Waterfall_Stream', type: 'cube', position: { x: 0.5, y: 2, z: 0 }, scale: { x: 1.5, y: 3, z: 0.3 }, color: '#4DA6FF' },
        { name: 'Waterfall_Mist', type: 'sphere', position: { x: 0.5, y: 0.5, z: 0 }, scale: { x: 2, y: 0.5, z: 2 }, color: '#B0E0E6' },
      ],
    };
  }

  // River with Bridge / Sungai dengan Jembatan
  if (p.includes('sungai') || p.includes('river') || p.includes('jembatan') || p.includes('bridge')) {
    return {
      name: 'River with Bridge', type: 'environment', vertices: 240, faces: 120,
      actors: [
        { name: 'River_Water', type: 'water', position: { x: 0, y: -0.1, z: 0 }, scale: { x: 20, y: 1, z: 5 } },
        { name: 'Bridge_Deck', type: 'cube', position: { x: 0, y: 0.3, z: 0 }, scale: { x: 1.5, y: 0.15, z: 5 }, color: '#8B7355' },
        { name: 'Bridge_Rail_L', type: 'cube', position: { x: -0.6, y: 0.6, z: 0 }, scale: { x: 0.08, y: 0.5, z: 5 }, color: '#8B4513' },
        { name: 'Bridge_Rail_R', type: 'cube', position: { x: 0.6, y: 0.6, z: 0 }, scale: { x: 0.08, y: 0.5, z: 5 }, color: '#8B4513' },
        { name: 'Bridge_Post_FL', type: 'cylinder', position: { x: -0.6, y: 0.4, z: 2.3 }, scale: { x: 0.1, y: 0.8, z: 0.1 }, color: '#8B4513' },
        { name: 'Bridge_Post_FR', type: 'cylinder', position: { x: 0.6, y: 0.4, z: 2.3 }, scale: { x: 0.1, y: 0.8, z: 0.1 }, color: '#8B4513' },
        { name: 'Bridge_Post_BL', type: 'cylinder', position: { x: -0.6, y: 0.4, z: -2.3 }, scale: { x: 0.1, y: 0.8, z: 0.1 }, color: '#8B4513' },
        { name: 'Bridge_Post_BR', type: 'cylinder', position: { x: 0.6, y: 0.4, z: -2.3 }, scale: { x: 0.1, y: 0.8, z: 0.1 }, color: '#8B4513' },
      ],
    };
  }

  // === ENVIRONMENT GENERATORS ===

  // Forest / Hutan
  if (p.includes('hutan') || p.includes('forest')) {
    const trees: GeneratedModel['actors'] = [];
    const treeTypes = ['oak', 'pine'];
    for (let i = 0; i < 12; i++) {
      const x = (Math.random() - 0.5) * 20;
      const z = (Math.random() - 0.5) * 20;
      const isPine = i % 2 === 0;
      trees.push({ name: `Forest_Tree_${i}`, type: 'cylinder', position: { x, y: 1.5, z }, scale: { x: 0.3, y: 3, z: 0.3 }, color: '#8B4513' });
      if (isPine) {
        trees.push({ name: `Forest_Leaves_${i}_1`, type: 'cone', position: { x, y: 4, z }, scale: { x: 1.2, y: 1.5, z: 1.2 }, color: '#006400' });
        trees.push({ name: `Forest_Leaves_${i}_2`, type: 'cone', position: { x, y: 5, z }, scale: { x: 0.9, y: 1.2, z: 0.9 }, color: '#228B22' });
      } else {
        trees.push({ name: `Forest_Canopy_${i}`, type: 'sphere', position: { x, y: 4.2, z }, scale: { x: 2, y: 1.8, z: 2 }, color: '#228B22' });
      }
    }
    // Add foliage
    trees.push({ name: 'Forest_Foliage', type: 'foliage', position: { x: 0, y: 0, z: 0 }, scale: { x: 20, y: 1, z: 20 } });
    return { name: 'Forest', type: 'environment', vertices: 960, faces: 480, actors: trees };
  }

  // Generic fallback - generate a basic cube model
  const name = prompt.replace(/^(buat|create|generate|tambah|add)\s*(model\s*(3d)?|3d)?\s*/i, '').trim() || 'Object';
  return {
    name: name.charAt(0).toUpperCase() + name.slice(1),
    type: 'prop',
    vertices: 24,
    faces: 12,
    actors: [
      { name: `${name}_main`, type: 'cube', position: { x: 0, y: 0.5, z: 0 }, scale: { x: 1, y: 1, z: 1 }, color: '#4488cc' },
    ],
  };
}

export function ModelGeneratorDialog({ isOpen, onClose, onGenerate, onLog }: ModelGeneratorProps) {
  const [promptText, setPromptText] = useState('');
  const [selectedCategory, setSelectedCategory] = useState<string | null>(null);
  const [generating, setGenerating] = useState(false);
  const [preview, setPreview] = useState<GeneratedModel | null>(null);

  if (!isOpen) return null;

  const handleGenerate = async (prompt: string) => {
    setGenerating(true);
    onLog(`[3D Gen] Generating model from prompt: "${prompt}"...`);

    await new Promise(r => setTimeout(r, 800));
    onLog(`[3D Gen] Analyzing prompt...`);
    await new Promise(r => setTimeout(r, 600));
    onLog(`[3D Gen] Generating geometry...`);
    await new Promise(r => setTimeout(r, 500));

    const model = generateModelFromPrompt(prompt);
    if (model) {
      setPreview(model);
      onLog(`[3D Gen] Generated "${model.name}" - ${model.vertices} vertices, ${model.faces} faces, ${model.actors.length} parts`);
    }
    setGenerating(false);
  };

  const handleAddToScene = () => {
    if (preview) {
      onGenerate(preview);
      onLog(`[3D Gen] Added "${preview.name}" to scene (${preview.actors.length} actors)`);
      setPreview(null);
      onClose();
    }
  };

  const inputStyle: React.CSSProperties = {
    width: '100%', padding: '8px 12px', background: '#1a1a1a', border: '1px solid #333',
    borderRadius: 4, color: '#e0e0e0', fontSize: 13, boxSizing: 'border-box',
  };

  return (
    <div style={{
      position: 'fixed', top: 0, left: 0, right: 0, bottom: 0,
      background: 'rgba(0,0,0,0.7)', zIndex: 10000,
      display: 'flex', alignItems: 'center', justifyContent: 'center',
    }} onClick={onClose}>
      <div style={{
        background: '#222', border: '1px solid #444', borderRadius: 8,
        width: 580, maxHeight: '85vh', overflow: 'auto', padding: 0,
      }} onClick={e => e.stopPropagation()}>
        {/* Header */}
        <div style={{
          display: 'flex', justifyContent: 'space-between', alignItems: 'center',
          padding: '12px 16px', borderBottom: '1px solid #333', background: '#2a2a2a',
        }}>
          <span style={{ color: '#e0e0e0', fontWeight: 700, fontSize: 14 }}>
            3D Model Generator
          </span>
          <button onClick={onClose} style={{
            background: 'none', border: 'none', color: '#888', fontSize: 18, cursor: 'pointer',
          }}>×</button>
        </div>

        <div style={{ padding: 16 }}>
          {/* Prompt input */}
          <div style={{ marginBottom: 12 }}>
            <label style={{ display: 'block', fontSize: 11, color: '#a0a0a0', marginBottom: 6 }}>
              Describe the 3D model you want to generate:
            </label>
            <div style={{ display: 'flex', gap: 8 }}>
              <input style={inputStyle} value={promptText} onChange={e => setPromptText(e.target.value)}
                placeholder="e.g., buat rumah, create dragon, buat mobil sport..."
                onKeyDown={e => e.key === 'Enter' && promptText.trim() && handleGenerate(promptText)} />
              <button onClick={() => promptText.trim() && handleGenerate(promptText)} disabled={generating || !promptText.trim()}
                style={{
                  padding: '0 20px', background: generating ? '#555' : '#0078d4', color: '#fff',
                  border: 'none', borderRadius: 4, cursor: generating ? 'wait' : 'pointer', fontSize: 12, fontWeight: 600,
                  whiteSpace: 'nowrap',
                }}>{generating ? 'Generating...' : 'Generate'}</button>
            </div>
          </div>

          {/* Preset categories */}
          <div style={{ marginBottom: 12 }}>
            <label style={{ display: 'block', fontSize: 11, color: '#a0a0a0', marginBottom: 6 }}>
              Or choose from presets:
            </label>
            <div style={{ display: 'flex', gap: 4, flexWrap: 'wrap', marginBottom: 8 }}>
              {Object.entries(MODEL_PRESETS).map(([key, cat]) => (
                <button key={key} onClick={() => setSelectedCategory(selectedCategory === key ? null : key)}
                  style={{
                    padding: '4px 10px', fontSize: 11, border: '1px solid #444',
                    background: selectedCategory === key ? '#0078d4' : '#333',
                    color: selectedCategory === key ? '#fff' : '#ccc', borderRadius: 3, cursor: 'pointer',
                  }}>{cat.category}</button>
              ))}
            </div>

            {selectedCategory && (
              <div style={{ display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: 6 }}>
                {MODEL_PRESETS[selectedCategory].models.map(model => (
                  <button key={model.name} onClick={() => handleGenerate(model.prompt)}
                    disabled={generating} style={{
                      padding: '8px', background: '#2a2a2a', border: '1px solid #444',
                      borderRadius: 4, cursor: 'pointer', color: '#ccc', fontSize: 11, textAlign: 'center',
                    }}>
                    {model.name}
                  </button>
                ))}
              </div>
            )}
          </div>

          {/* Preview */}
          {preview && (
            <div style={{
              marginTop: 12, padding: 12, background: '#1a1a1a', border: '1px solid #333', borderRadius: 6,
            }}>
              <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 8 }}>
                <span style={{ color: '#e0e0e0', fontWeight: 600, fontSize: 13 }}>{preview.name}</span>
                <span style={{ color: '#888', fontSize: 11 }}>{preview.type} | {preview.vertices} verts | {preview.faces} faces</span>
              </div>
              <div style={{ fontSize: 11, color: '#888', marginBottom: 8 }}>
                Parts: {preview.actors.map(a => a.name).join(', ')}
              </div>
              <div style={{ display: 'flex', gap: 8 }}>
                <button onClick={handleAddToScene} style={{
                  flex: 1, padding: '8px 0', background: '#4ec949', color: '#fff',
                  border: 'none', borderRadius: 4, cursor: 'pointer', fontSize: 12, fontWeight: 600,
                }}>Add to Scene</button>
                <button onClick={() => setPreview(null)} style={{
                  padding: '8px 16px', background: '#555', color: '#ccc',
                  border: 'none', borderRadius: 4, cursor: 'pointer', fontSize: 12,
                }}>Discard</button>
              </div>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
