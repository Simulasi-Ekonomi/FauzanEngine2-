#include "Runtime/RuntimePersistence.h"

#include <cstdio>

int main() {
    using namespace NeoEngine; RuntimeSettingsStore settings; std::vector<uint8_t> settingsBytes;
    if(!settings.Set("audio.volume","0.75")||!settings.Set("graphics.quality","medium")||settings.Set("apiToken","bad")||settings.LastError()!=RuntimePersistenceError::SensitiveContent||!settings.Serialize(settingsBytes)) return 1;
    RuntimeSettingsStore restored;if(!restored.Set("existing","keep")||!restored.Deserialize(settingsBytes)||restored.Count()!=2||!restored.Find("audio.volume")||*restored.Find("audio.volume")!="0.75") return 1;
    const std::vector<uint8_t> beforeCorrupt=settingsBytes;settingsBytes.back()^=0x7FU;if(restored.Deserialize(settingsBytes)||restored.LastError()!=RuntimePersistenceError::ChecksumMismatch||restored.Count()!=2) return 1;
    RuntimeSaveEnvelope save{"farm-world",7,{1,2,3,4,5}};std::vector<uint8_t> saveBytes;RuntimePersistenceError error=RuntimePersistenceError::None;if(!RuntimeSaveCodec::Serialize(save,saveBytes,error)) return 1;RuntimeSaveEnvelope loaded{"old",1,{9}};if(!RuntimeSaveCodec::Deserialize(saveBytes,loaded,error)||loaded.kind!="farm-world"||loaded.revision!=7||loaded.payload!=save.payload) return 1;
    const RuntimeSaveEnvelope before=loaded;saveBytes.push_back(0);if(RuntimeSaveCodec::Deserialize(saveBytes,loaded,error)||error!=RuntimePersistenceError::TrailingBytes||loaded.kind!=before.kind||loaded.revision!=before.revision) return 1;RuntimeSaveEnvelope sensitive{"farm-world",8,{'t','o','k','e','n'}};if(RuntimeSaveCodec::Serialize(sensitive,saveBytes,error)||error!=RuntimePersistenceError::SensitiveContent) return 1;
    std::printf("RUNTIME_PERSISTENCE_SMOKE_OK settings=2 atomic=1 save=1 checksum=1 sensitive=1\n");return 0;
}
