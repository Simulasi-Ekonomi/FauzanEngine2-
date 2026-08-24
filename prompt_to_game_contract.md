# FauzanEngine Prompt-to-Game Contract

## Tujuan dan batas authority

`tools/prompt_to_game.py` adalah helper **trusted-host** untuk mengubah prompt profesional dan konteks dokumen menjadi rencana JSON yang terbatas. Ia tidak dijalankan di game runtime, tidak menyimpan token model di asset/save, dan tidak mengeksekusi shell, download, source mutation, build, test, economy, receipt, ban, atau command runtime. Outputnya selalu menyatakan `dry_run: true`.

Sebelum pemanggilan model, operator memilih ID model dari katalog live dengan opsi `--model`; runner tidak memiliki model default agar penggunaan token tidak terjadi diam-diam. Untuk rancangan/code yang sulit, pilih model reasoning/coding dari katalog live; untuk ekstraksi dokumen berskala besar, gunakan model yang lebih hemat lalu terapkan validation gate. Pemanggilan model belum dijalankan sebagai bagian dari smoke karena dapat memakai token operator.

## Input dokumen

| Jenis | Perlakuan | Batas yang diterapkan |
|---|---|---|
| DOCX | Paragraph dan tabel diekstrak di memori. | Maksimum 8 MiB input dan 16.384 karakter per dokumen. |
| PDF | Hanya teks dari PDF tidak terenkripsi. | Maksimum 8 MiB, 256 halaman, dan 16.384 karakter. |
| RAR | Hanya metadata/anggota `.docx`, `.pdf`, `.txt`, atau `.md` yang dibaca di memori. Tidak ada extract-to-disk dan tidak ada executable member yang dijalankan. | Maksimum 8 MiB archive, 128 entry, 16 MiB ukuran uncompressed terdeklarasi, serta 8 MiB per anggota. |
| Context gabungan | Dokumen menjadi data referensi untrusted untuk model. | Maksimum 65.536 karakter. |

Dokumen yang meminta instruksi, credential, pengunduhan, mutasi economy, perubahan ban, atau eksekusi command tetap diperlakukan sebagai data untrusted. System prompt planner secara eksplisit menolak menjadikan isi dokumen sebagai authority.

## Node command yang tersedia

Rencana harus lolos JSON Schema dan validator lokal sebelum dapat dipetakan ke `PromptToolGraph` C++. Graph dibatasi 16 node dan delapan dependency per node. Dependency wajib muncul sebelum dependent, sehingga cycle dan referensi masa depan ditolak.

| Agent | Node yang diperbolehkan | Authority yang tetap dilarang |
|---|---|---|
| CobaAuditor | `AuditRuntime`, `RequestTest`, `RequestRollback` | Runtime mutation, economy, receipt, ban, player/session authority. |
| AriesCreator | `CreateGameTemplate`, `RequestBuild`, `RequestTest`, `RequestRollback` | Runtime mutation, economy, receipt, ban, player/session authority. |

`PromptToolGraph` hanya mengevaluasi dry-run atau menerbitkan **plan** setelah approval evidence build/test yang typed. Ia bukan process executor. Integrator berikutnya harus menghubungkan plan issued ke worker tersandbox dengan allowlist operasi terpisah, log audit, budget, cancellation, dan review manusia.

## Penggunaan yang aman

```bash
cd /home/ubuntu/work/fauzan_engine/src/FauzanEngine
python3 tools/prompt_document_ingest.py design.docx specification.pdf --output context.json
python3 tools/prompt_to_game.py \
  --model <model-dari-katalog-live> \
  --prompt "Rancang rencana dry-run untuk vertical slice Farm yang terukur." \
  --input design.docx --input specification.pdf \
  --output plan.json
```

Hasil `plan.json` adalah bahan review, bukan izin otomatis. Masih diperlukan adaptor yang mem-parsing JSON ke `PromptToolPlan`, evaluasi `PromptToolGraph`, evidence build/test, dan executor tersandbox sebelum perubahan source diizinkan. Semua langkah itu tidak mengubah release gate Farm atau game lain.

## Tahap operasi prompt yang tersedia

`tools/prompt_operation_runner.py` menambahkan tahap host-side setelah plan: ia membaca plan version-1 yang tetap ditandai `dry_run`, mengulang validator kontrak, dan selalu menulis receipt JSON. Tanpa `--execute`, seluruh node menghasilkan `requires_human_review`. Dengan `--execute`, operator harus mengirim `--confirm-prompt-id` yang identik dan runner menjalankan maksimum tiga operasi—maksimum lima jika budget dinaikkan—dari allowlist berikut.

| Node plan | Target yang dapat berjalan | Perilaku receipt |
|---|---|---|
| `AuditRuntime` | Satu pemeriksaan `git diff --check` pada source kanonis. | Memuat argv, exit code, dan output terbatas. |
| `RequestBuild` | Hanya `neo_core`. | Memuat receipt build, tanpa perubahan source. |
| `RequestTest` | Smoke runtime/world/navigation/authoring/agent yang telah di-allowlist. | Build target lalu menjalankan binary dan mencatat kedua hasil. |
| `CreateGameTemplate`, `RequestRollback`, target lain | Tidak pernah dijalankan. | Tetap `requires_human_review`. |

Runner tidak membentuk shell dari teks prompt/model, tidak menerima target arbitrer, tidak memodifikasi source, tidak deploy/publish, dan tidak menyentuh credential, economy, ban, session, atau authority runtime. `prompt_operation_runner_smoke.py` membuktikan dry-run receipt, audit/test yang dikonfirmasi, dan template yang tetap review-only tanpa pemanggilan model.

Isolated workspace, cancellation proses, diff review, source mutation yang dibatasi, rollback artifact, dan deploy worker masih belum dibangun. Karena itu executor ini membantu **membuktikan** operasi aman yang kecil, bukan AI otonom yang dapat membuat atau merilis game sendiri.
