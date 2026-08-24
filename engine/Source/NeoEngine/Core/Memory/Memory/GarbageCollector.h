#pragma once

class GarbageCollector {
public:
    // Menjalankan siklus pembersihan objek yang tidak lagi direferensikan
    static void Collect();

    // Penandaan objek untuk dicegah dari penghancuran (Mark phase skeleton)
    static void MarkAsRoot(class ObjectBase* Obj);
};
