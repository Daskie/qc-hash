#pragma once

//
// FastHash by Zilong Tan
// https://github.com/ztanml/fast-hash
//
// Modified by Austin Quick
//
// The MIT License
// Copyright (C) 2012 Zilong Tan (eric.zltan@gmail.com)
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include <cstdint>

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace qc_hash::fasthash
{
    static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8, "Unsupported architecture");

    template <typename K> struct Hash;

    constexpr uint64_t mix(uint64_t h) noexcept;
    constexpr uint32_t mix(uint32_t h) noexcept;

    uint64_t x64_64(const void * buf, size_t len, uint64_t seed) noexcept;

    uint32_t x64_32(const void * buf, size_t len, uint32_t seed) noexcept;
}

namespace qc_hash::fasthash
{
    template <typename K> requires ((std::is_integral_v<K> || std::is_pointer_v<K> || std::is_enum_v<K>) && sizeof(K) <= sizeof(size_t))
    struct Hash<K>
    {
        inline size_t operator()(const K key) const noexcept
        {
            if constexpr (sizeof(K) == 1) return mix(size_t(uint8_t(key)));
            if constexpr (sizeof(K) == 2) return mix(size_t(uint16_t(key)));
            if constexpr (sizeof(K) == 4) return mix(size_t(uint32_t(key)));
            if constexpr (sizeof(K) == 8) return mix(size_t(uint64_t(key)));
        }
    };

    template <typename K> requires (std::is_floating_point_v<K> && sizeof(K) <= sizeof(size_t))
    struct Hash<K>
    {
        inline size_t operator()(const K key) const noexcept
        {
            if constexpr (sizeof(K) == 4) return mix(size_t(reinterpret_cast<const uint32_t &>(key)));
            if constexpr (sizeof(K) == 8) return mix(size_t(reinterpret_cast<const uint64_t &>(key)));
        }
    };

    template <typename K> requires (std::is_convertible_v<K, std::string_view>)
    struct Hash<K>
    {
        inline size_t operator()(const std::string_view & key) const noexcept
        {
            if constexpr (sizeof(size_t) == 8) return x64_64(key.data(), key.size(), 0u);
            if constexpr (sizeof(size_t) == 4) return x64_32(key.data(), key.size(), 0u);
        }
    };

    template <typename T>
    struct Hash<std::unique_ptr<T>> : Hash<T *>
    {
        using Hash<T *>::operator();

        inline size_t operator()(const std::unique_ptr<T> & key) const noexcept
        {
            return operator()(key.get());
        }
    };

    inline constexpr uint64_t mix(uint64_t h) noexcept
    {
        h ^= h >> 23;
        h *= 0x2127599bf4325c37u;
        h ^= h >> 47;
        return h;
    }

    inline constexpr uint32_t mix(uint32_t h) noexcept
    {
        // The higher bits are usually better
        return mix(uint64_t(h)) >> 32;
    }

    inline uint64_t x64_64(const void * buf, size_t len, uint64_t seed) noexcept
    {
        constexpr uint64_t m{0x880355f21e6d1965u};

        const uint64_t * pos{static_cast<const uint64_t *>(buf)};
        const uint64_t * end{pos + (len >> 3)};
        uint64_t h{seed ^ (len * m)};

        while (pos != end) {
            h ^= mix(*pos);
            h *= m;
            ++pos;
        }

        const uint8_t * const pos2{reinterpret_cast<const uint8_t *>(pos)};
        uint64_t v{0u};

        switch (len & 7u) {
            case 7: v ^= uint64_t(pos2[6]) << 48;
            case 6: v ^= uint64_t(pos2[5]) << 40;
            case 5: v ^= uint64_t(pos2[4]) << 32;
            case 4: v ^= uint64_t(pos2[3]) << 24;
            case 3: v ^= uint64_t(pos2[2]) << 16;
            case 2: v ^= uint64_t(pos2[1]) << 8;
            case 1: v ^= uint64_t(pos2[0]);
                h ^= mix(v);
                h *= m;
        }

        return mix(h);
    }

    inline uint32_t x64_32(const void * buf, size_t len, uint32_t seed) noexcept
    {
        // The following trick converts the 64-bit hashcode to Fermat residue, which shall retain information from both
        // the higher and lower parts of hashcode
        const uint64_t h{x64_64(buf, len, seed)};
        return uint32_t(h - (h >> 32));
    }
}
