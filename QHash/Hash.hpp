#pragma once

//==============================================================================
// QHash (version 1.0)
//------------------------------------------------------------------------------
// Austin Quick, 2016 - 2019
// https://github.com/Daskie/QHash
// ...
//------------------------------------------------------------------------------



#include <cstdint>
#include <tuple>
#include <type_traits>
#include <string>
#include <string_view>



static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8, "Unsupported architecture");



namespace qc {

    using nat = intptr_t;
    using unat = uintptr_t;

    // Hash
    //==========================================================================
    // If the size of the key is less than or equal to the size of a word, a
    // perfect hash is returned using murmur3's mix functions.
    // Otherwise, a standard hash is computed using murmur3.

    template <typename K>
    struct Hash {

        // operator()
        //----------------------------------------------------------------------
        // Specializations exist for properly processing `std::string` and
        // `std::string_view`.
        // Template specialization can be used to define this method for other
        // types.

        unat operator()(const K & key) const;

    };

    // IdentityHash
    //==========================================================================
    // Returns the key reinterpreted as an unsigned integer.
    // A special case exists for pointer keys - the integer value is bit shifted
    // to the right such that the the lowest relevant bit is the lowest bit.
    // Only valid for keys no larger than a word.

    template <typename K>
    struct IdentityHash {

        static_assert(sizeof(K) <= sizeof(nat), "Types larger than a word should be properly hashed");

        // operator()
        //----------------------------------------------------------------------
        // Specializations exist for properly processing `std::string` and
        // `std::string_view`.
        // Template specialization can be used to define this method for other
        // types.

        unat operator()(const K & key) const;

    };

    // hash
    //--------------------------------------------------------------------------
    // Wrapper for the general murmur3 hash. Selects the best variant based on
    // the current architecture.

    unat hash(const void * key, unat n, unat seed = 0);



    // Murmur3
    //==========================================================================
    // https://github.com/PeterScott/murmur3
    //
    // MurmurHash3 was written by Austin Appleby, and is placed in the public
    // domain. The author hereby disclaims copyright to this source code.
    //
    // Note - The x86 and x64 versions do _not_ produce the same results, as the
    // algorithms are optimized for their respective platforms. You can still
    // compile and run any of them on any platform, but your performance with
    // the non-native version will be less than optimal.
    //--------------------------------------------------------------------------

    namespace murmur3 {

        // fmix32
        //----------------------------------------------------------------------
        // 32 bit perfect integer hash.

        constexpr uint32_t fmix32(uint32_t h);

        // fmix64
        //----------------------------------------------------------------------
        // 64 bit perfect integer hash.

        constexpr uint64_t fmix64(uint64_t h);

        // x86_32
        //----------------------------------------------------------------------
        // Produces a 32 bit hash; optimized for x86 platforms.

        uint32_t x86_32(const void * key, uint32_t n, uint32_t seed);

        // x86_128
        //----------------------------------------------------------------------
        // Produces a 128 bit hash; optimized for x86 platforms.

        std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> x86_128(const void * key, uint32_t n, uint32_t seed);

        // x64_128
        //----------------------------------------------------------------------
        // Produces a 128 bit hash; optimized for x64 platforms.

        std::pair<uint64_t, uint64_t> x64_128(const void * key, uint64_t n, uint64_t seed);

    }

}



#include "Hash.tpp"