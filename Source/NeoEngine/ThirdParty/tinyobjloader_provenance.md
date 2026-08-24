# TinyObjLoader vendor provenance

`tiny_obj_loader.h` adalah salinan tanpa modifikasi dari TinyObjLoader, berlisensi MIT dan menyimpan teks lisensi pada header itu sendiri.

| Field | Value |
|---|---|
| Upstream | https://github.com/tinyobjloader/tinyobjloader |
| Commit | `45636bdcef1a4fec140346b90c0b50bf0bc3e23b` (`release`) |
| File | `tiny_obj_loader.h` |
| SHA-256 | `cffc9c8746541bd14a3ba877cd3c52cb7ccfe3779b1d5863f7d13f05d26f1ef8` |
| License | MIT |

FauzanEngine hanya memakai API `ObjReader::ParseFromString` melalui adapter `ObjMeshImporter`; tidak mengaktifkan file I/O, resolver `.mtl`, pemuatan tekstur, atau parsing paralel.
