// First include in order to test its own includes
#include <qc-hash/qc-hash.hpp>

#include <unordered_map>

#include <gtest/gtest.h>

using s08 = int8_t;
using u08 = uint8_t;
using s16 = int16_t;
using u16 = uint16_t;
using s32 = int32_t;
using u32 = uint32_t;
using f32 = float;
using s64 = int64_t;
using u64 = uint64_t;
using f64 = double;

TEST(hash, uniqueness) {
    std::unordered_map<size_t, u08> map;
    qc_hash::Hash<u08> hash;
    for (int i{0}; i < 256; ++i) {
        auto [it, placed](map.try_emplace(hash(u08(i)), u08(i)));
        EXPECT_TRUE(placed);
    }
}

TEST(identityHash, arithmetic) {
    u64 v(0xFEDCBA9876543210);

    EXPECT_EQ(size_t(u08(v)), qc_hash::Hash<u08>()(reinterpret_cast<const u08 &>(v)));
    EXPECT_EQ(size_t(u08(v)), qc_hash::Hash<s08>()(reinterpret_cast<const s08 &>(v)));

    EXPECT_EQ(size_t(u16(v)), qc_hash::Hash<u16>()(reinterpret_cast<const u16 &>(v)));
    EXPECT_EQ(size_t(u16(v)), qc_hash::Hash<s16>()(reinterpret_cast<const s16 &>(v)));

    EXPECT_EQ(size_t(u32(v)), qc_hash::Hash<u32>()(reinterpret_cast<const u32 &>(v)));
    EXPECT_EQ(size_t(u32(v)), qc_hash::Hash<s32>()(reinterpret_cast<const s32 &>(v)));
    EXPECT_EQ(size_t(3311100716681205692u), qc_hash::Hash<f32>()(reinterpret_cast<const f32 &>(v)));

    EXPECT_EQ(size_t(u64(v)), qc_hash::Hash<u64>()(reinterpret_cast<const u64 &>(v)));
    EXPECT_EQ(size_t(u64(v)), qc_hash::Hash<s64>()(reinterpret_cast<const s64 &>(v)));
    EXPECT_EQ(size_t(282578663571615703u), qc_hash::Hash<f64>()(reinterpret_cast<const f64 &>(v)));
}

TEST(identityHash, pointerKeys) {
    EXPECT_EQ(1u, qc_hash::Hash<const u08 *>()(reinterpret_cast<const u08 *>(0b0001u)));
    EXPECT_EQ(1u, qc_hash::Hash<const u16 *>()(reinterpret_cast<const u16 *>(0b0010u)));
    EXPECT_EQ(1u, qc_hash::Hash<const u32 *>()(reinterpret_cast<const u32 *>(0b0100u)));
    EXPECT_EQ(1u, qc_hash::Hash<const u64 *>()(reinterpret_cast<const u64 *>(0b1000u)));

    EXPECT_EQ(1u, qc_hash::Hash<const void *>()(reinterpret_cast<const void *>(1u)));
}
