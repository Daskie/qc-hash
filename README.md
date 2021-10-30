# QC Hash

###### Extremely fast unordered map and set library for C++20

## Implementation Specialization

How do you write a faster hash map than Google?

Specialize.

Hash map optimization is all about memory layout and data. The form of the keys and values is central to this. Large
elements versus small, comparison complexity, hashability, memory indirection, all of these are first class factors when
it comes to fast and efficient design.

For this reason, one single hash map implementation will never be the most optimal for every kind of data. The ideal
optimizations for string keys will not be the same as for integers, which in turn will not be the same as for huge
structures. This is evident in the stratification of existing hash map libraries, such as Facebook's
[Folly](https://github.com/facebook/folly/blob/main/folly/container/F14.md), which is comprised of three different
implementations suited for different needs, plus one "fast" variant that just serves as a compile-time switch for the
others.

We have chosen to take a similar approach. Two or three different implementations tailored to a certain family of data,
plus a "generic" wrapper that picks one at compile time based on key and value types.

For the time being, only one implementation is complete, which specializes in "raw" keys described below. In time, at
least one additional implementation will be added that is specialized for strings and any other keys not already
covered. At that time the generic wrapper will also be added.


## RawMap & RawSet

The goal of `qc::hash::RawMap` and `qc::hash::RawSet` is to provide the fastest possible hash map/set implementation for
"raw" keys, especially those that fit within a word. Speed is prioritized over memory, but as a smaller memory footprint
leads to fewer cache misses, and thereby faster operations, memory usage is still fairly low.

### What Is a "Raw" Key?

"Raw" is a made-up shorthand for a type that is ***uniquely representable***. This means that for every possible value of the type
there is exactly one unique binary representation. The type trait
[`std::has_unique_object_representations`](https://en.cppreference.com/w/cpp/types/has_unique_object_representations)
nearly defines this property, but the type need not be trivially copyable.

#### Common Raw Types
- Signed integers
- Unsigned integers
- Enums
- Pointers
- `std::unique_ptr`
- `std::shared_ptr`

Additionally, any compound type comprised exclusively of raw constituents is itself raw, e.g. `std::pair<int, int>`.

#### Common NON-Raw Types
- Strings (Value stored in heap, not in object binary)
- Floating-point numbers (`+0` and `-0` have different binary representations, also the mess that is `NaN`)

### Which Keys are Fastest?

Essentially, any type that can be reinterpreted as an unsigned integer will perform best.

More exactly, any type `T` that satisfies these two criteria:
1. `sizeof(T) <= sizeof(uintmax_t)`
2. `alignof(T) >= sizeof(T)`

### Features

- Single header file for super easy integration.


- `qc::hash::RawMap` mirrors the interface of `std::unordered_map` and `qc::hash::RawSet` mirrors the interface of
  `std::unordered_set` with a couple of exceptions:
  - No reference stability. Elements may be moved in memory on any rehashing operation.
  - No `key_equal` type. Due to how this implementation works, it is necessary for direct binary equality to be
    equivalent to key equality, thus user-defined equality functions are not allowed.
  - The `erase(iterator)` method does not return an iterator. This is a significant optimization in the case that the
    next iterator is not needed.
  - The `erase` methods do **not** invalidate iterators. This allows an iterator to the next element to be obtained
    simply by incrementing the iterator of the erased element.
  - As this implementation does not use a bucket-based system, the bucket interface is not provided.


- `qc::hash::RawMap` and `qc::hash::RawSet` share the same code. A set is simply an alias of a map with value type `void`.
  Compile-time checks and switches allow for the few minor differences to work with no run time cost.


- Support for heterogeneous keys allows for efficient and convenient lookup. The heterogeneity mechanism can be
  specialized for user defined types.


- Written fully in modern C++20. Takes advantage of features such as perfect forwarding, constexpr if's, and concepts to
  boost efficiency, enhance readability, and simplify generated errors.

### Benchmarks

[**--> Check out this very detailed spreadsheet with charts benchmarking all aspects of the implementation! <--**](https://docs.google.com/spreadsheets/d/1wo7oWsK7VL30ExXHS0Jypd_cPPWWOXXYvZxgz9ywwu4/edit?usp=sharing)

Benchmarks were made against the standard library implementation (MSVC) and five significant third-party libraries.

I highly recommend [this writeup](https://martin.ankerl.com/2019/04/01/hashmap-benchmarks-01-overview/) by Martin Ankerl
on hash map benchmarks for a larger selection of libraries. The five comparison libraries I chose were the fastest on
his list that I was able to get working.

The following table shows approximately how many times faster `qc::hash::RawSet` is than each given library. Benchmark
using 10,000,000 `u64` elements on an x64 processor. See the above link for more details.

Library | Insertion | Access | Iteration | Erasure | Overall
:---|:---:|:---:|:---:|:---:|:---:
MSVC 16.0 `std::unordered_set` | 6.0x | 2.9x | 2.2x | 6.7x | **4.9x**
[Google Abseil's `absl::flat_hash_set`](https://github.com/abseil/abseil-cpp) | 3.0x | 2.0x | 0.4x | 6.0x | **3.2x**
[Martinus' `robin_hood::unordered_set`](https://github.com/martinus/robin-hood-hashing) | 1.7x | 2.0x | 0.4x | 1.6x | **1.5x**
[Skarupke's `ska::flat_hash_set`](https://github.com/skarupke/flat_hash_map) | 1.7x | 2.3x | 0.9x | 2.1x | **1.8x**
[Tessil's `tsl::robin_set`](https://github.com/Tessil/robin-map) | 1.7x | 2.3x | 1.1x | 2.6x | **2.0x**
[Tessil's `tsl::sparse_hash_set`](https://github.com/Tessil/sparse-map) | 5.9x | 3.8x | 0.4x | 5.3x | **4.4x**

Note how this implementation is relatively slow at iteration. This is an unfortunate, if not unexpected, drawback.
However the massive increase in insert, access, and erase speeds more than makes up for it.

Here is a small selection of charts from the aforelinked spreadsheet:

<a href="https://docs.google.com/spreadsheets/d/1wo7oWsK7VL30ExXHS0Jypd_cPPWWOXXYvZxgz9ywwu4/edit?usp=sharing">
  <img src="https://docs.google.com/spreadsheets/d/e/2PACX-1vTy_JVhjus1EXWHBFODZwp-y7__2knBeqmFWMczncPRtvg8FJ55icYjGQPvZOHlPAb9iwC8YKaRYxMA/pubchart?oid=1902091540&format=image"/>
  <img src="https://docs.google.com/spreadsheets/d/e/2PACX-1vTy_JVhjus1EXWHBFODZwp-y7__2knBeqmFWMczncPRtvg8FJ55icYjGQPvZOHlPAb9iwC8YKaRYxMA/pubchart?oid=40951260&format=image"/>
  <img src="https://docs.google.com/spreadsheets/d/e/2PACX-1vTy_JVhjus1EXWHBFODZwp-y7__2knBeqmFWMczncPRtvg8FJ55icYjGQPvZOHlPAb9iwC8YKaRYxMA/pubchart?oid=1021208717&format=image"/>
  <img src="https://docs.google.com/spreadsheets/d/e/2PACX-1vTy_JVhjus1EXWHBFODZwp-y7__2knBeqmFWMczncPRtvg8FJ55icYjGQPvZOHlPAb9iwC8YKaRYxMA/pubchart?oid=448918666&format=image"/>
  <img src="https://docs.google.com/spreadsheets/d/e/2PACX-1vTy_JVhjus1EXWHBFODZwp-y7__2knBeqmFWMczncPRtvg8FJ55icYjGQPvZOHlPAb9iwC8YKaRYxMA/pubchart?oid=1637762243&format=image"/>
  <img src="https://docs.google.com/spreadsheets/d/e/2PACX-1vTy_JVhjus1EXWHBFODZwp-y7__2knBeqmFWMczncPRtvg8FJ55icYjGQPvZOHlPAb9iwC8YKaRYxMA/pubchart?oid=1575105570&format=image"/>
</a>

### How it works
