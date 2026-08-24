import React, { useState, useEffect } from 'react';

interface SettingsDialogProps {
  isOpen: boolean;
  onClose: () => void;
}

const STORAGE_KEY = 'neoengine_settings';

interface NeoSettings {
  apiKey: string;
  apiProvider: 'github' | 'openai' | 'anthropic' | 'custom';
  apiEndpoint: string;
  editorTheme: 'dark' | 'light';
  autoSaveInterval: number;
  telemetryHost: string;
  telemetryPort: number;
  ariesAutoConnect: boolean;
  renderQuality: 'low' | 'medium' | 'high' | 'ultra';
  targetFPS: number;
}

const DEFAULT_SETTINGS: NeoSettings = {
  apiKey: '',
  apiProvider: 'github',
  apiEndpoint: 'https://models.inference.ai.azure.com',
  editorTheme: 'dark',
  autoSaveInterval: 300,
  telemetryHost: '127.0.0.1',
  telemetryPort: 9876,
  ariesAutoConnect: true,
  renderQuality: 'high',
  targetFPS: 60,
};

function loadSettings(): NeoSettings {
  try {
    const saved = localStorage.getItem(STORAGE_KEY);
    if (saved) return { ...DEFAULT_SETTINGS, ...JSON.parse(saved) };
  } catch { /* ignore */ }
  return { ...DEFAULT_SETTINGS };
}

function saveSettings(settings: NeoSettings) {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(settings));
}

// Export for use by other modules
export function getApiKey(): string {
  return loadSettings().apiKey;
}

export function getApiProvider(): string {
  return loadSettings().apiProvider;
}

export function getApiEndpoint(): string {
  return loadSettings().apiEndpoint;
}

export function SettingsDialog({ isOpen, onClose }: SettingsDialogProps) {
  const [settings, setSettings] = useState<NeoSettings>(DEFAULT_SETTINGS);
  const [activeTab, setActiveTab] = useState<'api' | 'editor' | 'engine'>('api');
  const [showApiKey, setShowApiKey] = useState(false);
  const [saved, setSaved] = useState(false);

  useEffect(() => {
    if (isOpen) {
      setSettings(loadSettings());
      setSaved(false);
    }
  }, [isOpen]);

  if (!isOpen) return null;

  const update = (partial: Partial<NeoSettings>) => {
    setSettings(prev => ({ ...prev, ...partial }));
    setSaved(false);
  };

  const handleSave = () => {
    saveSettings(settings);
    setSaved(true);
    setTimeout(() => setSaved(false), 2000);
  };

  const tabs = [
    { id: 'api' as const, label: 'API Keys' },
    { id: 'editor' as const, label: 'Editor' },
    { id: 'engine' as const, label: 'Engine' },
  ];

  const providers = [
    { value: 'github', label: 'GitHub Models', endpoint: 'https://models.inference.ai.azure.com' },
    { value: 'openai', label: 'OpenAI', endpoint: 'https://api.openai.com/v1' },
    { value: 'anthropic', label: 'Anthropic', endpoint: 'https://api.anthropic.com/v1' },
    { value: 'custom', label: 'Custom Endpoint', endpoint: '' },
  ];

  return (
    <div style={{
      position: 'fixed', inset: 0, zIndex: 10000,
      background: 'rgba(0,0,0,0.6)', display: 'flex',
      alignItems: 'center', justifyContent: 'center',
    }} onClick={(e) => { if (e.target === e.currentTarget) onClose(); }}>
      <div style={{
        background: '#2a2a2a', borderRadius: 8, width: 560, maxHeight: '80vh',
        border: '1px solid #444', overflow: 'hidden', display: 'flex', flexDirection: 'column',
      }}>
        {/* Header */}
        <div style={{
          display: 'flex', justifyContent: 'space-between', alignItems: 'center',
          padding: '12px 16px', borderBottom: '1px solid #444', background: '#333',
        }}>
          <span style={{ color: '#ddd', fontWeight: 700, fontSize: 14 }}>Settings</span>
          <button onClick={onClose} style={{
            background: 'none', border: 'none', color: '#888', fontSize: 18,
            cursor: 'pointer', padding: '0 4px',
          }}>&times;</button>
        </div>

        {/* Tabs */}
        <div style={{ display: 'flex', borderBottom: '1px solid #444', background: '#2e2e2e' }}>
          {tabs.map(tab => (
            <button key={tab.id} onClick={() => setActiveTab(tab.id)} style={{
              padding: '8px 20px', background: activeTab === tab.id ? '#3a3a3a' : 'transparent',
              border: 'none', borderBottom: activeTab === tab.id ? '2px solid #4a9eff' : '2px solid transparent',
              color: activeTab === tab.id ? '#fff' : '#888', cursor: 'pointer', fontSize: 12,
            }}>{tab.label}</button>
          ))}
        </div>

        {/* Content */}
        <div style={{ padding: 16, overflowY: 'auto', flex: 1 }}>
          {activeTab === 'api' && (
            <div style={{ display: 'flex', flexDirection: 'column', gap: 16 }}>
              <div>
                <label style={labelStyle}>AI Provider</label>
                <select value={settings.apiProvider} onChange={(e) => {
                  const provider = providers.find(p => p.value === e.target.value);
                  update({
                    apiProvider: e.target.value as NeoSettings['apiProvider'],
                    apiEndpoint: provider?.endpoint || settings.apiEndpoint,
                  });
                }} style={inputStyle}>
                  {providers.map(p => (
                    <option key={p.value} value={p.value}>{p.label}</option>
                  ))}
                </select>
              </div>

              <div>
                <label style={labelStyle}>API Key</label>
                <div style={{ display: 'flex', gap: 8 }}>
                  <input
                    type={showApiKey ? 'text' : 'password'}
                    value={settings.apiKey}
                    onChange={(e) => update({ apiKey: e.target.value })}
                    placeholder="Paste your API key here..."
                    style={{ ...inputStyle, flex: 1 }}
                  />
                  <button onClick={() => setShowApiKey(!showApiKey)} style={{
                    ...btnStyle, width: 60, fontSize: 11,
                  }}>{showApiKey ? 'Hide' : 'Show'}</button>
                </div>
                <span style={hintStyle}>
                  {settings.apiProvider === 'github'
                    ? 'Get your GitHub token from github.com/settings/tokens'
                    : settings.apiProvider === 'openai'
                    ? 'Get your key from platform.openai.com/api-keys'
                    : settings.apiProvider === 'anthropic'
                    ? 'Get your key from console.anthropic.com/settings/keys'
                    : 'Enter your custom API key'}
                </span>
              </div>

              <div>
                <label style={labelStyle}>API Endpoint</label>
                <input
                  type="text"
                  value={settings.apiEndpoint}
                  onChange={(e) => update({ apiEndpoint: e.target.value })}
                  placeholder="https://api.example.com/v1"
                  style={inputStyle}
                  disabled={settings.apiProvider !== 'custom'}
                />
              </div>

              <div style={{
                background: '#1e1e1e', padding: 12, borderRadius: 6,
                border: '1px solid #333', fontSize: 11, color: '#888',
              }}>
                <strong style={{ color: '#aaa' }}>Note:</strong> API key is stored in your browser's localStorage.
                It is never sent to our servers. The key is only used for direct API calls to the selected provider.
                Without an API key, Aries AI will run in offline mode with basic pattern matching.
              </div>
            </div>
          )}

          {activeTab === 'editor' && (
            <div style={{ display: 'flex', flexDirection: 'column', gap: 16 }}>
              <div>
                <label style={labelStyle}>Theme</label>
                <select value={settings.editorTheme} onChange={(e) =>
                  update({ editorTheme: e.target.value as 'dark' | 'light' })
                } style={inputStyle}>
                  <option value="dark">Dark (Unreal Style)</option>
                  <option value="light">Light</option>
                </select>
              </div>
              <div>
                <label style={labelStyle}>Auto-Save Interval (seconds)</label>
                <input type="number" value={settings.autoSaveInterval}
                  onChange={(e) => update({ autoSaveInterval: parseInt(e.target.value) || 300 })}
                  min={30} max={3600} style={inputStyle} />
              </div>
            </div>
          )}

          {activeTab === 'engine' && (
            <div style={{ display: 'flex', flexDirection: 'column', gap: 16 }}>
              <div>
                <label style={labelStyle}>Render Quality</label>
                <select value={settings.renderQuality} onChange={(e) =>
                  update({ renderQuality: e.target.value as NeoSettings['renderQuality'] })
                } style={inputStyle}>
                  <option value="low">Low</option>
                  <option value="medium">Medium</option>
                  <option value="high">High</option>
                  <option value="ultra">Ultra</option>
                </select>
              </div>
              <div>
                <label style={labelStyle}>Target FPS</label>
                <select value={settings.targetFPS} onChange={(e) =>
                  update({ targetFPS: parseInt(e.target.value) })
                } style={inputStyle}>
                  <option value={30}>30 FPS</option>
                  <option value={60}>60 FPS</option>
                  <option value={120}>120 FPS</option>
                  <option value={144}>144 FPS</option>
                </select>
              </div>
              <div>
                <label style={labelStyle}>Aries Telemetry Host</label>
                <input type="text" value={settings.telemetryHost}
                  onChange={(e) => update({ telemetryHost: e.target.value })}
                  style={inputStyle} />
              </div>
              <div>
                <label style={labelStyle}>Aries Telemetry Port</label>
                <input type="number" value={settings.telemetryPort}
                  onChange={(e) => update({ telemetryPort: parseInt(e.target.value) || 9876 })}
                  min={1024} max={65535} style={inputStyle} />
              </div>
              <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
                <input type="checkbox" checked={settings.ariesAutoConnect}
                  onChange={(e) => update({ ariesAutoConnect: e.target.checked })}
                  id="ariesAutoConnect" />
                <label htmlFor="ariesAutoConnect" style={{ color: '#ccc', fontSize: 12 }}>
                  Auto-connect to Aries AI on startup
                </label>
              </div>
            </div>
          )}
        </div>

        {/* Footer */}
        <div style={{
          display: 'flex', justifyContent: 'flex-end', gap: 8,
          padding: '12px 16px', borderTop: '1px solid #444', background: '#2e2e2e',
        }}>
          {saved && <span style={{ color: '#4ec949', fontSize: 12, alignSelf: 'center', marginRight: 8 }}>Saved!</span>}
          <button onClick={onClose} style={{ ...btnStyle, background: '#444' }}>Cancel</button>
          <button onClick={handleSave} style={{ ...btnStyle, background: '#4a9eff' }}>Save</button>
        </div>
      </div>
    </div>
  );
}

const labelStyle: React.CSSProperties = {
  display: 'block', color: '#aaa', fontSize: 11, marginBottom: 4, fontWeight: 600,
};

const inputStyle: React.CSSProperties = {
  width: '100%', padding: '8px 10px', background: '#1e1e1e', border: '1px solid #444',
  borderRadius: 4, color: '#ddd', fontSize: 12, outline: 'none', boxSizing: 'border-box',
};

const btnStyle: React.CSSProperties = {
  padding: '6px 16px', border: 'none', borderRadius: 4, color: '#fff',
  cursor: 'pointer', fontSize: 12,
};

const hintStyle: React.CSSProperties = {
  display: 'block', color: '#666', fontSize: 10, marginTop: 4,
};
