import type { NeoActor } from '../types/editor';

export const SCENE_BRIDGE_PROTOCOL = 1 as const;
const STORAGE_KEY = 'neoengine_runtime_scene_v1';

export interface SceneBridgeDocument {
  protocolVersion: typeof SCENE_BRIDGE_PROTOCOL;
  sceneName: string;
  revision: number;
  actors: Record<string, NeoActor>;
  checksum: string;
}

export interface SceneBridgeReceipt {
  ok: boolean;
  revision: number;
  checksum: string;
  actorCount: number;
  validationErrors: string[];
  source: 'runtime-bridge' | 'local-fallback';
}

export interface SceneBridgeResult {
  document: SceneBridgeDocument;
  receipt: SceneBridgeReceipt;
}

function stableValue(value: unknown): string {
  if (Array.isArray(value)) return `[${value.map(stableValue).join(',')}]`;
  if (value && typeof value === 'object') {
    return `{${Object.keys(value as Record<string, unknown>).sort().map((key) => `${JSON.stringify(key)}:${stableValue((value as Record<string, unknown>)[key])}`).join(',')}}`;
  }
  return JSON.stringify(value);
}

export function sceneChecksum(sceneName: string, revision: number, actors: Record<string, NeoActor>): string {
  const input = stableValue({ protocolVersion: SCENE_BRIDGE_PROTOCOL, sceneName, revision, actors });
  let hash = 2166136261;
  for (let index = 0; index < input.length; index += 1) {
    hash ^= input.charCodeAt(index);
    hash = Math.imul(hash, 16777619);
  }
  return (hash >>> 0).toString(16).padStart(8, '0');
}

export function validateScene(actors: Record<string, NeoActor>): string[] {
  const errors: string[] = [];
  const actorIds = new Set(Object.keys(actors));
  for (const [id, actor] of Object.entries(actors)) {
    if (actor.id !== id) errors.push(`actor ${id}: id mismatch`);
    if (!Number.isFinite(actor.transform.position.x) || !Number.isFinite(actor.transform.position.y) || !Number.isFinite(actor.transform.position.z)) errors.push(`actor ${id}: non-finite position`);
    if (!Number.isFinite(actor.transform.rotation.x) || !Number.isFinite(actor.transform.rotation.y) || !Number.isFinite(actor.transform.rotation.z)) errors.push(`actor ${id}: non-finite rotation`);
    if (!Number.isFinite(actor.transform.scale.x) || !Number.isFinite(actor.transform.scale.y) || !Number.isFinite(actor.transform.scale.z)) errors.push(`actor ${id}: non-finite scale`);
    if (actor.parentId !== null && !actorIds.has(actor.parentId)) errors.push(`actor ${id}: missing parent ${actor.parentId}`);
    const seen = new Set<string>([id]);
    let cursor = actor.parentId;
    while (cursor) {
      if (seen.has(cursor)) { errors.push(`actor ${id}: hierarchy cycle`); break; }
      seen.add(cursor);
      cursor = actors[cursor]?.parentId || null;
    }
    for (const childId of actor.children) {
      if (!actorIds.has(childId)) errors.push(`actor ${id}: missing child ${childId}`);
      else if (actors[childId].parentId !== id) errors.push(`actor ${id}: child link mismatch ${childId}`);
    }
  }
  return errors;
}

export function buildSceneDocument(sceneName: string, revision: number, actors: Record<string, NeoActor>): SceneBridgeDocument {
  const errors = validateScene(actors);
  if (errors.length > 0) throw new Error(`Scene validation failed: ${errors.join('; ')}`);
  return { protocolVersion: SCENE_BRIDGE_PROTOCOL, sceneName: sceneName.trim() || 'Untitled', revision, actors: JSON.parse(JSON.stringify(actors)) as Record<string, NeoActor>, checksum: sceneChecksum(sceneName.trim() || 'Untitled', revision, actors) };
}

export function commitLocalScene(sceneName: string, revision: number, actors: Record<string, NeoActor>): SceneBridgeResult {
  const document = buildSceneDocument(sceneName, revision, actors);
  if (typeof localStorage !== 'undefined') localStorage.setItem(STORAGE_KEY, JSON.stringify(document));
  return { document, receipt: { ok: true, revision, checksum: document.checksum, actorCount: Object.keys(actors).length, validationErrors: [], source: 'local-fallback' } };
}

export function restoreLocalScene(): SceneBridgeDocument | null {
  if (typeof localStorage === 'undefined') return null;
  try {
    const parsed = JSON.parse(localStorage.getItem(STORAGE_KEY) || 'null') as SceneBridgeDocument | null;
    if (!parsed || parsed.protocolVersion !== SCENE_BRIDGE_PROTOCOL || validateScene(parsed.actors).length > 0) return null;
    return parsed;
  } catch { return null; }
}

export async function commitRuntimeScene(sceneName: string, revision: number, actors: Record<string, NeoActor>): Promise<SceneBridgeResult> {
  const document = buildSceneDocument(sceneName, revision, actors);
  const baseUrl = (((import.meta as ImportMeta & { env?: Record<string, string | undefined> }).env?.VITE_SCENE_BRIDGE_URL) || '').replace(/\/$/, '');
  if (!baseUrl) return commitLocalScene(sceneName, revision, actors);
  const response = await fetch(`${baseUrl}/scene`, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(document) });
  const payload = await response.json() as { document?: SceneBridgeDocument; receipt?: SceneBridgeReceipt; error?: string };
  if (!response.ok || !payload.document || !payload.receipt) throw new Error(payload.error || `Runtime bridge returned ${response.status}`);
  return { document: payload.document, receipt: payload.receipt };
}
