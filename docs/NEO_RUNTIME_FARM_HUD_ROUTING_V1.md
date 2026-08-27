# NeoRuntime Farm HUD Routing V1

`NeoRuntime::RouteFarmHudPointer` dan `RouteFarmHudKeyboard` hanya mendelegasikan input ke `FarmRuntimeHud` dan `FarmPlayerInputBridge` yang sudah dimiliki runtime. API menolak state runtime yang tidak initialized maupun HUD/bridge yang tidak tersedia.

Routing dapat mengganti `FarmPlayerAction` terpilih saja. Ia tidak memanggil `FarmWorldTool`, tidak melakukan tick, dan tidak mengubah tile. Aksi Farm tetap memerlukan event interact yang diproses kemudian oleh urutan `NeoRuntime::Tick` kanonis.

`runtime_smoke` lulus pada Release dan AddressSanitizer `detect_leaks=1`, membuktikan routing tidak tersedia pada runtime tanpa HUD, seleksi keyboard tidak mengubah tile, serta Till baru berjalan pada tick setelah input interact. Ini bukan touch-device evidence, sistem menu umum, UI accessibility, atau game loop production.
