import http from 'node:http';

const portFlag = process.argv.indexOf('--port');
const port = Number(process.env.PORT || (portFlag >= 0 ? process.argv[portFlag + 1] : process.argv[2]) || 8787);
let current = null;
const protocolVersion = 1;

function stableValue(value) {
  if (Array.isArray(value)) return `[${value.map(stableValue).join(',')}]`;
  if (value && typeof value === 'object') return `{${Object.keys(value).sort().map((key) => `${JSON.stringify(key)}:${stableValue(value[key])}`).join(',')}}`;
  return JSON.stringify(value);
}
function checksum(document) {
  const input = stableValue({ protocolVersion, sceneName: document.sceneName, revision: document.revision, actors: document.actors });
  let hash = 2166136261;
  for (const character of input) { hash ^= character.charCodeAt(0); hash = Math.imul(hash, 16777619); }
  return (hash >>> 0).toString(16).padStart(8, '0');
}
function validate(document) {
  const errors = [];
  if (!document || document.protocolVersion !== protocolVersion) errors.push('protocolVersion must be 1');
  if (!document || typeof document.sceneName !== 'string' || document.sceneName.trim() === '') errors.push('sceneName is required');
  if (!document || !Number.isInteger(document.revision) || document.revision < 1) errors.push('revision must be a positive integer');
  const actors = document?.actors;
  if (!actors || typeof actors !== 'object' || Array.isArray(actors)) return [...errors, 'actors must be an object'];
  const ids = new Set(Object.keys(actors));
  for (const [id, actor] of Object.entries(actors)) {
    if (actor.id !== id) errors.push(`${id}: id mismatch`);
    if (!actor.transform || !Number.isFinite(actor.transform.position?.x) || !Number.isFinite(actor.transform.position?.y) || !Number.isFinite(actor.transform.position?.z)) errors.push(`${id}: invalid position`);
    if (actor.parentId !== null && !ids.has(actor.parentId)) errors.push(`${id}: missing parent`);
    for (const childId of actor.children || []) if (!ids.has(childId) || actors[childId].parentId !== id) errors.push(`${id}: invalid child link`);
    const seen = new Set([id]); let cursor = actor.parentId;
    while (cursor) { if (seen.has(cursor)) { errors.push(`${id}: hierarchy cycle`); break; } seen.add(cursor); cursor = actors[cursor]?.parentId || null; }
  }
  if (document.checksum !== checksum(document)) errors.push('checksum mismatch');
  return errors;
}
function send(response, status, payload) {
  response.writeHead(status, { 'Content-Type': 'application/json', 'Access-Control-Allow-Origin': '*', 'Access-Control-Allow-Headers': 'content-type', 'Access-Control-Allow-Methods': 'GET,POST,OPTIONS' });
  response.end(JSON.stringify(payload));
}
function readBody(request) {
  return new Promise((resolve, reject) => {
    let body = ''; request.on('data', (chunk) => { body += chunk; if (body.length > 5_000_000) reject(new Error('payload too large')); });
    request.on('end', () => { try { resolve(JSON.parse(body || '{}')); } catch { reject(new Error('invalid JSON')); } }); request.on('error', reject);
  });
}
const server = http.createServer(async (request, response) => {
  if (request.method === 'OPTIONS') return send(response, 204, {});
  if (request.url !== '/scene') return send(response, 404, { error: 'not found' });
  if (request.method === 'GET') return current ? send(response, 200, current) : send(response, 404, { error: 'no scene committed' });
  if (request.method !== 'POST') return send(response, 405, { error: 'method not allowed' });
  try {
    const document = await readBody(request);
    const errors = validate(document);
    if (errors.length) return send(response, 422, { error: 'scene rejected', validationErrors: errors });
    if (current && document.revision <= current.document.revision) return send(response, 409, { error: 'revision conflict', currentRevision: current.document.revision });
    const stored = { ...document, actors: JSON.parse(JSON.stringify(document.actors)) };
    current = { document: stored, receipt: { ok: true, revision: stored.revision, checksum: stored.checksum, actorCount: Object.keys(stored.actors).length, validationErrors: [], source: 'runtime-bridge' } };
    return send(response, 200, current);
  } catch (error) { return send(response, 400, { error: error instanceof Error ? error.message : 'bad request' }); }
});
server.listen(port, '0.0.0.0', () => console.log(`SCENE_BRIDGE_READY port=${port}`));
