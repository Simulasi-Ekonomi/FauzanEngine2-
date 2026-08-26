# Material Import Batch V1

`MaterialImportPipeline::ImportMtlSet` imports one bounded in-memory MTL byte sequence, marks the candidate material asset ready, stages an explicit ordered set of up to sixteen unique material names, and commits the registry, material staging store, and ordered receipts only after every requested material succeeds.

Empty, duplicate, oversized, or missing names fail closed and preserve all caller-owned registry, staging, and receipt state. The method does not read paths, resolve `mtllib`, watch files, refresh live bindings, upload GPU resources, or persist bytes. The material import pipeline smoke proves a two-material ordered commit, duplicate-name rollback, missing-material rollback, two assets, and three staged materials. It passes in Release and AddressSanitizer with `detect_leaks=1`; current non-Vulkan broad suites pass 128/128 in both configurations. This is not a filesystem, hot reload, GPU, or production asset pipeline.
