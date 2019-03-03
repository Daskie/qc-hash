# QHash

### Fast, lightweight hash map, hash set, and hashing implementation for `c++17`

Both `qc::Map` and `qc::Set` are fully drag'n'drop compatible with `std::unordered_map` and `std::unordered_set` respectively, adhering to the `c++17` standard, as well as some of the `c++20` standard.

---

### `qc::Map` and `qc::Set`

- `Map` and `Set` share the same code. A `Set` is simply defined as a `Map` with value type `void`
- Open addressing
- Linear probing
- Robin hood hashing
- Backing capacity is always a power of two for fast indexing
- Maximum load factor 1/2
- Minimum load factor is 1/8. This greatly helps when iterating over the container after erasing many elements
- Past the end, the buckets logically "loop" back around to the beginning. This allows rehashing to depend solely on the current load, and not on some maximum bucket size. Although the maximum load factor is hard set to 1/2 for performance, in theory 100% load is totally fine
- Memory overhead ranges from 0 bytes per element in the best case (seriously) to `max(alignof(Key), alignof(Val))` in the worst case. This is accomplished by intelligently placing the distance value (which can be as little as one byte) in the bucket such that total bucket size is minimized and distance size is maximized

# I plan to do a much more thorough write upin the future, but for now, here is the performance comparison with std::unordered_set

`qc::Set` vs `std::unordered_set` with 8 byte key, using an identity hash, compiled with MSVC on x64 architecture...

Operation | Speedup Factor
---|---
Insertion | **3.2x**
Access | **3.0x**
Iteration | **2.7x**
Erasure | **1.0x**

`qc::Set` vs `std::unordered_set` with 4 byte key, using an identity hash, compiled with MSVC on x86 architecture...

Operation | Speedup Factor
---|---
Insertion | **3.8x**
Access | **3.4x**
Iteration | **2.4x**
Erasure | **1.0x**

---

### `qc::Hash`

Wrapper for the [Murmur3](https://github.com/aappleby/smhasher/wiki/MurmurHash3) hash.

# Small key optimization
- Any key that fit within a word is instead hashed using the Murmur3 mixing functions, resulting in a fast, perfect hash
- Special care is taken with keys of odd size (3, 5, 6, and 7 bytes) to ensure there is no leakage from neighboring memory
- For the identity variant, pointers keys are right shifted such that the lowest bits are always relevant
