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
