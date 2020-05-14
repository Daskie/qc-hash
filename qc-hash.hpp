#pragma once

//
// QC Hash 2.1.1
//
// Austin Quick, 2016 - 2020
// https://github.com/Daskie/qc-hash
// ...
//

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

static_assert(sizeof(size_t) == 4u || sizeof(size_t) == 8u, "Unsupported architecture");

namespace qc::hash {

    //
    // If the size of the key is less than or equal to the size of a word, a
    // perfect hash is returned using murmur3's mix functions.
    // Otherwise, a standard hash is computed using murmur3.
    //
    template <typename K> struct Hash {

        //
        // Specializations exist for properly processing `std::string` and
        // `std::string_view`.
        // Template specialization can be used to define this method for other
        // types.
        //
        size_t operator()(const K & key) const;

    };

    //
    // Returns the key reinterpreted as an unsigned integer.
    // A special case exists for pointer keys - the integer value is bit shifted
    // to the right such that the the lowest relevant bit is the lowest bit.
    // Only valid for keys no larger than a word.
    //
    template <typename K> struct IdentityHash {

        static_assert(sizeof(K) <= sizeof(size_t), "Cannot apply identity hash to types larger than a word");

        //
        // Specializations exist for properly processing `std::string` and
        // `std::string_view`.
        // Template specialization can be used to define this method for other
        // types.
        //
        size_t operator()(const K & key) const;

    };

    //
    // Wrapper for the general murmur3 hash. Selects the best variant based on
    // the current architecture.
    //
    size_t hash(const void * key, size_t n, size_t seed = 0u);

}

//
// Murmur3 - https://github.com/PeterScott/murmur3
//
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.
//
// Note - The x86 and x64 versions do not produce the same results, as the
// algorithms are optimized for their respective platforms. You can still
// compile and run any of them on any platform, but your performance with
// the non-native version will be less than optimal.
//
namespace qc::hash::murmur3 {

    //
    // 32 bit perfect integer hash.
    //
    constexpr uint32_t fmix32(uint32_t h);

    //
    // 64 bit perfect integer hash.
    //
    constexpr uint64_t fmix64(uint64_t h);

    //
    // Produces a 32 bit hash; optimized for x86 platforms.
    //
    uint32_t x86_32(const void * key, uint32_t n, uint32_t seed);

    //
    // Produces a 128 bit hash; optimized for x86 platforms.
    //
    std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> x86_128(const void * key, uint32_t n, uint32_t seed);

    //
    // Produces a 128 bit hash; optimized for x64 platforms.
    //
    std::pair<uint64_t, uint64_t> x64_128(const void * key, uint64_t n, uint64_t seed);

}

// INLINE IMPLEMENTATION ///////////////////////////////////////////////////////////////////////////////////////////////

namespace qc::hash {

    template <int n> using _utype =
        std::conditional_t<n == 1u,  uint8_t,
        std::conditional_t<n == 2u, uint16_t,
        std::conditional_t<n == 4u, uint32_t,
        std::conditional_t<n == 8u, uint64_t,
        void>>>>;

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    constexpr int _log2Floor(T v) {
        int log(0);
        if constexpr (sizeof(T) >= 8u) if (v & 0xFFFFFFFF00000000) { v >>= 32; log += 32; }
        if constexpr (sizeof(T) >= 4u) if (v & 0x00000000FFFF0000) { v >>= 16; log += 16; }
        if constexpr (sizeof(T) >= 2u) if (v & 0x000000000000FF00) { v >>=  8; log +=  8; }
                                       if (v & 0x00000000000000F0) { v >>=  4; log +=  4; }
                                       if (v & 0x000000000000000C) { v >>=  2; log +=  2; }
                                       if (v & 0x0000000000000002) {           log +=  1; }
        return log;
    }

    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    constexpr T _ceil2(T v) {
        --v;
                                       v |= v >>  1;
                                       v |= v >>  2;
                                       v |= v >>  4;
        if constexpr (sizeof(T) >= 2u) v |= v >>  8;
        if constexpr (sizeof(T) >= 4u) v |= v >> 16;
        if constexpr (sizeof(T) >= 8u) v |= v >> 32;
        return ++v;
    }

    template <typename K>
    inline size_t Hash<K>::operator()(const K & key) const {
        if      constexpr (sizeof(K) == 1u) return size_t(qc::hash::murmur3::fmix32(reinterpret_cast<const  uint8_t &>(key)));
        else if constexpr (sizeof(K) == 2u) return size_t(qc::hash::murmur3::fmix32(reinterpret_cast<const uint16_t &>(key)));
        else if constexpr (sizeof(K) == 3u) return size_t(qc::hash::murmur3::fmix32(reinterpret_cast<const uint32_t &>(key) & 0x00FFFFFF));
        else if constexpr (sizeof(K) == 4u) return size_t(qc::hash::murmur3::fmix32(reinterpret_cast<const uint32_t &>(key)));
        else if constexpr (sizeof(K) == 5u) return size_t(qc::hash::murmur3::fmix64(reinterpret_cast<const uint64_t &>(key) & 0x000000FFFFFFFFFF));
        else if constexpr (sizeof(K) == 6u) return size_t(qc::hash::murmur3::fmix64(reinterpret_cast<const uint64_t &>(key) & 0x0000FFFFFFFFFFFF));
        else if constexpr (sizeof(K) == 7u) return size_t(qc::hash::murmur3::fmix64(reinterpret_cast<const uint64_t &>(key) & 0x00FFFFFFFFFFFFFF));
        else if constexpr (sizeof(K) == 8u) return size_t(qc::hash::murmur3::fmix64(reinterpret_cast<const uint64_t &>(key)));
        else return hash(&key, sizeof(K));
    }

    inline size_t Hash<std::string>::operator()(const std::string & key) const {
        return hash(key.data(), key.size());
    }

    inline size_t Hash<std::string_view>::operator()(const std::string_view & key) const {
        return hash(key.data(), key.size());
    }

    template <typename K>
    inline size_t IdentityHash<K>::operator()(const K & key) const {
        if constexpr (std::is_pointer_v<K>) {
            using T = std::remove_cv_t<std::remove_pointer_t<std::remove_cv_t<K>>>;
            if constexpr (std::is_same_v<T, void>) {
                return reinterpret_cast<const size_t &>(key);
            }
            else {
                return reinterpret_cast<const size_t &>(key) >> _log2Floor(alignof(T));
            }
        }
        else {
            if      constexpr (sizeof(K) == 1u) return size_t(reinterpret_cast<const  uint8_t &>(key));
            else if constexpr (sizeof(K) == 2u) return size_t(reinterpret_cast<const uint16_t &>(key));
            else if constexpr (sizeof(K) == 3u) return size_t(reinterpret_cast<const uint32_t &>(key) & 0x00FFFFFF);
            else if constexpr (sizeof(K) == 4u) return size_t(reinterpret_cast<const uint32_t &>(key));
            else if constexpr (sizeof(K) == 5u) return size_t(reinterpret_cast<const uint64_t &>(key) & 0x000000FFFFFFFFFF);
            else if constexpr (sizeof(K) == 6u) return size_t(reinterpret_cast<const uint64_t &>(key) & 0x0000FFFFFFFFFFFF);
            else if constexpr (sizeof(K) == 7u) return size_t(reinterpret_cast<const uint64_t &>(key) & 0x00FFFFFFFFFFFFFF);
            else if constexpr (sizeof(K) == 8u) return size_t(reinterpret_cast<const uint64_t &>(key));
        }
    }

    inline size_t hash(const void * key, size_t n, size_t seed) {
        if constexpr (sizeof(size_t) == 4u) {
            return murmur3::x86_32(key, uint32_t(n), uint32_t(seed));
        }
        else if constexpr (sizeof(size_t) == 8u) {
            auto [h1, h2](murmur3::x64_128(key, uint64_t(n), uint64_t(seed)));
            return h1 ^ h2;
        }
    }

}

namespace qc::hash::murmur3 {

    constexpr uint32_t rotl32(uint32_t x, int r) {
        return (x << r) | (x >> (32 - r));
    }

    constexpr uint64_t rotl64(uint64_t x, int r) {
        return (x << r) | (x >> (64 - r));
    }

    constexpr uint32_t fmix32(uint32_t h) {
        h ^= h >> 16;
        h *= 0x85EBCA6B;
        h ^= h >> 13;
        h *= 0xC2B2AE35;
        h ^= h >> 16;

        return h;
    }

    constexpr uint64_t fmix64(uint64_t h) {
        h ^= h >> 33;
        h *= 0xFF51AFD7ED558CCD;
        h ^= h >> 33;
        h *= 0xC4CEB9FE1A85EC53;
        h ^= h >> 33;

        return h;
    }

    inline uint32_t x86_32(const void * key, uint32_t n, uint32_t seed) {
        const uint8_t * data(reinterpret_cast<const uint8_t *>(key));
        const int32_t nblocks(n >> 2), nbytes(nblocks << 2);

        uint32_t h1(seed);

        constexpr uint32_t c1(0xCC9E2D51), c2(0x1B873593);

        const uint32_t * blocks(reinterpret_cast<const uint32_t *>(data + nbytes));

        for (int32_t i(-nblocks); i < 0u; ++i) {
            uint32_t k1(blocks[i]);

            k1 *= c1;
            k1  = rotl32(k1, 15);
            k1 *= c2;

            h1 ^= k1;
            h1  = rotl32(h1, 13);
            h1  = h1 * 5u + 0xE6546B64;
        }

        const uint8_t * tail(reinterpret_cast<const uint8_t *>(data + nbytes));

        uint32_t k1(0u);

        switch (n & 0b11) {
            case 0b11: k1 ^= uint32_t(tail[2]) << 16;
            case 0b10: k1 ^= uint32_t(tail[1]) << 8;
            case 0b01: k1 ^= uint32_t(tail[0]);
                k1 *= c1;
                k1  = rotl32(k1, 15);
                k1 *= c2;
                h1 ^= k1;
        };

        h1 ^= n;

        h1 = fmix32(h1);

        return h1;
    }

    inline std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> x86_128(const void * key, uint32_t n, uint32_t seed) {
        const uint8_t * data(reinterpret_cast<const uint8_t *>(key));
        const int32_t nblocks(n >> 4), nbytes(nblocks << 4);

        uint32_t h1(seed), h2(seed), h3(seed), h4(seed);

        constexpr uint32_t c1(0x239B961B), c2(0xAB0E9789), c3(0x38B34AE5), c4(0xA1E38B93);

        const uint32_t * blocks(reinterpret_cast<const uint32_t *>(data + nbytes));

        for (int32_t i(-nblocks); i < 0u; ++i) {
            uint32_t i4(i << 2);
            uint32_t k1(blocks[i4 + 0u]), k2(blocks[i4 + 1u]), k3(blocks[i4 + 2u]), k4(blocks[i4 + 3u]);

            k1 *= c1;
            k1  = rotl32(k1, 15);
            k1 *= c2;
            h1 ^= k1;

            h1  = rotl32(h1, 19);
            h1 += h2;
            h1  = h1 * 5u + 0x561CCD1B;

            k2 *= c2;
            k2  = rotl32(k2, 16);
            k2 *= c3;
            h2 ^= k2;

            h2  = rotl32(h2, 17);
            h2 += h3;
            h2  = h2 * 5u + 0x0BCAA747;

            k3 *= c3;
            k3  = rotl32(k3, 17);
            k3 *= c4;
            h3 ^= k3;

            h3  = rotl32(h3, 15);
            h3 += h4;
            h3  = h3 * 5u + 0x96CD1C35;

            k4 *= c4;
            k4  = rotl32(k4, 18);
            k4 *= c1;
            h4 ^= k4;

            h4  = rotl32(h4, 13);
            h4 += h1;
            h4  = h4 * 5u + 0x32AC3B17;
        }

        const uint8_t * tail(data + nbytes);

        uint32_t k1(0u), k2(0u), k3(0u), k4(0u);

        switch (n & 0b1111) {
            case 0b1111: k4 ^= uint32_t(tail[14]) << 16;
            case 0b1110: k4 ^= uint32_t(tail[13]) <<  8;
            case 0b1101: k4 ^= uint32_t(tail[12]) <<  0;
                k4 *= c4;
                k4  = rotl32(k4, 18);
                k4 *= c1;
                h4 ^= k4;

            case 0b1100: k3 ^= uint32_t(tail[11]) << 24;
            case 0b1011: k3 ^= uint32_t(tail[10]) << 16;
            case 0b1010: k3 ^= uint32_t(tail[ 9]) <<  8;
            case 0b1001: k3 ^= uint32_t(tail[ 8]) <<  0;
                k3 *= c3;
                k3  = rotl32(k3, 17);
                k3 *= c4;
                h3 ^= k3;

            case 0b1000: k2 ^= uint32_t(tail[7]) << 24;
            case 0b0111: k2 ^= uint32_t(tail[6]) << 16;
            case 0b0110: k2 ^= uint32_t(tail[5]) <<  8;
            case 0b0101: k2 ^= uint32_t(tail[4]) <<  0;
                k2 *= c2;
                k2  = rotl32(k2, 16);
                k2 *= c3;
                h2 ^= k2;

            case 0b0100: k1 ^= uint32_t(tail[3]) << 24;
            case 0b0011: k1 ^= uint32_t(tail[2]) << 16;
            case 0b0010: k1 ^= uint32_t(tail[1]) <<  8;
            case 0b0001: k1 ^= uint32_t(tail[0]) <<  0;
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

        return {h1, h2, h3, h4};
    }

    inline std::pair<uint64_t, uint64_t> x64_128(const void * key, uint64_t n, uint64_t seed) {
        const uint8_t * data(reinterpret_cast<const uint8_t *>(key));
        const uint64_t nblocks(n >> 4), nbytes(nblocks << 4);

        uint64_t h1(seed), h2(seed);

        constexpr uint64_t c1(0x87C37B91114253D5), c2(0x4CF5AD432745937F);

        const uint64_t * blocks(reinterpret_cast<const uint64_t *>(data));

        for (uint64_t i(0u); i < nblocks; ++i) {
            uint64_t i2(i << 1);
            uint64_t k1(blocks[i2 + 0u]);
            uint64_t k2(blocks[i2 + 1u]);

            k1 *= c1;
            k1  = rotl64(k1, 31);
            k1 *= c2;
            h1 ^= k1;

            h1  = rotl64(h1, 27);
            h1 += h2;
            h1  = h1 * 5u + 0x52DCE729;

            k2 *= c2;
            k2  = rotl64(k2, 33);
            k2 *= c1;
            h2 ^= k2;

            h2  = rotl64(h2, 31);
            h2 += h1;
            h2  = h2 * 5u + 0x38495AB5;
        }

        const uint8_t * tail(data + nbytes);

        uint64_t k1(0u), k2(0u);

        switch (n & 0b1111) {
            case 0b1111: k2 ^= uint64_t(tail[14]) << 48;
            case 0b1110: k2 ^= uint64_t(tail[13]) << 40;
            case 0b1101: k2 ^= uint64_t(tail[12]) << 32;
            case 0b1100: k2 ^= uint64_t(tail[11]) << 24;
            case 0b1011: k2 ^= uint64_t(tail[10]) << 16;
            case 0b1010: k2 ^= uint64_t(tail[ 9]) <<  8;
            case 0b1001: k2 ^= uint64_t(tail[ 8]) <<  0;
                k2 *= c2;
                k2  = rotl64(k2, 33);
                k2 *= c1;
                h2 ^= k2;

            case 0b1000: k1 ^= uint64_t(tail[7]) << 56;
            case 0b0111: k1 ^= uint64_t(tail[6]) << 48;
            case 0b0110: k1 ^= uint64_t(tail[5]) << 40;
            case 0b0101: k1 ^= uint64_t(tail[4]) << 32;
            case 0b0100: k1 ^= uint64_t(tail[3]) << 24;
            case 0b0011: k1 ^= uint64_t(tail[2]) << 16;
            case 0b0010: k1 ^= uint64_t(tail[1]) <<  8;
            case 0b0001: k1 ^= uint64_t(tail[0]) <<  0;
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

        return {h1, h2};
    }

}
