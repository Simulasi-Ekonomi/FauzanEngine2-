import React, { useMemo, useState } from 'react';
import { useEditorStore } from '../../stores/editorStore';

const assetIcons: Record<string, string> = {
  folder: '📁', mesh: '🔷', material: '🎨', texture: '🖼', blueprint: '📐', level: '🗺', animation: '🏃', sound: '🔊', particle: '✨', script: '📜',
};

export function ContentBrowser() {
  const { assets, currentPath, setCurrentPath, addSystemMessage } = useEditorStore();
  const addActor = useEditorStore((s) => s.addActor);
  const [filter, setFilter] = useState('');
  const pathParts = currentPath.split('/').filter(Boolean);
  const visibleAssets = useMemo(() => assets.filter((asset) => {
    const inPath = currentPath === '/Game' || asset.path.startsWith(`${currentPath}/`);
    return inPath && asset.name.toLowerCase().includes(filter.toLowerCase());
  }), [assets, currentPath, filter]);
  const openFolder = (path: string) => setCurrentPath(path);
  const dragAsset = (event: React.DragEvent, asset: (typeof assets)[number]) => {
    event.dataTransfer.setData('application/x-neoengine-asset', JSON.stringify(asset));
    event.dataTransfer.effectAllowed = 'copy';
  };
  const createActorFromAsset = (asset: (typeof assets)[number]) => {
    if (asset.type === 'folder') return;
    const actorType = asset.type === 'mesh' ? 'cube' : asset.type === 'blueprint' ? 'empty' : asset.type === 'level' ? 'empty' : 'sprite';
    addActor(actorType, asset.name, { components: [{ id: `asset_${asset.id}`, name: 'AssetReference', type: 'AssetReferenceComponent', properties: { assetId: asset.id, assetPath: asset.path } }] });
    addSystemMessage(`[Content] Spawned ${asset.name} from ${asset.path}.`);
  };

  return (
    <div className="neo-panel" style={{ height: '100%' }}>
      <div className="neo-panel-header"><span className="panel-icon">📂</span>Content Browser</div>
      <div style={{ padding: '4px 8px', borderBottom: '1px solid #1a1a1a', fontSize: 11, color: '#888', display: 'flex', alignItems: 'center', gap: 4 }}>
        <button className="breadcrumb-button" onClick={() => openFolder('/Game')}>Game</button>
        {pathParts.slice(1).map((part, i) => {
          const path = `/Game/${pathParts.slice(1, i + 2).join('/')}`;
          return <React.Fragment key={path}><span style={{ color: '#444' }}>›</span><button className="breadcrumb-button" onClick={() => openFolder(path)}>{part}</button></React.Fragment>;
        })}
      </div>
      <div style={{ padding: '4px 6px', borderBottom: '1px solid #1a1a1a' }}>
        <input type="text" placeholder="Search assets..." value={filter} onChange={(e) => setFilter(e.target.value)} style={{ width: '100%', height: 22, fontSize: 11 }} />
      </div>
      <div className="neo-panel-content">
        <div className="content-grid">
          {visibleAssets.map((asset) => (
            <div key={asset.id} className="content-item" title={`${asset.path}\nDouble-click to spawn actor`} draggable={asset.type !== 'folder'} onDragStart={(event) => dragAsset(event, asset)} onDoubleClick={() => asset.type === 'folder' ? openFolder(asset.path) : createActorFromAsset(asset)}>
              <div className="content-item-icon">{assetIcons[asset.type] || '📄'}</div>
              <div className="content-item-name">{asset.name}</div>
            </div>
          ))}
        </div>
      </div>
      <div style={{ borderTop: '1px solid #1a1a1a', padding: '3px 8px', fontSize: 10, color: '#555', display: 'flex', justifyContent: 'space-between' }}>
        <span>{visibleAssets.length} / {assets.length} items</span><span>{currentPath}</span>
      </div>
    </div>
  );
}
