#pragma once

//
// QC Hash 2.2.3
//
// Austin Quick, 2016 - 2021
// https://github.com/Daskie/qc-hash
// ...
//

#include <cstdint>

#include <string>
#include <string_view>
#include <type_traits>

#include <qc-hash/murmur3.hpp>

static_assert(sizeof(size_t) == 4u || sizeof(size_t) == 8u, "Unsupported architecture");
static_assert(std::is_same_v<size_t, uintptr_t>, "Unsupported architecture");

namespace qc_hash_chunk {

    using namespace qc_hash;

    //
    // For integral types, acts as the identity hash.
    // For floating point types, performs the Murmur3 mix function.
    // For pointers, acts as the identity hash, but shifts right to discard the lower dead bits.
    // For std::string and std::string_view, uses Murmur3.
    // No other types are currently supported.
    //
    template <typename K> struct Hash;

}

// INLINE IMPLEMENTATION ///////////////////////////////////////////////////////////////////////////////////////////////

namespace qc_hash_chunk {

    template <int n> using _utype =
        std::conditional_t<n == 1u,  uint8_t,
        std::conditional_t<n == 2u, uint16_t,
        std::conditional_t<n == 4u, uint32_t,
        std::conditional_t<n == 8u, uint64_t,
        void>>>>;

    template <typename T> requires (alignof(T) <= 16)
    consteval int _pointerShiftFor() {
        switch (alignof(T)) {
            case 2: return 1;
            case 4: return 2;
            case 8: return 3;
            case 16: return 4;
            default: return 0;
        }
    }

    template <typename K> requires (std::is_integral_v<K>)
    struct Hash<K> {
        constexpr size_t operator()(const K key) const noexcept {
            if constexpr (std::is_signed_v<K>) {
                return size_t(std::make_unsigned_t<K>(key));
            }
            else {
                return size_t(key);
            }
        }
    };

    template <>
    struct Hash<float> {
        constexpr size_t operator()(const float key) const noexcept {
            if constexpr (sizeof(size_t) == 8) {
                return murmur3::mix64(reinterpret_cast<const uint32_t &>(key));
            }
            else {
                return murmur3::mix32(reinterpret_cast<const uint32_t &>(key));
            }
        }
    };

    template <>
    struct Hash<double> {
        constexpr size_t operator()(const double key) const noexcept {
            return size_t(murmur3::mix64(reinterpret_cast<const uint64_t &>(key)));
        }
    };

    template <typename K> requires (std::is_pointer_v<K>)
    struct Hash<K> {
        constexpr size_t operator()(const K key) const noexcept {
            using T = std::remove_cv_t<std::remove_pointer_t<K>>;
            if constexpr (std::is_same_v<T, void>) {
                return reinterpret_cast<const size_t &>(key);
            }
            else {
                static constexpr int pointerShift{_pointerShiftFor<T>()};
                return reinterpret_cast<const size_t &>(key) >> pointerShift;
            }
        }
    };

    template <>
    struct Hash<std::string> {
        size_t operator()(const std::string & key) const noexcept {
            return murmur3::hash(key.data(), key.size());
        }
    };

    template <>
    struct Hash<std::string_view> {
        size_t operator()(const std::string_view & key) const noexcept {
            return murmur3::hash(key.data(), key.size());
        }
    };

}
