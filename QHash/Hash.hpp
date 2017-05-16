//==============================================================================
// QHash ///////////////////////////////////////////////////////////////////////
//==============================================================================
// Austin Quick, 2017 
//------------------------------------------------------------------------------



#pragma once



#include <iostream>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <memory>
#include <ostream>

#include "QMU/QMU.hpp"



namespace qmu {



struct  s128 {
    union {
        struct { s64 s64_1, s64_2; };
        struct { s32 s32_1, s32_2, s32_3, s32_4; };
    };
};

struct u128 {
    union {
        struct { u64 u64_1, u64_2; };
        struct { u32 u32_1, u32_2, u32_3, u32_4; };
    };
};

template<> struct precision<128> { using stype = s128; using utype = u128; };



//======================================================================================================================
// HASHING INTERFACE ///////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// hash
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------

template <nat t_p = k_nat_p, typename K>
precision_ut<t_p> hash(const K & key, unat seed);

template <nat t_p = k_nat_p, typename K>
precision_ut<t_p> hash(const K * key, nat nElements, unat seed);

template <nat t_p = k_nat_p>
precision_ut<t_p> hash(const std::string & key, unat seed);



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
//==============================================================================



namespace murmur3 {



//==============================================================================
// rotl
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------

u32 rotl32(u32 x, u32 n);
u64 rotl64(u64 x, u64 n);



//==============================================================================
// fmix
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------

u32 fmix32(u32 h);
u64 fmix64(u64 h);



//==============================================================================
// x86_32
//------------------------------------------------------------------------------
// Produces a 32 bit hash; optimized for x86 platforms.
//------------------------------------------------------------------------------

void x86_32(const void * key, nat n, u32 seed, void * out);

template <typename K, std::enable_if_t<sizeof(K) != 1 && sizeof(K) != 2 && sizeof(K) != 4 && sizeof(K) != 8, int> = 0>
void x86_32(const K & key, u32 seed, void * out);

template <typename K, std::enable_if_t<sizeof(K) == 1, int> = 0>
void x86_32(const K & key, u32 seed, void * out);

template <typename K, std::enable_if_t<sizeof(K) == 2, int> = 0>
void x86_32(const K & key, u32 seed, void * out);

template <typename K, std::enable_if_t<sizeof(K) == 4, int> = 0>
void x86_32(const K & key, u32 seed, void * out);

template <typename K, std::enable_if_t<sizeof(K) == 8, int> = 0>
void x86_32(const K & key, u32 seed, void * out);



//==============================================================================
// x86_128
//------------------------------------------------------------------------------
// Produces a 128 bit hash; optimized for x86 platforms.
//------------------------------------------------------------------------------

void x86_128(const void * key, nat n, u32 seed, void * out);

template <typename K, std::enable_if_t<sizeof(K) != 1 && sizeof(K) != 2 && sizeof(K) != 4 && sizeof(K) != 8, int> = 0>
void x86_128(const K & key, u32 seed, void * out);

template <typename K, std::enable_if_t<sizeof(K) == 1, int> = 0>
void x86_128(const K & key, u32 seed, void * out);

template <typename K, std::enable_if_t<sizeof(K) == 2, int> = 0>
void x86_128(const K & key, u32 seed, void * out);

template <typename K, std::enable_if_t<sizeof(K) == 4, int> = 0>
void x86_128(const K & key, u32 seed, void * out);

template <typename K, std::enable_if_t<sizeof(K) == 8, int> = 0>
void x86_128(const K & key, u32 seed, void * out);



//==============================================================================
// x64_128
//------------------------------------------------------------------------------
// Produces a 128 bit hash; optimized for x64 platforms.
//------------------------------------------------------------------------------

void x64_128(const void * key, nat n, u64 seed, void * out);

template <typename K, std::enable_if_t<sizeof(K) != 1 && sizeof(K) != 2 && sizeof(K) != 4 && sizeof(K) != 8, int> = 0>
void x64_128(const K & key, u64 seed, void * out);

template <typename K, std::enable_if_t<sizeof(K) == 1, int> = 0>
void x64_128(const K & key, u64 seed, void * out);

template <typename K, std::enable_if_t<sizeof(K) == 2, int> = 0>
void x64_128(const K & key, u64 seed, void * out);

template <typename K, std::enable_if_t<sizeof(K) == 4, int> = 0>
void x64_128(const K & key, u64 seed, void * out);

template <typename K, std::enable_if_t<sizeof(K) == 8, int> = 0>
void x64_128(const K & key, u64 seed, void * out);



}



//======================================================================================================================
// TECH ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



namespace tech {



//==============================================================================
// Hash
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------

template <nat>
struct Hash;

template <>
struct Hash<32> {

    // 32 bit hash
    template <nat t_p, typename K, std::enable_if_t<t_p == 32, int> = 0>
    static u32 hash(const K & key, unat seed);

    // 32 bit pointer hash
    template <nat t_p, typename K, std::enable_if_t<t_p == 32, int> = 0>
    static u32 hash(const K * key, nat nElements, unat seed);

    // 32 bit string hash
    template <nat t_p, std::enable_if_t<t_p == 32, int> = 0>
    static u32 hash(const std::string & key, unat seed);

    // 64 bit hash
    template <nat t_p, typename K, std::enable_if_t<t_p == 64, int> = 0>
    static u64 hash(const K & key, unat seed) ;

    // 64 bit pointer hash
    template <nat t_p, typename K, std::enable_if_t<t_p == 64, int> = 0>
    static u64 hash(const K * key, nat nElements, unat seed);

    // 64 bit string hash
    template <nat t_p, std::enable_if_t<t_p == 64, int> = 0>
    static u64 hash(const std::string & key, unat seed);

    // 128 bit hash
    template <nat t_p, typename K, std::enable_if_t<t_p == 128, int> = 0>
    static u128 hash(const K & key, unat seed);

    // 128 bit pointer hash
    template <nat t_p, typename K, std::enable_if_t<t_p == 128, int> = 0>
    static u128 hash(const K * key, nat nElements, unat seed);

    // 128 bit string hash
    template <nat t_p, std::enable_if_t<t_p == 128, int> = 0>
    static u128 hash(const std::string & key, unat seed);

};

template <>
struct Hash<64> {

    // 32 bit hash
    template <nat t_p, typename K, std::enable_if_t<t_p == 32, int> = 0>
    static u32 hash(const K & key, unat seed);

    // 32 bit pointer hash
    template <nat t_p, typename K, std::enable_if_t<t_p == 32, int> = 0>
    static u32 hash(const K * key, nat nElements, unat seed);

    // 32 bit string hash
    template <nat t_p, std::enable_if_t<t_p == 32, int> = 0>
    static u32 hash(const std::string & key, unat seed);

    // 64 bit hash
    template <nat t_p, typename K, std::enable_if_t<t_p == 64, int> = 0>
    static u64 hash(const K & key, unat seed);

    // 64 bit pointer hash
    template <nat t_p, typename K, std::enable_if_t<t_p == 64, int> = 0>
    static u64 hash(const K * key, nat nElements, unat seed);

    // 64 bit string hash
    template <nat t_p, std::enable_if_t<t_p == 64, int> = 0>
    static u64 hash(const std::string & key, unat seed);

    // 128 bit hash
    template <nat t_p, typename K, std::enable_if_t<t_p == 128, int> = 0>
    static u128 hash(const K & key, unat seed);

    // 128 bit pointer hash
    template <nat t_p, typename K, std::enable_if_t<t_p == 128, int> = 0>
    static u128 hash(const K * key, nat nElements, unat seed);

    // 128 bit string hash
    template <nat t_p, std::enable_if_t<t_p == 128, int> = 0>
    static u128 hash(const std::string & key, unat seed);

};



}



//==============================================================================================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION ==============================================================================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//==============================================================================================================================================================



//======================================================================================================================
// HASH INTERFACE IMPLEMENTATION ///////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// hash
//------------------------------------------------------------------------------

template <nat t_p, typename K>
precision_ut<t_p> hash(const K & key, unat seed) {
    return tech::Hash<k_nat_p>::hash<t_p>(key, seed);
}

template <nat t_p, typename K>
precision_ut<t_p> hash(const K * key, nat nElements, unat seed) {
    return tech::Hash<k_nat_p>::hash<t_p>(key, nElements, seed);
}

template <nat t_p>
precision_ut<t_p> hash(const std::string & key, unat seed) {
    return tech::Hash<k_nat_p>::hash<t_p>(key, seed);
}



//======================================================================================================================
// MURMUR3 IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



namespace murmur3 {



//==============================================================================
// rotl
//------------------------------------------------------------------------------

inline u32 rotl32(u32 x, u32 n) {
    return (x << n) | (x >> (32 - n));
}

inline u64 rotl64(u64 x, u64 n) {
    return (x << n) | (x >> (64 - n));
}



//==============================================================================
// fmix
//------------------------------------------------------------------------------

inline u32 fmix32(u32 h) {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}

inline u64 fmix64(u64 h) {
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;

    return h;
}



//==============================================================================
// x86_32
//------------------------------------------------------------------------------

inline void x86_32(const void * key, nat n, u32 seed, void * out) {
    const u08 * data(reinterpret_cast<const u08 *>(key));
    const nat nblocks(n / 4);

    u32 h1(seed);

    const u32 c1(0xcc9e2d51);
    const u32 c2(0x1b873593);

    const u32 * blocks(reinterpret_cast<const u32 *>(data + nblocks * 4));

    for (nat i(-nblocks); i < 0; ++i) {
        u32 k1(blocks[i]);

        k1 *= c1;
        k1  = rotl32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1  = rotl32(h1, 13);
        h1  = h1 * 5 + 0xe6546b64;
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

    h1 ^= n;

    h1 = fmix32(h1);

    reinterpret_cast<u32 *>(out)[0] = h1;
}

template <typename K, std::enable_if_t<sizeof(K) != 1 && sizeof(K) != 2 && sizeof(K) != 4 && sizeof(K) != 8, int>>
inline void x86_32(const K & key, u32 seed, void * out) {
    x86_32(&key, sizeof(K), seed, out);
}

template <typename K, std::enable_if_t<sizeof(K) == 1, int>>
inline void x86_32(const K & key, u32 seed, void * out) {
    const u08 * data(reinterpret_cast<const u08 *>(&key));

    u32 h1(seed);

    u32 k1(0);

    k1 ^= data[0];
    k1 *= 0xcc9e2d51;
    k1  = rotl32(k1, 15);
    k1 *= 0x1b873593;
    h1 ^= k1;

    h1 ^= 0b0001;

    h1 = fmix32(h1);

    reinterpret_cast<u32 *>(out)[0] = h1;
}

template <typename K, std::enable_if_t<sizeof(K) == 2, int>>
inline void x86_32(const K & key, u32 seed, void * out) {
    const u08 * data(reinterpret_cast<const u08 *>(&key));

    u32 h1(seed);

    u32 k1(0);

    k1 ^= data[1] << 8;
    k1 ^= data[0];
    k1 *= 0xcc9e2d51;
    k1  = rotl32(k1, 15);
    k1 *= 0x1b873593;
    h1 ^= k1;

    h1 ^= 0b0010;

    h1 = fmix32(h1);

    reinterpret_cast<u32 *>(out)[0] = h1;
}

template <typename K, std::enable_if_t<sizeof(K) == 4, int>>
inline void x86_32(const K & key, u32 seed, void * out) {
    const u32 * data(reinterpret_cast<const u32 *>(&key));

    u32 h1(seed);

    u32 k1(data[0]);

    k1 *= 0xcc9e2d51;
    k1  = rotl32(k1, 15);
    k1 *= 0x1b873593;

    h1 ^= k1;
    h1  = rotl32(h1, 13);
    h1  = h1 * 5 + 0xe6546b64;

    h1 ^= 0b0100;

    h1 = fmix32(h1);

    reinterpret_cast<u32 *>(out)[0] = h1;
}

template <typename K, std::enable_if_t<sizeof(K) == 8, int>>
inline void x86_32(const K & key, u32 seed, void * out) {
    const u32 * data(reinterpret_cast<const u32 *>(&key));

    u32 h1(seed);

    u32 k1(data[0]);

    k1 *= 0xcc9e2d51;
    k1  = rotl32(k1, 15);
    k1 *= 0x1b873593;

    h1 ^= k1;
    h1  = rotl32(h1, 13);
    h1  = h1 * 5 + 0xe6546b64;

    k1 = data[1];

    k1 *= 0xcc9e2d51;
    k1  = rotl32(k1, 15);
    k1 *= 0x1b873593;

    h1 ^= k1;
    h1  = rotl32(h1, 13);
    h1  = h1 * 5 + 0xe6546b64;

    h1 ^= 0b1000;

    h1 = fmix32(h1);

    reinterpret_cast<u32 *>(out)[0] = h1;
}



//==============================================================================
// x86_128
//------------------------------------------------------------------------------

inline void x86_128(const void * key, nat n, u32 seed, void * out) {
    const u08 * data(reinterpret_cast<const u08 *>(key));
    const nat nblocks(n / 16);

    u32 h1(seed);
    u32 h2(seed);
    u32 h3(seed);
    u32 h4(seed);

    const u32 c1(0x239b961b);
    const u32 c2(0xab0e9789);
    const u32 c3(0x38b34ae5);
    const u32 c4(0xa1e38b93);

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
        h1  = h1 * 5 + 0x561ccd1b;

        k2 *= c2;
        k2  = rotl32(k2, 16);
        k2 *= c3;
        h2 ^= k2;

        h2  = rotl32(h2, 17);
        h2 += h3;
        h2  = h2 * 5 + 0x0bcaa747;

        k3 *= c3;
        k3  = rotl32(k3, 17);
        k3 *= c4;
        h3 ^= k3;

        h3  = rotl32(h3, 15);
        h3 += h4;
        h3  = h3 * 5 + 0x96cd1c35;

        k4 *= c4;
        k4  = rotl32(k4, 18);
        k4 *= c1;
        h4 ^= k4;

        h4  = rotl32(h4, 13);
        h4 += h1;
        h4  = h4 * 5 + 0x32ac3b17;
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
        case 0b1010: k3 ^= tail[9] << 8;
        case 0b1001: k3 ^= tail[8] << 0;
            k3 *= c3;
            k3  = rotl32(k3, 17);
            k3 *= c4;
            h3 ^= k3;

        case 0b1000: k2 ^= tail[7] << 24;
        case 0b0111: k2 ^= tail[6] << 16;
        case 0b0110: k2 ^= tail[5] << 8;
        case 0b0101: k2 ^= tail[4] << 0;
            k2 *= c2;
            k2  = rotl32(k2, 16);
            k2 *= c3;
            h2 ^= k2;

        case 0b0100: k1 ^= tail[3] << 24;
        case 0b0011: k1 ^= tail[2] << 16;
        case 0b0010: k1 ^= tail[1] << 8;
        case 0b0001: k1 ^= tail[0] << 0;
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

    reinterpret_cast<u32 *>(out)[0] = h1;
    reinterpret_cast<u32 *>(out)[1] = h2;
    reinterpret_cast<u32 *>(out)[2] = h3;
    reinterpret_cast<u32 *>(out)[3] = h4;
}

template <typename K, std::enable_if_t<sizeof(K) != 1 && sizeof(K) != 2 && sizeof(K) != 4 && sizeof(K) != 8, int>>
inline void x86_128(const K & key, u32 seed, void * out) {
    x86_128(&key, sizeof(K), seed, out);
}

template <typename K, std::enable_if_t<sizeof(K) == 1, int>>
inline void x86_128(const K & key, u32 seed, void * out) {
    const u08 * data(reinterpret_cast<const u08 *>(&key));

    u32 h1(seed);
    u32 h2(seed);
    u32 h3(seed);
    u32 h4(seed);

    u32 k1(0);

    k1 ^= data[0] << 0;
    k1 *= 0x239b961b;
    k1  = rotl32(k1, 15);
    k1 *= 0xab0e9789;
    h1 ^= k1;

    h1 ^= 0b0001;
    h2 ^= 0b0001;
    h3 ^= 0b0001;
    h4 ^= 0b0001;

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

    reinterpret_cast<u32 *>(out)[0] = h1;
    reinterpret_cast<u32 *>(out)[1] = h2;
    reinterpret_cast<u32 *>(out)[2] = h3;
    reinterpret_cast<u32 *>(out)[3] = h4;
}

template <typename K, std::enable_if_t<sizeof(K) == 2, int>>
inline void x86_128(const K & key, u32 seed, void * out) {
    const u08 * data(reinterpret_cast<const u08 *>(&key));

    u32 h1(seed);
    u32 h2(seed);
    u32 h3(seed);
    u32 h4(seed);

    u32 k1(0);

    k1 ^= data[1] << 8;
    k1 ^= data[0] << 0;
    k1 *= 0x239b961b;
    k1  = rotl32(k1, 15);
    k1 *= 0xab0e9789;
    h1 ^= k1;

    h1 ^= 0b0010;
    h2 ^= 0b0010;
    h3 ^= 0b0010;
    h4 ^= 0b0010;

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

    reinterpret_cast<u32 *>(out)[0] = h1;
    reinterpret_cast<u32 *>(out)[1] = h2;
    reinterpret_cast<u32 *>(out)[2] = h3;
    reinterpret_cast<u32 *>(out)[3] = h4;
}

template <typename K, std::enable_if_t<sizeof(K) == 4, int>>
inline void x86_128(const K & key, u32 seed, void * out) {
    const u08 * data(reinterpret_cast<const u08 *>(&key));

    u32 h1(seed);
    u32 h2(seed);
    u32 h3(seed);
    u32 h4(seed);

    u32 k1(0);

    k1 ^= data[3] << 24;
    k1 ^= data[2] << 16;
    k1 ^= data[1] << 8;
    k1 ^= data[0] << 0;
    k1 *= 0x239b961b;
    k1  = rotl32(k1, 15);
    k1 *= 0xab0e9789;
    h1 ^= k1;

    h1 ^= 0b0100;
    h2 ^= 0b0100;
    h3 ^= 0b0100;
    h4 ^= 0b0100;

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

    reinterpret_cast<u32 *>(out)[0] = h1;
    reinterpret_cast<u32 *>(out)[1] = h2;
    reinterpret_cast<u32 *>(out)[2] = h3;
    reinterpret_cast<u32 *>(out)[3] = h4;
}

template <typename K, std::enable_if_t<sizeof(K) == 8, int>>
inline void x86_128(const K & key, u32 seed, void * out) {
    const u08 * data(reinterpret_cast<const u08 *>(&key));

    u32 h1(seed);
    u32 h2(seed);
    u32 h3(seed);
    u32 h4(seed);

    u32 k1(0);
    u32 k2(0);

    k2 ^= data[7] << 24;
    k2 ^= data[6] << 16;
    k2 ^= data[5] << 8;
    k2 ^= data[4] << 0;
    k2 *= 0xab0e9789;
    k2  = rotl32(k2, 16);
    k2 *= 0x38b34ae5;
    h2 ^= k2;

    k1 ^= data[3] << 24;
    k1 ^= data[2] << 16;
    k1 ^= data[1] << 8;
    k1 ^= data[0] << 0;
    k1 *= 0x239b961b;
    k1  = rotl32(k1, 15);
    k1 *= 0xab0e9789;
    h1 ^= k1;

    h1 ^= 0b1000;
    h2 ^= 0b1000;
    h3 ^= 0b1000;
    h4 ^= 0b1000;

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

    reinterpret_cast<u32 *>(out)[0] = h1;
    reinterpret_cast<u32 *>(out)[1] = h2;
    reinterpret_cast<u32 *>(out)[2] = h3;
    reinterpret_cast<u32 *>(out)[3] = h4;
}



//==============================================================================
// x64_128
//------------------------------------------------------------------------------

inline void x64_128(const void * key, nat n, u64 seed, void * out) {
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
        h1  = h1 * 5 + 0x52dce729;

        k2 *= c2;
        k2  = rotl64(k2, 33);
        k2 *= c1;
        h2 ^= k2;

        h2  = rotl64(h2, 31);
        h2 += h1;
        h2  = h2 * 5 + 0x38495ab5;
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

    reinterpret_cast<u64 *>(out)[0] = h1;
    reinterpret_cast<u64 *>(out)[1] = h2;
}

template <typename K, std::enable_if_t<sizeof(K) != 1 && sizeof(K) != 2 && sizeof(K) != 4 && sizeof(K) != 8, int>>
inline void x64_128(const K & key, u64 seed, void * out) {
    x64_128(&key, sizeof(K), seed, out);
}

template <typename K, std::enable_if_t<sizeof(K) == 1, int>>
inline void x64_128(const K & key, u64 seed, void * out) {
    const u08 * data(reinterpret_cast<const u08 *>(&key));

    u64 h1(seed);
    u64 h2(seed);

    u64 k1(0);

    k1 ^= static_cast<u64>(data[0]) << 0;
    k1 *= 0x87c37b91114253d5ULL;
    k1  = rotl64(k1, 31);
    k1 *= 0x4cf5ad432745937fULL;
    h1 ^= k1;

    h1 ^= 0b0001;
    h2 ^= 0b0001;

    h1 += h2;
    h2 += h1;

    h1 = fmix64(h1);
    h2 = fmix64(h2);

    h1 += h2;
    h2 += h1;

    reinterpret_cast<u64 *>(out)[0] = h1;
    reinterpret_cast<u64 *>(out)[1] = h2;
}

template <typename K, std::enable_if_t<sizeof(K) == 2, int>>
inline void x64_128(const K & key, u64 seed, void * out) {
    const u08 * data(reinterpret_cast<const u08 *>(&key));

    u64 h1(seed);
    u64 h2(seed);

    u64 k1(0);

    k1 ^= static_cast<u64>(data[1]) << 8;
    k1 ^= static_cast<u64>(data[0]) << 0;
    k1 *= 0x87c37b91114253d5ULL;
    k1  = rotl64(k1, 31);
    k1 *= 0x4cf5ad432745937fULL;
    h1 ^= k1;

    h1 ^= 0b0010;
    h2 ^= 0b0010;

    h1 += h2;
    h2 += h1;

    h1 = fmix64(h1);
    h2 = fmix64(h2);

    h1 += h2;
    h2 += h1;

    reinterpret_cast<u64 *>(out)[0] = h1;
    reinterpret_cast<u64 *>(out)[1] = h2;
}

template <typename K, std::enable_if_t<sizeof(K) == 4, int>>
inline void x64_128(const K & key, u64 seed, void * out) {
    const u08 * data(reinterpret_cast<const u08 *>(&key));

    u64 h1(seed);
    u64 h2(seed);

    u64 k1(0);

    k1 ^= static_cast<u64>(data[3]) << 24;
    k1 ^= static_cast<u64>(data[2]) << 16;
    k1 ^= static_cast<u64>(data[1]) <<  8;
    k1 ^= static_cast<u64>(data[0]) <<  0;
    k1 *= 0x87c37b91114253d5ULL;
    k1  = rotl64(k1, 31);
    k1 *= 0x4cf5ad432745937fULL;
    h1 ^= k1;

    h1 ^= 0b0100;
    h2 ^= 0b0100;

    h1 += h2;
    h2 += h1;

    h1 = fmix64(h1);
    h2 = fmix64(h2);

    h1 += h2;
    h2 += h1;

    reinterpret_cast<u64 *>(out)[0] = h1;
    reinterpret_cast<u64 *>(out)[1] = h2;
}

template <typename K, std::enable_if_t<sizeof(K) == 8, int>>
inline void x64_128(const K & key, u64 seed, void * out) {
    const u08 * data(reinterpret_cast<const u08 *>(&key));

    u64 h1(seed);
    u64 h2(seed);

    u64 k1(0);

    k1 ^= static_cast<u64>(data[7]) << 56;
    k1 ^= static_cast<u64>(data[6]) << 48;
    k1 ^= static_cast<u64>(data[5]) << 40;
    k1 ^= static_cast<u64>(data[4]) << 32;
    k1 ^= static_cast<u64>(data[3]) << 24;
    k1 ^= static_cast<u64>(data[2]) << 16;
    k1 ^= static_cast<u64>(data[1]) <<  8;
    k1 ^= static_cast<u64>(data[0]) <<  0;
    k1 *= 0x87c37b91114253d5ULL;
    k1  = rotl64(k1, 31);
    k1 *= 0x4cf5ad432745937fULL;
    h1 ^= k1;

    h1 ^= 0b1000;
    h2 ^= 0b1000;

    h1 += h2;
    h2 += h1;

    h1 = fmix64(h1);
    h2 = fmix64(h2);

    h1 += h2;
    h2 += h1;

    reinterpret_cast<u64 *>(out)[0] = h1;
    reinterpret_cast<u64 *>(out)[1] = h2;
}



}



//======================================================================================================================
// TECH IMPLEMENTATION /////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



namespace tech {



//==============================================================================
// hash
//------------------------------------------------------------------------------

template <nat t_p, typename K, std::enable_if_t<t_p == 32, int>>
inline u32 Hash<32>::hash(const K & key, unat seed) {
    u32 h;
    murmur3::x86_32(key, seed, &h);
    return h;
}

template <nat t_p, typename K, std::enable_if_t<t_p == 32, int>>
inline u32 Hash<32>::hash(const K * key, nat nElements, unat seed) {
    u32 h;
    murmur3::x86_32(key, nElements * sizeof(K), seed, &h);
    return h;
}

template <nat t_p, std::enable_if_t<t_p == 32, int>>
inline u32 Hash<32>::hash(const std::string & key, unat seed) {
    u32 h;
    murmur3::x86_32(key.c_str(), key.size(), seed, &h);
    return h;
}

template <nat t_p, typename K, std::enable_if_t<t_p == 64, int>>
inline u64 Hash<32>::hash(const K & key, unat seed) {
    u128 h;
    murmur3::x86_128(key, seed, &h);
    return h.u64_1;
}

template <nat t_p, typename K, std::enable_if_t<t_p == 64, int>>
inline u64 Hash<32>::hash(const K * key, nat nElements, unat seed) {
    u128 h;
    murmur3::x86_128(key, nElements * sizeof(K), seed, &h);
    return h.u64_1;
}

template <nat t_p, std::enable_if_t<t_p == 64, int>>
inline u64 Hash<32>::hash(const std::string & key, unat seed) {
    u128 h;
    murmur3::x86_128(key.c_str(), key.size(), seed, &h);
    return h.u64_1;
}

template <nat t_p, typename K, std::enable_if_t<t_p == 128, int>>
inline u128 Hash<32>::hash(const K & key, unat seed) {
    u128 h;
    murmur3::x86_128(key, seed, &h);
    return h;
}

template <nat t_p, typename K, std::enable_if_t<t_p == 128, int>>
inline u128 Hash<32>::hash(const K * key, nat nElements, unat seed) {
    u128 h;
    murmur3::x86_128(key, nElements * sizeof(K), seed, &h);
    return h;
}

template <nat t_p, std::enable_if_t<t_p == 128, int>>
inline u128 Hash<32>::hash(const std::string & key, unat seed) {
    u128 h;
    murmur3::x86_128(key.c_str(), key.size(), seed, &h);
    return h;
}

template <nat t_p, typename K, std::enable_if_t<t_p == 32, int>>
inline u32 Hash<64>::hash(const K & key, unat seed) {
    u128 h;
    murmur3::x64_128(key, seed, &h);
    return h.u32_1;
}

template <nat t_p, typename K, std::enable_if_t<t_p == 32, int>>
inline u32 Hash<64>::hash(const K * key, nat nElements, unat seed) {
    u128 h;
    murmur3::x64_128(key, nElements * sizeof(K), seed, &h);
    return h.u32_1;
}

template <nat t_p, std::enable_if_t<t_p == 32, int>>
inline u32 Hash<64>::hash(const std::string & key, unat seed) {
    u128 h;
    murmur3::x64_128(key.c_str(), key.length(), seed, &h);
    return h.u32_1;
}

template <nat t_p, typename K, std::enable_if_t<t_p == 64, int>>
inline u64 Hash<64>::hash(const K & key, unat seed) {
    u128 h;
    murmur3::x64_128(key, seed, &h);
    return h.u64_1;
}

template <nat t_p, typename K, std::enable_if_t<t_p == 64, int>>
inline u64 Hash<64>::hash(const K * key, nat nElements, unat seed) {
    u128 h;
    murmur3::x64_128(key, nElements * sizeof(K), seed, &h);
    return h.u64_1;
}

template <nat t_p, std::enable_if_t<t_p == 64, int>>
inline u64 Hash<64>::hash(const std::string & key, unat seed) {
    u128 h;
    murmur3::x64_128(key.c_str(), key.length(), seed, &h);
    return h.u64_1;
}

template <nat t_p, typename K, std::enable_if_t<t_p == 128, int>>
inline u128 Hash<64>::hash(const K & key, unat seed) {
    u128 h;
    murmur3::x64_128(key, seed, &h);
    return h;
}

template <nat t_p, typename K, std::enable_if_t<t_p == 128, int>>
inline u128 Hash<64>::hash(const K * key, nat nElements, unat seed) {
    u128 h;
    murmur3::x64_128(key, nElements * sizeof(K), seed, &h);
    return h;
}

template <nat t_p, std::enable_if_t<t_p == 128, int>>
inline u128 Hash<64>::hash(const std::string & key, unat seed) {
    u128 h;
    murmur3::x64_128(key.c_str(), key.length(), seed, &h);
    return h;
}



}



}



//==============================================================================