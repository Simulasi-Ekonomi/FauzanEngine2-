#include "Runtime/FlipbookFrameSelector.h"
#include <cmath>
#include <cstdio>
int main(){using namespace NeoEngine;FlipbookFrameSelector selector;SpriteSourceRect out{9U,9U,9U,9U};if(selector.Initialize({4U,2U,2U,1U,5U})||selector.LastError()!=FlipbookFrameSelectorError::InvalidConfiguration)return 1;if(!selector.Initialize({4U,2U,2U,1U,4U})||!selector.Select(0.0F,out)||out.x!=0U||out.y!=0U||!selector.Select(0.5F,out)||out.x!=0U||out.y!=1U||!selector.Select(1.0F,out)||out.x!=2U||out.y!=1U)return 1;const SpriteSourceRect preserved=out;if(selector.Select(std::nanf(""),out)||selector.LastError()!=FlipbookFrameSelectorError::InvalidSample||out.x!=preserved.x||out.y!=preserved.y)return 1;std::printf("FLIPBOOK_FRAME_SELECTOR_SMOKE_OK frames=4 final=%u,%u\n",out.x,out.y);return 0;}
