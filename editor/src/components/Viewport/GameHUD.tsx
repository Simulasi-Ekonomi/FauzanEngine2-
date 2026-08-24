import React from 'react';
import type { UIElement } from '../../engine/GameRuntime';

interface GameHUDProps {
  elements: UIElement[];
  isPlaying: boolean;
  onStop: () => void;
  onPause: () => void;
  isPaused: boolean;
}

export function GameHUD({ elements, isPlaying, onStop, onPause, isPaused }: GameHUDProps) {
  if (!isPlaying) return null;

  return (
    <div style={{
      position: 'absolute', top: 0, left: 0, right: 0, bottom: 0,
      pointerEvents: 'none', zIndex: 100,
    }}>
      {/* Play mode toolbar */}
      <div style={{
        position: 'absolute', top: 8, left: '50%', transform: 'translateX(-50%)',
        display: 'flex', gap: 4, pointerEvents: 'all',
        background: 'rgba(0,0,0,0.8)', padding: '4px 12px', borderRadius: 6,
        border: '1px solid #444',
      }}>
        <span style={{ color: '#4ec949', fontWeight: 700, fontSize: 12, marginRight: 8, display: 'flex', alignItems: 'center' }}>
          ▶ PLAYING
        </span>
        <button onClick={onPause} style={{
          background: isPaused ? '#e8a030' : '#555', border: 'none', color: '#fff',
          padding: '4px 12px', borderRadius: 3, cursor: 'pointer', fontSize: 11,
        }}>{isPaused ? '▶ Resume' : '⏸ Pause'}</button>
        <button onClick={onStop} style={{
          background: '#e04040', border: 'none', color: '#fff',
          padding: '4px 12px', borderRadius: 3, cursor: 'pointer', fontSize: 11,
        }}>⏹ Stop</button>
      </div>

      {/* Dynamic UI Elements from game scripts */}
      {elements.map((el) => {
        if (el.type === 'text') {
          return (
            <div key={el.id} style={{
              position: 'absolute',
              left: `${el.x}%`, top: `${el.y}%`,
              transform: 'translateX(-50%)',
              color: el.color || '#fff',
              fontSize: el.size || 14,
              fontWeight: 700,
              textShadow: '0 0 8px rgba(0,0,0,0.8), 0 2px 4px rgba(0,0,0,0.5)',
              fontFamily: "'Segoe UI', sans-serif",
              whiteSpace: 'nowrap',
            }}>{el.text}</div>
          );
        }

        if (el.type === 'score') {
          return (
            <div key={el.id} style={{
              position: 'absolute', right: 16, top: 16,
              background: 'rgba(0,0,0,0.7)', padding: '8px 16px',
              borderRadius: 6, border: '1px solid #444',
            }}>
              <div style={{ color: '#888', fontSize: 10, textTransform: 'uppercase', letterSpacing: 1 }}>Score</div>
              <div style={{ color: '#ffcc00', fontSize: 24, fontWeight: 700 }}>{el.score}</div>
            </div>
          );
        }

        if (el.type === 'health') {
          const pct = el.maxHealth ? ((el.health || 0) / el.maxHealth) * 100 : 0;
          const color = pct > 60 ? '#4ec949' : pct > 30 ? '#e8a030' : '#e04040';
          return (
            <div key={el.id} style={{
              position: 'absolute', left: 16, top: 16,
              background: 'rgba(0,0,0,0.7)', padding: '8px 12px',
              borderRadius: 6, border: '1px solid #444', width: 150,
            }}>
              <div style={{ color: '#888', fontSize: 10, marginBottom: 4 }}>HP: {el.health}/{el.maxHealth}</div>
              <div style={{ height: 8, background: '#333', borderRadius: 4, overflow: 'hidden' }}>
                <div style={{ height: '100%', width: `${pct}%`, background: color, borderRadius: 4, transition: 'width 0.3s' }} />
              </div>
            </div>
          );
        }

        return null;
      })}

      {/* Controls hint */}
      <div style={{
        position: 'absolute', bottom: 8, left: '50%', transform: 'translateX(-50%)',
        color: '#666', fontSize: 10, background: 'rgba(0,0,0,0.5)',
        padding: '2px 12px', borderRadius: 3,
      }}>
        Use keyboard to interact with the game
      </div>
    </div>
  );
}
