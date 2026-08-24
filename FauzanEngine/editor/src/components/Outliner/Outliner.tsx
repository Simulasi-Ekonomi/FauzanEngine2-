import React, { useState } from 'react';
import { useEditorStore } from '../../stores/editorStore';
import type { NeoActor, ActorType } from '../../types/editor';

const actorIcons: Record<string, string> = {
  empty: '○',
  cube: '⬜',
  sphere: '⚪',
  plane: '▬',
  cylinder: '⬛',
  light_directional: '☀',
  light_point: '💡',
  light_spot: '🔦',
  camera: '📷',
  static_mesh: '🔷',
  skeletal_mesh: '🦴',
  particle_system: '✨',
  audio_source: '🔊',
  trigger_volume: '🟩',
  player_start: '🚩',
  landscape: '🏔',
};

function ActorTreeItem({ actor, depth = 0 }: { actor: NeoActor; depth?: number }) {
  const selectedActorId = useEditorStore((s) => s.selectedActorId);
  const selectActor = useEditorStore((s) => s.selectActor);
  const actors = useEditorStore((s) => s.actors);
  const [expanded, setExpanded] = useState(true);
  
  const isSelected = selectedActorId === actor.id;
  const hasChildren = actor.children.length > 0;
  const icon = actorIcons[actor.type] || '○';

  return (
    <>
      <div
        className={`tree-item ${isSelected ? 'selected' : ''}`}
        style={{ paddingLeft: `${8 + depth * 16}px` } as React.CSSProperties}
        onClick={() => selectActor(actor.id)}
        onDoubleClick={() => {/* TODO: rename */}}
      >
        <span className="tree-expand" onClick={(e) => { e.stopPropagation(); setExpanded(!expanded); }}>
          {hasChildren ? (expanded ? '▼' : '▶') : ''}
        </span>
        <span className="tree-icon">{icon}</span>
        <span style={{ flex: 1, overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>
          {actor.name}
        </span>
        <span style={{ fontSize: 9, color: '#555', marginLeft: 4 }}>
          {actor.visible ? '👁' : ''}
        </span>
      </div>
      {expanded && actor.children.map((childId) => {
        const child = actors[childId];
        if (!child) return null;
        return <ActorTreeItem key={childId} actor={child} depth={depth + 1} />;
      })}
    </>
  );
}

export function Outliner() {
  const actors = useEditorStore((s) => s.actors);
  const selectActor = useEditorStore((s) => s.selectActor);
  const addActor = useEditorStore((s) => s.addActor);
  const removeActor = useEditorStore((s) => s.removeActor);
  const selectedActorId = useEditorStore((s) => s.selectedActorId);
  const [filter, setFilter] = useState('');

  const rootActors = Object.values(actors).filter((a) => a.parentId === null);
  const filteredActors = filter
    ? rootActors.filter((a) => a.name.toLowerCase().includes(filter.toLowerCase()))
    : rootActors;

  return (
    <div className="neo-panel" style={{ height: '100%' }}>
      <div className="neo-panel-header">
        <span className="panel-icon">🌐</span>
        World Outliner
      </div>
      <div style={{ padding: '4px 6px', borderBottom: '1px solid #1a1a1a' }}>
        <input
          type="text"
          placeholder="Search actors..."
          value={filter}
          onChange={(e) => setFilter(e.target.value)}
          style={{ width: '100%', height: 22, fontSize: 11 }}
        />
      </div>
      <div className="neo-panel-content">
        {filteredActors.map((actor) => (
          <ActorTreeItem key={actor.id} actor={actor} />
        ))}
      </div>
      <div style={{
        borderTop: '1px solid #1a1a1a',
        padding: '4px 6px',
        display: 'flex',
        gap: 4,
      }}>
        <button
          onClick={() => addActor('cube', 'NewActor')}
          style={{ flex: 1, fontSize: 10 }}
          title="Add Actor"
        >+ Add</button>
        <button
          onClick={() => selectedActorId && removeActor(selectedActorId)}
          style={{ flex: 1, fontSize: 10, background: selectedActorId ? '#6a2020' : '#333' }}
          disabled={!selectedActorId}
          title="Delete Selected"
        >✕ Delete</button>
      </div>
    </div>
  );
}
