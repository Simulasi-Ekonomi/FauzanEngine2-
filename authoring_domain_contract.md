# FauzanEngine Bounded Authoring Domain Contract

## Prinsip

Domain ini adalah fondasi data engine, bukan editor visual, generator konten otomatis, atau klaim bahwa game siap rilis. Semua ID adalah integer nonzero yang unik per jenis; text narrative memakai key/hash, bukan konten pemain atau kredensial. Struktur dibatasi supaya prompt/agent/dokumen tidak dapat menghasilkan graph tak terbatas atau menghabiskan memori.

| Domain | Kontrak minimum | Batas awal |
|---|---|---|
| Collision material | ID, hardness, friction, restitution, density dalam permille. | 64 material. |
| Skeleton | ID, maksimal 64 bone, parent harus muncul lebih dahulu, tepat satu root, panjang non-negatif. | 32 rig. |
| Character | ID, skeleton reference, collision material, health/stamina authoring values. | 128 definitions. |
| Building | ID, material, footprint grid valid, health. | 128 definitions. |
| Item | ID, item class, material, stack cap, weight. | 256 definitions. |
| Narrative | ID, content key/hash, maksimal empat branch destination yang dikenal. | 256 entries. |
| Actor NPC/monster | ID, class, character reference, behavior goal, movement rate dan material. | 256 definitions. |
| Scene | ID, maksimal 512 placement object yang mereferensikan actor/building. | 16 scenes. |

## Runtime bounded

`AuthoringSimulation` menginstansiasi scene ke `SceneWorld`, menyimpan transform grid actor, dan hanya menjalankan behavior deterministic sederhana: idle, patrol, wander, dan chase. Pergerakan memiliki batas world dan perubahan scene harus gagal tertutup jika entity/definition/scene tidak valid. Contact material menghasilkan respons hardness/friction/restitution terukur, tetapi belum mengubah XPBD collider atau renderer.

## Agent autonomy policy

| Operasi agent | Hasil default | Alasan |
|---|---|---|
| Diagnose failure, audit runtime, draft repair plan | Dry-run permitted. | Tidak mengubah source, artifact, runtime, atau data pemain. |
| Run bounded regression proposal | Approval dan evidence wajib. | Mengontrol biaya/waktu dan menjaga audit trail. |
| Propose learning dataset/evaluation | Proposal-only. | Tidak ada model training atau perubahan gameplay otomatis. |
| Apply source repair, runtime change, deploy, publish | Rejected. | Membutuhkan sandbox executor, review, artifact verification, secret boundary, dan persetujuan eksplisit yang belum tersedia. |
| Economy, receipt, ban, session, credential access | Rejected. | Authority tidak boleh berpindah ke agent. |

Self-repair yang diimplementasikan sekarang hanya berarti diagnosis terstruktur dan rencana rollback/test; self-learning berarti usulan dataset/evaluation; self-deploy tetap nonaktif. Tidak ada bagian dari kontrak ini yang memberikan kemampuan ala sistem agent eksternal untuk mengubah atau menerbitkan game tanpa persetujuan.
