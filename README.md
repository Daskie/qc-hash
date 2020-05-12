# QHash

### Fast and lightweight hashing, hash map, and hash set header-only implementation for `C++17`

Both `qc::Map` and `qc::Set` are fully drag'n'drop compatible with `std::unordered_map` and `std::unordered_set` respectively, adhering to the `C++17` standard and most of the `C++20` standard.

---

### `qc::Map` and `qc::Set`

- `Map` and `Set` share the same code. A `Set` is simply defined as a `Map` with value type `void`. This is a compile-time abstraction and incurs no performance cost
- Open addressing
- Linear probing
- Robin hood hashing
- Backing capacity is always a power of two for fast indexing
- Maximum load factor is 1/2
- Minimum load factor is 1/8
- Past the end, the buckets logically "loop" back around to the beginning. Thus, rehashing depends solely on the current load, and not on some maximum bucket size. This allows the backing memory to be 100% saturated, if the load factor is changed to 1
- Memory overhead ranges from 0 bytes per element in the best case (seriously) to `max(alignof(Key), alignof(Val))` in the worst case. This is accomplished by intelligently organizing the bucket structure such that the distance value fits "between the gaps" created when the key and value have different alignments.

### I plan to do a much more thorough write up in the future, but for now, here is the performance comparison with std::unordered_set

`qc::Set` vs `std::unordered_set` with 8 byte key, using an identity hash, compiled with MSVC (release) on x64 architecture...

Operation | Speedup Factor
---|---
Insertion | **3.2x**
Access | **3.0x**
Iteration | **2.7x**
Erasure | **1.0x**

`qc::Set` vs `std::unordered_set` with 4 byte key, using an identity hash, compiled with MSVC (release) on x86 architecture...

Operation | Speedup Factor
---|---
Insertion | **3.8x**
Access | **3.4x**
Iteration | **2.4x**
Erasure | **1.0x**

---

### `qc::Hash`

Wrapper for the [Murmur3](https://github.com/aappleby/smhasher/wiki/MurmurHash3) hash.

### Small key optimization
- Any key that fit within a word is instead hashed using the Murmur3 mixing functions, resulting in a fast, perfect hash
- Special care is taken with keys of odd size (3, 5, 6, and 7 bytes) to ensure there is no leakage from neighboring memory
- For identity hashing of pointers, the pointer is right shifted to discard the dead bits
