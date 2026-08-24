/**
 * NeoEngine Game Runtime - Browser-based game execution engine
 * Handles game logic, scripting, physics simulation, and play mode
 */

import type { NeoActor } from '../types/editor';

// Game script that can be attached to actors
export interface GameScript {
  id: string;
  name: string;
  actorId: string;
  code: string;
  enabled: boolean;
  compiled?: CompiledScript;
}

interface CompiledScript {
  onStart?: (ctx: ScriptContext) => void;
  onUpdate?: (ctx: ScriptContext, deltaTime: number) => void;
  onCollision?: (ctx: ScriptContext, other: string) => void;
  onInput?: (ctx: ScriptContext, key: string, pressed: boolean) => void;
  onDestroy?: (ctx: ScriptContext) => void;
}

// Context passed to scripts for interacting with the game world
export interface ScriptContext {
  actor: NeoActor;
  transform: {
    move: (x: number, y: number, z: number) => void;
    rotate: (x: number, y: number, z: number) => void;
    setPosition: (x: number, y: number, z: number) => void;
    getPosition: () => { x: number; y: number; z: number };
  };
  scene: {
    findActor: (name: string) => NeoActor | undefined;
    getAllActors: () => NeoActor[];
    spawn: (type: string, name: string, x: number, y: number, z: number) => string;
    destroy: (actorId: string) => void;
  };
  input: {
    isKeyDown: (key: string) => boolean;
    getMousePosition: () => { x: number; y: number };
  };
  time: {
    elapsed: number;
    delta: number;
  };
  ui: {
    showText: (text: string, x: number, y: number, size?: number, color?: string) => void;
    showScore: (score: number) => void;
    showHealth: (health: number, max: number) => void;
  };
  state: Record<string, unknown>;
  log: (msg: string) => void;
}

// Game state during play mode
export interface GameState {
  running: boolean;
  paused: boolean;
  startTime: number;
  elapsed: number;
  score: number;
  scripts: GameScript[];
  uiElements: UIElement[];
  logs: string[];
  inputState: Record<string, boolean>;
  mousePosition: { x: number; y: number };
  actorStates: Record<string, Record<string, unknown>>;
}

export interface UIElement {
  id: string;
  type: 'text' | 'score' | 'health';
  text?: string;
  x: number;
  y: number;
  size?: number;
  color?: string;
  score?: number;
  health?: number;
  maxHealth?: number;
}

// Pre-built game templates with actual game logic
export const GAME_TEMPLATES: Record<string, { scripts: Array<{ actorName: string; code: string }>; description: string }> = {
  sudoku: {
    description: 'Puzzle game - click cells to place numbers',
    scripts: [
      {
        actorName: 'GameBoard',
        code: `
// Sudoku Game Controller
let grid = [];
let selected = -1;
let solved = false;

function onStart(ctx) {
  ctx.state.score = 0;
  ctx.state.moves = 0;
  // Initialize 9x9 grid
  for (let i = 0; i < 81; i++) grid.push(0);
  // Set some starter numbers
  grid[0]=5; grid[1]=3; grid[4]=7;
  grid[9]=6; grid[12]=1; grid[13]=9; grid[14]=5;
  grid[19]=8; grid[25]=6;
  ctx.ui.showText("SUDOKU - Click cells to play", 50, 5, 18, "#00ff88");
  ctx.ui.showScore(0);
  ctx.log("Sudoku game started! Click cells to place numbers.");
}

function onUpdate(ctx, dt) {
  ctx.ui.showText("Moves: " + ctx.state.moves, 50, 90, 14, "#aaaaaa");
  if (solved) ctx.ui.showText("PUZZLE SOLVED!", 50, 50, 24, "#ffcc00");
}

function onInput(ctx, key, pressed) {
  if (!pressed) return;
  if (key >= "1" && key <= "9" && selected >= 0) {
    grid[selected] = parseInt(key);
    ctx.state.moves++;
    ctx.ui.showScore(ctx.state.moves);
    ctx.log("Placed " + key + " at cell " + selected);
  }
}`,
      },
    ],
  },
  farmville: {
    description: 'Farming simulation - plant, water, harvest crops',
    scripts: [
      {
        actorName: 'FarmLand',
        code: `
// Farm Controller
let crops = {};
let money = 100;
let day = 1;

function onStart(ctx) {
  ctx.state.money = money;
  ctx.state.day = day;
  ctx.ui.showText("FARMVILLE - Your Farm", 50, 3, 18, "#4ec949");
  ctx.ui.showScore(money);
  ctx.log("Welcome to your farm! Commands: plant, water, harvest");
  ctx.log("Starting money: $" + money);
}

function onUpdate(ctx, dt) {
  // Grow crops over time
  for (let id in crops) {
    if (crops[id].watered && crops[id].growth < 100) {
      crops[id].growth += dt * 2;
    }
  }
  ctx.ui.showText("Day " + day + " | Money: $" + money, 50, 5, 14, "#ffffff");
  ctx.ui.showText("Crops: " + Object.keys(crops).length + " planted", 50, 90, 12, "#aaaaaa");
}

function onInput(ctx, key, pressed) {
  if (!pressed) return;
  if (key === "p") { // Plant
    let plotId = "plot_" + Object.keys(crops).length;
    crops[plotId] = { type: "wheat", growth: 0, watered: false };
    money -= 10;
    ctx.ui.showScore(money);
    ctx.log("Planted wheat (-$10). Growth: 0%");
  }
  if (key === "w") { // Water
    for (let id in crops) { crops[id].watered = true; }
    ctx.log("Watered all crops!");
  }
  if (key === "h") { // Harvest
    let harvested = 0;
    for (let id in crops) {
      if (crops[id].growth >= 100) { harvested++; money += 30; delete crops[id]; }
    }
    if (harvested > 0) {
      ctx.ui.showScore(money);
      ctx.log("Harvested " + harvested + " crops! (+$" + (harvested * 30) + ")");
    } else {
      ctx.log("No crops ready to harvest yet.");
    }
  }
  if (key === "n") { day++; ctx.log("Advanced to day " + day); }
}`,
      },
    ],
  },
  fps: {
    description: 'First person shooter - move and shoot',
    scripts: [
      {
        actorName: 'SpawnPoint_A',
        code: `
// FPS Game Controller
let health = 100;
let ammo = 30;
let kills = 0;
let playerPos = { x: 0, y: 1, z: 0 };
let speed = 5;

function onStart(ctx) {
  ctx.state.health = health;
  ctx.state.ammo = ammo;
  ctx.ui.showHealth(health, 100);
  ctx.ui.showText("FPS ARENA", 50, 3, 18, "#ff4444");
  ctx.ui.showScore(kills);
  ctx.log("FPS Mode! WASD to move, SPACE to shoot, R to reload");
}

function onUpdate(ctx, dt) {
  // Move with WASD
  if (ctx.input.isKeyDown("w")) { playerPos.z -= speed * dt; ctx.transform.move(0, 0, -speed * dt); }
  if (ctx.input.isKeyDown("s")) { playerPos.z += speed * dt; ctx.transform.move(0, 0, speed * dt); }
  if (ctx.input.isKeyDown("a")) { playerPos.x -= speed * dt; ctx.transform.move(-speed * dt, 0, 0); }
  if (ctx.input.isKeyDown("d")) { playerPos.x += speed * dt; ctx.transform.move(speed * dt, 0, 0); }

  ctx.ui.showText("HP: " + health + " | Ammo: " + ammo + "/30 | Kills: " + kills, 50, 92, 14, "#ffffff");
  ctx.ui.showHealth(health, 100);
}

function onInput(ctx, key, pressed) {
  if (!pressed) return;
  if (key === " " && ammo > 0) {
    ammo--;
    ctx.log("BANG! Ammo: " + ammo);
    // Random hit chance
    if (Math.random() > 0.5) { kills++; ctx.ui.showScore(kills); ctx.log("Enemy eliminated! Kills: " + kills); }
  }
  if (key === "r") { ammo = 30; ctx.log("Reloaded! Ammo: 30"); }
}`,
      },
    ],
  },
  platformer: {
    description: 'Side-scrolling platformer - jump and collect',
    scripts: [
      {
        actorName: 'PlayerSpawn',
        code: `
// Platformer Controller
let posY = 1;
let velY = 0;
let grounded = true;
let coins = 0;
let lives = 3;
let posX = 0;

function onStart(ctx) {
  ctx.ui.showText("PLATFORMER", 50, 3, 18, "#ffcc00");
  ctx.ui.showScore(coins);
  ctx.ui.showHealth(lives, 3);
  ctx.log("Platformer! A/D to move, SPACE to jump, collect coins!");
}

function onUpdate(ctx, dt) {
  // Gravity
  if (!grounded) {
    velY -= 15 * dt;
    posY += velY * dt;
    ctx.transform.setPosition(posX, posY, 0);
  }
  if (posY <= 1) { posY = 1; velY = 0; grounded = true; ctx.transform.setPosition(posX, 1, 0); }

  // Movement
  if (ctx.input.isKeyDown("a") || ctx.input.isKeyDown("ArrowLeft")) { posX -= 4 * dt; ctx.transform.setPosition(posX, posY, 0); }
  if (ctx.input.isKeyDown("d") || ctx.input.isKeyDown("ArrowRight")) { posX += 4 * dt; ctx.transform.setPosition(posX, posY, 0); }

  ctx.ui.showText("Coins: " + coins + " | Lives: " + lives, 50, 92, 14, "#ffffff");
}

function onInput(ctx, key, pressed) {
  if (!pressed) return;
  if ((key === " " || key === "ArrowUp") && grounded) {
    velY = 8; grounded = false;
    ctx.log("Jump!");
  }
  if (key === "c") {
    coins += 10;
    ctx.ui.showScore(coins);
    ctx.log("Coin collected! Total: " + coins);
  }
}`,
      },
    ],
  },
  rpg: {
    description: 'RPG adventure - explore, fight, level up',
    scripts: [
      {
        actorName: 'HeroSpawn',
        code: `
// RPG Controller
let hero = { hp: 100, maxHp: 100, mp: 50, maxMp: 50, level: 1, exp: 0, atk: 10, def: 5 };
let inventory = ["Wooden Sword", "Health Potion x3"];
let inBattle = false;
let enemy = null;

function onStart(ctx) {
  ctx.ui.showHealth(hero.hp, hero.maxHp);
  ctx.ui.showText("RPG ADVENTURE", 50, 3, 18, "#8a4ec9");
  ctx.ui.showScore(hero.level);
  ctx.log("Welcome, Hero! Level " + hero.level);
  ctx.log("Inventory: " + inventory.join(", "));
  ctx.log("Press F to fight, H to heal, I for inventory");
}

function onUpdate(ctx, dt) {
  ctx.ui.showText("Lv." + hero.level + " | HP:" + hero.hp + "/" + hero.maxHp + " | MP:" + hero.mp + "/" + hero.maxMp + " | EXP:" + hero.exp, 50, 92, 12, "#ffffff");
  if (inBattle && enemy) {
    ctx.ui.showText("BATTLE: " + enemy.name + " HP:" + enemy.hp, 50, 50, 16, "#ff4444");
  }
  ctx.ui.showHealth(hero.hp, hero.maxHp);
}

function onInput(ctx, key, pressed) {
  if (!pressed) return;
  if (key === "f") {
    if (!inBattle) {
      let enemies = [{name:"Slime",hp:30,atk:5},{name:"Goblin",hp:50,atk:10},{name:"Wolf",hp:40,atk:8}];
      enemy = {...enemies[Math.floor(Math.random()*enemies.length)]};
      inBattle = true;
      ctx.log("A wild " + enemy.name + " appears! (HP:" + enemy.hp + ")");
    } else {
      // Attack
      let dmg = hero.atk + Math.floor(Math.random() * 5);
      enemy.hp -= dmg;
      ctx.log("You attack for " + dmg + " damage!");
      if (enemy.hp <= 0) {
        hero.exp += 25;
        ctx.log(enemy.name + " defeated! +25 EXP");
        if (hero.exp >= hero.level * 50) {
          hero.level++; hero.exp = 0; hero.maxHp += 20; hero.hp = hero.maxHp; hero.atk += 3;
          ctx.log("LEVEL UP! Now level " + hero.level);
          ctx.ui.showScore(hero.level);
        }
        inBattle = false; enemy = null;
      } else {
        let eDmg = Math.max(1, enemy.atk - hero.def + Math.floor(Math.random()*3));
        hero.hp -= eDmg;
        ctx.log(enemy.name + " hits you for " + eDmg + "!");
        if (hero.hp <= 0) { hero.hp = hero.maxHp; ctx.log("You were defeated... Respawned with full HP."); inBattle = false; enemy = null; }
      }
    }
  }
  if (key === "h" && hero.mp >= 10) {
    hero.mp -= 10; hero.hp = Math.min(hero.maxHp, hero.hp + 30);
    ctx.log("Healed for 30 HP! (MP: " + hero.mp + ")");
  }
  if (key === "i") { ctx.log("Inventory: " + inventory.join(", ")); }
}`,
      },
    ],
  },
};

// Compile a script string into callable functions
export function compileScript(code: string): CompiledScript | null {
  try {
    const fn = new Function('module', `
      const exports = {};
      ${code}
      return { onStart: typeof onStart !== 'undefined' ? onStart : undefined,
               onUpdate: typeof onUpdate !== 'undefined' ? onUpdate : undefined,
               onCollision: typeof onCollision !== 'undefined' ? onCollision : undefined,
               onInput: typeof onInput !== 'undefined' ? onInput : undefined,
               onDestroy: typeof onDestroy !== 'undefined' ? onDestroy : undefined };
    `);
    return fn({});
  } catch (e) {
    console.error('Script compilation error:', e);
    return null;
  }
}

// Game Runtime Engine
export class GameRuntime {
  private state: GameState;
  private actors: Record<string, NeoActor>;
  private updateCallback: (actors: Record<string, NeoActor>) => void;
  private uiCallback: (elements: UIElement[]) => void;
  private logCallback: (msg: string) => void;
  private animFrameId: number | null = null;
  private lastTime = 0;

  constructor(
    actors: Record<string, NeoActor>,
    scripts: GameScript[],
    updateCallback: (actors: Record<string, NeoActor>) => void,
    uiCallback: (elements: UIElement[]) => void,
    logCallback: (msg: string) => void,
  ) {
    this.actors = JSON.parse(JSON.stringify(actors));
    this.updateCallback = updateCallback;
    this.uiCallback = uiCallback;
    this.logCallback = logCallback;
    this.state = {
      running: false,
      paused: false,
      startTime: Date.now(),
      elapsed: 0,
      score: 0,
      scripts,
      uiElements: [],
      logs: [],
      inputState: {},
      mousePosition: { x: 0, y: 0 },
      actorStates: {},
    };

    // Compile all scripts
    for (const script of this.state.scripts) {
      script.compiled = compileScript(script.code) || undefined;
    }

    // Input handling
    this.handleKeyDown = this.handleKeyDown.bind(this);
    this.handleKeyUp = this.handleKeyUp.bind(this);
    this.handleMouseMove = this.handleMouseMove.bind(this);
  }

  private handleKeyDown(e: KeyboardEvent) {
    this.state.inputState[e.key] = true;
    for (const script of this.state.scripts) {
      if (script.compiled?.onInput && script.enabled) {
        const ctx = this.createContext(script.actorId);
        try { script.compiled.onInput(ctx, e.key, true); } catch (err) { this.logCallback(`[Script Error] ${err}`); }
      }
    }
  }

  private handleKeyUp(e: KeyboardEvent) {
    this.state.inputState[e.key] = false;
    for (const script of this.state.scripts) {
      if (script.compiled?.onInput && script.enabled) {
        const ctx = this.createContext(script.actorId);
        try { script.compiled.onInput(ctx, e.key, false); } catch (err) { this.logCallback(`[Script Error] ${err}`); }
      }
    }
  }

  private handleMouseMove(e: MouseEvent) {
    this.state.mousePosition = { x: e.clientX, y: e.clientY };
  }

  private createContext(actorId: string): ScriptContext {
    const actor = this.actors[actorId];
    if (!actor) {
      throw new Error(`Actor ${actorId} not found`);
    }

    const self = this;
    const state = this.state.actorStates[actorId] || {};
    this.state.actorStates[actorId] = state;

    return {
      actor,
      transform: {
        move: (x, y, z) => {
          actor.transform.position.x += x;
          actor.transform.position.y += y;
          actor.transform.position.z += z;
        },
        rotate: (x, y, z) => {
          actor.transform.rotation.x += x;
          actor.transform.rotation.y += y;
          actor.transform.rotation.z += z;
        },
        setPosition: (x, y, z) => {
          actor.transform.position.x = x;
          actor.transform.position.y = y;
          actor.transform.position.z = z;
        },
        getPosition: () => ({ ...actor.transform.position }),
      },
      scene: {
        findActor: (name) => Object.values(self.actors).find(a => a.name === name),
        getAllActors: () => Object.values(self.actors),
        spawn: (type, name, x, y, z) => {
          const id = `runtime_${Date.now()}_${Math.random().toString(36).slice(2)}`;
          self.actors[id] = {
            id, name, type: type as any,
            transform: { position: { x, y, z }, rotation: { x: 0, y: 0, z: 0 }, scale: { x: 1, y: 1, z: 1 } },
            visible: true, children: [], parentId: null, components: [],
          };
          return id;
        },
        destroy: (id) => { delete self.actors[id]; },
      },
      input: {
        isKeyDown: (key) => !!self.state.inputState[key],
        getMousePosition: () => ({ ...self.state.mousePosition }),
      },
      time: {
        elapsed: self.state.elapsed / 1000,
        delta: 0,
      },
      ui: {
        showText: (text, x, y, size = 14, color = '#ffffff') => {
          const id = `text_${x}_${y}`;
          const existing = self.state.uiElements.findIndex(e => e.id === id);
          const el: UIElement = { id, type: 'text', text, x, y, size, color };
          if (existing >= 0) self.state.uiElements[existing] = el;
          else self.state.uiElements.push(el);
        },
        showScore: (score) => {
          self.state.score = score;
          const id = 'score_display';
          const existing = self.state.uiElements.findIndex(e => e.id === id);
          const el: UIElement = { id, type: 'score', score, x: 95, y: 5 };
          if (existing >= 0) self.state.uiElements[existing] = el;
          else self.state.uiElements.push(el);
        },
        showHealth: (health, max) => {
          const id = 'health_display';
          const existing = self.state.uiElements.findIndex(e => e.id === id);
          const el: UIElement = { id, type: 'health', health, maxHealth: max, x: 5, y: 5 };
          if (existing >= 0) self.state.uiElements[existing] = el;
          else self.state.uiElements.push(el);
        },
      },
      state,
      log: (msg) => { self.logCallback(`[Game] ${msg}`); },
    };
  }

  start() {
    this.state.running = true;
    this.state.startTime = Date.now();
    this.lastTime = performance.now();

    window.addEventListener('keydown', this.handleKeyDown);
    window.addEventListener('keyup', this.handleKeyUp);
    window.addEventListener('mousemove', this.handleMouseMove);

    // Call onStart for all scripts
    for (const script of this.state.scripts) {
      if (script.compiled?.onStart && script.enabled) {
        const ctx = this.createContext(script.actorId);
        try { script.compiled.onStart(ctx); } catch (err) { this.logCallback(`[Script Error] ${err}`); }
      }
    }

    this.logCallback('[Engine] Game started! Press keys to interact.');
    this.gameLoop();
  }

  private gameLoop = () => {
    if (!this.state.running) return;

    const now = performance.now();
    const dt = Math.min((now - this.lastTime) / 1000, 0.1); // Cap dt at 100ms
    this.lastTime = now;
    this.state.elapsed = Date.now() - this.state.startTime;

    if (!this.state.paused) {
      // Call onUpdate for all scripts
      for (const script of this.state.scripts) {
        if (script.compiled?.onUpdate && script.enabled) {
          const ctx = this.createContext(script.actorId);
          ctx.time.delta = dt;
          try { script.compiled.onUpdate(ctx, dt); } catch (err) { this.logCallback(`[Script Error] ${err}`); }
        }
      }

      this.updateCallback({ ...this.actors });
      this.uiCallback([...this.state.uiElements]);
    }

    this.animFrameId = requestAnimationFrame(this.gameLoop);
  };

  pause() { this.state.paused = true; }
  resume() { this.state.paused = false; }

  stop() {
    this.state.running = false;
    if (this.animFrameId) cancelAnimationFrame(this.animFrameId);

    window.removeEventListener('keydown', this.handleKeyDown);
    window.removeEventListener('keyup', this.handleKeyUp);
    window.removeEventListener('mousemove', this.handleMouseMove);

    // Call onDestroy for all scripts
    for (const script of this.state.scripts) {
      if (script.compiled?.onDestroy && script.enabled) {
        const ctx = this.createContext(script.actorId);
        try { script.compiled.onDestroy(ctx); } catch (err) { /* ignore cleanup errors */ }
      }
    }

    this.logCallback('[Engine] Game stopped.');
  }

  getState(): GameState { return this.state; }
}
