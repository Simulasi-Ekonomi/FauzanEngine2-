/**
 * Aries Brain - Frontend Interface
 * Hybrid: GitHub Models API (online) + Gemma4 LiteRT (offline)
 * Agents: Aries (Director), Hermes (Narrative), Ruflo (World Builder)
 */

export type AgentName = 'aries' | 'hermes' | 'ruflo';

export interface GameProgress {
  phase: string;
  message: string;
  agent?: string;
  task?: string;
  progress?: string;
  data?: any;
  token?: string;
}

export interface AriesConfig {
  apiKey?: string;        // GitHub Models API token
  backendUrl?: string;    // Backend URL
  useLocal?: boolean;     // Pakai Gemma4 lokal
}

class AriesBrainClass {
  private config: AriesConfig = {};
  private ws: WebSocket | null = null;
  private sessionId: string;
  private isOnline = false;

  // Callbacks
  public onToken: ((agent: AgentName, token: string) => void) | null = null;
  public onProgress: ((update: GameProgress) => void) | null = null;
  public onStatusChange: ((status: string, online: boolean) => void) | null = null;

  constructor() {
    this.sessionId = `session_${Date.now()}`;
  }

  // =============================================
  // INIT
  // =============================================

  async initialize(config: AriesConfig) {
    this.config = config;

    // Test koneksi backend
    try {
      const url = config.backendUrl || 'http://localhost:8000';
      const resp = await fetch(`${url}/health`, { signal: AbortSignal.timeout(3000) });
      if (resp.ok) {
        this.isOnline = true;
        this.onStatusChange?.('Aries online (GitHub Models)', true);
        return;
      }
    } catch {}

    // Coba Android bridge (on-device Gemma4)
    if (typeof (window as any).NeoEngineBridge !== 'undefined') {
      (window as any).NeoEngineBridge.initAries();
      this.onStatusChange?.('Aries online (Gemma4 lokal)', true);
      this.setupAndroidCallbacks();
      return;
    }

    this.onStatusChange?.('Aries offline - set API key di Settings', false);
  }

  private setupAndroidCallbacks() {
    // Setup callback dari Android bridge
    (window as any).onAriesStatus = (status: string) => {
      this.onStatusChange?.(status, true);
    };

    (window as any).neoCallback = (id: string, data: string, done: boolean) => {
      if (done) {
        this.onToken?.('aries', data);
      }
    };

    (window as any).neoToken = (id: string, token: string) => {
      this.onToken?.('aries', token);
    };

    // === TAMBAHAN: Handler untuk command dari native (AI tool) ===
    (window as any).onAriesCommand = (action: string, dataJson: string) => {
      this.handleNativeCommand(action, dataJson);
    };
  }

  // === TAMBAHAN: Method untuk menangani command dari native ===
  private async handleNativeCommand(action: string, dataJson: string) {
    try {
      const data = JSON.parse(dataJson);
      console.log(`[AriesBrain] Native command: ${action}`, data);

      switch (action) {
        case 'ADD_ACTOR':
        case 'SET_TRANSFORM':
        case 'DELETE_ACTOR':
        case 'SET_MATERIAL':
          // Aksi ini sudah ditangani langsung oleh JNI, cukup log
          break;
        case 'GENERATE_3D':
          await this.generate3DAsset(data.description, data.type);
          break;
        case 'GENERATE_SPRITE':
          await this.generateSprite(data.description, data.width, data.height, data.style);
          break;
        case 'GENERATE_ANIMATED_CHAR':
          await this.generateAnimatedCharacter(data.description, data.animations);
          break;
        case 'CREATE_NPC':
          await this.createNPC(data.name, data.role, data.behavior, data.x, data.y, data.z);
          break;
        case 'GENERATE_WORLD_MAP':
        case 'GENERATE_MASSIVE_WORLD':
          await this.generateMassiveWorld(data.theme || data.description, data.sizeKm);
          break;
        case 'SET_PLAY_MODE':
          this.onProgress?.({ phase: 'play', message: data.playing ? 'Game started' : 'Game stopped' });
          break;
        case 'CREATE_WORLD':
          await this.createGameWorld(data.description);
          break;
        default:
          console.warn('[AriesBrain] Unhandled command:', action);
      }
    } catch (e) {
      console.error('[AriesBrain] Command error:', e);
    }
  }

  // === TAMBAHAN: Advanced Asset Generation Methods ===
  private async generate3DAsset(description: string, type: string) {
    this.onProgress?.({ phase: 'generating', message: `Generating 3D ${type}: ${description}` });
    try {
      const resp = await fetch(`${this.config.backendUrl || 'http://localhost:8000'}/api/generate-3d`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ prompt: description, type })
      });
      const result = await resp.json();
      this.onProgress?.({ phase: 'generated', message: `3D asset ready: ${result.file}` });
    } catch (e) {
      this.onProgress?.({ phase: 'error', message: `3D generation failed: ${e}` });
    }
  }

  private async generateSprite(desc: string, w: number, h: number, style: string) {
    this.onProgress?.({ phase: 'generating', message: `Generating sprite: ${desc}` });
    try {
      const resp = await fetch(`${this.config.backendUrl || 'http://localhost:8000'}/api/generate-sprite`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ prompt: desc, width: w, height: h, style })
      });
      const blob = await resp.blob();
      const url = URL.createObjectURL(blob);
      this.onProgress?.({ phase: 'generated', message: `Sprite ready`, data: { url } });
    } catch (e) {
      this.onProgress?.({ phase: 'error', message: `Sprite generation failed` });
    }
  }

  private async generateAnimatedCharacter(desc: string, animations: string) {
    this.onProgress?.({ phase: 'generating', message: `Generating animated character: ${desc}` });
    // Akan diproses backend, untuk sekarang placeholder
    setTimeout(() => this.onProgress?.({ phase: 'generated', message: `Character ready` }), 2000);
  }

  private async createNPC(name: string, role: string, behavior: string, x: number, y: number, z: number) {
    const bridge = (window as any).NeoEngineBridge;
    if (bridge?.nativeAddActor) {
      bridge.nativeAddActor('skeletal_mesh', name, x, y, z);
    }
    this.onProgress?.({ phase: 'npc', message: `NPC ${name} (${role}) created at (${x},${y},${z})` });
  }

  private async createGameWorld(description: string) {
    this.onProgress?.({ phase: 'world', message: `Creating world: ${description}` });
    const resp = await fetch(`${this.config.backendUrl || 'http://localhost:8000'}/game-director/create-world`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ prompt: description, session_id: this.sessionId })
    });
    const data = await resp.json();
    this.onProgress?.({ phase: 'world', message: `World creation started`, data });
  }

  private async generateMassiveWorld(theme: string, sizeKm: number) {
    this.onProgress?.({ phase: 'world', message: `Generating ${sizeKm}x${sizeKm} km world: ${theme}` });
    try {
      const resp = await fetch(`${this.config.backendUrl || 'http://localhost:8000'}/api/generate-massive-world`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ theme, sizeKm })
      });
      const meta = await resp.json();
      this.onProgress?.({ phase: 'world', message: `World precomputed. Seed: ${meta.seed}` });
      const bridge = (window as any).NeoEngineBridge;
      if (bridge?.startWorldStreaming) {
        bridge.startWorldStreaming(meta.seed, meta.sizeKm);
      }
    } catch (e) {
      this.onProgress?.({ phase: 'error', message: `World generation failed` });
    }
  }

  // =============================================
  // CHAT - chat dengan agent tertentu
  // =============================================

  async chat(message: string, agent: AgentName = 'aries'): Promise<void> {
    if (this.isOnline) {
      await this.chatOnline(message, agent);
    } else {
      await this.chatOffline(message);
    }
  }

  private async chatOnline(message: string, agent: AgentName) {
    const url = `${this.config.backendUrl || 'http://localhost:8000'}/game-director/chat`;

    try {
      const resp = await fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ message, session_id: this.sessionId })
      });

      if (!resp.body) return;
      const reader = resp.body.getReader();
      const decoder = new TextDecoder();

      while (true) {
        const { done, value } = await reader.read();
        if (done) break;

        const text = decoder.decode(value);
        const lines = text.split('\n');

        for (const line of lines) {
          if (line.startsWith('data: ') && line !== 'data: [DONE]') {
            try {
              const data = JSON.parse(line.slice(6));
              if (data.token) this.onToken?.(agent, data.token);
            } catch {}
          }
        }
      }
    } catch (e) {
      this.onToken?.(agent, `Error: ${e}`);
    }
  }

  private chatOffline(message: string) {
    return new Promise<void>((resolve) => {
      const bridge = (window as any).NeoEngineBridge;
      if (!bridge) {
        this.onToken?.('aries', 'Aries tidak tersedia. Set API key di Settings.');
        resolve();
        return;
      }

      const callbackId = `cb_${Date.now()}`;
      const origCallback = (window as any).neoCallback;

      (window as any).neoCallback = (id: string, data: string, done: boolean) => {
        if (id === callbackId) {
          this.onToken?.('aries', data);
          if (done) {
            (window as any).neoCallback = origCallback;
            resolve();
          }
        } else {
          origCallback?.(id, data, done);
        }
      };

      bridge.sendToAries(message, callbackId);
    });
  }

  // =============================================
  // CREATE GAME - orchestrate semua agent
  // =============================================

  async createGame(prompt: string): Promise<void> {
    const wsUrl = `${(this.config.backendUrl || 'http://localhost:8000')
      .replace('http', 'ws')}/game-director/create-game`;

    return new Promise((resolve, reject) => {
      this.ws = new WebSocket(wsUrl);

      this.ws.onopen = () => {
        this.ws?.send(JSON.stringify({
          prompt,
          session_id: this.sessionId
        }));
        this.onProgress?.({ phase: 'connecting', message: 'Terhubung ke Aries...' });
      };

      this.ws.onmessage = (event) => {
        try {
          const update: GameProgress = JSON.parse(event.data);
          this.onProgress?.(update);

          // Stream token ke UI jika ada
          if (update.token) {
            const agent = (update.agent as AgentName) || 'aries';
            this.onToken?.(agent, update.token);
          }

          if (update.phase === 'done' || update.phase === 'complete') {
            this.ws?.close();
            resolve();
          }

          if ((update as any).error) {
            reject(new Error((update as any).error));
          }
        } catch {}
      };

      this.ws.onerror = (e) => reject(e);
      this.ws.onclose = () => resolve();
    });
  }

  stopGeneration() {
    this.ws?.close();
  }

  // =============================================
  // QUICK ACTIONS
  // =============================================

  async analyzeScene(sceneData: any): Promise<void> {
    const prompt = `Analisa scene ini dan berikan saran:\n${JSON.stringify(sceneData, null, 2)}`;
    await this.chat(prompt, 'aries');
  }

  async generateCode(task: string): Promise<void> {
    if (typeof (window as any).NeoEngineBridge !== 'undefined') {
      const callbackId = `code_${Date.now()}`;
      (window as any).NeoEngineBridge.generateCode(task, callbackId);
    } else {
      await this.chat(`Buat code C++ untuk NeoEngine: ${task}`, 'aries');
    }
  }

  // =============================================
  // STATUS
  // =============================================

  get online() { return this.isOnline; }

  async getStatus() {
    try {
      const url = `${this.config.backendUrl || 'http://localhost:8000'}/game-director/status/${this.sessionId}`;
      const resp = await fetch(url);
      return resp.json();
    } catch {
      return { status: 'offline' };
    }
  }
}

export const AriesBrain = new AriesBrainClass();
export default AriesBrain;

// LEGACY EXPORTS - sync versions untuk kompatibilitas
export function storeDocument(filename: string, content: string): void {
  // Fire and forget
  AriesBrain.chat(`Simpan dokumen "${filename}": ${content.substring(0, 500)}`).catch(() => {});
}

export function processDocumentForAries(
  filename: string, content: string
): { response: string; actions: Record<string, unknown>[] } {
  // Trigger async tapi return sync placeholder
  AriesBrain.chat(`Analisis dokumen "${filename}": ${content.substring(0, 500)}`).catch(() => {});
  return {
    response: `Memproses dokumen "${filename}"... Aries sedang menganalisis.`,
    actions: []
  };
}

export function processWithAriesBrain(
  prompt: string, _context?: unknown
): { response: string; actions: Record<string, unknown>[] } {
  // Trigger async tapi return sync placeholder
  AriesBrain.chat(prompt).catch(() => {});
  return {
    response: `Aries memproses: ${prompt.substring(0, 100)}...`,
    actions: []
  };
}

export function queryLLM(
  prompt: string, _context?: unknown
): { response: string; actions: Record<string, unknown>[] } {
  return processWithAriesBrain(prompt, _context);
}
