// First include in order to test its own includes
#include <qc-hash/fasthash.hpp>

#include <algorithm>
#include <unordered_map>

#include <gtest/gtest.h>

#include <qc-core/core.hpp>

using namespace qc::types;

TEST(fasthash, uniqueness) {
    std::unordered_map<u8, int> lowHashBitsCount{};
    std::unordered_map<u8, int> highHashBitsCount{};
    const qc_hash::fasthash::Hash<u8> hash{};

    for (unsigned int i{0u}; i < 256u; ++i) {
        const size_t h{hash(u8(i))};
        const u8 lowBits{u8(h)};
        const u8 highBits{u8(h >> (std::numeric_limits<size_t>::digits - 8))};

        ++lowHashBitsCount[lowBits];
        ++highHashBitsCount[highBits];
    }

    const auto compare{[](const std::pair<u8, int> & e1, const std::pair<u8, int> & e2) { return e1.second < e2.second; }};
    const int maxLowCount{std::max_element(lowHashBitsCount.cbegin(), lowHashBitsCount.cend(), compare)->second};
    const int maxHighCount{std::max_element(highHashBitsCount.cbegin(), highHashBitsCount.cend(), compare)->second};

    EXPECT_LE(maxLowCount, 5);
    EXPECT_LE(maxHighCount, 5);
}

TEST(fasthash, value) {
    enum class DummyEnum8 : u8 {};
    enum class DummyEnum16 : u16 {};
    enum class DummyEnum32 : u32 {};
    enum class DummyEnum64 : u64 {};

    const u64 v{0xFEDCBA9876543210};
    const size_t h8{1330138311702210459u};
    const size_t h16{13976375472018752156u};
    const size_t h32{11238735334669857748u};
    const size_t h64{10051241818552738526u};

    EXPECT_EQ(h8, qc_hash::fasthash::Hash<u8>()(reinterpret_cast<const u8 &>(v)));
    EXPECT_EQ(h8, qc_hash::fasthash::Hash<s8>()(reinterpret_cast<const s8 &>(v)));
    EXPECT_EQ(h8, qc_hash::fasthash::Hash<DummyEnum8>()(reinterpret_cast<const DummyEnum8 &>(v)));

    EXPECT_EQ(h16, qc_hash::fasthash::Hash<u16>()(reinterpret_cast<const u16 &>(v)));
    EXPECT_EQ(h16, qc_hash::fasthash::Hash<s16>()(reinterpret_cast<const s16 &>(v)));
    EXPECT_EQ(h16, qc_hash::fasthash::Hash<DummyEnum16>()(reinterpret_cast<const DummyEnum16 &>(v)));

    EXPECT_EQ(h32, qc_hash::fasthash::Hash<u32>()(reinterpret_cast<const u32 &>(v)));
    EXPECT_EQ(h32, qc_hash::fasthash::Hash<s32>()(reinterpret_cast<const s32 &>(v)));
    EXPECT_EQ(h32, qc_hash::fasthash::Hash<f32>()(reinterpret_cast<const f32 &>(v)));
    EXPECT_EQ(h32, qc_hash::fasthash::Hash<DummyEnum32>()(reinterpret_cast<const DummyEnum32 &>(v)));

    EXPECT_EQ(h64, qc_hash::fasthash::Hash<u64>()(reinterpret_cast<const u64 &>(v)));
    EXPECT_EQ(h64, qc_hash::fasthash::Hash<s64>()(reinterpret_cast<const s64 &>(v)));
    EXPECT_EQ(h64, qc_hash::fasthash::Hash<f64>()(reinterpret_cast<const f64 &>(v)));
    EXPECT_EQ(h64, qc_hash::fasthash::Hash<DummyEnum64>()(reinterpret_cast<const DummyEnum64 &>(v)));

    EXPECT_EQ(sizeof(void *) == 4 ? h32 : h64, qc_hash::fasthash::Hash<const void *>()(reinterpret_cast<const void *>(v)));
}
