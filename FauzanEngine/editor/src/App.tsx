import React, { useState, useCallback } from 'react';
import { PanelGroup, Panel, PanelResizeHandle } from 'react-resizable-panels';
import { MenuBar } from './components/Layout/MenuBar';
import { Toolbar } from './components/Toolbar/Toolbar';
import { Viewport } from './components/Viewport/Viewport';
import { Outliner } from './components/Outliner/Outliner';
import { Properties } from './components/Properties/Properties';
import { ContentBrowser } from './components/ContentBrowser/ContentBrowser';
import { AIConsole } from './components/AIConsole/AIConsole';
import { StatusBar } from './components/StatusBar/StatusBar';
import { BuildPublishDialog } from './components/BuildPublish/BuildPublishDialog';
import { ModelGeneratorDialog, generateModelFromPrompt } from './components/BuildPublish/ModelGenerator';
import { SettingsDialog } from './components/Settings/SettingsDialog';
import { MonetizationManager } from './components/Monetization/MonetizationManager';
import type { GeneratedModel } from './components/BuildPublish/ModelGenerator';
import { GameRuntime, GAME_TEMPLATES } from './engine/GameRuntime';
import type { UIElement, GameScript } from './engine/GameRuntime';
import { useEditorStore } from './stores/editorStore';

export function App() {
  const [buildDialogOpen, setBuildDialogOpen] = useState(false);
  const [modelGenOpen, setModelGenOpen] = useState(false);
  const [settingsOpen, setSettingsOpen] = useState(false);
  const [monetizationOpen, setMonetizationOpen] = useState(false);
  const [gameUIElements, setGameUIElements] = useState<UIElement[]>([]);
  const [gameRuntime, setGameRuntime] = useState<GameRuntime | null>(null);

  const actors = useEditorStore((s) => s.actors);
  const addActor = useEditorStore((s) => s.addActor);
  const addActorBatch = useEditorStore((s) => s.addActorBatch);
  const addSystemMessage = useEditorStore((s) => s.addSystemMessage);
  const isPlaying = useEditorStore((s) => s.isPlaying);
  const addGeneratedAsset = useEditorStore((s) => s.addGeneratedAsset);
  const togglePlay = useEditorStore((s) => s.togglePlay);

  // Handle 3D model generation - NOW WITH PROPER TRANSFORMS AND COLORS
  const handleModelGenerate = useCallback((model: GeneratedModel) => {
    const batchDefs = model.actors.map((part) => ({
      type: part.type as any,
      name: part.name,
      transform: {
        position: part.position,
        scale: part.scale,
      },
      material: part.color ? { color: part.color } : undefined,
    }));
    addActorBatch(batchDefs);

    // Save to generated assets for marketplace
    addGeneratedAsset({
      name: model.name,
      type: model.type,
      category: model.type,
      data: JSON.stringify(model),
    });

    addSystemMessage(`[3D Gen] Added "${model.name}" to scene (${model.actors.length} parts, ${model.vertices} vertices, ${model.faces} faces)`);
  }, [addActorBatch, addSystemMessage, addGeneratedAsset]);

  // Handle AI special commands (called from AIConsole)
  const handleAISpecialCommand = useCallback((response: string) => {
    if (response.startsWith('__3D_MODEL_GENERATE__:')) {
      const prompt = response.replace('__3D_MODEL_GENERATE__:', '');
      const model = generateModelFromPrompt(prompt);
      if (model) {
        handleModelGenerate(model);
        addSystemMessage(`[Aries 3D] Generated "${model.name}" - ${model.vertices} vertices, ${model.actors.length} parts`);
      }
      return true;
    }
    if (response.startsWith('__ADD_ACTOR__:')) {
      const data = JSON.parse(response.replace('__ADD_ACTOR__:', ''));
      addActorBatch(data.map((a: any) => ({
        type: a.type, name: a.name,
        transform: a.transform ? { position: a.transform.position, scale: a.transform.scale } : undefined,
        material: a.color ? { color: a.color } : undefined,
      })));
      addSystemMessage(`[Aries] Added ${data.length} actors to scene`);
      return true;
    }
    if (response === '__OPEN_BUILD_DIALOG__') {
      setBuildDialogOpen(true);
      addSystemMessage('[Build] Opening Build & Publish dialog...');
      return true;
    }
    if (response === '__OPEN_MONETIZATION__') {
      setMonetizationOpen(true);
      addSystemMessage('[Monetization] Opening Monetization Manager...');
      return true;
    }
    if (response === '__START_PLAY_MODE__') {
      startPlayMode();
      return true;
    }
    return false;
  }, [addSystemMessage, handleModelGenerate]);

  // Start play mode with game scripts
  const startPlayMode = useCallback(() => {
    if (isPlaying) return;

    // Find matching game template scripts
    const scripts: GameScript[] = [];
    const actorNames = Object.values(actors).map(a => a.name);

    for (const [templateName, template] of Object.entries(GAME_TEMPLATES)) {
      for (const scriptDef of template.scripts) {
        const matchingActor = Object.values(actors).find(a => a.name === scriptDef.actorName);
        if (matchingActor) {
          scripts.push({
            id: `script_${templateName}_${matchingActor.id}`,
            name: `${templateName}_controller`,
            actorId: matchingActor.id,
            code: scriptDef.code,
            enabled: true,
          });
        }
      }
    }

    if (scripts.length === 0) {
      addSystemMessage('[Engine] No game scripts found. Create a game template first (e.g., "buat game farmville").');
      return;
    }

    togglePlay();
    const runtime = new GameRuntime(
      actors,
      scripts,
      () => {}, // actor updates are visual only
      (elements) => setGameUIElements(elements),
      (msg) => addSystemMessage(msg),
    );
    setGameRuntime(runtime);
    runtime.start();
    addSystemMessage(`[Engine] Play mode started with ${scripts.length} script(s). Use keyboard to interact!`);
  }, [actors, isPlaying, togglePlay, addSystemMessage]);

  // Stop play mode
  const stopPlayMode = useCallback(() => {
    if (gameRuntime) {
      gameRuntime.stop();
      setGameRuntime(null);
    }
    setGameUIElements([]);
    if (isPlaying) togglePlay();
  }, [gameRuntime, isPlaying, togglePlay]);

  // Pause/resume
  const togglePauseGame = useCallback(() => {
    if (gameRuntime) {
      const state = gameRuntime.getState();
      if (state.paused) gameRuntime.resume();
      else gameRuntime.pause();
    }
  }, [gameRuntime]);

  // Expose special command handler globally for AIConsole
  (window as any).__neoHandleSpecialCommand = handleAISpecialCommand;
  (window as any).__neoOpenBuildDialog = () => setBuildDialogOpen(true);
  (window as any).__neoOpenModelGen = () => setModelGenOpen(true);
  (window as any).__neoOpenSettings = () => setSettingsOpen(true);
  (window as any).__neoOpenMonetization = () => setMonetizationOpen(true);
  (window as any).__neoStartPlayMode = startPlayMode;
  (window as any).__neoStopPlayMode = stopPlayMode;

  return (
    <div style={{ width: '100%', height: '100%', display: 'flex', flexDirection: 'column' }}>
      <MenuBar />
      <Toolbar />
      <div style={{ flex: 1, overflow: 'hidden' }}>
        <PanelGroup direction="horizontal" style={{ height: '100%' }}>
          {/* Left: Outliner */}
          <Panel defaultSize={15} minSize={10} maxSize={30}>
            <Outliner />
          </Panel>
          <PanelResizeHandle style={{ width: 2, background: '#1a1a1a', cursor: 'col-resize' }} />
          
          {/* Center: Viewport + Bottom panels */}
          <Panel defaultSize={55} minSize={30}>
            <PanelGroup direction="vertical" style={{ height: '100%' }}>
              {/* Viewport */}
              <Panel defaultSize={65} minSize={30}>
                <div style={{ height: '100%', position: 'relative' }}>
                  <Viewport />
                  {/* Game HUD overlay */}
                  {isPlaying && (
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
                          PLAYING
                        </span>
                        <button onClick={togglePauseGame} style={{
                          background: '#555', border: 'none', color: '#fff',
                          padding: '4px 12px', borderRadius: 3, cursor: 'pointer', fontSize: 11,
                        }}>Pause</button>
                        <button onClick={stopPlayMode} style={{
                          background: '#e04040', border: 'none', color: '#fff',
                          padding: '4px 12px', borderRadius: 3, cursor: 'pointer', fontSize: 11,
                        }}>Stop</button>
                      </div>

                      {/* Dynamic UI from game scripts */}
                      {gameUIElements.map((el) => {
                        if (el.type === 'text') {
                          return (
                            <div key={el.id} style={{
                              position: 'absolute', left: `${el.x}%`, top: `${el.y}%`,
                              transform: 'translateX(-50%)', color: el.color || '#fff',
                              fontSize: el.size || 14, fontWeight: 700,
                              textShadow: '0 0 8px rgba(0,0,0,0.8)', whiteSpace: 'nowrap',
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
                              <div style={{ color: '#888', fontSize: 10, textTransform: 'uppercase' }}>Score</div>
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

                      <div style={{
                        position: 'absolute', bottom: 8, left: '50%', transform: 'translateX(-50%)',
                        color: '#666', fontSize: 10, background: 'rgba(0,0,0,0.5)',
                        padding: '2px 12px', borderRadius: 3,
                      }}>
                        Use keyboard to interact with the game
                      </div>
                    </div>
                  )}
                </div>
              </Panel>
              <PanelResizeHandle style={{ height: 2, background: '#1a1a1a', cursor: 'row-resize' }} />
              
              {/* Bottom: Content Browser + AI Console */}
              <Panel defaultSize={35} minSize={15}>
                <PanelGroup direction="horizontal" style={{ height: '100%' }}>
                  <Panel defaultSize={60} minSize={20}>
                    <ContentBrowser />
                  </Panel>
                  <PanelResizeHandle style={{ width: 2, background: '#1a1a1a', cursor: 'col-resize' }} />
                  <Panel defaultSize={40} minSize={20}>
                    <AIConsole />
                  </Panel>
                </PanelGroup>
              </Panel>
            </PanelGroup>
          </Panel>
          <PanelResizeHandle style={{ width: 2, background: '#1a1a1a', cursor: 'col-resize' }} />
          
          {/* Right: Properties/Details */}
          <Panel defaultSize={20} minSize={12} maxSize={35}>
            <Properties />
          </Panel>
        </PanelGroup>
      </div>
      <StatusBar />

      {/* Dialogs */}
      <BuildPublishDialog
        isOpen={buildDialogOpen}
        onClose={() => setBuildDialogOpen(false)}
        onLog={(msg) => addSystemMessage(msg)}
        actors={actors}
        scripts={[]}
      />
      <ModelGeneratorDialog
        isOpen={modelGenOpen}
        onClose={() => setModelGenOpen(false)}
        onGenerate={handleModelGenerate}
        onLog={(msg) => addSystemMessage(msg)}
      />
      <SettingsDialog
        isOpen={settingsOpen}
        onClose={() => setSettingsOpen(false)}
      />
      <MonetizationManager
        isOpen={monetizationOpen}
        onClose={() => setMonetizationOpen(false)}
        onLog={(msg) => addSystemMessage(msg)}
      />
    </div>
  );
}
