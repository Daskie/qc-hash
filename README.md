# QC Hash

###### Extremely fast unordered map and set library for C++20

### Contents
- [Implementation Specialization](#implementation-specialization)
- [RawMap & RawSet](#rawmap--rawset)
  - [What is a "Uniquely Representable" Type?](#what-is-a-uniquely-representable-type)
  - [Which Key and Value Types are Fastest?](#which-key-and-value-types-are-fastest)
  - [Features](#features)
  - [Design](#design)
  - [Additional Optimizations](#additional-optimizations)
  - [**Meta-less Data**](#meta-less-data)
  - [Benchmarks](#benchmarks)
- [TODO](#todo)

## Implementation Specialization

How do you write a faster hash map than Google?

Specialize.

Hash map optimization is all about memory layout and data. The size and alignment of elements, the meta data kept, how
it's all layed out in memory - all critical factors when it comes to fast and efficient design.

For this reason, one single hash map implementation will never be the most optimal for every kind of data. The ideal
optimizations for string keys will not be the same as for integers, which in turn will not be the same as for huge
compound types. This is evident in the stratification of existing hash map libraries, such as Facebook's
[Folly](https://github.com/facebook/folly/blob/main/folly/container/F14.md), which is comprised of three different
implementations suited for different needs, plus one "fast" variant that just serves as a compile-time switch for the
others.

We have chosen to take a similar approach. Two or three different implementations tailored to a certain family of data,
plus a "generic" wrapper that picks one at compile time based on key and value types.

For the time being, only one implementation is complete, `RawMap`/`RawSet`, which specializes in small keys that are
uniquely representable, a concept described below. In time, at least one additional implementation will be added that is
specialized for strings and any other keys not already covered. At that time the generic wrapper will also be added.


## RawMap & RawSet

The goal of `qc::hash::RawMap` and `qc::hash::RawSet` is to provide the fastest possible hash map/set implementation for
keys of ***uniquely representable*** type, especially those that fit within a word.

### What Is a "Uniquely Representable" Type?

A uniquely representable type has exactly one unique binary representation for each possible value of the type. The
standard type trait
[`std::has_unique_object_representations`](https://en.cppreference.com/w/cpp/types/has_unique_object_representations)
nearly defines this property, but the type need not be trivially copyable.

#### Common types that are uniquely representable
- Signed integers
- Unsigned integers
- Enums
- Pointers
- `std::unique_ptr`
- `std::shared_ptr`

Additionally, any compound type comprised exclusively of uniquely representable constituents is itself uniquely
representable, e.g. `std::pair<int, int>`.

#### Common types that are NOT uniquely representable
- `std::string` and `std::string_view` (value stored indirectly in heap, not in object binary)
- Floating point numbers (`+0` and `-0` have different binary representations, also the mess that is `NaN`)
- Any type that has virtual functions or base classes (v-table pointer may differ by object)

### Which Key and Value Types are Fastest?

For keys, essentially any type that can be reinterpreted as an unsigned integer will perform best.

More exactly, any key type `K` that satisfies these three criteria:
1. `sizeof(K) <= sizeof(uintmax_t)`
2. `sizeof(K) <= alignof(K)`
3. `sizeof(K)` is a power of two

For values, small to medium sized types are ideal, let's say less than 64 bytes roughly.

The larger each element is, the more memory most of the map/set functions need to cover, increasing cache pressure and
reducing overall speed.

### Features

#### Single header file
- Setup is super easy, simply copy and include `qc-hash.hpp`

#### Standards compliance
- `qc::hash::RawMap` mirrors `std::unordered_map`
- `qc::hash::RawSet` mirrors `std::unordered_set`
- There are a few exceptions:
  - No reference stability. Elements may be moved in memory on any rehashing operation
  - No `key_equal` type. Due to how this implementation works, it is necessary for direct binary equality to be
    equivalent to key equality, thus user-provided equality functions are not allowed
  - The `erase(iterator)` method does not return an iterator to the next element. This is a significant optimization in
    the general case of the next iterator not being needed. If the next iterator is needed, it can be obtained simply
    by incrementing the iterator to the erased element
  - No bucket interface as this implementation is not bucket based

#### RawMap and RawSet share the same code
- `RawSet` is simply an alias of `RawMap` with value type `void`
- Compile-time checks and switches facilitate the minor differences with no run time cost

#### Heterogeneous lookup
- Elements may be accessed using any key type compatable with the stored key type
- For example, a set of `std::unique_ptr<int>` may be accessed using `int *`
- The heterogeneity mechanism may be specialized for user defined types

#### Written in modern C++20
- Takes full advantage of features such as perfect forwarding, constexpr if's, concepts, and more
- Simpler template and compile-time control code for enhanced readability
- Concepts and type contraints improve compiler error messages

### Design

#### Open addressing
- Elements are stored in a single, contiguous backing array
- Allows lookups to happen directly, without indirection
- Typically means only a single cache miss for "cold" data
- Very fast for small elements, but performance does eventually drop off with increasing element size
- Invalidates reference stability, see [Standards Compliance](#standards-compliance)

#### Linear probing
- If a key is not found at first lookup, progresses forward through the slots until the key is found or a vacant slot
  is hit
- Grave tokens are inserted when elements are erased, making erasure a O(1) operation

#### Circuity
- The backing array is logically circular
- When inserting an element, if it would "fall off" the end, we simply wrap-around to the beginning instead of growing
- Allows for better saturation, even up to 100% if desired

#### Power of two capacity
- Capacity starts as a power of two, and doubles when the map/set grows, thus always remaining a power of two
- Allows the slot index of a key's hash to be found with a simple bitwise-and instead of an expensive integer modulo,
  a massive speedup
- This increases the likelihood to suffer from collisions if the keys have power-of-two patterns. See the
  [Identity hashing](#identity-hashing) section for more info

#### No per-element meta data
- Significant memory savings and performance gains
- Sets this implementation apart from others
- See the [Meta-less Data](#meta-less-data) section for an explanation

### Additional Optimizations

#### Lazy allocation
- New, empty maps/sets do not allocate memory
- Backing memory is not allocated until first insertion

#### Load factor of 0.5
- The map/set will grow upon exceeding 50% capacity
- This significatly decreases the chance of collision and speeds up operations at the cost of greater memory usage

#### Identity hashing
- The default hasher, `qc::hash::IdentityHash`, simply returns the lowest `size_t`'s worth of the key
- This is extremely fast for keys with decent low-order entropy
- Pointers are right-shifted by the log2 of the pointee type's alignment to drop trailing zero bits
- If low-order entropy is a concern, such as with keys expressing power-of-two patterns, an alteranative hasher is
  available. `qc::hash::FastHash` is very minimal, providing sufficient entropy while being as fast as possible
- The user may provide their own hasher if desired

### Meta-less Data

One of the biggest differentiators between hash map implementations is how they store element meta data. Some store it
alongside each element, others store it in a separate array. Some store precomputed hashes, others store
just a couple bits. Regardless of the strategy, meta data is necessary for a performant, fully featured hash map, right?

Wrong.

This implementation has no per-element meta data. How this works is explained below, but first, what are the
repercussions?

#### Pros
- Lowest memory usage - the only memory allocated is what is needed to store the elements at the desired load capacity
- Memory locality - element operations only touch a single place in memory
- Amazing insert, access, and erase performance due to better cache utilization and simpler program logic

#### Cons
- Worse iteration performance than some other strategies
- Only supports uniquely representable keys

#### How it works

Of all possible keys, two are considered "special". The first is the key with the binary representation `0b111...111`,
which is considered the "vacant" key. The second is the key with the binary representation `0b111...110`, which is
considered the "grave" key. If a key has any other binary representation it is considered a "normal" key.

The backing slots are split into three parts:
1. "Normal" slots: the largest part, these hold typical elements, grow when the map/set expands, and generally function
   as expected. Vacant slots contain the "vacant" special key, and grave slots contain the "grave" special key
2. "Special" slots: two slots that exist solely to store the two special elements should they be present. The first
   contains the "vacant" special key if it is vacant and the "grave" special key if it is present. The second contains
   the "grave" special key if it is vacant and the "vacant" special key if it is present
3. "Terminal" slots: two slots with key values of `0b000...000` which mark the end of the map/set

Together, the slots look something like: `|N|N|N|...|N|N|N|G|E|0|0|`, where `N` is a normal slot, `G` is the special
slot for the grave key, `E` is the special slot for the vacant key, and `0` is a terminal slot

When an operation is performed with a normal key, only the normal slots are used, and things work as one would expect.
When an operation is performed with a special key, only the corresponding special slot is used.

The two terminal slots remain constant over the lifetime of the map/set and exist solely to allow incrementing iterators
to determine if and where they are at the end of the slots.

### Benchmarks

[**--> Check out this very detailed spreadsheet with charts benchmarking all aspects of the implementation! <--**](https://docs.google.com/spreadsheets/d/1wo7oWsK7VL30ExXHS0Jypd_cPPWWOXXYvZxgz9ywwu4/edit?usp=sharing)

Benchmarks were made against the standard library implementation (MSVC) and five significant third-party libraries.

I highly recommend [this writeup](https://martin.ankerl.com/2019/04/01/hashmap-benchmarks-01-overview/) by Martin Ankerl
on hash map benchmarks for a larger selection of libraries and much more in-depth analysis. The five comparison
libraries I chose were the fastest on his list that didn't have ridiculous build requirements.

The following table shows approximately how many times faster `qc::hash::RawSet` is than each given library. Benchmark
using 10,000,000 `uint64_t` elements on an x64 intel processor. See the above link for more details.

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

Here is a small selection of the most notable charts from the
[spreadsheet](https://docs.google.com/spreadsheets/d/1wo7oWsK7VL30ExXHS0Jypd_cPPWWOXXYvZxgz9ywwu4/edit?usp=sharing):

<a href="https://docs.google.com/spreadsheets/d/1wo7oWsK7VL30ExXHS0Jypd_cPPWWOXXYvZxgz9ywwu4/edit?usp=sharing">
  <img src="https://docs.google.com/spreadsheets/d/e/2PACX-1vTy_JVhjus1EXWHBFODZwp-y7__2knBeqmFWMczncPRtvg8FJ55icYjGQPvZOHlPAb9iwC8YKaRYxMA/pubchart?oid=1902091540&format=image"/>
  <img src="https://docs.google.com/spreadsheets/d/e/2PACX-1vTy_JVhjus1EXWHBFODZwp-y7__2knBeqmFWMczncPRtvg8FJ55icYjGQPvZOHlPAb9iwC8YKaRYxMA/pubchart?oid=40951260&format=image"/>
  <img src="https://docs.google.com/spreadsheets/d/e/2PACX-1vTy_JVhjus1EXWHBFODZwp-y7__2knBeqmFWMczncPRtvg8FJ55icYjGQPvZOHlPAb9iwC8YKaRYxMA/pubchart?oid=1021208717&format=image"/>
  <img src="https://docs.google.com/spreadsheets/d/e/2PACX-1vTy_JVhjus1EXWHBFODZwp-y7__2knBeqmFWMczncPRtvg8FJ55icYjGQPvZOHlPAb9iwC8YKaRYxMA/pubchart?oid=448918666&format=image"/>
  <img src="https://docs.google.com/spreadsheets/d/e/2PACX-1vTy_JVhjus1EXWHBFODZwp-y7__2knBeqmFWMczncPRtvg8FJ55icYjGQPvZOHlPAb9iwC8YKaRYxMA/pubchart?oid=1637762243&format=image"/>
  <img src="https://docs.google.com/spreadsheets/d/e/2PACX-1vTy_JVhjus1EXWHBFODZwp-y7__2knBeqmFWMczncPRtvg8FJ55icYjGQPvZOHlPAb9iwC8YKaRYxMA/pubchart?oid=1575105570&format=image"/>
</a>

## TODO

- Alternative implementation for strings and other larger/complex types
- Allow user to request load capacity above 50%
