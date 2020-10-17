#include <unordered_map>

#include <gtest/gtest.h>

#include <qc-hash/qc-hash.hpp>

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
    qc::hash::Hash<u08> hash;
    for (int i{0}; i < 256; ++i) {
        auto [it, placed](map.try_emplace(hash(u08(i)), u08(i)));
        EXPECT_TRUE(placed);
    }
}

TEST(identityHash, arithmetic) {
    u64 v(0xFEDCBA9876543210);

    EXPECT_EQ(size_t(u08(v)), qc::hash::IdentityHash<u08>()(reinterpret_cast<const u08 &>(v)));
    EXPECT_EQ(size_t(u08(v)), qc::hash::IdentityHash<s08>()(reinterpret_cast<const s08 &>(v)));

    EXPECT_EQ(size_t(u16(v)), qc::hash::IdentityHash<u16>()(reinterpret_cast<const u16 &>(v)));
    EXPECT_EQ(size_t(u16(v)), qc::hash::IdentityHash<s16>()(reinterpret_cast<const s16 &>(v)));

    EXPECT_EQ(size_t(u32(v)), qc::hash::IdentityHash<u32>()(reinterpret_cast<const u32 &>(v)));
    EXPECT_EQ(size_t(u32(v)), qc::hash::IdentityHash<s32>()(reinterpret_cast<const s32 &>(v)));
    EXPECT_EQ(size_t(u32(v)), qc::hash::IdentityHash<f32>()(reinterpret_cast<const f32 &>(v)));

    EXPECT_EQ(size_t(u64(v)), qc::hash::IdentityHash<u64>()(reinterpret_cast<const u64 &>(v)));
    EXPECT_EQ(size_t(u64(v)), qc::hash::IdentityHash<s64>()(reinterpret_cast<const s64 &>(v)));
    EXPECT_EQ(size_t(u64(v)), qc::hash::IdentityHash<f64>()(reinterpret_cast<const f64 &>(v)));
}

TEST(identityHash, sizes) {
    struct S1 { u08 _0; };
    struct S2 { u08 _0, _1; };
    struct S3 { u08 _0, _1, _2; };
    struct S4 { u08 _0, _1, _2, _3; };
    struct S5 { u08 _0, _1, _2, _3, _4; };
    struct S6 { u08 _0, _1, _2, _3, _4, _5; };
    struct S7 { u08 _0, _1, _2, _3, _4, _5, _6; };
    struct S8 { u08 _0, _1, _2, _3, _4, _5, _6, _7; };

    u64 v(0xFEDCBA9876543210);

    EXPECT_EQ(0x0000000000000010u, qc::hash::IdentityHash<S1>()(reinterpret_cast<const S1 &>(v)));
    EXPECT_EQ(0x0000000000003210u, qc::hash::IdentityHash<S2>()(reinterpret_cast<const S2 &>(v)));
    EXPECT_EQ(0x0000000000543210u, qc::hash::IdentityHash<S3>()(reinterpret_cast<const S3 &>(v)));
    EXPECT_EQ(0x0000000076543210u, qc::hash::IdentityHash<S4>()(reinterpret_cast<const S4 &>(v)));
    EXPECT_EQ(0x0000009876543210u, qc::hash::IdentityHash<S5>()(reinterpret_cast<const S5 &>(v)));
    EXPECT_EQ(0x0000BA9876543210u, qc::hash::IdentityHash<S6>()(reinterpret_cast<const S6 &>(v)));
    EXPECT_EQ(0x00DCBA9876543210u, qc::hash::IdentityHash<S7>()(reinterpret_cast<const S7 &>(v)));
    EXPECT_EQ(0xFEDCBA9876543210u, qc::hash::IdentityHash<S8>()(reinterpret_cast<const S8 &>(v)));
}

TEST(identityHash, pointerKeys) {
    EXPECT_EQ(1u, qc::hash::IdentityHash<const u08 *>()(reinterpret_cast<const u08 *>(0b0001u)));
    EXPECT_EQ(1u, qc::hash::IdentityHash<const u16 *>()(reinterpret_cast<const u16 *>(0b0010u)));
    EXPECT_EQ(1u, qc::hash::IdentityHash<const u32 *>()(reinterpret_cast<const u32 *>(0b0100u)));
    EXPECT_EQ(1u, qc::hash::IdentityHash<const u64 *>()(reinterpret_cast<const u64 *>(0b1000u)));

    EXPECT_EQ(1u, qc::hash::IdentityHash<const void *>()(reinterpret_cast<const void *>(1u)));
}
