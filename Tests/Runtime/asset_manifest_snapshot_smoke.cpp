#include "Runtime/AssetManifestSnapshot.h"

#include <cstdio>

int main(){using namespace NeoEngine;AssetRegistry registry;if(!registry.ImportBytes("farm.tile",AssetKind::Texture,{}, {1,2,3})||!registry.Declare("farm.mat",AssetKind::Material,{"farm.tile"})||!registry.Declare("farm.scene",AssetKind::Scene,{"farm.mat"})||!registry.MarkReady("farm.tile")||!registry.MarkReady("farm.mat")||!registry.MarkReady("farm.scene"))return 1;AssetManifestSnapshot snapshot;std::vector<uint8_t> bytes;if(!snapshot.Capture(registry)||!snapshot.MatchesRegistry(registry)||!snapshot.Serialize(bytes))return 1;AssetManifestSnapshot restored;if(!restored.Deserialize(bytes)||!restored.MatchesRegistry(registry)||restored.Entries().size()!=3U)return 1;bytes.back()^=0x01U;if(restored.Deserialize(bytes)||restored.LastError()!=AssetManifestError::ChecksumMismatch||restored.Entries().size()!=3U)return 1;std::printf("ASSET_MANIFEST_SNAPSHOT_SMOKE_OK assets=3 deps=2 versioned=1 atomic=1\n");return 0;}
