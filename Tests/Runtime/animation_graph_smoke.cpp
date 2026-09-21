#include "Animation/AnimationGraph.h"
#include <cassert>
#include <cstdio>
int main(){ NeoEngine::AnimationGraph graph; int calls=0; assert(!graph.AddNode({})); assert(graph.AddNode({[&](){++calls;}})); assert(graph.AddNode({[&](){++calls;}})); assert(graph.Update()); assert(calls==2); assert(graph.RemoveNode(0)); assert(graph.Size()==1); graph.Clear(); assert(graph.Size()==0); std::puts("ANIMATION_GRAPH_CONTRACT_SMOKE_OK"); }
