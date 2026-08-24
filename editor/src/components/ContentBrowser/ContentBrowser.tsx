import React from 'react';
import { useEditorStore } from '../../stores/editorStore';

const assetIcons: Record<string, string> = {
  folder: '📁',
  mesh: '🔷',
  material: '🎨',
  texture: '🖼',
  blueprint: '📐',
  level: '🗺',
  animation: '🏃',
  sound: '🔊',
  particle: '✨',
  script: '📜',
};

export function ContentBrowser() {
  const { assets, currentPath } = useEditorStore();

  const pathParts = currentPath.split('/').filter(Boolean);

  return (
    <div className="neo-panel" style={{ height: '100%' }}>
      <div className="neo-panel-header">
        <span className="panel-icon">📂</span>
        Content Browser
      </div>
      {/* Breadcrumb */}
      <div style={{
        padding: '4px 8px',
        borderBottom: '1px solid #1a1a1a',
        fontSize: 11,
        color: '#888',
        display: 'flex',
        alignItems: 'center',
        gap: 4,
      }}>
        {pathParts.map((part, i) => (
          <React.Fragment key={i}>
            {i > 0 && <span style={{ color: '#444' }}>›</span>}
            <span style={{ cursor: 'pointer', color: i === pathParts.length - 1 ? '#ccc' : '#888' }}>
              {part}
            </span>
          </React.Fragment>
        ))}
      </div>
      {/* Search */}
      <div style={{ padding: '4px 6px', borderBottom: '1px solid #1a1a1a' }}>
        <input
          type="text"
          placeholder="Search assets..."
          style={{ width: '100%', height: 22, fontSize: 11 }}
        />
      </div>
      {/* Grid */}
      <div className="neo-panel-content">
        <div className="content-grid">
          {assets.map((asset) => (
            <div key={asset.id} className="content-item" title={asset.path}>
              <div className="content-item-icon">
                {assetIcons[asset.type] || '📄'}
              </div>
              <div className="content-item-name">{asset.name}</div>
            </div>
          ))}
        </div>
      </div>
      {/* Footer */}
      <div style={{
        borderTop: '1px solid #1a1a1a',
        padding: '3px 8px',
        fontSize: 10,
        color: '#555',
        display: 'flex',
        justifyContent: 'space-between',
      }}>
        <span>{assets.length} items</span>
        <span>{currentPath}</span>
      </div>
    </div>
  );
}
