import React from 'react';
import { useEditorStore } from '../../stores/editorStore';

export function StatusBar() {
  const { engineConnected, ariesConnected, fps, actors, selectedActorId, transformMode } = useEditorStore();
  const actorCount = Object.keys(actors).length;
  const selectedActor = selectedActorId ? actors[selectedActorId] : null;

  return (
    <div className="neo-statusbar">
      <div className="status-left">
        <div className="status-indicator">
          <div className={`status-dot ${ariesConnected ? 'connected' : 'disconnected'}`} />
          <span>Aries AI</span>
        </div>
        <div className="status-indicator">
          <div className={`status-dot ${engineConnected ? 'connected' : 'warning'}`} />
          <span>Engine Runtime</span>
        </div>
        <span>|</span>
        <span>FPS: {fps}</span>
        <span>Actors: {actorCount}</span>
        {selectedActor && (
          <>
            <span>|</span>
            <span style={{ color: '#aaa' }}>Selected: {selectedActor.name}</span>
          </>
        )}
      </div>
      <div className="status-right">
        <span>Mode: {transformMode}</span>
        <span>|</span>
        <span>NeoEngine Editor v1.0</span>
      </div>
    </div>
  );
}
