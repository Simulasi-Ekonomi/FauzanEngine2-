import React from 'react';
import { useEditorStore } from '../../stores/editorStore';

export function Toolbar() {
  const {
    transformMode, setTransformMode,
    transformSpace, setTransformSpace,
    isPlaying, isPaused,
    togglePlay, togglePause,
    snapEnabled, toggleSnap,
    gridVisible, toggleGrid,
    addActor,
  } = useEditorStore();

  return (
    <div className="neo-toolbar">
      {/* Save */}
      <button className="toolbar-btn" title="Save All (Ctrl+S)">💾</button>
      <div className="toolbar-separator" />

      {/* Transform Tools */}
      <button
        className={`toolbar-btn ${transformMode === 'translate' ? 'active' : ''}`}
        onClick={() => setTransformMode('translate')}
        title="Translate (W)"
      >✥</button>
      <button
        className={`toolbar-btn ${transformMode === 'rotate' ? 'active' : ''}`}
        onClick={() => setTransformMode('rotate')}
        title="Rotate (E)"
      >↻</button>
      <button
        className={`toolbar-btn ${transformMode === 'scale' ? 'active' : ''}`}
        onClick={() => setTransformMode('scale')}
        title="Scale (R)"
      >⤢</button>
      <div className="toolbar-separator" />

      {/* Transform Space */}
      <button
        className="toolbar-btn"
        onClick={() => setTransformSpace(transformSpace === 'world' ? 'local' : 'world')}
        title={`Space: ${transformSpace}`}
        style={{ width: 'auto', padding: '0 8px', fontSize: 10 }}
      >
        {transformSpace === 'world' ? '🌐 World' : '📦 Local'}
      </button>
      <div className="toolbar-separator" />

      {/* Snap & Grid */}
      <button
        className={`toolbar-btn ${snapEnabled ? 'active' : ''}`}
        onClick={toggleSnap}
        title="Toggle Snap"
      >⊞</button>
      <button
        className={`toolbar-btn ${gridVisible ? 'active' : ''}`}
        onClick={toggleGrid}
        title="Toggle Grid"
      >▦</button>
      <div className="toolbar-separator" />

      {/* Add Actor */}
      <button
        className="toolbar-btn"
        onClick={() => addActor('cube', 'Cube')}
        title="Add Cube"
        style={{ fontSize: 12 }}
      >⬜</button>
      <button
        className="toolbar-btn"
        onClick={() => addActor('sphere', 'Sphere')}
        title="Add Sphere"
        style={{ fontSize: 12 }}
      >⚪</button>
      <button
        className="toolbar-btn"
        onClick={() => addActor('light_point', 'PointLight')}
        title="Add Point Light"
      >💡</button>
      <button
        className="toolbar-btn"
        onClick={() => addActor('camera', 'Camera')}
        title="Add Camera"
      >📷</button>

      {/* Spacer */}
      <div style={{ flex: 1 }} />

      {/* Play Controls */}
      {!isPlaying ? (
        <button className="toolbar-btn toolbar-play" onClick={togglePlay} title="Play (Alt+P)">
          ▶ PLAY
        </button>
      ) : (
        <>
          <button className="toolbar-btn toolbar-stop" onClick={togglePlay} title="Stop">
            ■ STOP
          </button>
          <button
            className={`toolbar-btn ${isPaused ? 'active' : ''}`}
            onClick={togglePause}
            title="Pause"
            style={{ marginLeft: 4 }}
          >
            ⏸
          </button>
        </>
      )}
    </div>
  );
}
