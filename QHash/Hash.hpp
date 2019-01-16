//==============================================================================
// QHash ///////////////////////////////////////////////////////////////////////
//==============================================================================
// Austin Quick, 2016 - 2018
// https://github.com/Daskie/QHash
//------------------------------------------------------------------------------



#pragma once



static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8, "Unsupported architecture");



#include <tuple>



namespace qc {

    using nat = intptr_t;
    using unat = uintptr_t;

    //==========================================================================
    // Hash ////////////////////////////////////////////////////////////////////
    //==========================================================================
    // ...
    //--------------------------------------------------------------------------

    template <typename K>
    struct Hash {

        unat operator()(const K & key) const;

    };

    //==========================================================================
    // NoHash //////////////////////////////////////////////////////////////////
    //==========================================================================
    // ...
    //--------------------------------------------------------------------------

    template <typename K>
    struct NoHash {

        static_assert(sizeof(K) <= sizeof(unat));

        unat operator()(const K & key) const;

    };

    //==========================================================================
    // hash
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    unat hash(const void * key, unat n, unat seed = 0);



    //==========================================================================
    // Murmur3 /////////////////////////////////////////////////////////////////
    //==========================================================================
    // MurmurHash3 was written by Austin Appleby, and is placed in the public
    // domain. The author hereby disclaims copyright to this source code.
    //
    // Note - The x86 and x64 versions do _not_ produce the same results, as the
    // algorithms are optimized for their respective platforms. You can still
    // compile and run any of them on any platform, but your performance with
    // the non-native version will be less than optimal.
    //--------------------------------------------------------------------------
    // https://github.com/PeterScott/murmur3
    //==========================================================================

    namespace murmur3 {

        //======================================================================
        // fmix32
        //----------------------------------------------------------------------
        // 32 bit perfect integer hash.
        //----------------------------------------------------------------------

        constexpr uint32_t fmix32(uint32_t h);

        //======================================================================
        // fmix64
        //----------------------------------------------------------------------
        // 64 bit perfect integer hash.
        //----------------------------------------------------------------------

        constexpr uint64_t fmix64(uint64_t h);

        //======================================================================
        // x86_32
        //----------------------------------------------------------------------
        // Produces a 32 bit hash; optimized for x86 platforms.
        //----------------------------------------------------------------------

        uint32_t x86_32(const void * key, uint32_t n, uint32_t seed);

        //======================================================================
        // x86_128
        //----------------------------------------------------------------------
        // Produces a 128 bit hash; optimized for x86 platforms.
        //----------------------------------------------------------------------

        std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> x86_128(const void * key, uint32_t n, uint32_t seed);

        //======================================================================
        // x64_128
        //----------------------------------------------------------------------
        // Produces a 128 bit hash; optimized for x64 platforms.
        //----------------------------------------------------------------------

        std::pair<uint64_t, uint64_t> x64_128(const void * key, uint64_t n, uint64_t seed);

    }

}



#include "Hash.tpp"
