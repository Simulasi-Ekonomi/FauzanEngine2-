#include "Templates/MatchThreeGame.h"
#include <cstdio>
using namespace NeoEngine;int main(){MatchThreeGame a,b;bool ok=a.Swap(0,2,1,2)&&b.Swap(0,2,1,2);auto removed=a.Resolve();ok=ok&&removed==3&&b.Resolve()==3&&a.Score()==30&&a.DeterministicState()==b.DeterministicState()&&!a.Swap(0,0,4,4)&&a.LastError()==MatchThreeError::NotAdjacent;if(!ok){std::fprintf(stderr,"MATCH_THREE_SMOKE_FAIL\n");return 1;}std::printf("MATCH_THREE_SMOKE_OK removed=%u score=%u\n",removed,a.Score());}
