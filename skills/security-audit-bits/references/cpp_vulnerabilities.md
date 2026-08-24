# C++ Security Audit Checklist (Engine Focus)

## 🧠 Memory Management
- [ ] **Unchecked Nullptrs**: Pengecekan pointer setelah `malloc` atau `new`.
- [ ] **Dangling Pointers**: Pointer yang tetap menunjuk ke memori yang sudah di-`delete`.
- [ ] **Memory Leaks**: Alokasi di render-loop tanpa `free`/`delete` yang sesuai.

## ⚡ Concurrency (Multi-threading)
- [ ] **Race Conditions**: Akses variabel global dari thread render dan logic tanpa `std::mutex`.
- [ ] **Deadlocks**: Penguncian mutex yang tidak konsisten urutannya.

## 🛡️ Buffer & Bounds
- [ ] **Out-of-bounds**: Akses array vertex/index di luar ukuran buffer GPU.
- [ ] **Format Strings**: Penggunaan `printf` dengan input user yang tidak divalidasi.
