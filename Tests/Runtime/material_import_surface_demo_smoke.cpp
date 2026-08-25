#include "Demos/MaterialImportSurfaceDemo.h"

#include <cstdio>
#include <fstream>

int main() {
    using namespace NeoEngine; const char* output="/tmp/fauzanengine_material_import_surface_demo_smoke.ppm"; MaterialImportSurfaceDemoConfig invalid{}; invalid.frames=0U; MaterialImportSurfaceDemoReceipt receipt{}; MaterialImportSurfaceDemoError error{};
    if(RunMaterialImportSurfaceDemo(invalid,receipt,error)||error!=MaterialImportSurfaceDemoError::InvalidConfiguration)return 1;
    MaterialImportSurfaceDemoConfig config{}; config.width=96U; config.height=64U; config.frames=4U; config.hiddenSurface=true; config.ppmPath=output;
    if(!RunMaterialImportSurfaceDemo(config,receipt,error)||receipt.renderedFrames!=4U||receipt.presentedFrames!=4U||receipt.visiblePixels==0U||receipt.frameHash==0U||receipt.textureHash==0U||receipt.meshHash==0U||receipt.materialHash==0U||receipt.materialRgba==0U)return 1;
    std::ifstream artifact(output,std::ios::binary); char header[2]{}; if(!artifact.read(header,2)||header[0]!='P'||header[1]!='6')return 1; artifact.close(); std::remove(output);
    std::printf("MATERIAL_IMPORT_SURFACE_DEMO_SMOKE_OK frames=%u presented=%u pixels=%u hash=%llu material=%08X\n",receipt.renderedFrames,receipt.presentedFrames,receipt.visiblePixels,static_cast<unsigned long long>(receipt.frameHash),receipt.materialRgba); return 0;
}
