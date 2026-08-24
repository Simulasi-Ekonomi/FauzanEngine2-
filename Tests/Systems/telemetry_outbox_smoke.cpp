#include "Systems/TelemetryOutbox.h"
#include <cstdio>
using namespace NeoEngine;int main(){TelemetryOutbox a;bool ok=a.Enqueue("farm-1","{\"source\":\"farm\"}")&&!a.Enqueue("farm-1","{\"source\":\"farm\"}")&&!a.Enqueue("bad/id","{}")&&a.Pending().size()==1;auto data=a.Serialize();TelemetryOutbox b;ok=ok&&b.Deserialize(data)&&b.Pending().size()==1&&b.Acknowledge("farm-1")&&b.Pending().empty()&&!b.Acknowledge("farm-1");if(!ok){std::fprintf(stderr,"TELEMETRY_OUTBOX_SMOKE_FAIL\n");return 1;}std::printf("TELEMETRY_OUTBOX_SMOKE_OK bytes=%zu\n",data.size());}
