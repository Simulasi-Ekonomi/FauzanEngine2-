import type { ActorType, NeoActor, Transform } from '../types/editor';

const SCENE_ID = 'editor-default';
const editorEnvironment = (import.meta as ImportMeta & { env?: Record<string, string | undefined> }).env;
const API_BASE = (editorEnvironment?.VITE_NEOENGINE_API_URL || '').replace(/\/$/, '');

type SceneDocumentActorKind = 'empty' | 'mesh' | 'light' | 'camera' | 'player_start' | 'marker';

interface SceneDocumentActor {
  id: number;
  parent_id: number | null;
  kind: SceneDocumentActorKind;
  transform: { x: number; y: number; z: number; rx: number; ry: number; rz: number; sx: number; sy: number; sz: number };
  asset_id?: string;
}

interface SceneDocumentPayload {
  version: 1;
  scene_id: string;
  actors: SceneDocumentActor[];
}

interface SceneDocumentReceipt {
  scene_id: string;
  revision: number;
  checksum: string;
  actor_count: number;
}

function endpoint(path: string): string {
  return `${API_BASE}${path}`;
}

function stableActorId(sourceId: string): number {
  let hash = 2166136261;
  for (const character of sourceId) {
    hash ^= character.charCodeAt(0);
    hash = Math.imul(hash, 16777619);
  }
  return (hash >>> 0) || 1;
}

function actorKind(type: ActorType): SceneDocumentActorKind {
  if (type === 'camera') return 'camera';
  if (type === 'player_start') return 'player_start';
  if (type.startsWith('light_')) return 'light';
  if (['cube', 'sphere', 'plane', 'cylinder', 'cone', 'torus', 'ring', 'capsule', 'static_mesh', 'skeletal_mesh', 'landscape', 'water', 'foliage', 'sprite', 'tilemap'].includes(type)) return 'mesh';
  return type === 'empty' ? 'empty' : 'marker';
}

function transformPayload(transform: Transform): SceneDocumentActor['transform'] {
  return {
    x: transform.position.x,
    y: transform.position.y,
    z: transform.position.z,
    rx: (transform.rotation.x * Math.PI) / 180,
    ry: (transform.rotation.y * Math.PI) / 180,
    rz: (transform.rotation.z * Math.PI) / 180,
    sx: transform.scale.x,
    sy: transform.scale.y,
    sz: transform.scale.z,
  };
}

function assetId(actor: NeoActor): string | undefined {
  const mesh = actor.components.find((component) => component.type === 'StaticMeshComponent')?.properties.mesh;
  return typeof mesh === 'string' && mesh.length > 0 ? mesh : undefined;
}

export function createSceneDocument(actors: Record<string, NeoActor>): SceneDocumentPayload {
  const pairs = Object.values(actors)
    .map((actor) => ({ source: actor, id: stableActorId(actor.id) }))
    .sort((left, right) => left.source.id.localeCompare(right.source.id));
  const unique = new Set<number>();
  const ids = new Map<string, number>();
  for (const pair of pairs) {
    if (unique.has(pair.id)) throw new Error(`SceneDocument id collision for actor '${pair.source.id}'`);
    unique.add(pair.id);
    ids.set(pair.source.id, pair.id);
  }
  return {
    version: 1,
    scene_id: SCENE_ID,
    actors: pairs.map(({ source, id }) => ({
      id,
      parent_id: source.parentId === null ? null : ids.get(source.parentId) ?? null,
      kind: actorKind(source.type),
      transform: transformPayload(source.transform),
      ...(assetId(source) ? { asset_id: assetId(source) } : {}),
    })),
  };
}

async function jsonRequest<T>(path: string, init?: RequestInit): Promise<T> {
  const response = await fetch(endpoint(path), init);
  if (!response.ok) throw new Error(`SceneDocument request failed: ${response.status}`);
  return response.status === 204 ? (undefined as T) : response.json() as Promise<T>;
}

export async function syncSceneDocument(actors: Record<string, NeoActor>): Promise<SceneDocumentReceipt> {
  const payload = createSceneDocument(actors);
  let previous: SceneDocumentReceipt | null = null;
  try {
    previous = await jsonRequest<SceneDocumentReceipt>(`/scene/documents/${SCENE_ID}`);
  } catch (error) {
    if (!(error instanceof Error) || !error.message.endsWith(': 404')) throw error;
  }
  if (previous === null) {
    return jsonRequest<SceneDocumentReceipt>('/scene/documents', {
      method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(payload),
    });
  }
  return jsonRequest<SceneDocumentReceipt>(`/scene/documents/${SCENE_ID}`, {
    method: 'PUT', headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ ...payload, expected_revision: previous.revision }),
  });
}
