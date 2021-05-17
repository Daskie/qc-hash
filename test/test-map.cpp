// First include in order to test its own includes
#include <qc-hash/qc-map.hpp>

#include <cmath>

#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

#include <qc-core/core.hpp>
#include <qc-core/memory.hpp>

using namespace std::string_literals;
using namespace qc::types;

struct QcHashMapFriend {

    template <typename K, typename H, typename KE, typename A>
    static u8 controlAt(const qc_hash::Set<K, H, KE, A> & set, const size_t slotI) {
        return set._controls[slotI];
    }

    template <typename It>
    static u8 controlAt(const It it) {
        return *it._control;
    }

    template <typename K, typename H, typename KE, typename A>
    static const K & elementAt(const qc_hash::Set<K, H, KE, A> & set, const size_t slotI) {
        return set._elements[slotI].get();
    }

    template <typename It>
    static const auto & elementAt(const It it) {
        return *it._element;
    }

    template <typename K, typename H, typename KE, typename A, typename It>
    static size_t slotI(const qc_hash::Set<K, H, KE, A> & set, const It it) {
        return it._control - set._controls;
    }

    template <typename K, typename H, typename KE, typename A, typename It>
    static size_t dist(const qc_hash::Set<K, H, KE, A> & set, const It it) {
        const size_t slotI{QcHashMapFriend::slotI(set, it)};
        const size_t idealSlotI{set.slot(*it)};
        return slotI >= idealSlotI ? slotI - idealSlotI : set.slot_count() - idealSlotI + slotI;
    }

};

enum class TrackedVal : int {};

struct TrackedStats{
    int defConstructs{0u};
    int copyConstructs{0u};
    int moveConstructs{0u};
    int copyAssigns{0u};
    int moveAssigns{0u};
    int destructs{0u};

    int constructs() const { return defConstructs + copyConstructs + moveConstructs; }
    int assigns() const { return copyAssigns + moveAssigns; }
    int copies() const { return copyConstructs + copyAssigns; }
    int moves() const { return moveConstructs + moveAssigns; }
    int all() const { return constructs() + assigns() + destructs; }
};

TrackedStats operator+(const TrackedStats & stats1, const TrackedStats & stats2) {
    return TrackedStats{
        .defConstructs = stats1.defConstructs + stats2.defConstructs,
        .copyConstructs = stats1.copyConstructs + stats2.copyConstructs,
        .moveConstructs = stats1.moveConstructs + stats2.moveConstructs,
        .copyAssigns = stats1.copyAssigns + stats2.copyAssigns,
        .moveAssigns = stats1.moveAssigns + stats2.moveAssigns,
        .destructs = stats1.destructs + stats2.destructs
    };
}

TrackedStats operator-(const TrackedStats & stats1, const TrackedStats & stats2) {
    return TrackedStats{
        .defConstructs = stats1.defConstructs - stats2.defConstructs,
        .copyConstructs = stats1.copyConstructs - stats2.copyConstructs,
        .moveConstructs = stats1.moveConstructs - stats2.moveConstructs,
        .copyAssigns = stats1.copyAssigns - stats2.copyAssigns,
        .moveAssigns = stats1.moveAssigns - stats2.moveAssigns,
        .destructs = stats1.destructs - stats2.destructs
    };
}

bool operator==(const TrackedStats & stats1, const TrackedStats & stats2) {
    return
        stats1.defConstructs == stats2.defConstructs &&
        stats1.copyConstructs == stats2.copyConstructs &&
        stats1.moveConstructs == stats2.moveConstructs &&
        stats1.copyAssigns == stats2.copyAssigns &&
        stats1.moveAssigns == stats2.moveAssigns &&
        stats1.destructs == stats2.destructs;
}

struct Tracked {
    inline static TrackedStats totalStats{};

    static void resetTotals() {
        totalStats = {};
    }

    TrackedVal val{};
    TrackedStats stats{};

    explicit Tracked(const TrackedVal val) :
        val{val}
    {}

    Tracked() {
        ++stats.defConstructs;
        ++totalStats.defConstructs;
    }

    Tracked(const Tracked & other) :
        val{other.val},
        stats{other.stats}
    {
        ++stats.copyConstructs;
        ++totalStats.copyConstructs;
    }

    Tracked(Tracked && other) noexcept :
        val{std::exchange(other.val, {})},
        stats{std::exchange(other.stats, {})}
    {
        ++stats.moveConstructs;
        ++totalStats.moveConstructs;
    }

    Tracked & operator=(const Tracked & other) {
        val = other.val;
        stats = other.stats;

        ++stats.copyAssigns;
        ++totalStats.copyAssigns;

        return *this;
    }

    Tracked & operator=(Tracked && other) noexcept {
        val = std::exchange(other.val, {});
        stats = std::exchange(other.stats, {});

        ++stats.moveAssigns;
        ++totalStats.moveAssigns;

        return *this;
    }

    ~Tracked() {
        ++stats.destructs;
        ++totalStats.destructs;
    }

};

static bool operator==(const Tracked & t1, const Tracked & t2) {
    return t1.val == t2.val;
}

struct TrackedHash {
    size_t operator()(const Tracked & tracked) const noexcept {
        return qc_hash::config::DefaultHash<TrackedVal>{}(tracked.val);
    }
};

using TrackedSet = qc_hash::Set<Tracked, TrackedHash>;
using TrackedMap = qc_hash::Map<Tracked, Tracked, TrackedHash>;

template <typename K> using MemRecordSet = qc_hash::Set<K, qc_hash::config::DefaultHash<K>, std::equal_to<K>, qc::RecordAllocator<K>>;

using MemRecordTrackedSet = qc_hash::Set<Tracked, TrackedHash, std::equal_to<Tracked>, qc::RecordAllocator<Tracked>>;

TEST(set, constructor_default) {
    MemRecordSet<int> s{};
    EXPECT_EQ(qc_hash::config::minCapacity, s.capacity());
    EXPECT_EQ(size_t(0u), s.size());
    EXPECT_EQ(0u, s.get_allocator().allocations());
}

TEST(set, constructor_capacity) {
    qc::RecordAllocator<int> allocator{};
    EXPECT_EQ(size_t(  16u), (MemRecordSet<int>{   0u, allocator}.capacity()));
    EXPECT_EQ(size_t(  16u), (MemRecordSet<int>{   1u, allocator}.capacity()));
    EXPECT_EQ(size_t(  16u), (MemRecordSet<int>{  16u, allocator}.capacity()));
    EXPECT_EQ(size_t(  32u), (MemRecordSet<int>{  17u, allocator}.capacity()));
    EXPECT_EQ(size_t(  32u), (MemRecordSet<int>{  32u, allocator}.capacity()));
    EXPECT_EQ(size_t(  64u), (MemRecordSet<int>{  33u, allocator}.capacity()));
    EXPECT_EQ(size_t(  64u), (MemRecordSet<int>{  64u, allocator}.capacity()));
    EXPECT_EQ(size_t(1024u), (MemRecordSet<int>{1000u, allocator}.capacity()));
    EXPECT_EQ(0u, allocator.allocations());
}

TEST(set, constructor_range) {
    std::vector<int> values{
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
        20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
        30, 31, 32, 33, 34, 35, 36, 37, 38, 39
    };
    MemRecordSet<int> s{values.cbegin(), values.cend()};
    EXPECT_EQ(size_t(40u), s.size());
    EXPECT_EQ(size_t(64u), s.capacity());
    for (const int v : values) {
        EXPECT_TRUE(s.contains(v));
    }
    EXPECT_EQ(1u, s.get_allocator().allocations());
}

TEST(set, constructor_initializerList) {
    MemRecordSet<int> s{{
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
        20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
        30, 31, 32, 33, 34, 35, 36, 37, 38, 39
    }};
    EXPECT_EQ(size_t(40u), s.size());
    EXPECT_EQ(size_t(64u), s.capacity());
    for (int i{0}; i < 40; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
    EXPECT_EQ(1u, s.get_allocator().allocations());
}

TEST(set, constructor_copy) {
    MemRecordSet<int> s1{};
    for (int i{0}; i < 100; ++i) s1.insert(i);
    const size_t prevAllocCount{s1.get_allocator().allocations()};
    MemRecordSet<int> s2{s1};
    EXPECT_EQ(s1, s2);
    EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().allocations());
}

TEST(set, constructor_move) {
    MemRecordSet<int> s1{};
    for (int i{0}; i < 100; ++i) s1.insert(i);
    const size_t prevAllocCount{s1.get_allocator().allocations()};
    MemRecordSet<int> ref{s1};
    MemRecordSet<int> s2{std::move(s1)};
    EXPECT_EQ(ref, s2);
    EXPECT_TRUE(s1.empty());
    EXPECT_EQ(prevAllocCount, s2.get_allocator().allocations());
}

TEST(set, assignOperator_initializerList) {
    MemRecordSet<int> s{};
    for (int i{0}; i < 100; ++i) s.insert(i);
    EXPECT_EQ(128u, s.capacity());
    const size_t prevAllocCount{s.get_allocator().allocations()};

    s = {0, 1, 2, 3, 4, 5};
    EXPECT_EQ(size_t(6u), s.size());
    EXPECT_EQ(qc_hash::config::minCapacity, s.capacity());
    for (int i{0}; i < 6; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
}

TEST(set, assignOperator_copy) {
    // Test with trivial type
    {
        MemRecordSet<int> s1{};
        for (int i{0}; i < 100; ++i) s1.insert(i);
        const size_t prevAllocCount{s1.get_allocator().allocations()};

        MemRecordSet<int> s2{};
        s2 = s1;
        EXPECT_EQ(s1, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().allocations());

        s1 = s1;
        EXPECT_EQ(s1, s1);
        EXPECT_EQ(prevAllocCount, s1.get_allocator().allocations());
    }

    // Test with non-trivial type
    {
        MemRecordSet<std::string> s1{};
        for (int i{0}; i < 100; ++i) s1.insert(std::to_string(i));
        const size_t prevAllocCount{s1.get_allocator().allocations()};

        MemRecordSet<std::string> s2{};
        s2 = s1;
        EXPECT_EQ(s1, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().allocations());

        s2 = s2;
        EXPECT_EQ(s2, s2);
        EXPECT_EQ(prevAllocCount, s2.get_allocator().allocations());
    }
}

TEST(set, assignOperator_move) {
    MemRecordSet<int> s1{};
    for (int i{0}; i < 100; ++i) s1.insert(i);
    const size_t prevAllocCount{s1.get_allocator().allocations()};
    MemRecordSet<int> ref{s1};

    MemRecordSet<int> s2{};
    s2 = std::move(s1);
    EXPECT_EQ(ref, s2);
    EXPECT_TRUE(s1.empty());
    EXPECT_EQ(prevAllocCount, s2.get_allocator().allocations());

    s2 = std::move(s2);
    EXPECT_EQ(s2, s2);
    EXPECT_EQ(prevAllocCount, s2.get_allocator().allocations());

    s2 = std::move(s1);
    EXPECT_EQ(s1, s2);
}

// Keep parallel with `emplace_lVal` and `tryEmplace_lVal` tests
TEST(set, insert_lVal) {
    Tracked::resetTotals();
    MemRecordTrackedSet s{};

    for (int i{0}; i < 100; ++i) {
        const Tracked val{TrackedVal{i}};

        auto [it1, inserted1]{s.insert(val)};
        EXPECT_TRUE(inserted1);
        EXPECT_EQ(val, *it1);

        auto [it2, inserted2]{s.insert(val)};
        EXPECT_FALSE(inserted2);
        EXPECT_EQ(it1, it2);
    }

    EXPECT_EQ(100u, s.size());
    EXPECT_EQ(128u, s.capacity());
    EXPECT_EQ(4u, s.get_allocator().allocations());
    EXPECT_EQ(100, Tracked::totalStats.copyConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked::totalStats.moveConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked::totalStats.destructs);
    EXPECT_EQ(0, Tracked::totalStats.defConstructs);
    EXPECT_EQ(0, Tracked::totalStats.assigns());
}

// Keep parallel with `emplace_rVal` and `tryEmplace_rVal` tests
TEST(set, insert_rVal) {
    TrackedSet s{};
    Tracked val1{TrackedVal{7}};
    Tracked val2{TrackedVal{7}};

    {
        const auto [it, inserted]{s.insert(std::move(val1))};
        EXPECT_TRUE(inserted);
        const Tracked & tracked{*it};
        EXPECT_EQ(7, int(tracked.val));
        EXPECT_EQ(1u, tracked.stats.moveConstructs);
        EXPECT_EQ(1u, tracked.stats.all());
        EXPECT_EQ(0, int(val1.val));
    }

    {
        const auto [it, inserted]{s.insert(std::move(val2))};
        EXPECT_FALSE(inserted);
        EXPECT_EQ(s.end(), it);
        EXPECT_EQ(7, int(val2.val));
        EXPECT_EQ(0u, val2.stats.all());
    }
}

TEST(set, insert_range) {
    Tracked::resetTotals();

    std::vector<Tracked> values{};
    for (int i{0}; i < 100; ++i) values.emplace_back(TrackedVal{i});

    MemRecordTrackedSet s{};
    s.insert(values.cbegin(), values.cend());

    EXPECT_EQ(100u, s.size());
    EXPECT_EQ(128u, s.capacity());
    for (int i{0}; i < 100; ++i) {
        EXPECT_TRUE(s.contains(Tracked{TrackedVal{i}}));
    }
    EXPECT_EQ(4u, s.get_allocator().allocations());
    EXPECT_EQ(100u, Tracked::totalStats.copyConstructs);
    EXPECT_EQ(16u + 32u + 64u, Tracked::totalStats.moveConstructs);
    EXPECT_EQ(16u + 32u + 64u, Tracked::totalStats.destructs);
    EXPECT_EQ(0u, Tracked::totalStats.defConstructs);
    EXPECT_EQ(0u, Tracked::totalStats.assigns());
}

TEST(set, insert_initializerList) {
    MemRecordSet<int> s{};
    s.insert({0, 1, 2, 3, 4, 5});
    EXPECT_EQ(6u, s.size());
    EXPECT_EQ(16u, s.capacity());
    for (int i{0}; i < 6; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
    EXPECT_EQ(1u, s.get_allocator().allocations());
}

// Keep parallel with `insert_lVal` and `tryEmplace_lVal` tests
TEST(set, emplace_lVal) {
    Tracked::resetTotals();
    MemRecordTrackedSet s{};

    for (int i{0}; i < 100; ++i) {
        const Tracked val{TrackedVal{i}};

        auto [it1, inserted1]{s.emplace(val)};
        EXPECT_TRUE(inserted1);
        EXPECT_EQ(val, *it1);

        auto [it2, inserted2]{s.emplace(val)};
        EXPECT_FALSE(inserted2);
        EXPECT_EQ(it1, it2);
    }

    EXPECT_EQ(100u, s.size());
    EXPECT_EQ(128u, s.capacity());
    EXPECT_EQ(4u, s.get_allocator().allocations());
    EXPECT_EQ(100, Tracked::totalStats.copyConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked::totalStats.moveConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked::totalStats.destructs);
    EXPECT_EQ(0, Tracked::totalStats.defConstructs);
    EXPECT_EQ(0, Tracked::totalStats.assigns());
}

// Keep parallel with `insert_rVal` and `tryEmplace_rVal` tests
TEST(set, emplace_rVal) {
    TrackedSet s{};
    Tracked val1{TrackedVal{7}};
    Tracked val2{TrackedVal{7}};

    {
        const auto [it, inserted]{s.emplace(std::move(val1))};
        EXPECT_TRUE(inserted);
        const Tracked & tracked{*it};
        EXPECT_EQ(7, int(tracked.val));
        EXPECT_EQ(1u, tracked.stats.moveConstructs);
        EXPECT_EQ(1u, tracked.stats.all());
        EXPECT_EQ(0, int(val1.val));
    }

    {
        const auto [it, inserted]{s.emplace(std::move(val2))};
        EXPECT_FALSE(inserted);
        EXPECT_EQ(s.end(), it);
        EXPECT_EQ(7, int(val2.val));
        EXPECT_EQ(0u, val2.stats.all());
    }
}

TEST(set, emplace_keyArgs) {
    TrackedSet s{};
    const auto [it, inserted]{s.emplace(TrackedVal{7})};
    EXPECT_TRUE(inserted);
    EXPECT_EQ(7, int(it->val));
    EXPECT_EQ(1, it->stats.moveConstructs);
    EXPECT_EQ(1, it->stats.all());
}

// Keep parallel with `insert_lVal` and `emplace_lVal` tests
TEST(set, tryEmplace_lVal) {
    Tracked::resetTotals();
    MemRecordTrackedSet s{};

    for (int i{0}; i < 100; ++i) {
        const Tracked val{TrackedVal{i}};

        auto [it1, inserted1]{s.try_emplace(val)};
        EXPECT_TRUE(inserted1);
        EXPECT_EQ(val, *it1);

        auto [it2, inserted2]{s.try_emplace(val)};
        EXPECT_FALSE(inserted2);
        EXPECT_EQ(it1, it2);
    }

    EXPECT_EQ(100u, s.size());
    EXPECT_EQ(128u, s.capacity());
    EXPECT_EQ(4u, s.get_allocator().allocations());
    EXPECT_EQ(100, Tracked::totalStats.copyConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked::totalStats.moveConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked::totalStats.destructs);
    EXPECT_EQ(0, Tracked::totalStats.defConstructs);
    EXPECT_EQ(0, Tracked::totalStats.assigns());
}

// Keep parallel with `insert_rVal` and `emplace_rVal` tests
TEST(set, tryEmplace_rVal) {
    TrackedSet s{};
    Tracked val1{TrackedVal{7}};
    Tracked val2{TrackedVal{7}};

    {
        const auto [it, inserted]{s.try_emplace(std::move(val1))};
        EXPECT_TRUE(inserted);
        const Tracked & tracked{*it};
        EXPECT_EQ(7, int(tracked.val));
        EXPECT_EQ(1u, tracked.stats.moveConstructs);
        EXPECT_EQ(1u, tracked.stats.all());
        EXPECT_EQ(0, int(val1.val));
    }

    {
        const auto [it, inserted]{s.try_emplace(std::move(val2))};
        EXPECT_FALSE(inserted);
        EXPECT_EQ(s.end(), it);
        EXPECT_EQ(7, int(val2.val));
        EXPECT_EQ(0u, val2.stats.all());
    }
}

TEST(set, eraseKey) {
    qc_hash::Set<int> s{};

    EXPECT_FALSE(s.erase(0));

    for (int i{0}; i < 100; ++i) {
        s.insert(i);
    }
    EXPECT_EQ(size_t(100u), s.size());
    EXPECT_EQ(size_t(128u), s.capacity());
    EXPECT_FALSE(s.erase(100));

    for (int i{0}; i < 100; ++i) {
        EXPECT_TRUE(s.erase(i));
        EXPECT_EQ(size_t(100u - i - 1u), s.size());
    }
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(size_t(128u), s.capacity());
}

TEST(set, eraseIterator) {
    qc_hash::Set<int> s{};

    for (int i{0}; i < 100; ++i) {
        s.insert(i);
    }
    EXPECT_EQ(size_t(100u), s.size());
    EXPECT_EQ(size_t(128u), s.capacity());

    for (int i{0}; i < 100; ++i) {
        s.erase(s.find(i));
        EXPECT_EQ(size_t(100u - i - 1u), s.size());
    }
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(size_t(128u), s.capacity());
}

TEST(set, clear) {
    // Trivially destructible type
    {
        qc_hash::Set<int> s{};
        for (int i{0}; i < 100; ++i) s.insert(i);
        EXPECT_EQ(size_t(100u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());

        s.clear();
        EXPECT_EQ(size_t(0u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());
    }

    // Non-trivially destructible type
    {
        Tracked::resetTotals();

        TrackedSet s{};
        for (int i{0}; i < 100; ++i) s.emplace(TrackedVal{i});
        EXPECT_EQ(size_t(100u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());

        const TrackedStats preStats{Tracked::totalStats};

        s.clear();
        EXPECT_EQ(size_t(0u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());
        const TrackedStats clearStats{Tracked::totalStats - preStats};
        EXPECT_EQ(100, clearStats.destructs);
        EXPECT_EQ(100, clearStats.all());
    }
}

// Keep parallel with `count` test
TEST(set, contains) {
    qc_hash::Set<int> s{};
    for (int i{0}; i < 100; ++i) {
        s.insert(i);
        for (int j{0}; j <= i; ++j) {
            EXPECT_TRUE(s.contains(j));
        }
    }

    s.clear();
    for (int i{0}; i < 100; ++i) {
        EXPECT_FALSE(s.contains(i));
    }
}

// Keep parallel with `contains` test
TEST(set, count) {
    qc_hash::Set<int> s{};
    for (int i{0}; i < 100; ++i) {
        s.insert(i);
        for (int j{0}; j <= i; ++j) {
            EXPECT_EQ(1u, s.count(j));
        }
    }

    s.clear();
    for (int i{0}; i < 100; ++i) {
        EXPECT_EQ(0u, s.count(i));
    }
}

TEST(set, begin) {
    qc_hash::Set<int> s{};
    EXPECT_EQ(s.end(), s.begin());

    s.insert(7);
    EXPECT_NE(s.end(), s.begin());
    EXPECT_EQ(s.begin(), s.begin());
    EXPECT_EQ(7, *s.begin());

    auto it{s.begin()};
    ++it;
    EXPECT_EQ(s.end(), it);

    EXPECT_EQ(s.begin(), s.cbegin());
}

TEST(set, end) {
    qc_hash::Set<int> s{};
    EXPECT_EQ(s.begin(), s.end());
    EXPECT_EQ(s.end(), s.end());
    EXPECT_EQ(u8(0b11111111u), QcHashMapFriend::controlAt(s.end()));

    s.insert(7);
    EXPECT_NE(s.begin(), s.end());
    EXPECT_EQ(u8(0b11111111u), QcHashMapFriend::controlAt(s.end()));

    EXPECT_EQ(s.end(), s.cend());
}

// Keep parallel with `equal_range` test
TEST(set, find) {
    qc_hash::Set<int> s{};
    EXPECT_EQ(s.end(), s.find(0));

    for (int i{0}; i < 100; ++i) {
        s.insert(i);
    }
    for (int i{0}; i < 100; ++i) {
        const auto it{s.find(i)};
        EXPECT_NE(s.end(), it);
        EXPECT_EQ(i, *it);
    }

    EXPECT_EQ(s.end(), s.find(100));
}

// Keep parallel with `find` test
TEST(set, equalRange) {
    using ItPair = std::pair<qc_hash::Set<int>::iterator, qc_hash::Set<int>::iterator>;

    qc_hash::Set<int> s{};
    EXPECT_EQ((ItPair{s.end(), s.end()}), s.equal_range(0));

    for (int i{0}; i < 100; ++i) {
        s.insert(i);
    }
    for (int i{0}; i < 100; ++i) {
        const auto itPair{s.equal_range(i)};
        EXPECT_EQ(itPair.first, itPair.second);
        EXPECT_NE(s.end(), itPair.first);
        EXPECT_EQ(i, *itPair.first);
    }

    EXPECT_EQ((ItPair{s.end(), s.end()}), s.equal_range(100));
}

TEST(set, slot) {
    qc_hash::Set<int> s{128};
    s.insert(7);
    EXPECT_EQ(QcHashMapFriend::slotI(s, s.find(7)), s.slot(7));
}

// `reserve` method is synonymous with `rehash` method
TEST(set, reserve) {}

TEST(set, rehash) {
    qc_hash::Set<int> s{};

    EXPECT_EQ(qc_hash::config::minSlotCount, s.slot_count());

    s.rehash(0u);
    EXPECT_EQ(qc_hash::config::minSlotCount, s.slot_count());

    s.rehash(1u);
    EXPECT_EQ(qc_hash::config::minSlotCount, s.slot_count());

    for (int i{0}; i < 16; ++i) {
        s.insert(i);
    }
    EXPECT_EQ(size_t(32u), s.slot_count());

    s.emplace(16);
    EXPECT_EQ(size_t(64u), s.slot_count());

    for (int i{17}; i < 128; ++i) {
        s.emplace(i);
    }
    EXPECT_EQ(size_t(256u), s.slot_count());

    s.rehash(500u);
    EXPECT_EQ(size_t(128u), s.size());
    EXPECT_EQ(size_t(512u), s.slot_count());
    for (int i = 0; i < 128; ++i) {
        EXPECT_TRUE(s.contains(i));
    }

    s.rehash(10u);
    EXPECT_EQ(size_t(128u), s.size());
    EXPECT_EQ(size_t(256u), s.slot_count());
    for (int i = 0; i < 128; ++i) {
        EXPECT_TRUE(s.contains(i));
    }

    s.clear();
    EXPECT_EQ(size_t(0u), s.size());
    EXPECT_EQ(size_t(256u), s.slot_count());

    s.rehash(0u);
    EXPECT_EQ(qc_hash::config::minSlotCount, s.slot_count());
}

TEST(set, swap) {
    qc_hash::Set<int> s1{1, 2, 3};
    qc_hash::Set<int> s2{4, 5, 6};
    qc_hash::Set<int> s3{s1};
    qc_hash::Set<int> s4{s2};
    EXPECT_EQ(s1, s3);
    EXPECT_EQ(s2, s4);
    s3.swap(s4);
    EXPECT_EQ(s2, s3);
    EXPECT_EQ(s1, s4);
    std::swap(s3, s4);
    EXPECT_EQ(s1, s3);
    EXPECT_EQ(s2, s4);

    auto it1{s1.cbegin()};
    auto it2{s2.cbegin()};
    auto it3{it1};
    auto it4{it2};
    EXPECT_EQ(it1, it3);
    EXPECT_EQ(it2, it4);
    std::swap(it1, it2);
    EXPECT_EQ(it1, it4);
    EXPECT_EQ(it2, it3);
}

TEST(set, size_empty_capacity_slotCount) {
    qc_hash::Set<int> s{};
    EXPECT_EQ(0u, s.size());
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(qc_hash::config::minCapacity, s.capacity());
    EXPECT_EQ(qc_hash::config::minSlotCount, s.slot_count());

    for (int i{0}; i < 100; ++i) {
        s.insert(i);
    }
    EXPECT_EQ(100u, s.size());
    EXPECT_FALSE(s.empty());
    EXPECT_EQ(128u, s.capacity());
    EXPECT_EQ(256u, s.slot_count());
}

TEST(set, maxSize) {
    qc_hash::Set<int> s{};
    if constexpr (sizeof(size_t) == 4) {
        EXPECT_EQ(0b01000000'00000000'00000000'00000000u, s.max_size());
    }
    else if constexpr (sizeof(size_t) == 8) {
        EXPECT_EQ(0b01000000'00000000'00000000'00000000'00000000'00000000'00000000'00000000u, s.max_size());
    }
}

TEST(set, maxSlotCount) {
    qc_hash::Set<int> s{};
    if constexpr (sizeof(size_t) == 4) {
        EXPECT_EQ(0b10000000'00000000'00000000'00000000u, s.max_size());
    }
    else if constexpr (sizeof(size_t) == 8) {
        EXPECT_EQ(0b10000000'00000000'00000000'00000000'00000000'00000000'00000000'00000000u, s.max_size());
    }
}

TEST(set, maxLoadFactor) {
    qc_hash::Set<int> s{};
    EXPECT_EQ(0.5f, s.max_load_factor());

    s.insert(7);
    EXPECT_EQ(0.5f, s.max_load_factor());
}

TEST(set, equality) {
    qc_hash::Set<int> s1{}, s2{};
    for (int i{0}; i < 100; ++i) {
        s1.emplace(i);
        s2.emplace(i + 100);
    }
    EXPECT_TRUE(s1 == s1);
    EXPECT_TRUE(s1 != s2);

    s2 = s1;
    EXPECT_TRUE(s1 == s2);
}

TEST(set, iteratorTrivial) {
    static_assert(std::is_trivial_v<qc_hash::Set<int>::iterator>);
    static_assert(std::is_trivial_v<qc_hash::Set<int>::const_iterator>);
    static_assert(std::is_standard_layout_v<qc_hash::Set<int>::iterator>);
    static_assert(std::is_standard_layout_v<qc_hash::Set<int>::const_iterator>);
}

/*TEST(set, noPreemtiveRehash) {
    qc_hash::Set<int> s;
    for (int i{0}; i < int(qc_hash::config::minCapacity) - 1; ++i) s.emplace(i);
    EXPECT_EQ(qc_hash::config::minCapacity, s.capacity());
    s.emplace(int(qc_hash::config::minCapacity - 1));
    EXPECT_EQ(qc_hash::config::minCapacity, s.capacity());
    s.emplace(int(qc_hash::config::minCapacity - 1));
    EXPECT_EQ(qc_hash::config::minCapacity, s.capacity());
}

TEST(set, iterator) {
    struct A {
        int x;
        A(int x) : x(x) {}
        bool operator==(const A & other) const { return x == other.x; }
    };

    struct Hash {
        size_t operator()(const A & a) const {
            return size_t(a.x);
        }
    };

    qc_hash::Set<A, Hash> s;
    for (int i{0}; i < 128; ++i) {
        s.emplace(i);
    }
    int i{0};
    for (auto it(s.begin()); it != s.end(); ++it) {
        EXPECT_EQ(i, it->x);
        EXPECT_EQ(it->x, (*it).x);
        ++i;
    }

    // Just checking for compilation
    qc_hash::Set<int> t;
    qc_hash::Set<int>::iterator it1(t.begin());
    qc_hash::Set<int>::const_iterator cit1 = t.cbegin();
    //it1 = cit1;
    cit1 = it1;
    qc_hash::Set<int>::iterator it2(it1);
    it2 = it1;
    qc_hash::Set<int>::const_iterator cit2(cit1);
    cit2 = cit1;
    qc_hash::Set<int>::iterator it3(std::move(it1));
    it3 = std::move(it1);
    qc_hash::Set<int>::const_iterator cit3(std::move(cit1));
    cit3 = std::move(cit1);
    it1 == cit1;
    cit1 == it1;
}

TEST(set, forEachLoop) {
    qc_hash::Set<int> s;
    for (int i{0}; i < 128; ++i) {
        s.emplace(i);
    }
    int i{0};
    for (const auto & v : s) {
        EXPECT_EQ(i, v);
        ++i;
    }
}

TEST(set, circuity) {
    qc_hash::Set<int> s;
    s.rehash(64u);

    // A and B sections all hash to index 59
    // C and D sections all hash to index 54

    // Fill A section
    // |-----|-----|--------------------------------------------|-----|AAAAA|
    for (int i{0}; i < 5; ++i) {
        s.emplace(59 + i * 64u);
    }

    // Fill B section, overflowing to front
    // |BBBBB|-----|--------------------------------------------|-----|AAAAA|
    for (int i{5}; i < 10; ++i) {
        s.emplace(59 + i * 64u);
    }

    // Fill C section
    // |BBBBB|-----|--------------------------------------------|CCCCC|AAAAA|
    for (int i{0}; i < 5; ++i) {
        s.emplace(54 + i * 64u);
    }

    // Fill D section, pushing A section to after B section
    // |BBBBB|AAAAA|--------------------------------------------|CCCCC|DDDDD|
    for (int i{5}; i < 10; ++i) {
        s.emplace(54 + i * 64u);
    }

    EXPECT_EQ(size_t(64u), s.slot_count());
    //EXPECT_EQ(size_t(10u), s.bucket_size(59u));
    //EXPECT_EQ(size_t(10u), s.bucket_size(54u));

    auto it{s.begin()};
    for (int i{0}; it != s.end() && i < 5; ++it, ++i) {
        EXPECT_EQ(59 + (5 + i) * 64, *it);
    }
    for (int i{0}; it != s.end() && i < 5; ++it, ++i) {
        EXPECT_EQ(59 + i * 64, *it);
    }
    for (int i{0}; it != s.end() && i < 5; ++it, ++i) {
        EXPECT_EQ(54 + i * 64, *it);
    }
    for (int i{0}; it != s.end() && i < 5; ++it, ++i) {
        EXPECT_EQ(54 + (5 + i) * 64, *it);
    }

    // Remove A section
    // |BBBBB|-----|--------------------------------------------|CCCCC|DDDDD|
    for (int i{0}; i < 5; ++i) {
        s.erase(59 + i * 64);
    }

    it = s.begin();
    for (int i{0}; it != s.end() && i < 5; ++it, ++i) {
        EXPECT_EQ(59 + (5 + i) * 64, *it);
    }
    for (int i{0}; it != s.end() && i < 5; ++it, ++i) {
        EXPECT_EQ(54 + i * 64, *it);
    }
    for (int i{0}; it != s.end() && i < 5; ++it, ++i) {
        EXPECT_EQ(54 + (5 + i) * 64, *it);
    }

    // Remove C section, allowing B section to propagate back
    // |-----|-----|--------------------------------------------|DDDDD|BBBBB|
    for (int i{0}; i < 5; ++i) {
        s.erase(54 + i * 64);
    }

    //EXPECT_EQ(size_t(5u), s.bucket_size(59u));
    //EXPECT_EQ(size_t(5u), s.bucket_size(54u));

    it = s.begin();
    for (int i{0}; it != s.end() && i < 5; ++it, ++i) {
        EXPECT_EQ(54 + (9 - i) * 64, *it);
    }
    for (int i{0}; it != s.end() && i < 5; ++it, ++i) {
        EXPECT_EQ(59 + (5 + i) * 64, *it);
    }
}*/

struct SetDistStats {
    size_t min, max, median;
    double mean, stdDev;
    std::unordered_map<size_t, size_t> histo;
};

template <typename V, typename H>
SetDistStats calcStats(const qc_hash::Set<V, H> & set) {
    SetDistStats distStats{};

    distStats.min = ~size_t(0u);
    for (auto it{set.cbegin()}; it != set.cend(); ++it) {
        const size_t dist{QcHashMapFriend::dist(set, it)};
        ++distStats.histo[dist];
        if (dist < distStats.min) distStats.min = dist;
        else if (dist > distStats.max) distStats.max = dist;
        distStats.mean += dist;
    }
    distStats.mean /= double(set.size());

    for (auto it{set.cbegin()}; it != set.cend(); ++it) {
        const size_t dist{QcHashMapFriend::dist(set, it)};
        double diff{double(dist) - distStats.mean};
        distStats.stdDev += diff * diff;
    }
    distStats.stdDev = std::sqrt(distStats.stdDev / double(set.size()));

    size_t medianCount{0u};
    for (const auto distCount : distStats.histo) {
        if (distCount.second > medianCount) {
            distStats.median = distCount.first;
            medianCount = distCount.second;
        }
    }

    return distStats;
}

/*void printHisto(const SetStats & stats) {
    int sizeDigits = stats.max ? (int)log10(stats.max) + 1 : 1;
    size_t maxCount = stats.histo.at(stats.median);
    int countDigits = maxCount ? (int)log10(maxCount) + 1 : 1;
    int maxLength = 80 - sizeDigits - countDigits - 5; // 5 is for "[][]" & \n
    int length;
    for (auto bucket_size : stats.histo) {
        cout << "[";
        cout << std::setw(sizeDigits);
        cout << bucket_size.first << "][";
        cout << std::setw(countDigits);
        cout << bucket_size.second;
        cout << "]";
        length = int((double)maxLength * bucket_size.second / maxCount + 0.5f);
        for (int j = 0; j < length; ++j) {
            cout << '-';
        }
        cout << endl;
    }
}*/

TEST(set, stats) {
    constexpr size_t size{8192};

    qc_hash::Set<int> s{size};
    for (int i{0}; i < size; ++i) {
        s.insert(i);
    }

    const SetDistStats stats{calcStats(s)};
    EXPECT_EQ(size, stats.histo.at(0));
    EXPECT_EQ(size, stats.histo.at(1));
    EXPECT_NEAR(0.5, stats.mean, 1.0e-6);
    EXPECT_NEAR(0.7, stats.stdDev, 0.1);
}

/*TEST(set, terminator) {
    // TODO: find a better way to do this
    struct Entry {
        int val;
        unsigned int dist;
    };

    qc_hash::Set<int> s;
    s.insert(0);
    for (int i{0}; i < 5; ++i) {
        const auto it{s.end()};
        EXPECT_EQ(std::numeric_limits<unsigned int>::max(), reinterpret_cast<const Entry *>(reinterpret_cast<const size_t &>(it))->dist);
        s.rehash(2u * s.slot_count());
    }
}

template <typename K, typename T> using RecordMap = qc_hash::Map<K, T, qc_hash::Hash<K>, std::equal_to<K>, qc::RecordAllocator<std::conditional_t<std::is_same_v<T, void>, K, std::pair<K, T>>>>;
template <typename K> using MemRecordSet = qc_hash::Set<K, qc_hash::Hash<K>, std::equal_to<K>, qc::RecordAllocator<K>>;

template <typename K, typename T> void testStaticMemory() {
    static constexpr size_t capacity{128u};
    static constexpr size_t slotCount{capacity * 2u};

    MemRecordSet<K> s(capacity);
    s.emplace(K{});
    EXPECT_EQ(size_t(sizeof(size_t) * 4u), sizeof(qc_hash::Set<K>));
    EXPECT_EQ(slotCount * (1u + sizeof(K)) + 8u, s.get_allocator().current());

    RecordMap<K, T> m(capacity);
    m.emplace(K{}, T{});
    EXPECT_EQ(size_t(sizeof(size_t) * 4u), sizeof(qc_hash::Map<K, T>));
    EXPECT_EQ(slotCount * (1u + sizeof(std::pair<K, T>)) + 8u, m.get_allocator().current());
}

TEST(set, staticMemory) {
    testStaticMemory<s8, s8>();
    testStaticMemory<s8, s16>();
    testStaticMemory<s8, s32>();
    testStaticMemory<s8, s64>();
    testStaticMemory<s16, s8>();
    testStaticMemory<s16, s16>();
    testStaticMemory<s16, s32>();
    testStaticMemory<s16, s64>();
    testStaticMemory<s32, s8>();
    testStaticMemory<s32, s16>();
    testStaticMemory<s32, s32>();
    testStaticMemory<s32, s64>();
    testStaticMemory<s64, s8>();
    testStaticMemory<s64, s16>();
    testStaticMemory<s64, s32>();
    testStaticMemory<s64, s64>();

    testStaticMemory<std::string, std::string>();
    testStaticMemory<s16, std::string>();
    testStaticMemory<std::string, s16>();

    testStaticMemory<s8, std::tuple<s8, s8, s8>>();
}

TEST(set, dynamicMemory) {
    MemRecordSet<int> s(1024u);
    const size_t slotSize{1u + sizeof(int)};

    size_t current{0u}, total{0u}, allocations{0u}, deallocations{0u};
    EXPECT_EQ(current, s.get_allocator().current());
    EXPECT_EQ(total, s.get_allocator().total());
    EXPECT_EQ(allocations, s.get_allocator().allocations());
    EXPECT_EQ(deallocations, s.get_allocator().deallocations());

    s.rehash(64u);
    EXPECT_EQ(current, s.get_allocator().current());
    EXPECT_EQ(total, s.get_allocator().total());
    EXPECT_EQ(allocations, s.get_allocator().allocations());
    EXPECT_EQ(deallocations, s.get_allocator().deallocations());

    for (int i{0}; i < 32; ++i) s.emplace(i);
    current = 64u * slotSize + 8u;
    total += current;
    ++allocations;
    EXPECT_EQ(size_t(64u), s.slot_count());
    EXPECT_EQ(current, s.get_allocator().current());
    EXPECT_EQ(total, s.get_allocator().total());
    EXPECT_EQ(allocations, s.get_allocator().allocations());
    EXPECT_EQ(deallocations, s.get_allocator().deallocations());

    s.emplace(64);
    current = 128u * slotSize + 8u;
    total += current;
    ++allocations;
    ++deallocations;
    EXPECT_EQ(current, s.get_allocator().current());
    EXPECT_EQ(total, s.get_allocator().total());
    EXPECT_EQ(allocations, s.get_allocator().allocations());
    EXPECT_EQ(deallocations, s.get_allocator().deallocations());

    s.clear();
    EXPECT_EQ(current, s.get_allocator().current());
    EXPECT_EQ(total, s.get_allocator().total());
    EXPECT_EQ(allocations, s.get_allocator().allocations());
    EXPECT_EQ(deallocations, s.get_allocator().deallocations());

    s.rehash(1024u);
    current = 1024u * slotSize + 8u;
    total += current;
    ++allocations;
    ++deallocations;
    EXPECT_EQ(current, s.get_allocator().current());
    EXPECT_EQ(total, s.get_allocator().total());
    EXPECT_EQ(allocations, s.get_allocator().allocations());
    EXPECT_EQ(deallocations, s.get_allocator().deallocations());

    for (int i{0}; i < 128; ++i) s.emplace(i);
    EXPECT_EQ(current, s.get_allocator().current());
    EXPECT_EQ(total, s.get_allocator().total());
    EXPECT_EQ(allocations, s.get_allocator().allocations());
    EXPECT_EQ(deallocations, s.get_allocator().deallocations());

    s.emplace(128);
    EXPECT_EQ(current, s.get_allocator().current());
    EXPECT_EQ(total, s.get_allocator().total());
    EXPECT_EQ(allocations, s.get_allocator().allocations());
    EXPECT_EQ(deallocations, s.get_allocator().deallocations());

    s.erase(128);
    EXPECT_EQ(current, s.get_allocator().current());
    EXPECT_EQ(total, s.get_allocator().total());
    EXPECT_EQ(allocations, s.get_allocator().allocations());
    EXPECT_EQ(deallocations, s.get_allocator().deallocations());

    while (!s.empty()) s.erase(s.begin());
    EXPECT_EQ(current, s.get_allocator().current());
    EXPECT_EQ(total, s.get_allocator().total());
    EXPECT_EQ(allocations, s.get_allocator().allocations());
    EXPECT_EQ(deallocations, s.get_allocator().deallocations());
}

template <typename K, typename T>
int memoryUsagePer() {
    RecordMap<K, T> m({{}});
    return int((m.get_allocator().current() - 8u) / m.slot_count());
}

TEST(set, sensitivity) {
    struct Sensitive {
        Sensitive() = delete;
        Sensitive(const Sensitive &) = delete;
        Sensitive(Sensitive &&) = default;
        Sensitive & operator=(const Sensitive &) = delete;
        Sensitive & operator=(Sensitive &&) = default;
    };

    struct SensitiveHash {
        size_t operator()(const Sensitive & a) const {
            return 0u;
        }
    };

    qc_hash::Set<Sensitive, SensitiveHash> s;
    qc_hash::Map<Sensitive, Sensitive, SensitiveHash> m;
}

TEST(set, copyAversion) {
    Tracker::reset();

    qc_hash::Map<Tracker, Tracker, TrackerHash> m;
    EXPECT_FALSE(Tracker::copies());
    for (int i{0}; i < 100; ++i) {
        m.emplace(i, i);
    }
    EXPECT_FALSE(Tracker::copies());
    qc_hash::Map<Tracker, Tracker, TrackerHash> m2(std::move(m));
    EXPECT_FALSE(Tracker::copies());
    m = std::move(m2);
    EXPECT_FALSE(Tracker::copies());
    while (!m.empty()) m.erase(m.begin());
    EXPECT_FALSE(Tracker::copies());
}*/

TEST(set, asMap) {
    qc_hash::Set<int> s{};
    // These should all fail to compile with error about not being for sets
    //s.at(0);
    //s[0];
    //s.emplace(0, 0);
    //s.emplace(std::piecewise_construct, std::make_tuple(0), std::make_tuple(0));
    //s.try_emplace(0, 0);
}

TEST(map, general) {
    Tracked::resetTotals();
    TrackedMap m{100};

    for (int i{0}; i < 25; ++i) {
        const auto [it, inserted]{m.emplace(Tracked{TrackedVal{i}}, TrackedVal{i + 100})};
        EXPECT_TRUE(inserted);
        EXPECT_EQ(i, int(it->first.val));
        EXPECT_EQ(i + 100, int(it->second.val));
    }
    TrackedStats stats{Tracked::totalStats};
    EXPECT_EQ(25u, stats.moveConstructs);
    EXPECT_EQ(25u, stats.destructs);
    EXPECT_EQ(50u, stats.all());

    for (int i{25}; i < 50; ++i) {
        const auto [it, inserted]{m.emplace(std::piecewise_construct, std::forward_as_tuple(TrackedVal{i}), std::forward_as_tuple(TrackedVal{i + 100}))};
        EXPECT_TRUE(inserted);
        EXPECT_EQ(i, int(it->first.val));
        EXPECT_EQ(i + 100, int(it->second.val));
    }
    stats = Tracked::totalStats - stats;
    EXPECT_EQ(25u, stats.moveConstructs);
    EXPECT_EQ(25u, stats.destructs);
    EXPECT_EQ(50u, stats.all());

    for (int i{50}; i < 75; ++i) {
        const auto [it, inserted]{m.try_emplace(Tracked{TrackedVal{i}}, TrackedVal{i + 100})};
        EXPECT_TRUE(inserted);
        EXPECT_EQ(i, int(it->first.val));
        EXPECT_EQ(i + 100, int(it->second.val));
    }
    stats = Tracked::totalStats - stats;
    EXPECT_EQ(25u, stats.moveConstructs);
    EXPECT_EQ(25u, stats.destructs);
    EXPECT_EQ(50u, stats.all());

    for (int i{50}; i < 75; ++i) {
        const auto [it, inserted]{m.try_emplace(Tracked{TrackedVal{i}}, TrackedVal{i + 100})};
        EXPECT_FALSE(inserted);
        EXPECT_EQ(m.cend(), it);
    }
    stats = Tracked::totalStats - stats;
    EXPECT_EQ(0u, stats.moveConstructs);
    EXPECT_EQ(25u, stats.destructs);
    EXPECT_EQ(25u, stats.all());

    for (int i{75}; i < 100; ++i) {
        const auto [it, inserted]{m.insert(std::pair<Tracked, Tracked>{Tracked{TrackedVal{i}}, Tracked{TrackedVal{i + 100}}})};
        EXPECT_TRUE(inserted);
        EXPECT_EQ(i, int(it->first.val));
        EXPECT_EQ(i + 100, int(it->second.val));
    }
    stats = Tracked::totalStats - stats;
    EXPECT_EQ(50u, stats.moveConstructs);
    EXPECT_EQ(50u, stats.destructs);
    EXPECT_EQ(100u, stats.all());

    for (int i{0}; i < 100; ++i) {
        EXPECT_EQ(i + 100, int(m.at(Tracked{TrackedVal{i}}).val));
        EXPECT_EQ(i + 100, int(m[Tracked{TrackedVal{i}}].val));
    }

    EXPECT_THROW(m.at(Tracked{TrackedVal{100}}), std::out_of_range);
    EXPECT_EQ(Tracked{}, m[Tracked{TrackedVal{100}}]);
    m[Tracked{TrackedVal{100}}] = Tracked{TrackedVal{200}};
    EXPECT_EQ(Tracked{TrackedVal{200}}, m[Tracked{TrackedVal{100}}]);

    TrackedMap m2{m};
    EXPECT_EQ(m, m2);

    m2[Tracked{TrackedVal{100}}].val = TrackedVal{400};
    EXPECT_NE(m, m2);
}
