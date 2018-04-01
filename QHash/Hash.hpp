//==============================================================================
// QHash ///////////////////////////////////////////////////////////////////////
//==============================================================================
// Austin Quick, 2016 - 2017 
//------------------------------------------------------------------------------



#pragma once



#include <string>



namespace qc {



namespace config {

namespace hash {

constexpr bool smallKeyOptimization = true;

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

template <typename K>
size_t hash(const K & key);

size_t hash(const std::string & key);

template <typename K>
size_t hashv(const K * key, size_t n);



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
// x86_32
//------------------------------------------------------------------------------
// Produces a 32 bit hash; optimized for x86 platforms.
//------------------------------------------------------------------------------

std::uint32_t x86_32(const void * key, std::uint32_t n, std::uint32_t seed);

//==============================================================================
// x86_128
//------------------------------------------------------------------------------
// Produces a 128 bit hash; optimized for x86 platforms.
//------------------------------------------------------------------------------

std::tuple<std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t> x86_128(const void * key, std::uint32_t n, std::uint32_t seed);

//==============================================================================
// x64_128
//------------------------------------------------------------------------------
// Produces a 128 bit hash; optimized for x64 platforms.
//------------------------------------------------------------------------------

std::pair<std::uint64_t, std::uint64_t> x64_128(const void * key, std::uint64_t n, std::uint64_t seed);

}



//==============================================================================================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//==============================================================================================================================================================



namespace detail {

template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
constexpr int log2Floor(T v) {
    static_assert(sizeof(T) <= 8, "log2Floor function needs updated for larger integer types");

    int log(0);

    if constexpr (sizeof(T) >= 8) {
        if (v & 0xFFFFFFFF00000000ULL) { v >>= 32; log += 32; }
    }
    if constexpr (sizeof(T) >= 4) {
        if (v & 0x00000000FFFF0000ULL) { v >>= 16; log += 16; }
    }
    if constexpr (sizeof(T) >= 2) {
        if (v & 0x000000000000FF00ULL) { v >>=  8; log +=  8; }
    }
    if (    v & 0x00000000000000F0ULL) { v >>=  4; log +=  4; }
    if (    v & 0x000000000000000CULL) { v >>=  2; log +=  2; }
    if (    v & 0x0000000000000002ULL) {           log +=  1; }

    return log;
}

}



//======================================================================================================================
// HASH INTERFACE IMPLEMENTATION ///////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// hash
//------------------------------------------------------------------------------

template <typename K>
size_t hash(const K & key) {
    if constexpr (
        config::hash::smallKeyOptimization &&
        (sizeof(K) == 1 || sizeof(K) == 2 || sizeof(K) == 4 || sizeof(K) == 8)
    ) {
        size_t h;
        if constexpr (sizeof(K) == 1) h = reinterpret_cast<const std:: uint8_t &>(key);
        if constexpr (sizeof(K) == 2) h = reinterpret_cast<const std::uint16_t &>(key);
        if constexpr (sizeof(K) == 4) h = reinterpret_cast<const std::uint32_t &>(key);
        if constexpr (sizeof(K) == 8) h = reinterpret_cast<const std::uint64_t &>(key);

        if constexpr (std::is_pointer_v<std::remove_cv_t<std::remove_reference_t<K>>>) {
            constexpr int tSize(sizeof(std::remove_pointer_t<std::remove_cv_t<std::remove_reference_t<K>>>));
            constexpr int shift(detail::log2Floor(tSize));
            h >>= shift;
        }

        return h;
    }
    else {
        return hashv(&key, 1);
    }
}

size_t hash(const std::string & key) {
    return hashv(key.c_str(), key.size());
}

template <typename K>
size_t hashv(const K * key, size_t n) {
    static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8, "unsupported architecture");

    if constexpr (sizeof(size_t) == 4) {
        return murmur3::x86_32(key, n * sizeof(K), 0);
    }
    if constexpr (sizeof(size_t) == 8) {
        auto [h1, h2](murmur3::x64_128(key, n * sizeof(K), 0));
        return h1 ^ h2;
    }
}



//======================================================================================================================
// MURMUR3 IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



namespace murmur3 {



constexpr std::uint32_t rotl32(std::uint32_t x, int r) {
  return (x << r) | (x >> (32 - r));
}

constexpr std::uint64_t rotl64(std::uint64_t x, int r) {
  return (x << r) | (x >> (64 - r));
}

constexpr std::uint32_t fmix32(std::uint32_t h) {
    h ^= h >> 16;
    h *= std::uint32_t(0x85ebca6b);
    h ^= h >> 13;
    h *= std::uint32_t(0xc2b2ae35);
    h ^= h >> 16;

    return h;
}

constexpr std::uint64_t fmix64(std::uint64_t h) {
    h ^= h >> 33;
    h *= std::uint64_t(0xff51afd7ed558ccdULL);
    h ^= h >> 33;
    h *= std::uint64_t(0xc4ceb9fe1a85ec53ULL);
    h ^= h >> 33;

    return h;
}



//==============================================================================
// x86_32
//------------------------------------------------------------------------------

inline std::uint32_t x86_32(const void * key, std::uint32_t n, std::uint32_t seed) {
    const std::uint8_t * data(reinterpret_cast<const std::uint8_t *>(key));
    const std::int32_t nblocks(n / 4);

    std::uint32_t h1(seed);

    constexpr std::uint32_t c1(0xcc9e2d51);
    constexpr std::uint32_t c2(0x1b873593);

    const std::uint32_t * blocks(reinterpret_cast<const std::uint32_t *>(data + nblocks * 4));

    for (std::int32_t i(-nblocks); i < 0; ++i) {
        std::uint32_t k1(blocks[i]);

        k1 *= c1;
        k1  = rotl32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1  = rotl32(h1, 13);
        h1  = h1 * std::uint32_t(5) + std::uint32_t(0xe6546b64);
    }

    const std::uint8_t * tail(reinterpret_cast<const std::uint8_t *>(data + nblocks * 4));

    std::uint32_t k1(0);

    switch (n & 0b11) {
        case 0b11: k1 ^= std::uint32_t(tail[2]) << 16;
        case 0b10: k1 ^= std::uint32_t(tail[1]) << 8;
        case 0b01: k1 ^= std::uint32_t(tail[0]);
            k1 *= c1;
            k1  = rotl32(k1, 15);
            k1 *= c2;
            h1 ^= k1;
    };

    h1 ^= n;

    h1 = fmix32(h1);

    return h1;
}



//==============================================================================
// x86_128
//------------------------------------------------------------------------------

inline std::tuple<std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t> x86_128(const void * key, std::uint32_t n, std::uint32_t seed) {
    const std::uint8_t * data(reinterpret_cast<const std::uint8_t *>(key));
    const std::int32_t nblocks(n / 16);

    std::uint32_t h1(seed);
    std::uint32_t h2(seed);
    std::uint32_t h3(seed);
    std::uint32_t h4(seed);

    constexpr std::uint32_t c1(0x239b961b);
    constexpr std::uint32_t c2(0xab0e9789);
    constexpr std::uint32_t c3(0x38b34ae5);
    constexpr std::uint32_t c4(0xa1e38b93);

    const std::uint32_t * blocks(reinterpret_cast<const std::uint32_t *>(data + nblocks * 16));

    for (std::int32_t i(-nblocks); i < 0; ++i) {
        std::uint32_t k1(blocks[i * 4 + 0]);
        std::uint32_t k2(blocks[i * 4 + 1]);
        std::uint32_t k3(blocks[i * 4 + 2]);
        std::uint32_t k4(blocks[i * 4 + 3]);

        k1 *= c1;
        k1  = rotl32(k1, 15);
        k1 *= c2;
        h1 ^= k1;

        h1  = rotl32(h1, 19);
        h1 += h2;
        h1  = h1 * std::uint32_t(5) + std::uint32_t(0x561ccd1b);

        k2 *= c2;
        k2  = rotl32(k2, 16);
        k2 *= c3;
        h2 ^= k2;

        h2  = rotl32(h2, 17);
        h2 += h3;
        h2  = h2 * std::uint32_t(5) + std::uint32_t(0x0bcaa747);

        k3 *= c3;
        k3  = rotl32(k3, 17);
        k3 *= c4;
        h3 ^= k3;

        h3  = rotl32(h3, 15);
        h3 += h4;
        h3  = h3 * std::uint32_t(5) + std::uint32_t(0x96cd1c35);

        k4 *= c4;
        k4  = rotl32(k4, 18);
        k4 *= c1;
        h4 ^= k4;

        h4  = rotl32(h4, 13);
        h4 += h1;
        h4  = h4 * std::uint32_t(5) + std::uint32_t(0x32ac3b17);
    }

    const std::uint8_t * tail(data + nblocks * 16);

    std::uint32_t k1(0);
    std::uint32_t k2(0);
    std::uint32_t k3(0);
    std::uint32_t k4(0);

    switch (n & 0b1111) {
        case 0b1111: k4 ^= std::uint32_t(tail[14]) << 16;
        case 0b1110: k4 ^= std::uint32_t(tail[13]) <<  8;
        case 0b1101: k4 ^= std::uint32_t(tail[12]) <<  0;
            k4 *= c4;
            k4  = rotl32(k4, 18);
            k4 *= c1;
            h4 ^= k4;

        case 0b1100: k3 ^= std::uint32_t(tail[11]) << 24;
        case 0b1011: k3 ^= std::uint32_t(tail[10]) << 16;
        case 0b1010: k3 ^= std::uint32_t(tail[ 9]) <<  8;
        case 0b1001: k3 ^= std::uint32_t(tail[ 8]) <<  0;
            k3 *= c3;
            k3  = rotl32(k3, 17);
            k3 *= c4;
            h3 ^= k3;

        case 0b1000: k2 ^= std::uint32_t(tail[7]) << 24;
        case 0b0111: k2 ^= std::uint32_t(tail[6]) << 16;
        case 0b0110: k2 ^= std::uint32_t(tail[5]) <<  8;
        case 0b0101: k2 ^= std::uint32_t(tail[4]) <<  0;
            k2 *= c2;
            k2  = rotl32(k2, 16);
            k2 *= c3;
            h2 ^= k2;

        case 0b0100: k1 ^= std::uint32_t(tail[3]) << 24;
        case 0b0011: k1 ^= std::uint32_t(tail[2]) << 16;
        case 0b0010: k1 ^= std::uint32_t(tail[1]) <<  8;
        case 0b0001: k1 ^= std::uint32_t(tail[0]) <<  0;
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

    return std::tuple<std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t>(h1, h2, h3, h4);
}



//==============================================================================
// x64_128
//------------------------------------------------------------------------------

inline std::pair<std::uint64_t, std::uint64_t> x64_128(const void * key, std::uint64_t n, std::uint64_t seed) {
    const std::uint8_t * data(reinterpret_cast<const std::uint8_t *>(key));
    const std::int64_t nblocks(n / 16);

    std::uint64_t h1(seed);
    std::uint64_t h2(seed);

    const std::uint64_t c1(0x87c37b91114253d5ULL);
    const std::uint64_t c2(0x4cf5ad432745937fULL);

    const std::uint64_t * blocks(reinterpret_cast<const std::uint64_t *>(data));

    for (std::int64_t i(0); i < nblocks; ++i) {
        std::uint64_t k1(blocks[i * 2 + 0]);
        std::uint64_t k2(blocks[i * 2 + 1]);

        k1 *= c1;
        k1  = rotl64(k1, 31);
        k1 *= c2;
        h1 ^= k1;

        h1  = rotl64(h1, 27);
        h1 += h2;
        h1  = h1 * std::uint64_t(5) + std::uint64_t(0x52dce729);

        k2 *= c2;
        k2  = rotl64(k2, 33);
        k2 *= c1;
        h2 ^= k2;

        h2  = rotl64(h2, 31);
        h2 += h1;
        h2  = h2 * std::uint64_t(5) + std::uint64_t(0x38495ab5);
    }

    const std::uint8_t * tail(data + nblocks * 16);

    std::uint64_t k1(0);
    std::uint64_t k2(0);

    switch (n & 0b1111) {
        case 0b1111: k2 ^= std::uint64_t(tail[14]) << 48;
        case 0b1110: k2 ^= std::uint64_t(tail[13]) << 40;
        case 0b1101: k2 ^= std::uint64_t(tail[12]) << 32;
        case 0b1100: k2 ^= std::uint64_t(tail[11]) << 24;
        case 0b1011: k2 ^= std::uint64_t(tail[10]) << 16;
        case 0b1010: k2 ^= std::uint64_t(tail[ 9]) <<  8;
        case 0b1001: k2 ^= std::uint64_t(tail[ 8]) <<  0;
            k2 *= c2;
            k2  = rotl64(k2, 33);
            k2 *= c1;
            h2 ^= k2;

        case 0b0001: k1 ^= std::uint64_t(tail[0]) <<  0;
        case 0b1000: k1 ^= std::uint64_t(tail[7]) << 56;
        case 0b0111: k1 ^= std::uint64_t(tail[6]) << 48;
        case 0b0110: k1 ^= std::uint64_t(tail[5]) << 40;
        case 0b0101: k1 ^= std::uint64_t(tail[4]) << 32;
        case 0b0100: k1 ^= std::uint64_t(tail[3]) << 24;
        case 0b0011: k1 ^= std::uint64_t(tail[2]) << 16;
        case 0b0010: k1 ^= std::uint64_t(tail[1]) <<  8;
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

    return std::pair<std::uint64_t, std::uint64_t>(h1, h2);
}



}



}