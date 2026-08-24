// NeoEngine Editor - Type Definitions

export interface Vector3 {
  x: number;
  y: number;
  z: number;
}

export interface Transform {
  position: Vector3;
  rotation: Vector3;
  scale: Vector3;
}

export interface NeoActor {
  id: string;
  name: string;
  type: ActorType;
  transform: Transform;
  visible: boolean;
  children: string[];
  parentId: string | null;
  components: NeoComponent[];
}

export type ActorType =
  | 'empty'
  | 'cube'
  | 'sphere'
  | 'plane'
  | 'cylinder'
  | 'cone'
  | 'torus'
  | 'ring'
  | 'capsule'
  | 'light_directional'
  | 'light_point'
  | 'light_spot'
  | 'camera'
  | 'static_mesh'
  | 'skeletal_mesh'
  | 'particle_system'
  | 'audio_source'
  | 'trigger_volume'
  | 'player_start'
  | 'landscape'
  | 'sprite'
  | 'tilemap'
  | 'text_3d'
  | 'water'
  | 'foliage';

export interface MaterialProperties {
  color?: string;
  roughness?: number;
  metalness?: number;
  emissive?: string;
  emissiveIntensity?: number;
  opacity?: number;
  transparent?: boolean;
  wireframe?: boolean;
  texturePath?: string;
  normalMapPath?: string;
  side?: 'front' | 'back' | 'double';
}

export interface SpriteProperties {
  texturePath?: string;
  color?: string;
  width?: number;
  height?: number;
  flipX?: boolean;
  flipY?: boolean;
}

export interface TilemapProperties {
  tileWidth?: number;
  tileHeight?: number;
  columns?: number;
  rows?: number;
  texturePath?: string;
  tileData?: number[][];
}

export interface NeoComponent {
  id: string;
  type: string;
  name: string;
  properties: Record<string, PropertyValue>;
  material?: MaterialProperties;
}

export type PropertyValue = string | number | boolean | Vector3 | number[];

export interface AssetItem {
  id: string;
  name: string;
  type: AssetType;
  path: string;
  thumbnail?: string;
}

export type AssetType =
  | 'folder'
  | 'mesh'
  | 'material'
  | 'texture'
  | 'blueprint'
  | 'level'
  | 'animation'
  | 'sound'
  | 'particle'
  | 'script';

export interface ChatMessage {
  id: string;
  role: 'user' | 'assistant' | 'system';
  content: string;
  timestamp: number;
}

export type TransformMode = 'translate' | 'rotate' | 'scale';
export type TransformSpace = 'world' | 'local';
export type ViewMode = 'perspective' | 'top' | 'front' | 'right';

export interface ActorCreateOptions {
  transform?: Partial<Transform>;
  material?: MaterialProperties;
  components?: NeoComponent[];
  visible?: boolean;
  parentId?: string | null;
}

export interface EditorState {
  // Scene
  actors: Record<string, NeoActor>;
  selectedActorId: string | null;
  
  // Tools
  transformMode: TransformMode;
  transformSpace: TransformSpace;
  viewMode: ViewMode;
  isPlaying: boolean;
  isPaused: boolean;
  snapEnabled: boolean;
  gridVisible: boolean;
  
  // AI
  chatMessages: ChatMessage[];
  isAiThinking: boolean;
  
  // Connection
  engineConnected: boolean;
  ariesConnected: boolean;
  fps: number;
  
  // Content Browser
  currentPath: string;
  assets: AssetItem[];
  
  // Actions
  selectActor: (id: string | null) => void;
  addActor: (type: ActorType, name?: string, options?: ActorCreateOptions) => void;
  addActorBatch: (actors: Array<{ type: ActorType; name: string; transform?: Partial<Transform>; material?: MaterialProperties }>) => void;
  removeActor: (id: string) => void;
  updateActorTransform: (id: string, transform: Partial<Transform>) => void;
  renameActor: (id: string, name: string) => void;
  setTransformMode: (mode: TransformMode) => void;
  setTransformSpace: (space: TransformSpace) => void;
  setViewMode: (mode: ViewMode) => void;
  togglePlay: () => void;
  togglePause: () => void;
  toggleSnap: () => void;
  toggleGrid: () => void;
  sendMessage: (content: string) => void;
  addSystemMessage: (content: string) => void;
  setConnectionStatus: (engine: boolean, aries: boolean) => void;
  setFps: (fps: number) => void;

  // Asset Management
  generatedAssets: Array<{ id: string; name: string; type: string; category: string; data: string; thumbnail?: string; createdAt: number }>;
  addGeneratedAsset: (asset: { name: string; type: string; category: string; data: string; thumbnail?: string }) => string;
  getAssetsByCategory: (category: string) => Array<{ id: string; name: string; type: string; category: string; data: string; thumbnail?: string; createdAt: number }>;

  // Scene Management
  newScene: () => void;
  saveScene: () => void;
  saveSceneAs: () => void;
  loadSceneFromFile: () => void;
  exportScene: () => void;
  importScene: () => void;

  // Undo/Redo
  undo: () => void;
  redo: () => void;

  // Selection
  duplicateSelected: () => void;
  selectAll: () => void;
}
