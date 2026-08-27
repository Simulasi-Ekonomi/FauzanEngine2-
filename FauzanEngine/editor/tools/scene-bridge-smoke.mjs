const baseUrl = process.env.BRIDGE_URL || 'http://127.0.0.1:8787';
const actor = (id, parentId = null) => ({ id, name: id, type: 'cube', parentId, children: [], visible: true, transform: { position: { x: 0, y: 0, z: 0 }, rotation: { x: 0, y: 0, z: 0 }, scale: { x: 1, y: 1, z: 1 } }, components: [] });
function stableValue(value) { if (Array.isArray(value)) return `[${value.map(stableValue).join(',')}]`; if (value && typeof value === 'object') return `{${Object.keys(value).sort().map((key) => `${JSON.stringify(key)}:${stableValue(value[key])}`).join(',')}}`; return JSON.stringify(value); }
function checksum(document) { const input = stableValue({ protocolVersion: 1, sceneName: document.sceneName, revision: document.revision, actors: document.actors }); let hash = 2166136261; for (const character of input) { hash ^= character.charCodeAt(0); hash = Math.imul(hash, 16777619); } return (hash >>> 0).toString(16).padStart(8, '0'); }
async function request(method, document) { const response = await fetch(`${baseUrl}/scene`, { method, headers: { 'Content-Type': 'application/json' }, body: document ? JSON.stringify(document) : undefined }); return { status: response.status, body: await response.json() }; }
const existing = await request('GET');
const nextRevision = existing.status === 200 ? existing.body.document.revision + 1 : 1;
const document = { protocolVersion: 1, sceneName: 'BridgeSmoke', revision: nextRevision, actors: { root: actor('root') }, checksum: '' }; document.checksum = checksum(document);
const committed = await request('POST', document); if (committed.status !== 200 || !committed.body.receipt?.ok) throw new Error(`valid commit failed: ${committed.status}`);
const fetched = await request('GET'); if (fetched.status !== 200 || fetched.body.document.checksum !== document.checksum) throw new Error('round-trip failed');
const stale = await request('POST', document); if (stale.status !== 409) throw new Error(`stale revision accepted: ${stale.status}`);
const invalid = { ...document, revision: document.revision + 1, actors: { child: actor('child', 'missing') } }; invalid.checksum = checksum(invalid); const rejected = await request('POST', invalid); if (rejected.status !== 422) throw new Error(`invalid scene accepted: ${rejected.status}`);
console.log(`SCENE_BRIDGE_SMOKE_OK commit=${committed.status} roundtrip=${fetched.status} stale=${stale.status} invalid=${rejected.status} checksum=${document.checksum}`);
