# Canonical Active C++ Source Manifest

Generated from `Source/NeoEngine/CMakeLists.txt` on 2026-09-21.

This manifest is an audit artifact, not a second build definition. The canonical build authority remains CMake.

## Rules

- Every source listed here must exist and be compiled by `NeoEngineRuntime`.
- Duplicate source entries are forbidden.
- C++ files outside this manifest are **not** production-authoritative until explicitly migrated, deprecated, or excluded with evidence.
- This manifest must be regenerated whenever canonical CMake source registration changes.

## Integrity

- Canonical source entries: **111**
- Duplicate entries: **0**
- Duplicate paths: none

## Sources

- `Core/ECS/ArchetypeManager.cpp`
- `Runtime/AtomicSaveFile.cpp`
- `Runtime/PBRMaterial.cpp`
- `Runtime/BRDFLut.cpp`
- `Runtime/PBRLighting.cpp`
- `Runtime/PBRIBL.cpp`
- `Runtime/PBREnvironment.cpp`
- `Runtime/PBREnvironmentDescriptorSet.cpp`
- `Runtime/PBRRenderPipeline.cpp`
- `Runtime/ShaderLibrary.cpp`
- `Runtime/AssetRegistry.cpp`
- `Runtime/AssetResourceManager.cpp`
- `Runtime/AssetStreamingQueue.cpp`
- `Runtime/Vulkan3DRenderer.cpp`
- `Runtime/VulkanPresentProbe.cpp`
- `Runtime/VulkanTexturedPresent.cpp`
- `Runtime/NeoRuntime.cpp`
- `Runtime/NeoRuntimeVulkan3D.cpp`
- `Runtime/FarmRuntimeSession.cpp`
- `Runtime/RuntimeClock.cpp`
- `Runtime/RuntimePersistence.cpp`
- `Runtime/SceneWorld.cpp`
- `Runtime/CanonicalRuntimeWorld.cpp`
- `Runtime/SceneECSBridge.cpp`
- `Runtime/MeshStaging.cpp`
- `Runtime/MaterialStaging.cpp`
- `Runtime/TextureStaging.cpp`
- `Runtime/TinyObjLoader.cpp`
- `Runtime/ObjMeshImporter.cpp`
- `Runtime/MtlMaterialImporter.cpp`
- `Runtime/PpmTexture.cpp`
- `Runtime/BmpTexture.cpp`
- `Runtime/EditorScenePrefabCodec.cpp`
- `Runtime/SceneSpriteAdapter.cpp`
- `Runtime/RenderCamera.cpp`
- `Runtime/MeshRenderer.cpp`
- `Runtime/SpriteBatch.cpp`
- `Runtime/SoftwareRenderer.cpp`
- `Runtime/WavAudioParser.cpp`
- `Runtime/AudioMixer.cpp`
- `Runtime/AudioComponent.cpp`
- `Runtime/SdlAudioBridge.cpp`
- `Runtime/InputState.cpp`
- `Runtime/SdlInputBridge.cpp`
- `Runtime/UiInputRouter.cpp`
- `Runtime/EditorSceneDocumentCodec.cpp`
- `Runtime/EditorSceneDocument.cpp`
- `Runtime/EditorSceneMeshBinder.cpp`
- `Runtime/EditorSceneSpriteBinder.cpp`
- `Runtime/SceneMeshAdapter.cpp`
- `Runtime/SceneRenderAdapter.cpp`
- `Runtime/EditorScenePrefab.cpp`
- `Runtime/PrefabStaging.cpp`
- `Runtime/EditorSceneSession.cpp`
- `Runtime/EditorSceneAgentAPI.cpp`
- `Runtime/SoftwareSurfacePresenter.cpp`
- `Runtime/GridRouteFollower.cpp`
- `Runtime/FarmRuntimeHud.cpp`
- `Runtime/RuntimeTimeSystem.cpp`
- `Runtime/EventSignalBus.cpp`
- `Runtime/FarmRenderAssetManifest.cpp`
- `Runtime/FarmRenderAdapter.cpp`
- `Runtime/FarmSpriteRenderAdapter.cpp`
- `Runtime/ActorComponentWorld.cpp`
- `Runtime/ReplicationWorld.cpp`
- `Runtime/KinematicMotionController.cpp`
- `Runtime/FarmPlayerInputBridge.cpp`
- `Runtime/RuntimeTimerQueue.cpp`
- `Runtime/MovementAuthority.cpp`
- `Runtime/FarmActionPanelController.cpp`
- `Runtime/UiCanvasRenderer.cpp`
- `Runtime/BitmapTextRenderer.cpp`
- `Runtime/UiLayoutResolver.cpp`
- `Runtime/RouteRootMotionAdapter.cpp`
- `Runtime/InputMotionBridge.cpp`
- `Animation/SkeletalAnimationController.cpp`
- `Animation/SkeletalPoseClip.cpp`
- `Animation/Bone.cpp`
- `Core/Math/Mat4_Transform.cpp`
- `Animation/SkeletalPosePlayer.cpp`
- `Animation/Skeleton.cpp`
- `Animation/Skinning.cpp`
- `Systems/AuthorityLoopbackServer.cpp`
- `Systems/AuthoritativeCommandGate.cpp`
- `Systems/AuthorityWireProtocol.cpp`
- `Systems/FarmAuthoritativeService.cpp`
- `Systems/FarmAuthoritativeSessionHost.cpp`
- `Systems/FarmAuthoritativeSessionLoopback.cpp`
- `Systems/FarmAuthorityCheckpoint.cpp`
- `Systems/FarmCanonicalGameTool.cpp`
- `Systems/FarmCommerceEntitlementLedger.cpp`
- `Systems/FarmCommerceCheckpoint.cpp`
- `Systems/FarmCommerceCheckpointFile.cpp`
- `Systems/FarmWorldTool.cpp`
- `Systems/ItemSerialTracker.cpp`
- `Systems/TelemetryOutbox.cpp`
- `Systems/TrustSafetySystem.cpp`
- `Systems/GridNavigation.cpp`
- `Systems/WorldAuthoring.cpp`
- `Systems/CurriculumSystem.cpp`
- `Systems/AgricultureCurriculum.cpp`
- `Systems/AuthoringCatalog.cpp`
- `Systems/FarmSystem.cpp`
- `Templates/GameTemplateRegistry.cpp`
- `Templates/MatchThreeGame.cpp`
- `Templates/RpgSandboxGame.cpp`
- `Templates/SudokuGame.cpp`
- `Templates/TowerDefenseGame.cpp`
- `Threading/JobSystem.cpp`
- `Renderer/GPUDrivenRenderer.cpp`
- `Renderer/GPUFrustumCulling.cpp`
