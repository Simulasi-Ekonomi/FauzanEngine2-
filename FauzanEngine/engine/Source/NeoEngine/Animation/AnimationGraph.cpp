#include "AnimationGraph.h"
void AnimationGraph::AddNode(const AnimationNode& node){ if(node.update) nodes.push_back(node); }
void AnimationGraph::Update(){ for(auto& node:nodes) if(node.update) node.update(); }