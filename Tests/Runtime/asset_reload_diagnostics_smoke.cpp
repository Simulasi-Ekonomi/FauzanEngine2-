#include "Runtime/AssetReloadDiagnostics.h"

#include <cstdio>

int main(){using namespace NeoEngine;AssetRegistry registry;if(!registry.Declare("tile",AssetKind::Texture,{})||!registry.Declare("material",AssetKind::Material,{"tile"})||!registry.Declare("prefab",AssetKind::Prefab,{"material"})||!registry.Declare("scene",AssetKind::Scene,{"prefab"}))return 1;AssetReloadDiagnostics diagnostics;if(!diagnostics.BuildPlan(registry,"tile")||diagnostics.AffectedIds().size()!=4U||diagnostics.AffectedIds()[0]!="tile"||diagnostics.AffectedIds()[3]!="scene"||diagnostics.BuildPlan(registry,"missing")||diagnostics.LastError()!=AssetReloadDiagnosticsError::MissingAsset)return 1;std::printf("ASSET_RELOAD_DIAGNOSTICS_SMOKE_OK changed=tile affected=4 mutation=0\n");return 0;}
