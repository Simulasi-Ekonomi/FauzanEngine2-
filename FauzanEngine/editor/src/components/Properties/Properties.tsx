import React from 'react';
import { useEditorStore } from '../../stores/editorStore';
import type { Vector3 } from '../../types/editor';

function VectorInput({ label, value, onChange }: {
  label: string;
  value: Vector3;
  onChange: (v: Vector3) => void;
}) {
  return (
    <div className="property-row">
      <span className="property-label">{label}</span>
      <div className="property-value">
        <input
          className="vec-input x"
          type="number"
          step={0.1}
          value={value.x}
          onChange={(e) => onChange({ ...value, x: parseFloat(e.target.value) || 0 })}
          title="X"
        />
        <input
          className="vec-input y"
          type="number"
          step={0.1}
          value={value.y}
          onChange={(e) => onChange({ ...value, y: parseFloat(e.target.value) || 0 })}
          title="Y"
        />
        <input
          className="vec-input z"
          type="number"
          step={0.1}
          value={value.z}
          onChange={(e) => onChange({ ...value, z: parseFloat(e.target.value) || 0 })}
          title="Z"
        />
      </div>
    </div>
  );
}

function ComponentSection({ actorId, component }: { actorId: string; component: any }) {
  const setComponentProperty = useEditorStore((s) => s.setComponentProperty);
  return (
    <div className="property-group">
      <div className="property-group-header">
        <span style={{ marginRight: 6 }}>▼</span>
        {component.name} ({component.type})
      </div>
      {Object.entries(component.properties).map(([key, value]) => (
        <div className="property-row" key={key}>
          <span className="property-label">{key}</span>
          <div className="property-value">
            {typeof value === 'boolean' ? (
              <input type="checkbox" checked={value as boolean} onChange={(e) => setComponentProperty(actorId, component.id, key, e.target.checked)} />
            ) : typeof value === 'number' ? (
              <input type="number" step={0.1} value={value as number} onChange={(e) => setComponentProperty(actorId, component.id, key, Number(e.target.value) || 0)} style={{ width: '100%' }} />
            ) : (
              <input type="text" value={String(value)} onChange={(e) => setComponentProperty(actorId, component.id, key, e.target.value)} style={{ width: '100%' }} />
            )}
          </div>
        </div>
      ))}
    </div>
  );
}

export function Properties() {
  const actors = useEditorStore((s) => s.actors);
  const selectedActorId = useEditorStore((s) => s.selectedActorId);
  const updateActorTransform = useEditorStore((s) => s.updateActorTransform);
  const renameActor = useEditorStore((s) => s.renameActor);
  const addComponent = useEditorStore((s) => s.addComponent);
  const reparentActor = useEditorStore((s) => s.reparentActor);

  const actor = selectedActorId ? actors[selectedActorId] : null;

  return (
    <div className="neo-panel" style={{ height: '100%' }}>
      <div className="neo-panel-header">
        <span className="panel-icon">📋</span>
        Details
      </div>
      <div className="neo-panel-content">
        {!actor ? (
          <div style={{ padding: 20, textAlign: 'center', color: '#555', fontSize: 11 }}>
            Select an actor to view its properties
          </div>
        ) : (
          <>
            {/* Actor Info */}
            <div className="property-group">
              <div className="property-group-header">
                <span style={{ marginRight: 6 }}>▼</span>
                Actor
              </div>
              <div className="property-row">
                <span className="property-label">Name</span>
                <div className="property-value">
                  <input
                    type="text"
                    value={actor.name}
                    onChange={(e) => renameActor(actor.id, e.target.value)}
                    style={{ width: '100%' }}
                  />
                </div>
              </div>
              <div className="property-row">
                <span className="property-label">Type</span>
                <div className="property-value">
                  <input type="text" value={actor.type} readOnly style={{ width: '100%', color: '#888' }} />
                </div>
              </div>
              <div className="property-row">
                <span className="property-label">ID</span>
                <div className="property-value">
                  <input type="text" value={actor.id} readOnly style={{ width: '100%', color: '#555', fontSize: 9 }} />
                </div>
              </div>
              <div className="property-row">
                <span className="property-label">Parent</span>
                <div className="property-value">
                  <select value={actor.parentId || ''} onChange={(e) => reparentActor(actor.id, e.target.value || null)} style={{ width: '100%', height: 21, fontSize: 11 }}>
                    <option value="">None (Root)</option>
                    {Object.values(actors).filter((candidate) => candidate.id !== actor.id).map((candidate) => (
                      <option key={candidate.id} value={candidate.id}>{candidate.name}</option>
                    ))}
                  </select>
                </div>
              </div>
            </div>

            {/* Transform */}
            <div className="property-group">
              <div className="property-group-header">
                <span style={{ marginRight: 6 }}>▼</span>
                Transform
              </div>
              <VectorInput
                label="Location"
                value={actor.transform.position}
                onChange={(position) => updateActorTransform(actor.id, { position })}
              />
              <VectorInput
                label="Rotation"
                value={actor.transform.rotation}
                onChange={(rotation) => updateActorTransform(actor.id, { rotation })}
              />
              <VectorInput
                label="Scale"
                value={actor.transform.scale}
                onChange={(scale) => updateActorTransform(actor.id, { scale })}
              />
            </div>

            {/* Components */}
            {actor.components.map((comp) => (
              <ComponentSection key={comp.id} actorId={actor.id} component={comp} />
            ))}

            {/* Add Component Button */}
            <div style={{ padding: 8 }}>
              <button onClick={() => addComponent(actor.id, 'SceneComponent')} style={{
                width: '100%', padding: '6px', background: '#2a4a2a', color: '#88cc88', fontSize: 11, borderRadius: 3,
              }}>
                + Add Component
              </button>
            </div>
          </>
        )}
      </div>
    </div>
  );
}
