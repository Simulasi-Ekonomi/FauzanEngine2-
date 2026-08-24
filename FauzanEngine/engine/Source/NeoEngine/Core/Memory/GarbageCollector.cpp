#include <cassert>
#include "GarbageCollector.h"
#include "../Object/ObjectRegistry.h"
#include "../Object/ObjectBase.h"
#include <android/log.h>

void GarbageCollector::Collect() {
    __android_log_print(ANDROID_LOG_INFO, "NeoEngine_GC", "Initiating Garbage Collection cycle...");
    // Mark & Sweep logic akan diintegrasikan dengan ObjectRegistry di tahap audit
}

void GarbageCollector::MarkAsRoot(ObjectBase* Obj) {
    // Mencegah objek penting (seperti World atau Persistent Actors) dari GC
}
