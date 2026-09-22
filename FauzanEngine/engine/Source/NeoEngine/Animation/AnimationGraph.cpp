#include "AnimationGraph.h"
void AnimationGraph::AddNode(const AnimationNode&n){if(n.update)nodes.push_back(n);} void AnimationGraph::Update(){for(auto&n:nodes)if(n.update)n.update();}