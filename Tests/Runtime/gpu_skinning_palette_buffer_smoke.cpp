#include "Animation/GPUSkinningPaletteBuffer.h"
#include <cassert>
int main(){ NeoEngine::GPUSkinningPaletteBuffer palette; assert(!palette.IsValid()); assert(palette.BoneCount()==0U); palette.Destroy(); return 0; }
