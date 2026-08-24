#pragma once
#include <cstdint>

namespace NeoEngine {

template<typename T>
class SecureVar {
public:
    SecureVar(T initialValue) {
        m_Key = static_cast<uint32_t>(rand());
        Set(initialValue);
    }

    void Set(T value) {
        // Simpan nilai asli dalam bentuk ter-XOR dengan key
        m_EncryptedValue = *reinterpret_cast<uintptr_t*>(&value) ^ m_Key;
    }

    T Get() const {
        uintptr_t decrypted = m_EncryptedValue ^ m_Key;
        return *reinterpret_cast<T*>(&decrypted);
    }

    // Overload operator agar bisa dipakai seperti variabel biasa
    SecureVar& operator=(T value) { Set(value); return *this; }
    operator T() const { return Get(); }

private:
    uintptr_t m_EncryptedValue;
    uint32_t m_Key;
};

} // namespace
