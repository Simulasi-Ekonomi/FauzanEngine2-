#pragma once
#include <string>
#include <cstddef>

// Struktur metadata untuk identifikasi runtime (Reflection Minimal)
struct TypeInfo {
    const char* Name;
    const TypeInfo* Parent;
    size_t Size;

    bool IsChildOf(const TypeInfo* Other) const {
        const TypeInfo* Current = this;
        while (Current) {
            if (Current == Other) return true;
            Current = Current->Parent;
        }
        return false;
    }
};
