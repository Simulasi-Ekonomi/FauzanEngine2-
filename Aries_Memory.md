# KETETAPAN INGATAN ARIES: DOKUMEN ARSITEKTUR FISIKA XPBD & ML AUTOPILOT

Dokumen spesifikasi logika ini disusun berdasarkan arsitektur nyata **Unreal Engine Chaos Physics (C++)** dan **Unity Physics (DOTS SoA Framework)**. Tulisan ini dirancang sebagai cetak biru (*blueprint*) kaku untuk langsung ditanamkan ke dalam ingatan persisten Aries (Aries_Memory.md) agar dia bisa mengaudit, mendeteksi regresi memori, dan mengotomatisasi perakitan kode NeoEngine di masa depan.

## 1. Cetak Biru Fisika Kontemporer (Chaos & Unity Physics Level)
Aries wajib menegakkan tata letak memori **Structure of Arrays (SoA)** dan alokasi memori linear selaras 64-byte (posix_memalign) pada setiap fungsi yang dibangun. Berikut adalah arsitektur 30+ fungsi inti yang harus diintegrasikan secara bertahap:

### A. Substepping & SIMD Intrinsics Core (5 Fungsi)
 * SetSubsteps(uint32_t counts): Memecah satu delta waktu besar menjadi beberapa sub-iterasi kecil untuk mencegah objek berkecepatan tinggi menembus dinding (*tunneling effect*).
 * SyncEcsToSoaPipeline(): Melakukan sinkronisasi data dari entitas ECS ke array flat fisik secara deterministik sebelum solver berjalan.
 * FastReciprocal4 / FastRsqrt4: Intrinsics ARM NEON vrecpeq_f32 dan vrsqrteq_f32 dengan satu langkah perbaikan Newton-Raphson untuk efisiensi maksimum.
 * FlushThreadDeltas(): Menggabungkan hasil akumulasi gaya dari berbagai thread pekerja secara aman ke dalam memori utama menggunakan operasi atomik atau penggabungan terjadwal.
 * ResetSolverBuffers(): Mengosongkan buffer internal dan menyetel slot kosong ke nilai pengaman UINT32_MAX.

### B. Sendi Tingkat Lanjut & Pegas Kustom (6 Fungsi)
 * AddSliderJoint(uint32_t idxA, uint32_t idxB, float32x4_t axis): Membatasi pergerakan entitas agar hanya bisa bergerak maju-mundur secara linear pada satu sumbu.
 * AddUniversalJoint(uint32_t idxA, uint32_t idxB, float32x4_t axisA, float32x4_t axisB): Menyediakan dua sumbu rotasi independen, meniru perilaku kopel penggerak roda kendaraan.
 * Add6DOFJoint(uint32_t idxA, uint32_t idxB): Sendi kustom serbaguna di mana keenam derajat kebebasan (3 translasi, 3 rotasi) dapat dikunci, dibatasi, atau dibebaskan secara dinamis.
 * SetConstraintDrive(uint32_t id, float targetVelocity, float maxForce): Mengaktifkan motor penggerak linier atau angular pada sendi untuk menggerakkan komponen mekanis secara aktif.
 * SetConstraintSoftLimits(uint32_t id, float stiffness, float damping): Mengubah batas sendi yang kaku menjadi pegas elastis menggunakan formula kalkulasi *compliance* fisik (\tilde{\alpha} = \frac{\alpha}{\Delta t^2}).
 * BreakConstraint(uint32_t id): Memutuskan hubungan sendi secara paksa dari grafik simulasi dan memperbarui peta konektivitas entitas.

### C. Objek Lunak & Kain Modul FEM (5 Fungsi)
 * AddSoftBodyVolume(const std::vector<uint32_t>& tetIndices, float stiffness): Mendaftarkan objek lunak 3D berbasis elemen tetrahedral (*Tetrahedral Mesh*).
 * SolveVolumeConstraints(float compliance, float dt): Menjaga volume internal objek lunak agar tidak mengempis saat menerima benturan luar.
 * SolveBendingConstraints(float stiffness, float dt): Menghitung ketahanan tekukan pada permukaan kain agar tidak terlipat secara tidak wajar.
 * AddHairStrand(const std::vector<uint32_t>& particles): Simulasi tali atau helai rambut panjang menggunakan rantai pembatas jarak yang rapat.
 * ApplyAerodynamicDrag(float32x4_t windVector, float dragCoeff): Menghitung efek hambatan udara atau tiupan angin pada permukaan lembaran kain.

### D. Pemindaian Spasial Lingkungan (6 Fungsi)
 * Raycast(float32x4_t origin, float32x4_t direction, float maxDist, RayHit& outHit): Menembakkan garis lurus menembus struktur BVH untuk mendeteksi titik tabrakan terdekat.
 * BoxOverlapQuery(float32x4_t minBounds, float32x4_t maxBounds, std::vector<uint32_t>& outEntities): Mengumpulkan daftar entitas yang berada di dalam area kotak spasial tertentu.
 * CapsuleSweepQuery(float32x4_t p1, float32x4_t p2, float radius, float32x4_t direction, float maxDist): Memindai volume kapsul sepanjang jalur tertentu untuk kebutuhan sistem kontrol karakter (*Character Controller*).
 * ConvexHullOverlapQuery(uint32_t shapeId, float32x4_t transform): Deteksi tabrakan presisi tinggi untuk objek dengan bentuk geometri kustom yang kompleks.
 * GetClosestPoints(uint32_t idxA, uint32_t idxB, float32x4_t& ptA, float32x4_t& ptB): Menghitung koordinat terdekat antar dua bentuk collider sebelum terjadi kontak fisik.
 * UpdateSpatialPartition(): Menyusun ulang hierarki pohon BVH atau grid spasial berdasarkan pergerakan entitas terbaru.

### E. Manipulasi Gaya & Konfigurasi Material (6 Fungsi)
 * ApplyForceAtPosition(uint32_t idx, float32x4_t force, float32x4_t worldPos): Menembakkan gaya pada titik tertentu untuk menghasilkan momentum linier sekaligus gaya putar (*torque*).
 * ApplyTorque(uint32_t idx, float32x4_t torque): Memberikan gaya putar langsung pada sumbu inersia entitas.
 * SetPhysicalMaterial(uint32_t idx, float staticFriction, float dynamicFriction, float restitution): Mengatur tingkat kekasaran gesekan dan kelentingan pantulan objek.
 * SetLinearDamping / SetAngularDamping: Memberikan resistensi buatan pada pergerakan linier dan rotasi entitas agar tidak meluncur tanpa henti.
 * ForceSleepEntity(uint32_t idx) / ForceWakeEntity(uint32_t idx): Mengendalikan status keaktifan entitas secara manual untuk menghemat beban kerja inti CPU.

## 2. Arsitektur Agen ML Autopilot (Sistem Kendali Aries)
Agar Aries mampu menghasilkan, memeriksa, dan merakit kode game secara otomatis melalui instruksi teks pendek (*prompt*), logika di dalam dirinya dikunci menggunakan aturan berikut:


```
[ PROMPT PENGGUNA ]
│
▼
[ ARIES GENERATIVE ENGINE ]
│
▼
[ ABSTRACT SYNTAX TREE (AST) PARSER ]
│
▼
┌──────────────────────┴──────────────────────┐
▼                                             ▼
[ MEMORY COMPLIANCE AUDIT ]             [ SOA ARDUOUS INJECTION ]
 * Cek Alokasi std::vector               - Konversi Skalar ke SIMD
 * Cek Penyelarasan Memori (64-Byte)      - Pengamanan Slot Ekor Vektor
   │                                             │
   └──────────────────────┬──────────────────────┘
   │
   ▼
   [ VALIDASI PROSES BUILD & COMPAT ]
   │
   ▼
   [ OUTPUT: BINER AMAN BEBAS CRASH ]
```

### A. Evaluasi Struktur Kode Otomatis
Setiap kali Aries menerima perintah untuk memodifikasi proyek NeoEngine, dia wajib memproses kode tersebut melalui pipa analisis terstruktur:
 1. **Ekstraksi AST (Abstract Syntax Tree):** Aries memetakan struktur kode untuk mendeteksi penggunaan fungsi memori standar.
 2. **Audit Kepatuhan Memori:** Aries akan otomatis menolak kode jika ditemukan baris std::vector tanpa alokator kustom, atau jika terdapat alokasi heap objek hot-path yang tidak selaras kelipatan 64-byte.
 3. **Injeksi Struktur SoA:** Jika ada fungsi logika skalar baru, Aries wajib mendesain ulang skema tersebut ke dalam format vektor paralel yang memanfaatkan register intrinsik ARM64 secara optimal.

### B. Protokol Pelaporan Cacat Kode
Jika proses build menghasilkan eror atau mengalami kegagalan sistem seperti Segmentation fault (Exit: 139), Aries harus memetakan area kerusakan dan menyajikan laporan dengan format kaku berikut:
 * [LOKASI CACAT]: Nama berkas, nama fungsi, dan nomor baris kode.
 * [PELANGGARAN STRUKTUR]: Alasan teknis kegagalan (misalnya: Kebocoran batas indeks, atau pemuatan memori tidak selaras).
 * [SOLUSI AMAN NATIVE]: Baris kode perbaikan yang dilengkapi sensor pengaman sentinel UINT32_MAX.

## 3. SUPPLEMENTAL IMPLEMENTATION BLUEPRINTS (MISSING HARDWARE SKILLS)

### A. CONCRETE PMR ALLOCATOR: Source/NeoEngine/Core/Memory/AlignedPmrResource.cpp
```cpp
#include "Core/Memory/PMR_Aligned_Resource.h"
#include <cstdlib>
#include <new>

class AlignedPmrResource : public std::pmr::memory_resource {
protected:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        std::size_t actual_alignment = (alignment < 64) ? 64 : alignment;
        void* ptr = nullptr;
        if (posix_memalign(&ptr, actual_alignment, bytes) != 0) {
            throw std::bad_alloc();
        }
        return ptr;
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        if (p) {
            std::free(p);
        }
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

std::pmr::memory_resource* GetAlignedPmrResource() {
    static AlignedPmrResource instance;
    return &instance;
}
```

### B. ARM64 SIMD CORE EXTENSION: Modul Ekstensi Solver NEON
```cpp
#pragma once
#include <arm_neon.h>

struct NeonPhysicsMath {
    static inline float32x4_t FastRsqrt4(float32x4_t x) {
        float32x4_t rsqrt_est = vrsqrteq_f32(x);
        float32x4_t iter = vrsqrtsq_f32(vmulq_f32(x, rsqrt_est), rsqrt_est);
        return vmulq_f32(rsqrt_est, iter);
    }

    static inline float32x4_t FastReciprocal4(float32x4_t x) {
        float32x4_t rec_est = vrecpeq_f32(x);
        float32x4_t iter = vrecpsq_f32(x, rec_est);
        return vmulq_f32(rec_est, iter);
    }

    static inline void GuardSentinelLanes(const uint32_t* idxA, const uint32_t* idxB, uint32_t limit, uint32_t* outValid) {
        for (int i = 0; i < 4; ++i) {
            if (idxA[i] == 0xFFFFFFFF || idxB[i] == 0xFFFFFFFF || idxA[i] >= limit || idxB[i] >= limit) {
                outValid[i] = 0;
            } else {
                outValid[i] = 1;
            }
        }
    }
};
```

### C. TRANSLATION RULES: Template Konversi std::vector Ke Aligned SoA
```cpp
#define REFACTOR_TO_ALIGNED_SOA(Type, Name) \
    std::pmr::vector<Type> Name{GetAlignedPmrResource()};

// Aturan migrasi otomatis oleh pipeline kognitif:
// REFACTOR_TO_ALIGNED_SOA(float, m_flatPosX)
// REFACTOR_TO_ALIGNED_SOA(float, m_flatPosZ)
// REFACTOR_TO_ALIGNED_SOA(float, m_flatVelX)
// REFACTOR_TO_ALIGNED_SOA(float, m_flatVelZ)
// REFACTOR_TO_ALIGNED_SOA(uint32_t, m_flatEntityIDs)
```
