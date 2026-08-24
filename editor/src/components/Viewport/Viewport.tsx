import React, { useRef, useState, useEffect, useMemo } from 'react';
import { Canvas, useFrame, useThree } from '@react-three/fiber';
import { OrbitControls, Grid, TransformControls, GizmoHelper, GizmoViewport, Environment, Stars, Plane, Sky } from '@react-three/drei';
import * as THREE from 'three';
import { useEditorStore } from '../../stores/editorStore';
import type { MaterialProperties } from '../../types/editor';

// Get material properties from actor components
function getActorMaterial(actor: any): MaterialProperties | null {
  const renderComp = actor.components?.find((c: any) => c.type === 'RenderMaterialComponent');
  if (renderComp?.material) return renderComp.material;
  // Check properties directly
  const props = renderComp?.properties || {};
  if (props.color) {
    return {
      color: props.color as string,
      roughness: (props.roughness as number) ?? 0.6,
      metalness: (props.metalness as number) ?? 0.2,
      emissive: (props.emissive as string) || '#000000',
      emissiveIntensity: (props.emissiveIntensity as number) ?? 0,
      opacity: (props.opacity as number) ?? 1,
      transparent: (props.transparent as boolean) ?? false,
      wireframe: (props.wireframe as boolean) ?? false,
    };
  }
  return null;
}

// Animated water shader material
function WaterSurface({ position, scale }: { position: [number, number, number]; scale: [number, number, number] }) {
  const meshRef = useRef<THREE.Mesh>(null);
  const materialRef = useRef<THREE.ShaderMaterial>(null);

  const uniforms = useMemo(() => ({
    uTime: { value: 0 },
    uColor: { value: new THREE.Color('#1a6eb5') },
    uColorDeep: { value: new THREE.Color('#0a3d6b') },
  }), []);

  useFrame((state) => {
    if (materialRef.current) {
      materialRef.current.uniforms.uTime.value = state.clock.elapsedTime;
    }
  });

  return (
    <mesh ref={meshRef} position={[position[0], position[1], position[2]]} rotation={[-Math.PI / 2, 0, 0]} scale={scale}>
      <planeGeometry args={[1, 1, 64, 64]} />
      <shaderMaterial
        ref={materialRef}
        uniforms={uniforms}
        transparent
        vertexShader={`
          uniform float uTime;
          varying vec2 vUv;
          varying float vElevation;
          void main() {
            vUv = uv;
            vec3 pos = position;
            float wave1 = sin(pos.x * 2.0 + uTime * 1.5) * 0.15;
            float wave2 = sin(pos.y * 3.0 + uTime * 1.0) * 0.1;
            float wave3 = sin((pos.x + pos.y) * 1.5 + uTime * 2.0) * 0.08;
            pos.z += wave1 + wave2 + wave3;
            vElevation = pos.z;
            gl_Position = projectionMatrix * modelViewMatrix * vec4(pos, 1.0);
          }
        `}
        fragmentShader={`
          uniform vec3 uColor;
          uniform vec3 uColorDeep;
          varying vec2 vUv;
          varying float vElevation;
          void main() {
            float mixFactor = smoothstep(-0.15, 0.15, vElevation);
            vec3 color = mix(uColorDeep, uColor, mixFactor);
            float foam = smoothstep(0.1, 0.15, vElevation) * 0.4;
            color += vec3(foam);
            gl_FragColor = vec4(color, 0.85);
          }
        `}
      />
    </mesh>
  );
}

// Foliage/grass instance renderer
function FoliageField({ position, scale }: { position: [number, number, number]; scale: [number, number, number] }) {
  const meshRef = useRef<THREE.InstancedMesh>(null);
  const count = 200;

  useEffect(() => {
    if (!meshRef.current) return;
    const dummy = new THREE.Object3D();
    const spreadX = Math.max(scale[0], 1);
    const spreadZ = Math.max(scale[2], 1);

    for (let i = 0; i < count; i++) {
      const x = (Math.random() - 0.5) * spreadX;
      const z = (Math.random() - 0.5) * spreadZ;
      const y = position[1];
      const s = 0.3 + Math.random() * 0.7;
      dummy.position.set(x, y, z);
      dummy.scale.set(s, s * (0.5 + Math.random()), s);
      dummy.rotation.y = Math.random() * Math.PI;
      dummy.updateMatrix();
      meshRef.current.setMatrixAt(i, dummy.matrix);
    }
    meshRef.current.instanceMatrix.needsUpdate = true;
  }, [position, scale]);

  return (
    <instancedMesh ref={meshRef} args={[undefined, undefined, count]} position={[position[0], 0, position[2]]}>
      <coneGeometry args={[0.15, 0.6, 4]} />
      <meshStandardMaterial color="#2d8a4e" roughness={0.9} />
    </instancedMesh>
  );
}

// Sprite billboard renderer
function SpriteActor({ actor }: { actor: any }) {
  const meshRef = useRef<THREE.Mesh>(null);
  const selectedActorId = useEditorStore((s) => s.selectedActorId);
  const selectActor = useEditorStore((s) => s.selectActor);
  const isSelected = selectedActorId === actor.id;

  const comp = actor.components?.find((c: any) => c.properties?.color);
  const color = (comp?.properties?.color as string) || '#ffffff';
  const width = (comp?.properties?.width as number) || 2;
  const height = (comp?.properties?.height as number) || 2;

  return (
    <mesh
      ref={meshRef}
      position={[actor.transform.position.x, actor.transform.position.y + height / 2, actor.transform.position.z]}
      onClick={(e) => { e.stopPropagation(); selectActor(actor.id); }}
    >
      <planeGeometry args={[width, height]} />
      <meshBasicMaterial color={color} transparent opacity={0.9} side={THREE.DoubleSide} />
      {isSelected && (
        <lineSegments>
          <edgesGeometry args={[new THREE.PlaneGeometry(width + 0.1, height + 0.1)]} />
          <lineBasicMaterial color="#ff8800" />
        </lineSegments>
      )}
    </mesh>
  );
}

// Tilemap renderer
function TilemapActor({ actor }: { actor: any }) {
  const meshRef = useRef<THREE.Group>(null);
  const selectedActorId = useEditorStore((s) => s.selectedActorId);
  const selectActor = useEditorStore((s) => s.selectActor);
  const isSelected = selectedActorId === actor.id;

  const comp = actor.components?.find((c: any) => c.type === 'TilemapComponent');
  const tileW = (comp?.properties?.tileWidth as number) || 1;
  const tileH = (comp?.properties?.tileHeight as number) || 1;
  const cols = (comp?.properties?.columns as number) || 4;
  const rows = (comp?.properties?.rows as number) || 4;

  const tileColors = ['#4a7c59', '#6b8f5e', '#8fbc5e', '#c4a94d', '#a0522d', '#556b2f', '#228b22'];

  return (
    <group
      ref={meshRef}
      position={[actor.transform.position.x, actor.transform.position.y, actor.transform.position.z]}
      onClick={(e) => { e.stopPropagation(); selectActor(actor.id); }}
    >
      {Array.from({ length: rows }).map((_, r) =>
        Array.from({ length: cols }).map((_, c) => {
          const colorIdx = Math.floor(Math.random() * tileColors.length);
          return (
            <mesh key={`${r}-${c}`} position={[c * tileW - (cols * tileW) / 2 + tileW / 2, 0, r * tileH - (rows * tileH) / 2 + tileH / 2]}>
              <boxGeometry args={[tileW - 0.02, 0.1, tileH - 0.02]} />
              <meshStandardMaterial color={tileColors[(r * cols + c) % tileColors.length]} roughness={0.8} />
            </mesh>
          );
        })
      )}
    </group>
  );
}

function SceneActor({ actor }: { actor: any }) {
  const meshRef = useRef<THREE.Mesh>(null);
  const selectedActorId = useEditorStore((s) => s.selectedActorId);
  const selectActor = useEditorStore((s) => s.selectActor);
  const isSelected = selectedActorId === actor.id;
  const mat = getActorMaterial(actor);

  // Extended geometry map with all new types
  const geometryMap: Record<string, JSX.Element> = {
    cube: <boxGeometry args={[1, 1, 1]} />,
    sphere: <sphereGeometry args={[0.5, 32, 32]} />,
    plane: <planeGeometry args={[1, 1]} />,
    cylinder: <cylinderGeometry args={[0.5, 0.5, 1, 32]} />,
    cone: <coneGeometry args={[0.5, 1, 32]} />,
    torus: <torusGeometry args={[0.4, 0.15, 16, 48]} />,
    ring: <torusGeometry args={[0.4, 0.05, 8, 48]} />,
    capsule: <capsuleGeometry args={[0.3, 0.5, 16, 32]} />,
    static_mesh: <boxGeometry args={[1, 1, 1]} />,
  };

  // Light types
  if (actor.type === 'light_directional') {
    return (
      <group position={[actor.transform.position.x, actor.transform.position.y, actor.transform.position.z]}>
        <directionalLight
          intensity={1.5}
          color={mat?.color || '#fffaf0'}
          castShadow
          shadow-mapSize={[2048, 2048]}
          shadow-camera-far={50}
          shadow-camera-left={-20}
          shadow-camera-right={20}
          shadow-camera-top={20}
          shadow-camera-bottom={-20}
        />
        {isSelected && (
          <mesh onClick={(e) => { e.stopPropagation(); selectActor(actor.id); }}>
            <sphereGeometry args={[0.3, 8, 8]} />
            <meshBasicMaterial color="#ffaa00" wireframe />
          </mesh>
        )}
      </group>
    );
  }

  if (actor.type === 'light_point') {
    return (
      <group position={[actor.transform.position.x, actor.transform.position.y, actor.transform.position.z]}>
        <pointLight
          intensity={2}
          distance={20}
          color={mat?.color || '#ffaa00'}
          castShadow
        />
        <mesh onClick={(e) => { e.stopPropagation(); selectActor(actor.id); }}>
          <sphereGeometry args={[0.15, 8, 8]} />
          <meshBasicMaterial color={isSelected ? '#ffcc00' : (mat?.color || '#ffaa00')} />
          {mat?.emissive && mat.emissiveIntensity && mat.emissiveIntensity > 0 && (
            <pointLight intensity={mat.emissiveIntensity} color={mat.emissive} distance={5} />
          )}
        </mesh>
      </group>
    );
  }

  if (actor.type === 'light_spot') {
    return (
      <group position={[actor.transform.position.x, actor.transform.position.y, actor.transform.position.z]}
        rotation={[
          (actor.transform.rotation.x * Math.PI) / 180,
          (actor.transform.rotation.y * Math.PI) / 180,
          (actor.transform.rotation.z * Math.PI) / 180,
        ]}>
        <spotLight intensity={3} angle={0.4} penumbra={0.5} castShadow color={mat?.color || '#ffffff'} />
        <mesh onClick={(e) => { e.stopPropagation(); selectActor(actor.id); }}>
          <coneGeometry args={[0.1, 0.3, 8]} />
          <meshBasicMaterial color={isSelected ? '#ffcc00' : '#ffaa00'} />
        </mesh>
      </group>
    );
  }

  if (actor.type === 'camera') {
    return (
      <group position={[actor.transform.position.x, actor.transform.position.y, actor.transform.position.z]}>
        <mesh onClick={(e) => { e.stopPropagation(); selectActor(actor.id); }}>
          <boxGeometry args={[0.3, 0.2, 0.4]} />
          <meshBasicMaterial color={isSelected ? '#00aaff' : '#555'} wireframe={!isSelected} />
        </mesh>
      </group>
    );
  }

  if (actor.type === 'player_start') {
    return (
      <group position={[actor.transform.position.x, actor.transform.position.y, actor.transform.position.z]}>
        <mesh onClick={(e) => { e.stopPropagation(); selectActor(actor.id); }}>
          <cylinderGeometry args={[0.05, 0.3, 1.5, 8]} />
          <meshBasicMaterial color={isSelected ? '#00ff88' : '#22aa55'} />
        </mesh>
        <mesh position={[0, 0.9, 0]}>
          <sphereGeometry args={[0.12, 8, 8]} />
          <meshBasicMaterial color={isSelected ? '#00ff88' : '#22aa55'} />
        </mesh>
      </group>
    );
  }

  // Special type renderers
  if (actor.type === 'sprite') return <SpriteActor actor={actor} />;
  if (actor.type === 'tilemap') return <TilemapActor actor={actor} />;
  if (actor.type === 'water') {
    return (
      <WaterSurface
        position={[actor.transform.position.x, actor.transform.position.y, actor.transform.position.z]}
        scale={[actor.transform.scale.x, actor.transform.scale.y, actor.transform.scale.z]}
      />
    );
  }
  if (actor.type === 'foliage') {
    return (
      <FoliageField
        position={[actor.transform.position.x, actor.transform.position.y, actor.transform.position.z]}
        scale={[actor.transform.scale.x, actor.transform.scale.y, actor.transform.scale.z]}
      />
    );
  }

  // Standard geometry rendering
  const geometry = geometryMap[actor.type];
  if (!geometry) return null;

  const rotation: [number, number, number] = actor.type === 'plane'
    ? [-Math.PI / 2, 0, 0]
    : [
        (actor.transform.rotation.x * Math.PI) / 180,
        (actor.transform.rotation.y * Math.PI) / 180,
        (actor.transform.rotation.z * Math.PI) / 180,
      ];

  // Material properties with fallback to defaults
  const matColor = mat?.color || (isSelected ? '#4488cc' : '#666666');
  const matRoughness = mat?.roughness ?? 0.6;
  const matMetalness = mat?.metalness ?? 0.2;
  const matEmissive = mat?.emissive || '#000000';
  const matEmissiveIntensity = mat?.emissiveIntensity ?? 0;
  const matOpacity = mat?.opacity ?? 1;
  const matTransparent = mat?.transparent ?? false;
  const matWireframe = mat?.wireframe ?? false;

  return (
    <mesh
      ref={meshRef}
      position={[actor.transform.position.x, actor.transform.position.y, actor.transform.position.z]}
      rotation={rotation}
      scale={[actor.transform.scale.x, actor.transform.scale.y, actor.transform.scale.z]}
      onClick={(e) => { e.stopPropagation(); selectActor(actor.id); }}
      castShadow
      receiveShadow
    >
      {geometry}
      <meshStandardMaterial
        color={matColor}
        roughness={matRoughness}
        metalness={matMetalness}
        emissive={matEmissive}
        emissiveIntensity={matEmissiveIntensity}
        opacity={matOpacity}
        transparent={matTransparent || matOpacity < 1}
        wireframe={matWireframe}
      />
      {isSelected && (
        <lineSegments>
          <edgesGeometry args={[meshRef.current?.geometry || undefined]} />
          <lineBasicMaterial color="#ff8800" />
        </lineSegments>
      )}
    </mesh>
  );
}

function FPSCounter() {
  const setFps = useEditorStore((s) => s.setFps);
  const frameCount = useRef(0);
  const lastTime = useRef(performance.now());

  useFrame(() => {
    frameCount.current++;
    const now = performance.now();
    if (now - lastTime.current >= 1000) {
      setFps(frameCount.current);
      frameCount.current = 0;
      lastTime.current = now;
    }
  });

  return null;
}

// =========================================================
// CAMERA STREAMING UPDATER FOR MASSIVE WORLD
// =========================================================
function CameraStreamingUpdater() {
  const { camera } = useThree();
  useFrame(() => {
    if (typeof (window as any).NeoEngineBridge !== 'undefined') {
      (window as any).NeoEngineBridge.updateCameraPosition(
        camera.position.x,
        camera.position.y,
        camera.position.z
      );
    }
  });
  return null;
}

function SceneContent() {
  const actors = useEditorStore((s) => s.actors);
  const selectedActorId = useEditorStore((s) => s.selectedActorId);
  const selectActor = useEditorStore((s) => s.selectActor);
  const gridVisible = useEditorStore((s) => s.gridVisible);
  const transformMode = useEditorStore((s) => s.transformMode);
  const updateActorTransform = useEditorStore((s) => s.updateActorTransform);
  const selectedActor = selectedActorId ? actors[selectedActorId] : null;
  const transformRef = useRef<any>(null);

  return (
    <>
      <ambientLight intensity={0.3} />
      <FPSCounter />
      <CameraStreamingUpdater />

      {gridVisible && (
        <Grid
          args={[100, 100]}
          position={[0, -0.01, 0]}
          cellSize={1}
          cellThickness={0.5}
          cellColor="#333333"
          sectionSize={10}
          sectionThickness={1}
          sectionColor="#444444"
          fadeDistance={80}
          infiniteGrid
        />
      )}

      {Object.values(actors).map((actor) => (
        <SceneActor key={actor.id} actor={actor} />
      ))}

      <mesh
        visible={false}
        onClick={(e) => {
          if (e.intersections.length <= 1) {
            selectActor(null);
          }
        }}
      >
        <planeGeometry args={[1000, 1000]} />
        <meshBasicMaterial />
      </mesh>
    </>
  );
}

export function Viewport() {
  const fps = useEditorStore((s) => s.fps);
  const actors = useEditorStore((s) => s.actors);
  const viewMode = useEditorStore((s) => s.viewMode);
  const isPlaying = useEditorStore((s) => s.isPlaying);
  const transformMode = useEditorStore((s) => s.transformMode);

  const actorCount = Object.keys(actors).length;

  return (
    <div className="neo-panel" style={{ height: '100%', position: 'relative' }}>
      <div className="neo-tabs">
        <div className="neo-tab active">Viewport</div>
      </div>
      <div style={{ flex: 1, position: 'relative' }}>
        <Canvas
          shadows
          camera={{ position: [8, 6, 10], fov: 60, near: 0.1, far: 5000 }}
          style={{ background: '#1e1e2e' }}
          gl={{ antialias: true, toneMapping: THREE.ACESFilmicToneMapping }}
        >
          <color attach="background" args={['#1a1a2e']} />
          <fog attach="fog" args={['#1a1a2e', 50, 150]} />
          <SceneContent />
          <OrbitControls
            makeDefault
            enableDamping
            dampingFactor={0.1}
            minDistance={1}
            maxDistance={200}
          />
          <GizmoHelper alignment="bottom-right" margin={[60, 60]}>
            <GizmoViewport labelColor="white" axisHeadScale={0.8} />
          </GizmoHelper>
        </Canvas>

        {/* Viewport Overlay */}
        <div className="viewport-overlay">
          <div className="viewport-stats">
            <div>FPS: {fps}</div>
            <div>Actors: {actorCount}</div>
            <div>Mode: {transformMode}</div>
            <div>View: {viewMode}</div>
          </div>
          <div className="viewport-mode">
            {isPlaying ? '▶ PLAYING' : 'EDITOR'}
          </div>
        </div>
      </div>
    </div>
  );
}
