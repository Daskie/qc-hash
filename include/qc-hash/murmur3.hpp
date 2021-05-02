#pragma once

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

#include <cstdint>

#include <utility>

namespace qc_hash::murmur3 {

    static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8, "Unsupported architecture");

    //
    // ...
    //
    template <typename K> struct Hash;

    //
    // 32 bit perfect integer hash.
    //
    constexpr uint32_t mix(uint32_t h) noexcept;

    //
    // 64 bit perfect integer hash.
    //
    constexpr uint64_t mix(uint64_t h) noexcept;

    //
    // Produces a 32 bit hash; optimized for x86 platforms.
    //
    uint32_t x86_32(const void * key, uint32_t n, uint32_t seed) noexcept;

    //
    // Produces a 128 bit hash; optimized for x64 platforms.
    //
    std::pair<uint64_t, uint64_t> x64_128(const void * key, uint64_t n, uint64_t seed) noexcept;

    //
    // Wrapper that elects the best murmur3 function based on the current architecture.
    //
    size_t hash(const void * key, size_t n, size_t seed = 0u) noexcept;

} // namespace qc_hash::murmur3

namespace qc_hash::murmur3 {

    inline constexpr uint32_t _rotl(const uint32_t x, const int r) noexcept {
        return (x << r) | (x >> (32 - r));
    }

    inline constexpr uint64_t _rotl(const uint64_t x, const int r) noexcept {
        return (x << r) | (x >> (64 - r));
    }

    template <typename K> requires ((std::is_integral_v<K> || std::is_pointer_v<K> || std::is_enum_v<K>) && sizeof(K) <= sizeof(size_t))
    struct Hash<K> {
        size_t operator()(const K key) const noexcept {
            if constexpr (sizeof(K) == 1) return mix(size_t(uint8_t(key)));
            if constexpr (sizeof(K) == 2) return mix(size_t(uint16_t(key)));
            if constexpr (sizeof(K) == 4) return mix(size_t(uint32_t(key)));
            if constexpr (sizeof(K) == 8) return mix(size_t(uint64_t(key)));
        }
    };

    template <typename K> requires (std::is_floating_point_v<K> && sizeof(K) <= sizeof(size_t))
    struct Hash<K> {
        size_t operator()(const K key) const noexcept {
            if constexpr (sizeof(K) == 4) return mix(size_t(reinterpret_cast<const uint32_t &>(key)));
            if constexpr (sizeof(K) == 8) return mix(size_t(reinterpret_cast<const uint64_t &>(key)));
        }
    };

    template <typename K> requires (std::is_convertible_v<K, std::string_view>)
    struct Hash<K> {
        size_t operator()(const std::string_view & key) const noexcept {
            if constexpr (sizeof(size_t) == 4) return x86_32(key.data(), key.size(), 0u);
            if constexpr (sizeof(size_t) == 8) return x64_128(key.data(), key.size(), 0u).first;
        }
    };

    inline constexpr uint32_t mix(uint32_t h) noexcept {
        h ^= h >> 16;
        h *= 0x85EBCA6Bu;
        h ^= h >> 13;
        h *= 0xC2B2AE35u;
        h ^= h >> 16;

        return h;
    }

    inline constexpr uint64_t mix(uint64_t h) noexcept {
        h ^= h >> 33;
        h *= 0xFF51AFD7ED558CCDu;
        h ^= h >> 33;
        h *= 0xC4CEB9FE1A85EC53u;
        h ^= h >> 33;

        return h;
    }

    inline uint32_t x86_32(const void * const key, const uint32_t n, const uint32_t seed) noexcept {
        static constexpr uint32_t c1{0xCC9E2D51u};
        static constexpr uint32_t c2{0x1B873593u};

        const uint8_t * const data{reinterpret_cast<const uint8_t *>(key)};
        const uint32_t nblocks{n >> 2};
        const uint32_t nbytes{nblocks << 2};
        const uint32_t * const blocks{reinterpret_cast<const uint32_t *>(data + nbytes)};
        const uint8_t * const tail{reinterpret_cast<const uint8_t *>(data + nbytes)};

        uint32_t h1{seed};

        for (int32_t i{-int32_t(nblocks)}; i < 0; ++i) {
            uint32_t k1{blocks[i]};

            k1 *= c1;
            k1  = _rotl(k1, 15);
            k1 *= c2;

            h1 ^= k1;
            h1  = _rotl(h1, 13);
            h1  = h1 * 5u + 0xE6546B64u;
        }

        uint32_t k1{0u};

        switch (n & 3u) {
            case 3u: k1 ^= uint32_t(tail[2]) << 16; [[fallthrough]];
            case 2u: k1 ^= uint32_t(tail[1]) <<  8; [[fallthrough]];
            case 1u: k1 ^= uint32_t(tail[0]);
                k1 *= c1;
                k1  = _rotl(k1, 15);
                k1 *= c2;
                h1 ^= k1;
        }

        h1 ^= n;

        return mix(h1);
    }

    inline std::pair<uint64_t, uint64_t> x64_128(const void * const key, const uint64_t n, const uint64_t seed) noexcept {
        static constexpr uint64_t c1{0x87C37B91114253D5u};
        static constexpr uint64_t c2{0x4CF5AD432745937Fu};

        const uint8_t * const data{reinterpret_cast<const uint8_t *>(key)};
        const uint64_t nblocks{n >> 4};
        const uint64_t nbytes{nblocks << 4};
        const uint64_t * const blocks{reinterpret_cast<const uint64_t *>(data)};
        const uint8_t * const tail{data + nbytes};

        uint64_t h1{seed};
        uint64_t h2{seed};

        for (uint64_t i{0u}; i < nblocks; ++i) {
            const uint64_t i2{i << 1};
            uint64_t k1{blocks[i2 + 0u]};
            uint64_t k2{blocks[i2 + 1u]};

            k1 *= c1;
            k1  = _rotl(k1, 31);
            k1 *= c2;
            h1 ^= k1;

            h1  = _rotl(h1, 27);
            h1 += h2;
            h1  = h1 * 5u + 0x52DCE729u;

            k2 *= c2;
            k2  = _rotl(k2, 33);
            k2 *= c1;
            h2 ^= k2;

            h2  = _rotl(h2, 31);
            h2 += h1;
            h2  = h2 * 5u + 0x38495AB5u;
        }

        uint64_t k1{0u};
        uint64_t k2{0u};

        switch (n & 15u) {
            case 15u: k2 ^= uint64_t(tail[14]) << 48; [[fallthrough]];
            case 14u: k2 ^= uint64_t(tail[13]) << 40; [[fallthrough]];
            case 13u: k2 ^= uint64_t(tail[12]) << 32; [[fallthrough]];
            case 12u: k2 ^= uint64_t(tail[11]) << 24; [[fallthrough]];
            case 11u: k2 ^= uint64_t(tail[10]) << 16; [[fallthrough]];
            case 10u: k2 ^= uint64_t(tail[ 9]) <<  8; [[fallthrough]];
            case  9u: k2 ^= uint64_t(tail[ 8]) <<  0;
                k2 *= c2;
                k2  = _rotl(k2, 33);
                k2 *= c1;
                h2 ^= k2;
                [[fallthrough]];

            case 8u: k1 ^= uint64_t(tail[7]) << 56; [[fallthrough]];
            case 7u: k1 ^= uint64_t(tail[6]) << 48; [[fallthrough]];
            case 6u: k1 ^= uint64_t(tail[5]) << 40; [[fallthrough]];
            case 5u: k1 ^= uint64_t(tail[4]) << 32; [[fallthrough]];
            case 4u: k1 ^= uint64_t(tail[3]) << 24; [[fallthrough]];
            case 3u: k1 ^= uint64_t(tail[2]) << 16; [[fallthrough]];
            case 2u: k1 ^= uint64_t(tail[1]) <<  8; [[fallthrough]];
            case 1u: k1 ^= uint64_t(tail[0]) <<  0;
                k1 *= c1;
                k1  = _rotl(k1, 31);
                k1 *= c2;
                h1 ^= k1;
        }

        h1 ^= n;
        h2 ^= n;

        h1 += h2;
        h2 += h1;

        h1 = mix(h1);
        h2 = mix(h2);

        h1 += h2;
        h2 += h1;

        return {h1, h2};
    }

    inline size_t hash(const void * const key, const size_t n, const size_t seed) noexcept {
        if constexpr (sizeof(size_t) == 4u) {
            return x86_32(key, n, seed);
        }
        else if constexpr (sizeof(size_t) == 8u) {
            return x64_128(key, n, seed).first;
        }
    }

} // namespace qc_hash::murmur3
