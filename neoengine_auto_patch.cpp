
// NEOENGINE AUTO-PATCH BY ARIES S7
// Penerapan standar memori berdasarkan hasil belajar 794 struktur C++

#include <memory>
// Mengganti Raw Pointer ke Smart Pointer untuk mencegah leak
auto core_engine = std::make_unique<NeoEngineRuntime>();
core_engine->initialize();
