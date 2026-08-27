import { spawn } from 'node:child_process';

const root = process.cwd();
const bridgePort = 8789;
const editorPort = 4180;
const children = [];
function start(command, args, env) {
  const child = spawn(command, args, { cwd: root, env: { ...process.env, ...env }, stdio: ['ignore', 'pipe', 'pipe'], detached: true });
  child.stdout.on('data', (data) => process.stdout.write(`[${args[0]}] ${data}`));
  child.stderr.on('data', (data) => process.stderr.write(`[${args[0]}] ${data}`));
  children.push(child);
  return child;
}
function stopAll() { for (const child of children) { try { process.kill(-child.pid, 'SIGTERM'); } catch { child.kill('SIGTERM'); } } }
async function waitFor(url, attempts = 50) { for (let attempt = 0; attempt < attempts; attempt += 1) { try { const response = await fetch(url); if (response.status < 500) return; } catch {} await new Promise((resolve) => setTimeout(resolve, 100)); } throw new Error(`timeout waiting for ${url}`); }
try {
  start('node', ['tools/scene-bridge.mjs', '--port', String(bridgePort)], {});
  await waitFor(`http://127.0.0.1:${bridgePort}/scene`);
  start('npm', ['run', 'dev', '--', '--host', '127.0.0.1', '--port', String(editorPort)], { VITE_SCENE_BRIDGE_URL: `http://127.0.0.1:${bridgePort}` });
  await waitFor(`http://127.0.0.1:${editorPort}/`);
  const bridge = spawn('npm', ['run', 'test:bridge'], { cwd: root, env: { ...process.env, BRIDGE_URL: `http://127.0.0.1:${bridgePort}` }, stdio: 'inherit' });
  if ((await new Promise((resolve) => bridge.on('close', resolve))) !== 0) throw new Error('bridge smoke failed');
  const browser = spawn('npm', ['run', 'test:browser'], { cwd: root, env: { ...process.env, EDITOR_URL: `http://127.0.0.1:${editorPort}` }, stdio: 'inherit' });
  if ((await new Promise((resolve) => browser.on('close', resolve))) !== 0) throw new Error('browser smoke failed');
  const receipt = await (await fetch(`http://127.0.0.1:${bridgePort}/scene`)).json();
  if (receipt.receipt?.source !== 'runtime-bridge') throw new Error('browser did not commit to runtime bridge');
  console.log(`EDITOR_ACCEPTANCE_OK bridge=${receipt.receipt.source} revision=${receipt.receipt.revision} actors=${receipt.receipt.actorCount}`);
} finally { stopAll(); }
