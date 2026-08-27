# Farm Commerce Entitlement Ledger V1

`FarmCommerceEntitlementLedger` adalah boundary proses-lokal untuk entitlement coin Farm. Ia menerima `FarmProviderReceipt` hanya untuk `playerId` yang dikonfigurasi, memerlukan ID receipt bukan nol, jumlah entitlement positif, payload authority ASCII terbatas, status tidak dibalik, dan verifier caller yang mengesahkan receipt. Setelah itu ia meneruskan satu `VerifiedTopUpReceipt` ke `FarmWorldTool`, sehingga pemeriksaan Farm dan TrustSafety yang sudah ada tetap menjadi jalur mutasi tunggal.

Ledger menolak player yang berbeda, verifier gagal, receipt ID duplikat, receipt yang dibalik, input tidak valid, kapasitas penuh, dan kegagalan penerapan Farm. Tidak ada pembayaran atau koneksi provider dilakukan oleh modul ini. Receipt yang sudah diterima disimpan sebagai ID dan jumlah untuk rekonsiliasi in-memory; rekonsiliasi mengharuskan pasangan ID/jumlah sama persis.

Setiap tindakan menghasilkan audit receipt V1 salinan-milik-caller yang merekam sequence, hasil, ID receipt, jumlah entitlement, dan simulation tick world. Modifikasi caller pada receipt yang diterima tidak dapat mengubah `LastAudit()` ledger. Audit log dibatasi 256 entri dan accepted-receipt ledger dibatasi 1.024 entri; ketika batas tercapai, operasi ditolak.

> Modul ini bukan payment processor, verifikasi tanda tangan provider, refund/reversal processor, durable ledger, rekonsiliasi eksternal, TLS, keamanan kunci, atau bukti monetisasi/store readiness. Provider nyata harus melakukan verifikasi server-side dan meneruskan hanya receipt yang tervalidasi ke boundary ini.

`farm_commerce_entitlement_smoke` lulus pada Release dan AddressSanitizer `detect_leaks=1`; ia memeriksa receipt diterima, wrong-player, verifier gagal, duplicate, reversal untuk receipt diterima, rekonsiliasi mismatch/sukses, dan isolasi salinan audit.
