//==============================================================================
// QHash ///////////////////////////////////////////////////////////////////////
//==============================================================================
// Austin Quick, 2016 - 2017 
//------------------------------------------------------------------------------



#pragma once



#include <string>

#include "QCU/Bits.hpp"



namespace qcu {



struct u128 {

    union {
        struct { u64 h1, h2; };
        struct { u32 q1, q2, q3, q4; };
    };

    u128() = default;
    u128(u64 h1, u64 h2) : h1(h1), h2(h2) {}
    u128(u32 q1, u32 q2, u32 q3, u32 q4) : q1(q1), q2(q2), q3(q3), q4(q4) {}
    template <typename T, eif_t<std::is_unsigned_v<T>> = 0> u128(T v) : h1(v), h2(0) {}

};

template <> struct precision<16> { using utype = u128; };



}



namespace qhm {



using namespace qcu;



namespace config {

namespace hash {

constexpr bool smallKeyOptimization = false;

}

}



//======================================================================================================================
// HASHING INTERFACE ///////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// hash
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------

template <int t_p = k_nat_p, typename K>
precision_ut<t_p> hash(const K & key, unat seed = 0);

template <int t_p = k_nat_p>
precision_ut<t_p> hash(const std::string & key, unat seed = 0);

template <int t_p = k_nat_p, typename K>
precision_ut<t_p> hashv(const K * key, unat nElements, unat seed = 0);



//======================================================================================================================
// Murmur3 /////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
//
// Note - The x86 and x64 versions do _not_ produce the same results, as the
// algorithms are optimized for their respective platforms. You can still
// compile and run any of them on any platform, but your performance with the
// non-native version will be less than optimal.
//------------------------------------------------------------------------------
// https://github.com/PeterScott/murmur3
//==============================================================================



namespace murmur3 {



//==============================================================================
// rotl
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------

constexpr u32 rotl32(u32 x, u32 n);
constexpr u64 rotl64(u64 x, u64 n);



//==============================================================================
// fmix
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------

constexpr u32 fmix32(u32 h);
constexpr u64 fmix64(u64 h);



//==============================================================================
// x86_32
//------------------------------------------------------------------------------
// Produces a 32 bit hash; optimized for x86 platforms.
//------------------------------------------------------------------------------

u32 x86_32(const void * key, unat n, u32 seed);



//==============================================================================
// x86_128
//------------------------------------------------------------------------------
// Produces a 128 bit hash; optimized for x86 platforms.
//------------------------------------------------------------------------------

u128 x86_128(const void * key, unat n, u32 seed);



//==============================================================================
// x64_128
//------------------------------------------------------------------------------
// Produces a 128 bit hash; optimized for x64 platforms.
//------------------------------------------------------------------------------

u128 x64_128(const void * key, unat n, u64 seed);



}



//==============================================================================================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//==============================================================================================================================================================



//======================================================================================================================
// HASH INTERFACE IMPLEMENTATION ///////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// hash
//------------------------------------------------------------------------------

template <int t_p, typename K>
precision_ut<t_p> hash(const K & key, unat seed) {
    static_assert(t_p == 4 || t_p == 8 || t_p == 16, "unsupported precision");

    if constexpr (config::hash::smallKeyOptimization && sizeof(K) <= t_p && t_p % sizeof(K) == 0) {
        precision_ut<t_p> h(*reinterpret_cast<const precision_ut<sizeof(K)> *>(&key));

        bits::scramble(h);

        if constexpr (t_p == k_nat_p) {
            h ^= seed;
        }
        if constexpr (t_p == 4 && k_nat_p == 8) {
            h ^= precision_ut<t_p>(seed) ^ precision_ut<t_p>(seed >> 32);
        }
        if constexpr (t_p == 8 && k_nat_p == 4) {
            h ^= precision_ut<t_p>(seed) | (precision_ut<t_p>(seed) << 32);
        }
        if constexpr (t_p == 16 && k_nat_p == 4) {
            h.q1 ^= seed; h.q2 ^= seed; h.q3 ^= seed; h.q4 ^= seed;
        }
        if constexpr (t_p == 16 && k_nat_p == 8) {
            h.h1 ^= seed; h.h2 ^= seed;
        }

        return h;
    }
    else {
        return hashv<t_p>(&key, 1, seed);
    }
}

template <int t_p>
precision_ut<t_p> hash(const std::string & key, unat seed) {
    return hashv<t_p>(key.c_str(), key.size(), seed);
}

template <int t_p, typename K>
precision_ut<t_p> hashv(const K * key, unat nElements, unat seed) {
    static_assert(t_p == 4 || t_p == 8 || t_p == 16, "unsupported precision");

    if constexpr (t_p == 4) {
        if constexpr (k_nat_p == 4) return murmur3::x86_32(key, nElements * sizeof(K), seed);
        if constexpr (k_nat_p == 8) return murmur3::x64_128(key, nElements * sizeof(K), seed).q1;
    }
    if constexpr (t_p == 8) {
        if constexpr (k_nat_p == 4) return murmur3::x86_128(key, nElements * sizeof(K), seed).h1;
        if constexpr (k_nat_p == 8) return murmur3::x64_128(key, nElements * sizeof(K), seed).h1;
    }
    if constexpr (t_p == 16) {
        if constexpr (k_nat_p == 4) return murmur3::x86_128(key, nElements * sizeof(K), seed);
        if constexpr (k_nat_p == 8) return murmur3::x64_128(key, nElements * sizeof(K), seed);
    }
}



//======================================================================================================================
// MURMUR3 IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



namespace murmur3 {



//==============================================================================
// rotl
//------------------------------------------------------------------------------

constexpr u32 rotl32(u32 x, u32 n) {
    return (x << n) | (x >> (static_cast<u32>(32) - n));
}

constexpr u64 rotl64(u64 x, u64 n) {
    return (x << n) | (x >> (static_cast<u64>(64) - n));
}



//==============================================================================
// fmix
//------------------------------------------------------------------------------

constexpr u32 fmix32(u32 h) {
    h ^= h >> 16;
    h *= u32(0x85ebca6b);
    h ^= h >> 13;
    h *= u32(0xc2b2ae35);
    h ^= h >> 16;

    return h;
}

constexpr u64 fmix64(u64 h) {
    h ^= h >> 33;
    h *= u64(0xff51afd7ed558ccdULL);
    h ^= h >> 33;
    h *= u64(0xc4ceb9fe1a85ec53ULL);
    h ^= h >> 33;

    return h;
}



//==============================================================================
// x86_32
//------------------------------------------------------------------------------

inline u32 x86_32(const void * key, unat n, u32 seed) {
    const u08 * data(reinterpret_cast<const u08 *>(key));
    const nat nblocks(n / 4);

    u32 h1(seed);

    constexpr u32 c1(0xcc9e2d51);
    constexpr u32 c2(0x1b873593);

    const u32 * blocks(reinterpret_cast<const u32 *>(data + nblocks * 4));

    for (nat i(-nblocks); i < 0; ++i) {
        u32 k1(blocks[i]);

        k1 *= c1;
        k1  = rotl32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1  = rotl32(h1, 13);
        h1  = h1 * u32(5) + u32(0xe6546b64);
    }

    const u08 * tail(reinterpret_cast<const u08 *>(data + nblocks * 4));

    u32 k1(0);

    switch (n & 0b11) {
        case 0b11: k1 ^= tail[2] << 16;
        case 0b10: k1 ^= tail[1] << 8;
        case 0b01: k1 ^= tail[0];
            k1 *= c1;
            k1  = rotl32(k1, 15);
            k1 *= c2;
            h1 ^= k1;
    };

    h1 ^= u32(n);

    h1 = fmix32(h1);

    return h1;
}



//==============================================================================
// x86_128
//------------------------------------------------------------------------------

inline u128 x86_128(const void * key, unat n, u32 seed) {
    const u08 * data(reinterpret_cast<const u08 *>(key));
    const nat nblocks(n / 16);

    u32 h1(seed);
    u32 h2(seed);
    u32 h3(seed);
    u32 h4(seed);

    constexpr u32 c1(0x239b961b);
    constexpr u32 c2(0xab0e9789);
    constexpr u32 c3(0x38b34ae5);
    constexpr u32 c4(0xa1e38b93);

    const u32 * blocks(reinterpret_cast<const u32 *>(data + nblocks * 16));

    for (nat i(-nblocks); i < 0; ++i) {
        u32 k1(blocks[i * 4 + 0]);
        u32 k2(blocks[i * 4 + 1]);
        u32 k3(blocks[i * 4 + 2]);
        u32 k4(blocks[i * 4 + 3]);

        k1 *= c1;
        k1  = rotl32(k1, 15);
        k1 *= c2;
        h1 ^= k1;

        h1  = rotl32(h1, 19);
        h1 += h2;
        h1  = h1 * u32(5) + u32(0x561ccd1b);

        k2 *= c2;
        k2  = rotl32(k2, 16);
        k2 *= c3;
        h2 ^= k2;

        h2  = rotl32(h2, 17);
        h2 += h3;
        h2  = h2 * u32(5) + u32(0x0bcaa747);

        k3 *= c3;
        k3  = rotl32(k3, 17);
        k3 *= c4;
        h3 ^= k3;

        h3  = rotl32(h3, 15);
        h3 += h4;
        h3  = h3 * u32(5) + u32(0x96cd1c35);

        k4 *= c4;
        k4  = rotl32(k4, 18);
        k4 *= c1;
        h4 ^= k4;

        h4  = rotl32(h4, 13);
        h4 += h1;
        h4  = h4 * u32(5) + u32(0x32ac3b17);
    }

    const u08 * tail(data + nblocks * 16);

    u32 k1(0);
    u32 k2(0);
    u32 k3(0);
    u32 k4(0);

    switch (n & 0b1111) {
        case 0b1111: k4 ^= tail[14] << 16;
        case 0b1110: k4 ^= tail[13] << 8;
        case 0b1101: k4 ^= tail[12] << 0;
            k4 *= c4;
            k4  = rotl32(k4, 18);
            k4 *= c1;
            h4 ^= k4;

        case 0b1100: k3 ^= tail[11] << 24;
        case 0b1011: k3 ^= tail[10] << 16;
        case 0b1010: k3 ^= tail[ 9] <<  8;
        case 0b1001: k3 ^= tail[ 8] <<  0;
            k3 *= c3;
            k3  = rotl32(k3, 17);
            k3 *= c4;
            h3 ^= k3;

        case 0b1000: k2 ^= tail[7] << 24;
        case 0b0111: k2 ^= tail[6] << 16;
        case 0b0110: k2 ^= tail[5] <<  8;
        case 0b0101: k2 ^= tail[4] <<  0;
            k2 *= c2;
            k2  = rotl32(k2, 16);
            k2 *= c3;
            h2 ^= k2;

        case 0b0100: k1 ^= tail[3] << 24;
        case 0b0011: k1 ^= tail[2] << 16;
        case 0b0010: k1 ^= tail[1] <<  8;
        case 0b0001: k1 ^= tail[0] <<  0;
            k1 *= c1;
            k1  = rotl32(k1, 15);
            k1 *= c2;
            h1 ^= k1;
    };

    h1 ^= n;
    h2 ^= n;
    h3 ^= n;
    h4 ^= n;

    h1 += h2;
    h1 += h3;
    h1 += h4;
    h2 += h1;
    h3 += h1;
    h4 += h1;

    h1 = fmix32(h1);
    h2 = fmix32(h2);
    h3 = fmix32(h3);
    h4 = fmix32(h4);

    h1 += h2;
    h1 += h3;
    h1 += h4;
    h2 += h1;
    h3 += h1;
    h4 += h1;

    return u128(h1, h2, h3, h4);
}



//==============================================================================
// x64_128
//------------------------------------------------------------------------------

inline u128 x64_128(const void * key, unat n, u64 seed) {
    const u08 * data(reinterpret_cast<const u08 *>(key));
    const nat nblocks(n / 16);

    u64 h1(seed);
    u64 h2(seed);

    const u64 c1(0x87c37b91114253d5ULL);
    const u64 c2(0x4cf5ad432745937fULL);

    const u64 * blocks(reinterpret_cast<const u64 *>(data));

    for (nat i(0); i < nblocks; ++i) {
        u64 k1(blocks[i * 2 + 0]);
        u64 k2(blocks[i * 2 + 1]);

        k1 *= c1;
        k1  = rotl64(k1, 31);
        k1 *= c2;
        h1 ^= k1;

        h1  = rotl64(h1, 27);
        h1 += h2;
        h1  = h1 * u64(5) + u64(0x52dce729);

        k2 *= c2;
        k2  = rotl64(k2, 33);
        k2 *= c1;
        h2 ^= k2;

        h2  = rotl64(h2, 31);
        h2 += h1;
        h2  = h2 * u64(5) + u64(0x38495ab5);
    }

    const u08 * tail(data + nblocks * 16);

    u64 k1(0);
    u64 k2(0);

    switch (n & 0b1111) {
        case 0b1111: k2 ^= static_cast<u64>(tail[14]) << 48;
        case 0b1110: k2 ^= static_cast<u64>(tail[13]) << 40;
        case 0b1101: k2 ^= static_cast<u64>(tail[12]) << 32;
        case 0b1100: k2 ^= static_cast<u64>(tail[11]) << 24;
        case 0b1011: k2 ^= static_cast<u64>(tail[10]) << 16;
        case 0b1010: k2 ^= static_cast<u64>(tail[ 9]) <<  8;
        case 0b1001: k2 ^= static_cast<u64>(tail[ 8]) <<  0;
            k2 *= c2;
            k2  = rotl64(k2, 33);
            k2 *= c1;
            h2 ^= k2;

        case 0b1000: k1 ^= static_cast<u64>(tail[ 7]) << 56;
        case 0b0111: k1 ^= static_cast<u64>(tail[ 6]) << 48;
        case 0b0110: k1 ^= static_cast<u64>(tail[ 5]) << 40;
        case 0b0101: k1 ^= static_cast<u64>(tail[ 4]) << 32;
        case 0b0100: k1 ^= static_cast<u64>(tail[ 3]) << 24;
        case 0b0011: k1 ^= static_cast<u64>(tail[ 2]) << 16;
        case 0b0010: k1 ^= static_cast<u64>(tail[ 1]) <<  8;
        case 0b0001: k1 ^= static_cast<u64>(tail[ 0]) <<  0;
            k1 *= c1;
            k1  = rotl64(k1, 31);
            k1 *= c2;
            h1 ^= k1;
    };

    h1 ^= n;
    h2 ^= n;

    h1 += h2;
    h2 += h1;

    h1 = fmix64(h1);
    h2 = fmix64(h2);

    h1 += h2;
    h2 += h1;

    return u128(h1, h2);
}



}



}