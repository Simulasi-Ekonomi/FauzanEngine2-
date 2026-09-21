#include "Editor/EditorProvider.h"
#include <cassert>
#include <cstdio>
int main(){auto& p=NeoEngine::EditorProvider::Get(); assert(!p.ExecuteCommand("","x")); assert(p.LastError()==NeoEngine::EditorProviderError::InvalidCommand); int seen=0; p.SetCommandCallback([&](const NeoEngine::EditorCommand& c){assert(c.sequence==1U);assert(c.action=="pause");++seen;}); assert(p.ExecuteCommand("pause","{}")); assert(seen==1); assert(p.CommandSequence()==1U); assert(p.GetEditorStateJSON().find("commandSequence")!=std::string::npos); std::puts("EDITOR_PROVIDER_CONTRACT_SMOKE_OK");}
