#include "MeshStreamer.h"
#include <cstdint>
#include <cstring>
#include "GPUMemoryBudget.h"
namespace NeoEngine {
void MeshStreamer::UploadMesh(const void* vertices){
    if(!vertices) return;
    // MeshStreamer currently receives an opaque upload payload; retain ownership in the renderer/RHI upload queue rather than dereferencing an unknown layout.
}
}
