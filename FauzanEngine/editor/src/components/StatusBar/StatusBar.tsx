import React from 'react';
import { useEditorStore } from '../../stores/editorStore';

export function StatusBar() {
  const { engineConnected, ariesConnected, fps, actors, selectedActorId, transformMode, sceneName, sceneRevision, bridgeStatus, bridgeChecksum, isDirty, lastSavedAt } = useEditorStore();
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
        <span>|</span>
        <span className={isDirty ? 'status-dirty' : 'status-clean'}>{isDirty ? '● Unsaved' : '✓ Saved'}</span>
        <span className={`bridge-status bridge-${bridgeStatus}`} title={bridgeChecksum ? `Scene bridge checksum ${bridgeChecksum}` : 'No bridge receipt'}>Bridge: {bridgeStatus} r{sceneRevision}</span>
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
        <span>{sceneName}{lastSavedAt ? ` · saved ${new Date(lastSavedAt).toLocaleTimeString()}` : ''}</span>
        <span>|</span>
        <span>W/E/R · Ctrl+S · Ctrl+Z/Y</span>
      </div>
    </div>
  );
}
