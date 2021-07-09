// First include in order to test its own includes
#include <qc-hash/fasthash.hpp>

#include <algorithm>
#include <unordered_map>

#include <gtest/gtest.h>

#include <qc-core/core.hpp>

using namespace qc::types;

TEST(fasthash, uniqueness)
{
    std::unordered_map<u8, int> lowLowHashBitsCount{};
    std::unordered_map<u8, int> highLowHashBitsCount{};
    std::unordered_map<u8, int> lowHighHashBitsCount{};
    std::unordered_map<u8, int> highHighHashBitsCount{};
    const qc_hash::fasthash::Hash<size_t> hash{};

    for (size_t lowK{0u}; lowK < 256u; ++lowK) {
        constexpr int shift{std::numeric_limits<size_t>::digits - 8};
        const size_t highK{lowK << shift};
        const size_t lowH{hash(lowK)};
        const size_t highH{hash(highK)};
        const u8 lowLowBits{u8(lowH)};
        const u8 highLowBits{u8(lowH >> shift)};
        const u8 lowHighBits{u8(highH)};
        const u8 highHighBits{u8(highH >> shift)};

        ++lowLowHashBitsCount[lowLowBits];
        ++highLowHashBitsCount[highLowBits];
        ++lowHighHashBitsCount[lowHighBits];
        ++highHighHashBitsCount[highHighBits];
    }

    const auto compare{[](const std::pair<u8, int> & e1, const std::pair<u8, int> & e2) { return e1.second < e2.second; }};
    const int maxLowLowCount{std::max_element(lowLowHashBitsCount.cbegin(), lowLowHashBitsCount.cend(), compare)->second};
    const int maxHighLowCount{std::max_element(highLowHashBitsCount.cbegin(), highLowHashBitsCount.cend(), compare)->second};
    const int maxLowHighCount{std::max_element(lowHighHashBitsCount.cbegin(), lowHighHashBitsCount.cend(), compare)->second};
    const int maxHighHighCount{std::max_element(highHighHashBitsCount.cbegin(), highHighHashBitsCount.cend(), compare)->second};

    EXPECT_LE(maxLowLowCount, 5);
    EXPECT_LE(maxHighLowCount, 5);
    EXPECT_LE(maxLowHighCount, 5);
    EXPECT_LE(maxHighHighCount, 5);
}

TEST(fasthash, value)
{
    enum class DummyEnum8 : u8 {};
    enum class DummyEnum16 : u16 {};
    enum class DummyEnum32 : u32 {};
    enum class DummyEnum64 : u64 {};

    const u64 v{0xFEDCBA9876543210};
#ifdef _WIN64
    const size_t h8{1330138311702210459u};
    const size_t h16{13976375472018752156u};
    const size_t h32{11238735334669857748u};
    const size_t h64{10051241818552738526u};
#else
    const size_t h8{309696959u};
    const size_t h16{3254128497u};
    const size_t h32{2616721981u};
#endif

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

#ifdef _WIN64
    EXPECT_EQ(h64, qc_hash::fasthash::Hash<u64>()(reinterpret_cast<const u64 &>(v)));
    EXPECT_EQ(h64, qc_hash::fasthash::Hash<s64>()(reinterpret_cast<const s64 &>(v)));
    EXPECT_EQ(h64, qc_hash::fasthash::Hash<f64>()(reinterpret_cast<const f64 &>(v)));
    EXPECT_EQ(h64, qc_hash::fasthash::Hash<DummyEnum64>()(reinterpret_cast<const DummyEnum64 &>(v)));
#endif

#ifdef _WIN64
    EXPECT_EQ(h64, qc_hash::fasthash::Hash<const void *>()(reinterpret_cast<const void *>(v)));
#else
    EXPECT_EQ(h32, qc_hash::fasthash::Hash<const void *>()(reinterpret_cast<const void *>(v)));
#endif
}
