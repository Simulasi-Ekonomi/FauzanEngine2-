#include "Core/EngineLoop.h"
#include <cstdio>
#include <stdexcept>
using namespace NeoEngine;
int main(){ bool blocked=false; try{EngineLoop::Init();}catch(const std::logic_error& error){blocked=std::string_view(error.what()).find("NOT_IMPLEMENTED")!=std::string_view::npos;} if(!blocked){std::fprintf(stderr,"LEGACY_ENGINE_LOOP_SMOKE_FAIL\n");return 1;}std::printf("LEGACY_ENGINE_LOOP_SMOKE_OK state=not_implemented\n"); }
