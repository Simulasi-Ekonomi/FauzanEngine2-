import React, { useState, useMemo } from 'react';

interface WorldPreset {
  name: string;
  treeDensity: number;
  rockDensity: number;
  resourceDensity: number;
  monsterDensity: number;
  buildingDensity: number;
  cityCount: number;
}

const PRESETS: Record<string, WorldPreset> = {
  realistic: {
    name: '🌍 Realistic',
    treeDensity: 200,
    rockDensity: 50,
    resourceDensity: 10,
    monsterDensity: 2,
    buildingDensity: 0.05,
    cityCount: 8,
  },
  dense: {
    name: '🌳 Dense Forest',
    treeDensity: 800,
    rockDensity: 100,
    resourceDensity: 20,
    monsterDensity: 5,
    buildingDensity: 0.1,
    cityCount: 15,
  },
  sparse: {
    name: '🏜️ Sparse',
    treeDensity: 50,
    rockDensity: 20,
    resourceDensity: 5,
    monsterDensity: 0.5,
    buildingDensity: 0.01,
    cityCount: 3,
  },
  perfectWorld: {
    name: '⚔️ Perfect World',
    treeDensity: 350,
    rockDensity: 80,
    resourceDensity: 15,
    monsterDensity: 3,
    buildingDensity: 0.08,
    cityCount: 12,
  },
};

interface WorldGeneratorPanelProps {
  isOpen: boolean;
  onClose: () => void;
  onGenerate: (config: WorldPreset & { sizeKm: number; seed: number }) => void;
}

export function WorldGeneratorPanel({ isOpen, onClose, onGenerate }: WorldGeneratorPanelProps) {
  const [sizeKm, setSizeKm] = useState(100);
  const [seed, setSeed] = useState(() => Math.floor(Math.random() * 1000000));
  const [treeDensity, setTreeDensity] = useState(PRESETS.perfectWorld.treeDensity);
  const [rockDensity, setRockDensity] = useState(PRESETS.perfectWorld.rockDensity);
  const [resourceDensity, setResourceDensity] = useState(PRESETS.perfectWorld.resourceDensity);
  const [monsterDensity, setMonsterDensity] = useState(PRESETS.perfectWorld.monsterDensity);
  const [buildingDensity, setBuildingDensity] = useState(PRESETS.perfectWorld.buildingDensity);
  const [cityCount, setCityCount] = useState(PRESETS.perfectWorld.cityCount);

  const totalArea = sizeKm * sizeKm;

  const totals = useMemo(() => ({
    trees: Math.round(treeDensity * totalArea),
    rocks: Math.round(rockDensity * totalArea),
    resources: Math.round(resourceDensity * totalArea),
    monsters: Math.round(monsterDensity * totalArea),
    buildings: Math.round(buildingDensity * totalArea),
  }), [sizeKm, treeDensity, rockDensity, resourceDensity, monsterDensity, buildingDensity]);

  const applyPreset = (preset: WorldPreset) => {
    setTreeDensity(preset.treeDensity);
    setRockDensity(preset.rockDensity);
    setResourceDensity(preset.resourceDensity);
    setMonsterDensity(preset.monsterDensity);
    setBuildingDensity(preset.buildingDensity);
    setCityCount(preset.cityCount);
  };

  if (!isOpen) return null;

  return (
    <div style={{
      position: 'fixed', inset: 0, zIndex: 10000,
      background: 'rgba(0,0,0,0.7)', display: 'flex',
      alignItems: 'center', justifyContent: 'center',
    }} onClick={onClose}>
      <div style={{
        background: '#2a2a2a', borderRadius: 8, width: 650, maxHeight: '90vh',
        border: '1px solid #444', overflow: 'auto', padding: 0,
      }} onClick={e => e.stopPropagation()}>
        <div style={{
          padding: '12px 16px', borderBottom: '1px solid #444', background: '#1e3a1e',
          display: 'flex', justifyContent: 'space-between', alignItems: 'center',
        }}>
          <span style={{ color: '#4ec949', fontWeight: 700, fontSize: 14 }}>🌍 World Generator</span>
          <button onClick={onClose} style={{
            background: 'none', border: 'none', color: '#888', fontSize: 18, cursor: 'pointer',
          }}>×</button>
        </div>

        <div style={{ padding: 16 }}>
          {/* Presets */}
          <div style={{ marginBottom: 16 }}>
            <label style={{ color: '#aaa', fontSize: 11, marginBottom: 6, display: 'block' }}>Presets</label>
            <div style={{ display: 'flex', gap: 6, flexWrap: 'wrap' }}>
              {Object.entries(PRESETS).map(([key, preset]) => (
                <button key={key} onClick={() => applyPreset(preset)} style={{
                  padding: '6px 12px', background: '#333', border: '1px solid #555',
                  borderRadius: 4, color: '#ccc', fontSize: 11, cursor: 'pointer',
                }}>{preset.name}</button>
              ))}
            </div>
          </div>

          {/* Size & Seed */}
          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 12, marginBottom: 16 }}>
            <div>
              <label style={{ color: '#aaa', fontSize: 11 }}>World Size (km)</label>
              <input type="number" value={sizeKm} onChange={e => setSizeKm(Number(e.target.value))}
                min={10} max={500} step={10}
                style={{ width: '100%', padding: 6, background: '#1a1a1a', border: '1px solid #444', borderRadius: 4, color: '#fff' }} />
            </div>
            <div>
              <label style={{ color: '#aaa', fontSize: 11 }}>Seed</label>
              <div style={{ display: 'flex', gap: 4 }}>
                <input type="number" value={seed} onChange={e => setSeed(Number(e.target.value))}
                  style={{ flex: 1, padding: 6, background: '#1a1a1a', border: '1px solid #444', borderRadius: 4, color: '#fff' }} />
                <button onClick={() => setSeed(Math.floor(Math.random() * 1000000))} style={{
                  padding: '0 10px', background: '#444', border: 'none', borderRadius: 4, color: '#fff', cursor: 'pointer',
                }}>🎲</button>
              </div>
            </div>
          </div>

          {/* Density Sliders */}
          <SliderRow label="🌲 Tree Density (per km²)" value={treeDensity} onChange={setTreeDensity} min={0} max={2000} step={10} />
          <SliderRow label="🪨 Rock Density (per km²)" value={rockDensity} onChange={setRockDensity} min={0} max={500} step={5} />
          <SliderRow label="💎 Resource Nodes (per km²)" value={resourceDensity} onChange={setResourceDensity} min={0} max={50} step={1} />
          <SliderRow label="👹 Monster Spawns (per km²)" value={monsterDensity} onChange={setMonsterDensity} min={0} max={20} step={0.5} />
          <SliderRow label="🏠 Buildings (per km²)" value={buildingDensity} onChange={setBuildingDensity} min={0} max={2} step={0.01} />
          <SliderRow label="🏙️ Major Cities" value={cityCount} onChange={setCityCount} min={0} max={50} step={1} />

          {/* Totals Preview */}
          <div style={{
            marginTop: 16, padding: 12, background: '#1a1a1a', borderRadius: 6,
            border: '1px solid #333', display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: 8,
          }}>
            <Stat label="Total Trees" value={totals.trees.toLocaleString()} />
            <Stat label="Total Rocks" value={totals.rocks.toLocaleString()} />
            <Stat label="Total Resources" value={totals.resources.toLocaleString()} />
            <Stat label="Total Monsters" value={totals.monsters.toLocaleString()} />
            <Stat label="Total Buildings" value={totals.buildings.toLocaleString()} />
            <Stat label="Major Cities" value={cityCount} />
          </div>

          {/* Generate Button */}
          <button onClick={() => onGenerate({
            name: 'Custom',
            treeDensity, rockDensity, resourceDensity, monsterDensity, buildingDensity, cityCount,
            sizeKm, seed
          })} style={{
            width: '100%', marginTop: 16, padding: '12px 0', background: '#2a5a2a', border: '1px solid #4ec949',
            borderRadius: 4, color: '#4ec949', fontSize: 14, fontWeight: 700, cursor: 'pointer',
          }}>Generate World</button>
        </div>
      </div>
    </div>
  );
}

function SliderRow({ label, value, onChange, min, max, step }: {
  label: string; value: number; onChange: (v: number) => void;
  min: number; max: number; step: number;
}) {
  return (
    <div style={{ marginBottom: 12 }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: 4 }}>
        <span style={{ color: '#ccc', fontSize: 11 }}>{label}</span>
        <span style={{ color: '#4ec949', fontSize: 11, fontWeight: 600 }}>{value.toFixed(2)}</span>
      </div>
      <input type="range" min={min} max={max} step={step} value={value}
        onChange={e => onChange(parseFloat(e.target.value))}
        style={{ width: '100%', accentColor: '#4ec949' }} />
    </div>
  );
}

function Stat({ label, value }: { label: string; value: string }) {
  return (
    <div style={{ textAlign: 'center' }}>
      <div style={{ color: '#666', fontSize: 9 }}>{label}</div>
      <div style={{ color: '#4ec949', fontSize: 14, fontWeight: 700 }}>{value}</div>
    </div>
  );
}
