import React, { useEffect, useState } from 'react';
import type { GameRuntime } from '../../engine/GameRuntime';

export function ProfilerOverlay({ runtime }: { runtime: GameRuntime | null }) {
  const [snapshot, setSnapshot] = useState({ elapsed: 0, actors: 0, scripts: 0, ui: 0, logs: 0, input: 0 });
  useEffect(() => {
    if (!runtime) return;
    const read = () => {
      const state = runtime.getState();
      setSnapshot({ elapsed: state.elapsed, actors: Object.keys(state.actorStates).length, scripts: state.scripts.length, ui: state.uiElements.length, logs: state.logs.length, input: Object.values(state.inputState).filter(Boolean).length });
    };
    read();
    const timer = window.setInterval(read, 250);
    return () => window.clearInterval(timer);
  }, [runtime]);
  if (!runtime) return null;
  return (
    <div className="pie-profiler" aria-label="Play-in-Editor profiler">
      <div className="pie-profiler-title">PIE PROFILER</div>
      <div className="pie-profiler-grid">
        <span>TIME</span><strong>{snapshot.elapsed.toFixed(2)}s</strong>
        <span>ACTORS</span><strong>{snapshot.actors}</strong>
        <span>SCRIPTS</span><strong>{snapshot.scripts}</strong>
        <span>UI</span><strong>{snapshot.ui}</strong>
        <span>INPUT</span><strong>{snapshot.input}</strong>
        <span>LOGS</span><strong>{snapshot.logs}</strong>
      </div>
    </div>
  );
}
