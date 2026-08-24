import React, { useState, useRef, useEffect } from 'react';
import { useEditorStore } from '../../stores/editorStore';

export function AIConsole() {
  const chatMessages = useEditorStore((s) => s.chatMessages);
  const isAiThinking = useEditorStore((s) => s.isAiThinking);
  const sendMessage = useEditorStore((s) => s.sendMessage);
  const [input, setInput] = useState('');
  const messagesEndRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [chatMessages]);

  // Watch for special command responses
  useEffect(() => {
    if (chatMessages.length === 0) return;
    const lastMsg = chatMessages[chatMessages.length - 1];
    if (lastMsg.role === 'assistant' && lastMsg.content.startsWith('__')) {
      const handler = (window as any).__neoHandleSpecialCommand;
      if (handler) handler(lastMsg.content);
    }
  }, [chatMessages]);

  const handleSend = () => {
    if (!input.trim()) return;
    sendMessage(input.trim());
    setInput('');
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      handleSend();
    }
  };

  // Filter out special command messages from display
  const displayMessages = chatMessages.filter(msg =>
    !(msg.role === 'assistant' && msg.content.startsWith('__'))
  );

  return (
    <div className="neo-panel" style={{ height: '100%' }}>
      <div className="neo-panel-header">
        <span className="panel-icon">🤖</span>
        Aries AI Console
        {isAiThinking && (
          <span style={{ marginLeft: 8, color: '#0078d4', fontSize: 10, fontWeight: 400 }}>
            thinking...
          </span>
        )}
      </div>
      <div className="ai-console">
        <div className="ai-messages">
          {displayMessages.map((msg) => (
            <div key={msg.id} className={`ai-message ${msg.role}`}>
              {msg.role === 'user' && (
                <div style={{ fontSize: 9, color: '#88bbff', marginBottom: 2 }}>You</div>
              )}
              {msg.role === 'assistant' && (
                <div style={{ fontSize: 9, color: '#88ff88', marginBottom: 2 }}>Aries</div>
              )}
              <div style={{ whiteSpace: 'pre-wrap' }}>{msg.content}</div>
            </div>
          ))}
          {isAiThinking && (
            <div className="ai-message assistant">
              <div style={{ fontSize: 9, color: '#88ff88', marginBottom: 2 }}>Aries</div>
              <div style={{ color: '#888' }}>
                <span className="thinking-dots">Menganalisis perintah</span>
                <style>{`
                  .thinking-dots::after {
                    content: '...';
                    animation: dots 1.5s steps(4, end) infinite;
                  }
                  @keyframes dots {
                    0%, 20% { content: ''; }
                    40% { content: '.'; }
                    60% { content: '..'; }
                    80%, 100% { content: '...'; }
                  }
                `}</style>
              </div>
            </div>
          )}
          <div ref={messagesEndRef} />
        </div>
        <div className="ai-input-area">
          <input
            type="text"
            placeholder="Ketik: diskusi, analisa scene, ide game, monetisasi, buat rumah, help"
            value={input}
            onChange={(e) => setInput(e.target.value)}
            onKeyDown={handleKeyDown}
            disabled={isAiThinking}
          />
          <button onClick={handleSend} disabled={isAiThinking || !input.trim()}>
            Kirim
          </button>
        </div>
      </div>
    </div>
  );
}
