// First include in order to test its own includes
#include <qc-hash/qc-map-flat.hpp>

#include <cmath>

#include <map>
#include <vector>

#include <gtest/gtest.h>

#include <qc-core/core.hpp>
#include <qc-core/memory.hpp>

using namespace std::string_literals;
using namespace qc::types;

struct QcHashMapFriend {

    template <typename K, typename H, typename KE, typename A>
    static const u8 & getControl(const qc_hash_flat::Set<K, H, KE, A> & set, const size_t slotI) {
        return set._controls[slotI];
    }

    template <typename It>
    static const u8 & getControl(const It it) {
        return *it._control;
    }

    template <typename K, typename H, typename KE, typename A>
    static int getControlHash(const qc_hash_flat::Set<K, H, KE, A> & set, const size_t slotI) {
        return set._controls[slotI] & 0b1111111u;
    }

    template <typename K, typename H, typename KE, typename A>
    static const K & getElement(const qc_hash_flat::Set<K, H, KE, A> & set, const size_t slotI) {
        return set._elements()[slotI].get();
    }

    template <typename K, typename H, typename KE, typename A>
    static bool isPresent(const qc_hash_flat::Set<K, H, KE, A> & set, const size_t slotI) {
        return set._controls[slotI] >> 7;
    }

    template <typename K, typename H, typename KE, typename A>
    static bool isEmpty(const qc_hash_flat::Set<K, H, KE, A> & set, const size_t slotI) {
        return !set._controls[slotI];
    }

    template <typename K, typename H, typename KE, typename A>
    static bool isGrave(const qc_hash_flat::Set<K, H, KE, A> & set, const size_t slotI) {
        return set._controls[slotI] == qc_hash_flat::_graveControl;
    }

    template <typename K, typename H, typename KE, typename A, typename It>
    static size_t slotI(const qc_hash_flat::Set<K, H, KE, A> & set, const It it) {
        return it._control - set._controls;
    }

    template <typename K, typename H, typename KE, typename A, typename It>
    static size_t dist(const qc_hash_flat::Set<K, H, KE, A> & set, const It it) {
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

    bool operator==(const TrackedStats &) const = default;
};

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
        return qc_hash_flat::config::DefaultHash<TrackedVal>{}(tracked.val);
    }
};

using TrackedSet = qc_hash_flat::Set<Tracked, TrackedHash>;
using TrackedMap = qc_hash_flat::Map<Tracked, Tracked, TrackedHash>;

template <typename K> using MemRecordSet = qc_hash_flat::Set<K, qc_hash_flat::config::DefaultHash<K>, std::equal_to<K>, qc::memory::RecordAllocator<K>>;
template <typename K, typename V> using MemRecordMap = qc_hash_flat::Map<K, V, qc_hash_flat::config::DefaultHash<K>, std::equal_to<K>, qc::memory::RecordAllocator<std::pair<K, V>>>;

using MemRecordTrackedSet = qc_hash_flat::Set<Tracked, TrackedHash, std::equal_to<Tracked>, qc::memory::RecordAllocator<Tracked>>;

// Top 7 bits are control hash, bits [8,15] are payload, and bottom 8 bits are slot
struct NittyGrittyVal {

    size_t val;

    NittyGrittyVal() = default;

    NittyGrittyVal(const int controlHash, const int payload, const int slot) :
        val{
            (size_t(controlHash) << (std::numeric_limits<size_t>::digits - 7)) |
            ((size_t(payload) & 0xFFu) << 8) |
            (size_t(slot) & 0xFFu)
        }
    {}

    int controlHash() const {
        return int(val >> (std::numeric_limits<size_t>::digits - 7));
    }

    int payload() const {
        return int((val >> 8) & 0xFFu);
    }

    int slot() const {
        return int(val & 0xFFu);
    }

    bool operator==(const NittyGrittyVal & other) const = default;

};

static_assert(std::is_trivial_v<NittyGrittyVal>);

struct NittyGrittyHash {
    size_t operator()(const NittyGrittyVal k) const noexcept {
        return k.val & ~size_t(0xFF00u);
    }
};

using NittyGrittySet = qc_hash_flat::Set<NittyGrittyVal, NittyGrittyHash>;

TEST(setFlat, constructor_default) {
    MemRecordSet<int> s{};
    EXPECT_EQ(qc_hash_flat::config::minCapacity, s.capacity());
    EXPECT_EQ(size_t(0u), s.size());
    EXPECT_EQ(0u, s.get_allocator().stats().allocations);
}

TEST(setFlat, constructor_capacity) {
    qc::memory::RecordAllocator<int> allocator{};
    EXPECT_EQ(size_t(  16u), (MemRecordSet<int>{   0u, allocator}.capacity()));
    EXPECT_EQ(size_t(  16u), (MemRecordSet<int>{   1u, allocator}.capacity()));
    EXPECT_EQ(size_t(  16u), (MemRecordSet<int>{  16u, allocator}.capacity()));
    EXPECT_EQ(size_t(  32u), (MemRecordSet<int>{  17u, allocator}.capacity()));
    EXPECT_EQ(size_t(  32u), (MemRecordSet<int>{  32u, allocator}.capacity()));
    EXPECT_EQ(size_t(  64u), (MemRecordSet<int>{  33u, allocator}.capacity()));
    EXPECT_EQ(size_t(  64u), (MemRecordSet<int>{  64u, allocator}.capacity()));
    EXPECT_EQ(size_t(1024u), (MemRecordSet<int>{1000u, allocator}.capacity()));
    EXPECT_EQ(0u, allocator.stats().allocations);
}

TEST(setFlat, constructor_range) {
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
    EXPECT_EQ(1u, s.get_allocator().stats().allocations);
}

TEST(setFlat, constructor_initializerList) {
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
    EXPECT_EQ(1u, s.get_allocator().stats().allocations);
}

TEST(setFlat, constructor_copy) {
    // Trivial type
    {
        MemRecordSet<int> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.insert(i);
        }
        const size_t prevAllocCount{s1.get_allocator().stats().allocations};
        MemRecordSet<int> s2{s1};
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s1, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().stats().allocations);
    }

    // Non-trivial type
    {
        MemRecordSet<std::string> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.insert(std::to_string(i));
        }
        const size_t prevAllocCount{s1.get_allocator().stats().allocations};
        MemRecordSet<std::string> s2{s1};
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s1, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().stats().allocations);
    }
}

TEST(setFlat, constructor_move) {
    // Trivial type
    {
        MemRecordSet<int> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.insert(i);
        }
        const size_t prevAllocCount{s1.get_allocator().stats().allocations};
        MemRecordSet<int> ref{s1};
        MemRecordSet<int> s2{std::move(s1)};
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(ref, s2);
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc_hash_flat::config::minCapacity, s1.capacity());
        EXPECT_EQ(prevAllocCount, s2.get_allocator().stats().allocations);
    }
    // Non-trivial type
    {
        MemRecordSet<std::string> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.insert(std::to_string(i));
        }
        const size_t prevAllocCount{s1.get_allocator().stats().allocations};
        MemRecordSet<std::string> ref{s1};
        MemRecordSet<std::string> s2{std::move(s1)};
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(ref, s2);
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc_hash_flat::config::minCapacity, s1.capacity());
        EXPECT_EQ(prevAllocCount, s2.get_allocator().stats().allocations);
    }
}

TEST(setFlat, assignOperator_initializerList) {
    MemRecordSet<int> s{};
    for (int i{0}; i < 100; ++i) {
        s.insert(i);
    }
    EXPECT_EQ(128u, s.capacity());
    const size_t prevAllocCount{s.get_allocator().stats().allocations};

    s = {0, 1, 2, 3, 4, 5};
    EXPECT_EQ(size_t(6u), s.size());
    EXPECT_EQ(qc_hash_flat::config::minCapacity, s.capacity());
    for (int i{0}; i < 6; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
}

TEST(setFlat, assignOperator_copy) {
    // Trivial type
    {
        MemRecordSet<int> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.insert(i);
        }
        const size_t prevAllocCount{s1.get_allocator().stats().allocations};

        MemRecordSet<int> s2{};
        s2 = s1;
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s1, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().stats().allocations);

        s2 = s2;
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s2, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().stats().allocations);
    }

    // Non-trivial type
    {
        MemRecordSet<std::string> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.insert(std::to_string(i));
        }
        const size_t prevAllocCount{s1.get_allocator().stats().allocations};

        MemRecordSet<std::string> s2{};
        s2 = s1;
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s1, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().stats().allocations);

        s2 = s2;
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s2, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().stats().allocations);
    }
}

TEST(setFlat, assignOperator_move) {
    // Trivial type
    {
        MemRecordSet<int> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.insert(i);
        }
        const size_t prevAllocCount{s1.get_allocator().stats().allocations};
        MemRecordSet<int> ref{s1};

        MemRecordSet<int> s2{};
        s2 = std::move(s1);
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(ref, s2);
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc_hash_flat::config::minCapacity, s1.capacity());
        EXPECT_EQ(prevAllocCount, s2.get_allocator().stats().allocations);

        s2 = std::move(s2);
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s2, s2);
        EXPECT_EQ(prevAllocCount, s2.get_allocator().stats().allocations);

        s2 = std::move(s1);
        EXPECT_TRUE(s2.empty());
        EXPECT_EQ(qc_hash_flat::config::minCapacity, s2.capacity());
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc_hash_flat::config::minCapacity, s1.capacity());
        EXPECT_EQ(s1, s2);
    }

    // Non-trivial type
    {
        MemRecordSet<std::string> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.insert(std::to_string(i));
        }
        const size_t prevAllocCount{s1.get_allocator().stats().allocations};
        MemRecordSet<std::string> ref{s1};

        MemRecordSet<std::string> s2{};
        s2 = std::move(s1);
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(ref, s2);
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc_hash_flat::config::minCapacity, s1.capacity());
        EXPECT_EQ(prevAllocCount, s2.get_allocator().stats().allocations);

        s2 = std::move(s2);
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s2, s2);
        EXPECT_EQ(prevAllocCount, s2.get_allocator().stats().allocations);

        s2 = std::move(s1);
        EXPECT_TRUE(s2.empty());
        EXPECT_EQ(qc_hash_flat::config::minCapacity, s2.capacity());
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc_hash_flat::config::minCapacity, s1.capacity());
        EXPECT_EQ(s1, s2);
    }
}

// Keep parallel with `emplace_lVal` and `tryEmplace_lVal` tests
TEST(setFlat, insert_lVal) {
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
    EXPECT_EQ(4u, s.get_allocator().stats().allocations);
    EXPECT_EQ(100, Tracked::totalStats.copyConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked::totalStats.moveConstructs);
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked::totalStats.destructs);
    EXPECT_EQ(0, Tracked::totalStats.defConstructs);
    EXPECT_EQ(0, Tracked::totalStats.assigns());
}

// Keep parallel with `emplace_rVal` and `tryEmplace_rVal` tests
TEST(setFlat, insert_rVal) {
    TrackedSet s{};
    Tracked val1{TrackedVal{7}};
    Tracked val2{TrackedVal{7}};

    const auto [it1, inserted1]{s.insert(std::move(val1))};
    EXPECT_TRUE(inserted1);
    const Tracked & tracked{*it1};
    EXPECT_EQ(7, int(tracked.val));
    EXPECT_EQ(1, tracked.stats.moveConstructs);
    EXPECT_EQ(1, tracked.stats.all());
    EXPECT_EQ(0, int(val1.val));

    const auto [it2, inserted2]{s.insert(std::move(val2))};
    EXPECT_FALSE(inserted2);
    EXPECT_EQ(it1, it2);
    EXPECT_EQ(7, int(val2.val));
    EXPECT_EQ(0, val2.stats.all());
}

TEST(setFlat, insert_range) {
    MemRecordTrackedSet s{};
    std::vector<Tracked> values{};
    for (int i{0}; i < 100; ++i) values.emplace_back(TrackedVal{i});

    Tracked::resetTotals();
    s.insert(values.cbegin(), values.cend());
    EXPECT_EQ(100u, s.size());
    EXPECT_EQ(128u, s.capacity());
    for (int i{0}; i < 100; ++i) {
        EXPECT_TRUE(s.contains(Tracked{TrackedVal{i}}));
    }
    EXPECT_EQ(4u, s.get_allocator().stats().allocations);
    EXPECT_EQ(100, Tracked::totalStats.copyConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked::totalStats.moveConstructs);
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked::totalStats.destructs);
    EXPECT_EQ(0, Tracked::totalStats.defConstructs);
    EXPECT_EQ(0, Tracked::totalStats.assigns());
}

TEST(setFlat, insert_initializerList) {
    MemRecordSet<int> s{};
    s.insert({0, 1, 2, 3, 4, 5});
    EXPECT_EQ(6u, s.size());
    EXPECT_EQ(16u, s.capacity());
    for (int i{0}; i < 6; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
    EXPECT_EQ(1u, s.get_allocator().stats().allocations);
}

// Keep parallel with `insert_lVal` and `tryEmplace_lVal` tests
TEST(setFlat, emplace_lVal) {
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
    EXPECT_EQ(4u, s.get_allocator().stats().allocations);
    EXPECT_EQ(100, Tracked::totalStats.copyConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked::totalStats.moveConstructs);
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked::totalStats.destructs);
    EXPECT_EQ(0, Tracked::totalStats.defConstructs);
    EXPECT_EQ(0, Tracked::totalStats.assigns());
}

// Keep parallel with `insert_rVal` and `tryEmplace_rVal` tests
TEST(setFlat, emplace_rVal) {
    TrackedSet s{};
    Tracked val1{TrackedVal{7}};
    Tracked val2{TrackedVal{7}};

    const auto [it1, inserted1]{s.emplace(std::move(val1))};
    EXPECT_TRUE(inserted1);
    const Tracked & tracked{*it1};
    EXPECT_EQ(7, int(tracked.val));
    EXPECT_EQ(1, tracked.stats.moveConstructs);
    EXPECT_EQ(1, tracked.stats.all());
    EXPECT_EQ(0, int(val1.val));

    const auto [it2, inserted2]{s.emplace(std::move(val2))};
    EXPECT_FALSE(inserted2);
    EXPECT_EQ(it1, it2);
    EXPECT_EQ(7, int(val2.val));
    EXPECT_EQ(0, val2.stats.all());
}

TEST(setFlat, emplace_keyArgs) {
    TrackedSet s{};
    const auto [it, inserted]{s.emplace(TrackedVal{7})};
    EXPECT_TRUE(inserted);
    EXPECT_EQ(7, int(it->val));
    EXPECT_EQ(1, it->stats.moveConstructs);
    EXPECT_EQ(1, it->stats.all());
}

// Keep parallel with `insert_lVal` and `emplace_lVal` tests
TEST(setFlat, tryEmplace_lVal) {
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
    EXPECT_EQ(4u, s.get_allocator().stats().allocations);
    EXPECT_EQ(100, Tracked::totalStats.copyConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked::totalStats.moveConstructs);
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked::totalStats.destructs);
    EXPECT_EQ(0, Tracked::totalStats.defConstructs);
    EXPECT_EQ(0, Tracked::totalStats.assigns());
}

// Keep parallel with `insert_rVal` and `emplace_rVal` tests
TEST(setFlat, tryEmplace_rVal) {
    TrackedSet s{};
    Tracked val1{TrackedVal{7}};
    Tracked val2{TrackedVal{7}};

    const auto [it1, inserted1]{s.try_emplace(std::move(val1))};
    EXPECT_TRUE(inserted1);
    const Tracked & tracked{*it1};
    EXPECT_EQ(7, int(tracked.val));
    EXPECT_EQ(1, tracked.stats.moveConstructs);
    EXPECT_EQ(1, tracked.stats.all());
    EXPECT_EQ(0, int(val1.val));

    const auto [it2, inserted2]{s.try_emplace(std::move(val2))};
    EXPECT_FALSE(inserted2);
    EXPECT_EQ(it1, it2);
    EXPECT_EQ(7, int(val2.val));
    EXPECT_EQ(0, val2.stats.all());
}

TEST(setFlat, eraseKey) {
    TrackedSet s{};

    Tracked::resetTotals();
    EXPECT_FALSE(s.erase(Tracked{TrackedVal{0}}));
    EXPECT_EQ(1, Tracked::totalStats.destructs);
    EXPECT_EQ(1, Tracked::totalStats.all());

    Tracked::resetTotals();
    for (int i{0}; i < 100; ++i) {
        s.emplace(TrackedVal{i});
    }
    EXPECT_EQ(size_t(100u), s.size());
    EXPECT_EQ(size_t(128u), s.capacity());
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked::totalStats.moveConstructs);
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked::totalStats.destructs);
    EXPECT_EQ(200 + 32 + 64 + 128, Tracked::totalStats.all());

    Tracked::resetTotals();
    EXPECT_FALSE(s.erase(Tracked{TrackedVal{100}}));
    EXPECT_EQ(1, Tracked::totalStats.destructs);
    EXPECT_EQ(1, Tracked::totalStats.all());

    Tracked::resetTotals();
    for (int i{0}; i < 100; ++i) {
        EXPECT_TRUE(s.erase(Tracked{TrackedVal{i}}));
        EXPECT_EQ(size_t(100u - i - 1u), s.size());
    }
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(size_t(128u), s.capacity());
    EXPECT_EQ(200, Tracked::totalStats.destructs);
    EXPECT_EQ(200, Tracked::totalStats.all());
    Tracked::resetTotals();
}

TEST(setFlat, eraseIterator) {
    qc_hash_flat::Set<int> s{};

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

TEST(setFlat, clear) {
    // Trivially destructible type
    {
        qc_hash_flat::Set<int> s{};
        for (int i{0}; i < 100; ++i) s.insert(i);
        EXPECT_EQ(size_t(100u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());

        s.clear();
        EXPECT_EQ(size_t(0u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());
    }

    // Non-trivially destructible type
    {

        TrackedSet s{};
        for (int i{0}; i < 100; ++i) s.emplace(TrackedVal{i});
        EXPECT_EQ(size_t(100u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());

        Tracked::resetTotals();
        s.clear();
        EXPECT_EQ(size_t(0u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());
        EXPECT_EQ(100, Tracked::totalStats.destructs);
        EXPECT_EQ(100, Tracked::totalStats.all());
    }
}

// Keep parallel with `count` test
TEST(setFlat, contains) {
    qc_hash_flat::Set<int> s{};
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
TEST(setFlat, count) {
    qc_hash_flat::Set<int> s{};
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

TEST(setFlat, begin) {
    qc_hash_flat::Set<int> s{};
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

TEST(setFlat, end) {
    qc_hash_flat::Set<int> s{};
    EXPECT_EQ(s.begin(), s.end());
    EXPECT_EQ(s.end(), s.end());

    s.insert(7);
    EXPECT_NE(s.begin(), s.end());
    EXPECT_EQ(u8(0b11111111u), QcHashMapFriend::getControl(s.end()));

    EXPECT_EQ(s.end(), s.cend());
}

// Keep parallel with `equal_range` test
TEST(setFlat, find) {
    qc_hash_flat::Set<int> s{};
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
TEST(setFlat, equalRange) {
    using ItPair = std::pair<qc_hash_flat::Set<int>::iterator, qc_hash_flat::Set<int>::iterator>;

    qc_hash_flat::Set<int> s{};
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

TEST(setFlat, slot) {
    qc_hash_flat::Set<int> s(128);
    s.insert(7);
    EXPECT_EQ(QcHashMapFriend::slotI(s, s.find(7)), s.slot(7));
}

// `reserve` method is synonymous with `rehash` method
TEST(setFlat, reserve) {}

TEST(setFlat, rehash) {
    qc_hash_flat::Set<int> s{};

    EXPECT_EQ(qc_hash_flat::config::minSlotCount, s.slot_count());

    s.rehash(0u);
    EXPECT_EQ(qc_hash_flat::config::minSlotCount, s.slot_count());

    s.rehash(1u);
    EXPECT_EQ(qc_hash_flat::config::minSlotCount, s.slot_count());

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
    EXPECT_EQ(qc_hash_flat::config::minSlotCount, s.slot_count());
}

TEST(setFlat, swap) {
    qc_hash_flat::Set<int> s1{1, 2, 3};
    qc_hash_flat::Set<int> s2{4, 5, 6};
    qc_hash_flat::Set<int> s3{s1};
    qc_hash_flat::Set<int> s4{s2};
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

TEST(setFlat, size_empty_capacity_slotCount) {
    qc_hash_flat::Set<int> s{};
    EXPECT_EQ(0u, s.size());
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(qc_hash_flat::config::minCapacity, s.capacity());
    EXPECT_EQ(qc_hash_flat::config::minSlotCount, s.slot_count());

    for (int i{0}; i < 100; ++i) {
        s.insert(i);
    }
    EXPECT_EQ(100u, s.size());
    EXPECT_FALSE(s.empty());
    EXPECT_EQ(128u, s.capacity());
    EXPECT_EQ(256u, s.slot_count());
}

TEST(setFlat, maxSize) {
    qc_hash_flat::Set<int> s{};
    if constexpr (sizeof(size_t) == 4) {
        EXPECT_EQ(0b01000000'00000000'00000000'00000000u, s.max_size());
    }
    else if constexpr (sizeof(size_t) == 8) {
        EXPECT_EQ(0b01000000'00000000'00000000'00000000'00000000'00000000'00000000'00000000u, s.max_size());
    }
}

TEST(setFlat, maxSlotCount) {
    qc_hash_flat::Set<int> s{};
    if constexpr (sizeof(size_t) == 4) {
        EXPECT_EQ(0b10000000'00000000'00000000'00000000u, s.max_slot_count());
    }
    else if constexpr (sizeof(size_t) == 8) {
        EXPECT_EQ(0b10000000'00000000'00000000'00000000'00000000'00000000'00000000'00000000u, s.max_slot_count());
    }
}

TEST(setFlat, maxLoadFactor) {
    qc_hash_flat::Set<int> s{};
    EXPECT_EQ(0.5f, s.max_load_factor());

    s.insert(7);
    EXPECT_EQ(0.5f, s.max_load_factor());
}

TEST(setFlat, equality) {
    qc_hash_flat::Set<int> s1{}, s2{};
    for (int i{0}; i < 100; ++i) {
        s1.emplace(i);
        s2.emplace(i + 100);
    }
    EXPECT_TRUE(s1 == s1);
    EXPECT_TRUE(s1 != s2);

    s2 = s1;
    EXPECT_TRUE(s1 == s2);
}

TEST(setFlat, iteratorTrivial) {
    static_assert(std::is_trivial_v<qc_hash_flat::Set<int>::iterator>);
    static_assert(std::is_trivial_v<qc_hash_flat::Set<int>::const_iterator>);
    static_assert(std::is_standard_layout_v<qc_hash_flat::Set<int>::iterator>);
    static_assert(std::is_standard_layout_v<qc_hash_flat::Set<int>::const_iterator>);
}

TEST(setFlat, iterator) {
    NittyGrittySet s{};
    for (int i{0}; i < 100; ++i) {
        s.emplace(i, i, i);
    }

    int i{0};
    for (auto it{s.begin()}; it != s.end(); ++it) {
        EXPECT_EQ((NittyGrittyVal{i, i, i}.val + 1), ++it->val);
        EXPECT_EQ((NittyGrittyVal{i, i, i}.val), --it->val);
        ++i;
    }
}

TEST(setFlat, forEachLoop) {
    NittyGrittySet s{};
    for (int i{0}; i < 100; ++i) {
        s.emplace(i, i, i);
    }

    int i{0};
    for (NittyGrittyVal & val : s) {
        EXPECT_EQ((NittyGrittyVal{i, i, i}.val + 1), ++val.val);
        EXPECT_EQ((NittyGrittyVal{i, i, i}.val), --val.val);
        ++i;
    }
}

TEST(setFlat, iteratorConversion) {
    // Just checking for compilation

    qc_hash_flat::Set<int> s{1, 2, 3};

    qc_hash_flat::Set<int>::iterator it1(s.begin());
    qc_hash_flat::Set<int>::const_iterator cit1 = s.cbegin();

    //it1 = cit1; // Should not compile
    static_assert(!std::is_convertible_v<qc_hash_flat::Set<int>::const_iterator, qc_hash_flat::Set<int>::iterator>);
    cit1 = it1;

    ++*it1;
    //++*cit1; // Should not compile
    static_assert(std::is_const_v<std::remove_reference_t<decltype(*cit1)>>);

    qc_hash_flat::Set<int>::iterator it2{it1};
    it2 = it1;
    qc_hash_flat::Set<int>::const_iterator cit2{cit1};
    cit2 = cit1;

    qc_hash_flat::Set<int>::iterator it3{std::move(it1)};
    it3 = std::move(it1);
    qc_hash_flat::Set<int>::const_iterator cit3{std::move(cit1)};
    cit3 = std::move(cit1);

    it1 == cit1;
    cit1 == it1;
}

TEST(setFlat, nittyGrittyBasic) {
    NittyGrittySet s(16);

    s.emplace(0, 0, 0);
    EXPECT_TRUE(QcHashMapFriend::isPresent(s, 0u));
    EXPECT_EQ(0, QcHashMapFriend::getControlHash(s, 0u));
    EXPECT_EQ(0, QcHashMapFriend::getElement(s, 0u).slot());
    for (int i{1}; i < 32; ++i) {
        EXPECT_FALSE(QcHashMapFriend::isPresent(s, i));
        EXPECT_EQ(0, QcHashMapFriend::getControlHash(s, i));
    }

    for (int i{1}; i < 16; ++i) {
        s.emplace(i, 0, i);
    }
    for (int i{0}; i < 16; ++i) {
        EXPECT_TRUE(QcHashMapFriend::isPresent(s, i));
        EXPECT_EQ(i, QcHashMapFriend::getControlHash(s, i));
        EXPECT_EQ(i, QcHashMapFriend::getElement(s, i).slot());
    }
    for (int i{16}; i < 32; ++i) {
        EXPECT_TRUE(QcHashMapFriend::isEmpty(s, i));
    }

    for (int i{0}; i < 16; ++i) {
        s.erase(NittyGrittyVal{i, 0, i});
    }
    for (int i{0}; i < 16; ++i) {
        EXPECT_TRUE(QcHashMapFriend::isGrave(s, i));
    }
    for (int i{16}; i < 32; ++i) {
        EXPECT_TRUE(QcHashMapFriend::isEmpty(s, i));
    }
}

TEST(setFlat, nittyGrittyGraveClear) {
    // Trivial type clear
    {
        NittyGrittySet s(16);
        for (int i{0}; i < 16; ++i) {
            s.emplace(i, i, i);
        }
        for (int i{0}; i < 16; ++i) {
            s.erase(NittyGrittyVal{i, i, i});
        }

        for (int i{0}; i < 16; ++i) {
            EXPECT_TRUE(QcHashMapFriend::isGrave(s, i));
        }

        s.clear();

        for (int i{0}; i < 32; ++i) {
            EXPECT_TRUE(QcHashMapFriend::isEmpty(s, i));
        }
    }

    // Trivial type rehash
    {
        NittyGrittySet s(16);
        for (int i{0}; i < 16; ++i) {
            const NittyGrittyVal val{i, i, i};
            s.insert(val);
            s.erase(val);
        }

        for (int i{0}; i < 16; ++i) {
            EXPECT_TRUE(QcHashMapFriend::isGrave(s, i));
        }

        s.reserve(32);

        for (int i{0}; i < 64; ++i) {
            EXPECT_TRUE(QcHashMapFriend::isEmpty(s, i));
        }
    }

    // Non-trivial type clear
    {
        qc_hash_flat::Set<std::string> s(16);
        for (int i{0}; i < 16; ++i) {
            s.insert(std::to_string(i));
        }
        for (int i{0}; i < 16; ++i) {
            s.erase(std::to_string(i));
        }

        int graveCount{0};
        for (int i{0}; i < 32; ++i) {
            if (QcHashMapFriend::isGrave(s, i)) {
                ++graveCount;
            }
        }
        EXPECT_EQ(16, graveCount);

        s.clear();

        graveCount = 0;
        for (int i{0}; i < 32; ++i) {
            if (QcHashMapFriend::isGrave(s, i)) {
                ++graveCount;
            }
        }
        EXPECT_EQ(0, graveCount);
    }

    // Non-trivial type rehash
    {
        qc_hash_flat::Set<std::string> s(16);
        for (int i{0}; i < 16; ++i) {
            s.insert(std::to_string(i));
        }
        for (int i{0}; i < 16; ++i) {
            s.erase(std::to_string(i));
        }

        int graveCount{0};
        for (int i{0}; i < 32; ++i) {
            if (QcHashMapFriend::isGrave(s, i)) {
                ++graveCount;
            }
        }
        EXPECT_EQ(16, graveCount);

        s.reserve(32);

        graveCount = 0;
        for (int i{0}; i < 64; ++i) {
            if (QcHashMapFriend::isGrave(s, i)) {
                ++graveCount;
            }
        }
        EXPECT_EQ(0, graveCount);
    }
}

TEST(setFlat, nittyGrittyCollisionAndWrap) {
    // Same slot, different control hashes
    {
        NittyGrittySet s(16);

        for (int i{28}; i < 32; ++i) {
            s.emplace(i, i, 28);
        }
        for (int i{0}; i < 12; ++i) {
            s.emplace(i, i, 28);
        }
        for (int i{28}; i < 32; ++i) {
            EXPECT_TRUE(QcHashMapFriend::isPresent(s, i));
            EXPECT_EQ(i, QcHashMapFriend::getControlHash(s, i));
            EXPECT_EQ(i, QcHashMapFriend::getElement(s, i).payload());
            EXPECT_EQ(28, QcHashMapFriend::getElement(s, i).slot());
        }
        for (int i{0}; i < 12; ++i) {
            EXPECT_TRUE(QcHashMapFriend::isPresent(s, i));
            EXPECT_EQ(i, QcHashMapFriend::getControlHash(s, i));
            EXPECT_EQ(i, QcHashMapFriend::getElement(s, i).payload());
            EXPECT_EQ(28, QcHashMapFriend::getElement(s, i).slot());
        }
        for (int i{12}; i < 28; ++i) {
            EXPECT_TRUE(QcHashMapFriend::isEmpty(s, i));
        }
    }

    // Different slots, same control hash
    {
        NittyGrittySet s(16);

        for (int i{28}; i < 32; ++i) {
            s.emplace(0, i, i);
        }
        for (int i{0}; i < 12; ++i) {
            s.emplace(0, i, i);
        }
        for (int i{28}; i < 32; ++i) {
            EXPECT_TRUE(QcHashMapFriend::isPresent(s, i));
            EXPECT_EQ(0, QcHashMapFriend::getControlHash(s, i));
            EXPECT_EQ(i, QcHashMapFriend::getElement(s, i).payload());
            EXPECT_EQ(i, QcHashMapFriend::getElement(s, i).slot());
        }
        for (int i{0}; i < 12; ++i) {
            EXPECT_TRUE(QcHashMapFriend::isPresent(s, i));
            EXPECT_EQ(0, QcHashMapFriend::getControlHash(s, i));
            EXPECT_EQ(i, QcHashMapFriend::getElement(s, i).payload());
            EXPECT_EQ(i, QcHashMapFriend::getElement(s, i).slot());
        }
        for (int i{12}; i < 28; ++i) {
            EXPECT_TRUE(QcHashMapFriend::isEmpty(s, i));
        }
    }

    // Same slot, same control hash
    {
        NittyGrittySet s(16);

        for (int i{28}; i < 32; ++i) {
            s.emplace(0, i, 28);
        }
        for (int i{0}; i < 12; ++i) {
            s.emplace(0, i, 28);
        }
        for (int i{28}; i < 32; ++i) {
            EXPECT_TRUE(QcHashMapFriend::isPresent(s, i));
            EXPECT_EQ(0, QcHashMapFriend::getControlHash(s, i));
            EXPECT_EQ(i, QcHashMapFriend::getElement(s, i).payload());
            EXPECT_EQ(28, QcHashMapFriend::getElement(s, i).slot());
        }
        for (int i{0}; i < 12; ++i) {
            EXPECT_TRUE(QcHashMapFriend::isPresent(s, i));
            EXPECT_EQ(0, QcHashMapFriend::getControlHash(s, i));
            EXPECT_EQ(i, QcHashMapFriend::getElement(s, i).payload());
            EXPECT_EQ(28, QcHashMapFriend::getElement(s, i).slot());
        }
        for (int i{12}; i < 28; ++i) {
            EXPECT_TRUE(QcHashMapFriend::isEmpty(s, i));
        }
    }
}

TEST(setFlat, nittyGrittyGraveFill) {
    NittyGrittySet s(16);

    s.emplace(0, 0, 0);
    s.emplace(1, 1, 0);
    s.emplace(2, 2, 0);
    s.erase(NittyGrittyVal{1, 1, 0});
    EXPECT_TRUE(QcHashMapFriend::isPresent(s, 0));
    EXPECT_EQ(0, QcHashMapFriend::getElement(s, 0).payload());
    EXPECT_TRUE(QcHashMapFriend::isGrave(s, 1));
    EXPECT_TRUE(QcHashMapFriend::isPresent(s, 2));
    EXPECT_EQ(2, QcHashMapFriend::getElement(s, 2).payload());

    s.emplace(3, 3, 0);
    EXPECT_TRUE(QcHashMapFriend::isPresent(s, 0));
    EXPECT_EQ(0, QcHashMapFriend::getElement(s, 0).payload());
    EXPECT_TRUE(QcHashMapFriend::isPresent(s, 1));
    EXPECT_EQ(3, QcHashMapFriend::getElement(s, 1).payload());
    EXPECT_TRUE(QcHashMapFriend::isPresent(s, 2));
    EXPECT_EQ(2, QcHashMapFriend::getElement(s, 2).payload());
}

TEST(setFlat, nittyGrittyGraveContinuation) {
    NittyGrittySet s(16);

    for (int i{24}; i < 32; ++i) {
        s.emplace(i, i, 24);
    }
    for (int i{0}; i < 8; ++i) {
        s.emplace(i, i, 24);
    }

    for (int i{24}; i < 32; ++i) {
        s.erase(NittyGrittyVal{i, i, 24});
    }
    for (int i{0}; i < 7; ++i) {
        s.erase(NittyGrittyVal{i, i, 24});
    }

    EXPECT_EQ(1u, s.size());
    EXPECT_EQ(16u, s.capacity());
    for (int i{24}; i < 32; ++i) {
        EXPECT_TRUE(QcHashMapFriend::isGrave(s, i));
    }
    for (int i{0}; i < 7; ++i) {
        EXPECT_TRUE(QcHashMapFriend::isGrave(s, i));
    }

    auto it{s.cbegin()};
    EXPECT_EQ((NittyGrittyVal{7, 7, 24}), *it);
    ++it;
    EXPECT_EQ(s.cend(), it);

    it = s.find(NittyGrittyVal{7, 7, 24});
    EXPECT_NE(s.cend(), it);
    EXPECT_EQ((NittyGrittyVal{7, 7, 24}), *it);

    EXPECT_TRUE(s.erase(NittyGrittyVal{7, 7, 24}));
    EXPECT_TRUE(QcHashMapFriend::isGrave(s, 7));
}

TEST(setFlat, singleElementInitializerList) {
    qc_hash_flat::Set<int> s{100};
    EXPECT_EQ(1u, s.size());
    EXPECT_EQ(qc_hash_flat::config::minCapacity, s.capacity());
    EXPECT_EQ(100, *s.cbegin());
}

TEST(setFlat, noPreemtiveRehash) {
    qc_hash_flat::Set<int> s{};
    for (int i{0}; i < int(qc_hash_flat::config::minCapacity) - 1; ++i) s.insert(i);
    EXPECT_EQ(qc_hash_flat::config::minCapacity, s.capacity());
    s.emplace(int(qc_hash_flat::config::minCapacity - 1));
    EXPECT_EQ(qc_hash_flat::config::minCapacity, s.capacity());
    s.emplace(int(qc_hash_flat::config::minCapacity - 1));
    EXPECT_EQ(qc_hash_flat::config::minCapacity, s.capacity());
}

struct SetDistStats {
    size_t min, max, median;
    double mean, stdDev;
    std::map<size_t, size_t> histo;
};

template <typename V, typename H>
SetDistStats calcStats(const qc_hash_flat::Set<V, H> & set) {
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

TEST(setFlat, stats) {
    constexpr size_t size{8192};

    qc_hash_flat::Set<int> s(size);
    for (int i{0}; i < size; ++i) {
        s.insert(i);
    }

    const SetDistStats stats{calcStats(s)};
    EXPECT_EQ(0, stats.median);
    EXPECT_NEAR(0.5, stats.mean, 0.1);
    EXPECT_NEAR(1.5, stats.stdDev, 0.1);
}

TEST(setFlat, terminator) {
    qc_hash_flat::Set<int> s{};
    s.insert(0);

    for (int i{0}; i < 4; ++i) {
        const auto it{s.end()};
        const u8 & endControl{QcHashMapFriend::getControl(it)};
        const u64 * endControlBlock{reinterpret_cast<const u64 *>(&endControl)};
        EXPECT_EQ(0u, size_t(endControlBlock) &7u);
        EXPECT_EQ(~u64(0u), *endControlBlock);

        s.rehash(2u * s.slot_count());
    }
}

template <typename K, typename T> void testStaticMemory() {
    static constexpr size_t capacity{128u};
    static constexpr size_t slotCount{capacity * 2u};

    MemRecordSet<K> s(capacity);
    s.emplace(K{});
    EXPECT_EQ(sizeof(size_t) * 4u, sizeof(qc_hash_flat::Set<K>));
    EXPECT_EQ(slotCount * (1u + sizeof(K)) + 8u, s.get_allocator().stats().current);

    MemRecordMap<K, T> m(capacity);
    m.emplace(K{}, T{});
    EXPECT_EQ(sizeof(size_t) * 4u, sizeof(qc_hash_flat::Map<K, T>));
    EXPECT_EQ(slotCount * (1u + sizeof(std::pair<K, T>)) + 8u, m.get_allocator().stats().current);
}

TEST(setFlat, staticMemory) {
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

TEST(setFlat, dynamicMemory) {
    MemRecordSet<int> s(1024u);
    const size_t slotSize{1u + sizeof(int)};

    size_t current{0u}, total{0u}, allocations{0u}, deallocations{0u};
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.rehash(64u);
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    for (int i{0}; i < 32; ++i) s.emplace(i);
    current = 64u * slotSize + 8u;
    total += current;
    ++allocations;
    EXPECT_EQ(size_t(64u), s.slot_count());
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.emplace(64);
    current = 128u * slotSize + 8u;
    total += current;
    ++allocations;
    ++deallocations;
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.clear();
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.rehash(1024u);
    current = 1024u * slotSize + 8u;
    total += current;
    ++allocations;
    ++deallocations;
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    for (int i{0}; i < 128; ++i) s.emplace(i);
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.emplace(128);
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.erase(128);
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    while (!s.empty()) s.erase(s.begin());
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);
}

TEST(setFlat, mapGeneral) {
    TrackedMap m{100};

    Tracked::resetTotals();
    for (int i{0}; i < 25; ++i) {
        const auto [it, inserted]{m.emplace(Tracked{TrackedVal{i}}, TrackedVal{i + 100})};
        EXPECT_TRUE(inserted);
        EXPECT_EQ(i, int(it->first.val));
        EXPECT_EQ(i + 100, int(it->second.val));
    }
    EXPECT_EQ(25, Tracked::totalStats.moveConstructs);
    EXPECT_EQ(25, Tracked::totalStats.destructs);
    EXPECT_EQ(50, Tracked::totalStats.all());

    Tracked::resetTotals();
    for (int i{25}; i < 50; ++i) {
        const auto [it, inserted]{m.emplace(std::piecewise_construct, std::forward_as_tuple(TrackedVal{i}), std::forward_as_tuple(TrackedVal{i + 100}))};
        EXPECT_TRUE(inserted);
        EXPECT_EQ(i, int(it->first.val));
        EXPECT_EQ(i + 100, int(it->second.val));
    }
    EXPECT_EQ(25, Tracked::totalStats.moveConstructs);
    EXPECT_EQ(25, Tracked::totalStats.destructs);
    EXPECT_EQ(50, Tracked::totalStats.all());

    Tracked::resetTotals();
    for (int i{50}; i < 75; ++i) {
        const auto [it, inserted]{m.try_emplace(Tracked{TrackedVal{i}}, TrackedVal{i + 100})};
        EXPECT_TRUE(inserted);
        EXPECT_EQ(i, int(it->first.val));
        EXPECT_EQ(i + 100, int(it->second.val));
    }
    EXPECT_EQ(25, Tracked::totalStats.moveConstructs);
    EXPECT_EQ(25, Tracked::totalStats.destructs);
    EXPECT_EQ(50, Tracked::totalStats.all());

    Tracked::resetTotals();
    for (int i{50}; i < 75; ++i) {
        const auto [it, inserted]{m.try_emplace(Tracked{TrackedVal{i}}, TrackedVal{i + 100})};
        EXPECT_FALSE(inserted);
        EXPECT_EQ(i, int(it->first.val));
    }
    EXPECT_EQ(0, Tracked::totalStats.moveConstructs);
    EXPECT_EQ(25, Tracked::totalStats.destructs);
    EXPECT_EQ(25, Tracked::totalStats.all());

    Tracked::resetTotals();
    for (int i{75}; i < 100; ++i) {
        const auto [it, inserted]{m.insert(std::pair<Tracked, Tracked>{Tracked{TrackedVal{i}}, Tracked{TrackedVal{i + 100}}})};
        EXPECT_TRUE(inserted);
        EXPECT_EQ(i, int(it->first.val));
        EXPECT_EQ(i + 100, int(it->second.val));
    }
    EXPECT_EQ(100, Tracked::totalStats.moveConstructs);
    EXPECT_EQ(100, Tracked::totalStats.destructs);
    EXPECT_EQ(200, Tracked::totalStats.all());

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
