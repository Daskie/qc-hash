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
1 byte | 1.2x
2 bytes | 1.8x
4 bytes | 2.2x
8 bytes | **3.7x**
16 bytes | 1.3x
32 bytes | 2.4x
64 bytes | 4.2x
1024 bytes | 7.6x
64 bit pointer | **3.5x**

---

### `qc::Map`

Highly optimized bucket-based hashmap.

`qc::Map` vs `std::unordered_map` performance with 64 bit integer key and value...

Operation | Speedup Factor with `qc::Hash` | Speedup Factor with `std::hash`
---|---|---
Insertion | **1.5x** | 0.9x
Access | **4.7x** | 0.9x
Erasure | **11.2x** | 1.8x
