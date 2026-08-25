#include "Runtime/RenderCamera.h"
#include "Runtime/FlipbookFrameSelector.h"
#include "Runtime/SceneSpriteAdapter.h"
#include "Runtime/SceneWorld.h"
#include "Runtime/SoftwareRenderer.h"
#include "Runtime/SpriteBatch.h"
#include "Runtime/TextureStaging.h"

#include <cstdio>

int main(){using namespace NeoEngine;CpuTextureResource atlas{"atlas",1U,TextureSourceFormat::PpmP6,2U,1U,{255U,0U,0U,255U,0U,0U,255U,255U}};SoftwareRenderer renderer;if(!renderer.Initialize(64U,32U))return 1;RenderCamera camera;if(!camera.Initialize({RenderCameraMode::Orthographic,{},2.0F,60.0F,2.0F,0.1F,10.0F}))return 1;SpriteBatch batch;if(!batch.Queue({-1.0F,0.0F,1.0F,1.5F,1.5F,0,0,0xFFFFFFFFU,&atlas,0.0F,false,true,0U,0U,1U,1U})||!batch.Queue({1.0F,0.0F,1.0F,1.5F,1.5F,0,0,0xFFFFFFFFU,&atlas,0.0F,false,true,1U,0U,1U,1U})||!batch.Flush(renderer,camera))return 1;uint32_t red=0U,blue=0U;for(uint32_t pixel:renderer.Pixels()){if(pixel==0xFFFF0000U)++red;if(pixel==0xFF0000FFU)++blue;}SceneWorld world;SceneEntity actor{};if(!world.Create(actor)||!world.SetTransform(actor,{0.0F,0.0F,1.0F})||!world.UpdateTransforms())return 1;const Transform3* before=world.GetTransform(actor);if(before==nullptr||before->x!=0.0F)return 1;SceneSpriteAdapter staged;if(!staged.AddStaged(actor,atlas,1.0F,1.0F,0,0,0xFFFFFFFFU))return 1;FlipbookFrameSelector selector;if(!selector.Initialize({2U,1U,1U,1U,2U}))return 1;SpriteSourceRect frame{};if(!selector.Select(1.0F,frame))return 1;SoftwareRenderer frameRenderer;if(!frameRenderer.Initialize(64U,32U))return 1;SpriteBatch frameBatch;if(!staged.QueueFrame(world,frameBatch,frame)||!frameBatch.Flush(frameRenderer,camera))return 1;uint32_t stagedBlue=0U;for(uint32_t pixel:frameRenderer.Pixels())if(pixel==0xFF0000FFU)++stagedBlue;const Transform3* after=world.GetTransform(actor);if(after==nullptr||after->x!=before->x||stagedBlue==0U)return 1;SpriteBatch invalid; if(invalid.Queue({0.0F,0.0F,1.0F,1.0F,1.0F,0,0,0xFFFFFFFFU,&atlas,0.0F,false,true,2U,0U,1U,1U})||invalid.LastError()!=SpriteBatchError::InvalidSourceRect)return 1;std::printf("SPRITE_ATLAS_SOURCE_RECT_SMOKE_OK red=%u blue=%u staged=%u atomic=1\n",red,blue,stagedBlue);return red>0U&&blue>0U?0:1;}
