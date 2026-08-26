# Farm Runtime Save Codec V1

`FarmRuntimeSaveCodec` composes canonical in-memory `FarmSystem::Serialize` / `Deserialize` with `RuntimeSaveCodec` under the exact kind `farm-world`. Encoding accepts only a ready Farm system and a nonzero caller revision. Decoding validates the runtime envelope, exact kind, nonzero revision, and a separate candidate Farm deserialize before it applies the bytes to the target Farm and updates the caller revision.

Malformed envelopes, wrong kinds, zero revisions, and corrupt Farm payloads fail closed and leave caller Farm state and revision intact. This codec does not read or write files, contact services, authenticate players, merge conflicts, encrypt cloud data, or establish production save readiness. The dedicated smoke proves in-memory Farm state round-trip at revision 7 and wrong-kind/checksum failure preservation. It passes in Release and AddressSanitizer with `detect_leaks=1`; current non-Vulkan broad suites pass 130/130 in both configurations.
