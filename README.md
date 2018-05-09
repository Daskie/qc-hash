# QHash

### Fast, lightweight hashing and hashmap header implementation for `c++17`

Both `qc::Hash` and `qc::Map` are fully drag'n'drop replaceable with `std::hash` and `std::unordered_map` respectively.

---

### `qc::Hash`

Optimized for short keys and uses the [Murmur3](https://github.com/aappleby/smhasher/wiki/MurmurHash3) hash for keys of arbitrary length.

Special hash for pointers, which disregards irrelevant low order bits.
Additionally, provides `qc::hash` convenience function to directly hash arbitrary data via void pointer.

`qc::Hash` vs `std::hash` performance...

Key Type | Speedup Factor
---|---
1 byte | 1.1x
2 bytes | 1.4x
4 bytes | 2.1x
8 bytes | 3.6x
16 bytes | 1.3x
32 bytes | 2.6x
64 bytes | 3.7x
1024 bytes | 7.5x
64 bit pointer | 3.3x

---

### `qc::Map`

Highly optimized bucket-based hashmap.
