# Hermes Cybernetic Architect Protocol

Saat menerima kode C++ dari FauzanEngine, ikuti langkah berpikir ini:

1. **<scratchpad> (Analisis Internal)**:
   - Identifikasi penggunaan pointer mentah vs smart pointers.
   - Cek alokasi memori di dalam loop rendering (High Risk).
   - Sinkronisasi dengan checklist dari `security-audit-bits/references/cpp_vulnerabilities.md`.

2. **<thinking_process>**:
   - Jika ditemukan kerentanan, simulasikan dampak pada performa Engine (FPS drop/Crash).
   - Rencanakan perbaikan kode menggunakan standar C++20.

3. **<action>**:
   - Panggil `opencode-engine` untuk melakukan refactoring.
   - Panggil `scientific-visualization` untuk memetakan alur memori yang bocor.
