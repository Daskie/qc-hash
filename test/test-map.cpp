// First include in order to test its own includes
#include <qc-hash/qc-map.hpp>

#include <cmath>

#include <map>
#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

#include <qc-core/core.hpp>
#include <qc-core/memory.hpp>

using namespace std::string_literals;
using namespace qc::types;

struct QcHashMapFriend {

    template <typename K, typename H, typename KE, typename A>
    static const K & getElement(const qc_hash::Set<K, H, KE, A> & set, const size_t slotI) {
        return set._elements[slotI];
    }

    template <typename K, typename H, typename KE, typename A>
    static bool isPresent(const qc_hash::Set<K, H, KE, A> & set, const size_t slotI) {
        return bool(set._raw(set._elements[slotI])) == bool(slotI);
    }

    template <typename K, typename H, typename KE, typename A>
    static bool isEmpty(const qc_hash::Set<K, H, KE, A> & set, const size_t slotI) {
        return !isPresent(set, slotI);
    }

    template <typename K, typename H, typename KE, typename A, typename It>
    static size_t slotI(const qc_hash::Set<K, H, KE, A> & set, const It it) {
        return it._element - set._elements;
    }

    template <typename K, typename H, typename KE, typename A, typename It>
    static size_t dist(const qc_hash::Set<K, H, KE, A> & set, const It it) {
        const size_t slotI{QcHashMapFriend::slotI(set, it)};
        const size_t idealSlotI{set.slot(*it)};
        return slotI >= idealSlotI ? slotI - idealSlotI : set.slot_count() - idealSlotI + slotI;
    }

};

struct TrackedStats2{
    u8 defConstructs{0u};
    u8 copyConstructs{0u};
    u8 moveConstructs{0u};
    u8 copyAssigns{0u};
    u8 moveAssigns{0u};
    u8 destructs{0u};

    int constructs() const { return defConstructs + copyConstructs + moveConstructs; }
    int assigns() const { return copyAssigns + moveAssigns; }
    int copies() const { return copyConstructs + copyAssigns; }
    int moves() const { return moveConstructs + moveAssigns; }
    int all() const { return constructs() + assigns() + destructs; }

    bool operator==(const TrackedStats2 &) const = default;
};

// For some reason memory allocation goes bananas when this is named `Tracked`?????
// And by bananas, I mean placement new for some reason writes into later bytes
struct Tracked2 {
    inline static TrackedStats2 totalStats{};

    inline static std::unordered_map<const Tracked2 *, TrackedStats2> registry{};

    static void resetTotals() {
        totalStats = {};
    }

    int val{};

    explicit Tracked2(const int val) :
        val{val}
    {
        registry[this] = {};
    }

    Tracked2() {
        registry[this] = {};
        ++registry[this].defConstructs;
        ++totalStats.defConstructs;
    }

    Tracked2(const Tracked2 & other) :
        val{other.val}
    {
        registry[this] = registry[&other];

        ++registry[this].copyConstructs;
        ++totalStats.copyConstructs;
    }

    Tracked2(Tracked2 && other) noexcept :
        val{std::exchange(other.val, {})}
    {
        registry[this] = registry[&other];

        ++registry[this].moveConstructs;
        ++totalStats.moveConstructs;
    }

    Tracked2 & operator=(const Tracked2 & other) {
        val = other.val;
        registry[this] = registry[&other];

        ++registry[this].copyAssigns;
        ++totalStats.copyAssigns;

        return *this;
    }

    Tracked2 & operator=(Tracked2 && other) noexcept {
        val = std::exchange(other.val, 0);
        registry[this] = registry[&other];

        ++registry[this].moveAssigns;
        ++totalStats.moveAssigns;

        return *this;
    }

    ~Tracked2() {
        ++registry[this].destructs;
        ++totalStats.destructs;
    }

    const TrackedStats2 & stats() const {
        return registry[this];
    }

};

static bool operator==(const Tracked2 & t1, const Tracked2 & t2) {
    return t1.val == t2.val;
}

template <>
struct qc_hash::TrivialHash<Tracked2> {
    size_t operator()(const Tracked2 & tracked) const noexcept {
        return size_t(tracked.val);
    }
};

template <typename K> using MemRecordSet = qc_hash::Set<K, qc_hash::TrivialHash<K>, std::equal_to<K>, qc::memory::RecordAllocator<K>>;
template <typename K, typename V> using MemRecordMap = qc_hash::Map<K, V, qc_hash::TrivialHash<K>, std::equal_to<K>, qc::memory::RecordAllocator<std::pair<K, V>>>;

TEST(set, constructor_default) {
    MemRecordSet<int> s{};
    EXPECT_EQ(qc_hash::config::minCapacity, s.capacity());
    EXPECT_EQ(size_t(0u), s.size());
    EXPECT_EQ(0u, s.get_allocator().allocations());
}

TEST(set, constructor_capacity) {
    qc::memory::RecordAllocator<int> allocator{};
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
    // Trivial type
    if (false){
        MemRecordSet<int> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.insert(i);
        }
        const size_t prevAllocCount{s1.get_allocator().allocations()};
        MemRecordSet<int> s2{s1};
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s1, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().allocations());
    }

    // Non-trivial type
    {
        MemRecordSet<Tracked2> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.emplace(i);
        }
        const size_t prevAllocCount{s1.get_allocator().allocations()};
        MemRecordSet<Tracked2> s2{s1};
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s1, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().allocations());
    }
}

TEST(set, constructor_move) {
    // Trivial type
    {
        MemRecordSet<int> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.insert(i);
        }
        const size_t prevAllocCount{s1.get_allocator().allocations()};
        MemRecordSet<int> ref{s1};
        MemRecordSet<int> s2{std::move(s1)};
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(ref, s2);
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc_hash::config::minCapacity, s1.capacity());
        EXPECT_EQ(prevAllocCount, s2.get_allocator().allocations());
    }
    // Non-trivial type
    {
        MemRecordSet<Tracked2> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.emplace(i);
        }
        const size_t prevAllocCount{s1.get_allocator().allocations()};
        MemRecordSet<Tracked2> ref{s1};
        MemRecordSet<Tracked2> s2{std::move(s1)};
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(ref, s2);
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc_hash::config::minCapacity, s1.capacity());
        EXPECT_EQ(prevAllocCount, s2.get_allocator().allocations());
    }
}

TEST(set, assignOperator_initializerList) {
    MemRecordSet<int> s{};
    for (int i{0}; i < 100; ++i) {
        s.insert(i);
    }
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
    // Trivial type
    {
        MemRecordSet<int> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.insert(i);
        }
        const size_t prevAllocCount{s1.get_allocator().allocations()};

        MemRecordSet<int> s2{};
        s2 = s1;
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s1, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().allocations());

        s2 = s2;
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s2, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().allocations());
    }

    // Non-trivial type
    {
        MemRecordSet<Tracked2> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.emplace(i);
        }
        const size_t prevAllocCount{s1.get_allocator().allocations()};

        MemRecordSet<Tracked2> s2{};
        s2 = s1;
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s1, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().allocations());

        s2 = s2;
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s2, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().allocations());
    }
}

TEST(set, assignOperator_move) {
    // Trivial type
    {
        MemRecordSet<int> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.insert(i);
        }
        const size_t prevAllocCount{s1.get_allocator().allocations()};
        MemRecordSet<int> ref{s1};

        MemRecordSet<int> s2{};
        s2 = std::move(s1);
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(ref, s2);
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc_hash::config::minCapacity, s1.capacity());
        EXPECT_EQ(prevAllocCount, s2.get_allocator().allocations());

        s2 = std::move(s2);
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s2, s2);
        EXPECT_EQ(prevAllocCount, s2.get_allocator().allocations());

        s2 = std::move(s1);
        EXPECT_TRUE(s2.empty());
        EXPECT_EQ(qc_hash::config::minCapacity, s2.capacity());
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc_hash::config::minCapacity, s1.capacity());
        EXPECT_EQ(s1, s2);
    }

    // Non-trivial type
    {
        MemRecordSet<Tracked2> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.emplace(i);
        }
        const size_t prevAllocCount{s1.get_allocator().allocations()};
        MemRecordSet<Tracked2> ref{s1};

        MemRecordSet<Tracked2> s2{};
        s2 = std::move(s1);
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(ref, s2);
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc_hash::config::minCapacity, s1.capacity());
        EXPECT_EQ(prevAllocCount, s2.get_allocator().allocations());

        s2 = std::move(s2);
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s2, s2);
        EXPECT_EQ(prevAllocCount, s2.get_allocator().allocations());

        s2 = std::move(s1);
        EXPECT_TRUE(s2.empty());
        EXPECT_EQ(qc_hash::config::minCapacity, s2.capacity());
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc_hash::config::minCapacity, s1.capacity());
        EXPECT_EQ(s1, s2);
    }
}

// Keep parallel with `emplace_lVal` and `tryEmplace_lVal` tests
TEST(set, insert_lVal) {
    Tracked2::resetTotals();
    MemRecordSet<Tracked2> s{};

    for (int i{0}; i < 100; ++i) {
        const Tracked2 val{i};

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
    EXPECT_EQ(100, Tracked2::totalStats.copyConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.destructs);
    EXPECT_EQ(0, Tracked2::totalStats.defConstructs);
    EXPECT_EQ(0, Tracked2::totalStats.assigns());
}

// Keep parallel with `emplace_rVal` and `tryEmplace_rVal` tests
TEST(set, insert_rVal) {
    qc_hash::Set<Tracked2> s{};
    Tracked2 val1{7};
    Tracked2 val2{7};

    const auto [it1, inserted1]{s.insert(std::move(val1))};
    EXPECT_TRUE(inserted1);
    const Tracked2 & tracked{*it1};
    EXPECT_EQ(7, int(tracked.val));
    EXPECT_EQ(1, tracked.stats().moveConstructs);
    EXPECT_EQ(1, tracked.stats().all());
    EXPECT_EQ(0, int(val1.val));

    const auto [it2, inserted2]{s.insert(std::move(val2))};
    EXPECT_FALSE(inserted2);
    EXPECT_EQ(it1, it2);
    EXPECT_EQ(7, int(val2.val));
    EXPECT_EQ(0, val2.stats().all());
}

TEST(set, insert_range) {
    MemRecordSet<Tracked2> s{};
    std::vector<Tracked2> values{};
    for (int i{0}; i < 100; ++i) values.emplace_back(i);

    Tracked2::resetTotals();
    s.insert(values.cbegin(), values.cend());
    EXPECT_EQ(100u, s.size());
    EXPECT_EQ(128u, s.capacity());
    for (int i{0}; i < 100; ++i) {
        EXPECT_TRUE(s.contains(Tracked2{i}));
    }
    EXPECT_EQ(4u, s.get_allocator().allocations());
    EXPECT_EQ(100, Tracked2::totalStats.copyConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.destructs);
    EXPECT_EQ(0, Tracked2::totalStats.defConstructs);
    EXPECT_EQ(0, Tracked2::totalStats.assigns());
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
    Tracked2::resetTotals();
    MemRecordSet<Tracked2> s{};

    for (int i{0}; i < 100; ++i) {
        const Tracked2 val{i};

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
    EXPECT_EQ(100, Tracked2::totalStats.copyConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.destructs);
    EXPECT_EQ(0, Tracked2::totalStats.defConstructs);
    EXPECT_EQ(0, Tracked2::totalStats.assigns());
}

// Keep parallel with `insert_rVal` and `tryEmplace_rVal` tests
TEST(set, emplace_rVal) {
    qc_hash::Set<Tracked2> s{};
    Tracked2 val1{7};
    Tracked2 val2{7};

    const auto [it1, inserted1]{s.emplace(std::move(val1))};
    EXPECT_TRUE(inserted1);
    const Tracked2 & tracked{*it1};
    EXPECT_EQ(7, int(tracked.val));
    EXPECT_EQ(1, tracked.stats().moveConstructs);
    EXPECT_EQ(1, tracked.stats().all());
    EXPECT_EQ(0, int(val1.val));

    const auto [it2, inserted2]{s.emplace(std::move(val2))};
    EXPECT_FALSE(inserted2);
    EXPECT_EQ(it1, it2);
    EXPECT_EQ(7, int(val2.val));
    EXPECT_EQ(0, val2.stats().all());
}

TEST(set, emplace_keyArgs) {
    qc_hash::Set<Tracked2> s{};
    const auto [it, inserted]{s.emplace(7)};
    EXPECT_TRUE(inserted);
    EXPECT_EQ(7, int(it->val));
    EXPECT_EQ(1, it->stats().moveConstructs);
    EXPECT_EQ(1, it->stats().all());
}

// Keep parallel with `insert_lVal` and `emplace_lVal` tests
TEST(set, tryEmplace_lVal) {
    Tracked2::resetTotals();
    MemRecordSet<Tracked2> s{};

    for (int i{0}; i < 100; ++i) {
        const Tracked2 val{i};

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
    EXPECT_EQ(100, Tracked2::totalStats.copyConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.destructs);
    EXPECT_EQ(0, Tracked2::totalStats.defConstructs);
    EXPECT_EQ(0, Tracked2::totalStats.assigns());
}

// Keep parallel with `insert_rVal` and `emplace_rVal` tests
TEST(set, tryEmplace_rVal) {
    qc_hash::Set<Tracked2> s{};
    Tracked2 val1{7};
    Tracked2 val2{7};

    const auto [it1, inserted1]{s.try_emplace(std::move(val1))};
    EXPECT_TRUE(inserted1);
    const Tracked2 & tracked{*it1};
    EXPECT_EQ(7, int(tracked.val));
    EXPECT_EQ(1, tracked.stats().moveConstructs);
    EXPECT_EQ(1, tracked.stats().all());
    EXPECT_EQ(0, int(val1.val));

    const auto [it2, inserted2]{s.try_emplace(std::move(val2))};
    EXPECT_FALSE(inserted2);
    EXPECT_EQ(it1, it2);
    EXPECT_EQ(7, int(val2.val));
    EXPECT_EQ(0, val2.stats().all());
}

TEST(set, eraseKey) {
    qc_hash::Set<Tracked2> s{};

    Tracked2::resetTotals();
    EXPECT_FALSE(s.erase(Tracked2{0}));
    EXPECT_EQ(1, Tracked2::totalStats.destructs);
    EXPECT_EQ(1, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{0}; i < 100; ++i) {
        s.emplace(i);
    }
    EXPECT_EQ(size_t(100u), s.size());
    EXPECT_EQ(size_t(128u), s.capacity());
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.destructs);
    EXPECT_EQ(200 + 32 + 64 + 128, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    EXPECT_FALSE(s.erase(Tracked2{100}));
    EXPECT_EQ(1, Tracked2::totalStats.destructs);
    EXPECT_EQ(1, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{0}; i < 100; ++i) {
        EXPECT_TRUE(s.erase(Tracked2{i}));
        EXPECT_EQ(size_t(100u - i - 1u), s.size());
    }
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(size_t(128u), s.capacity());
    EXPECT_EQ(200, Tracked2::totalStats.destructs);
    EXPECT_EQ(200, Tracked2::totalStats.all());
    Tracked2::resetTotals();
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

        qc_hash::Set<Tracked2> s{};
        for (int i{0}; i < 100; ++i) s.emplace(i);
        EXPECT_EQ(size_t(100u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());

        Tracked2::resetTotals();
        s.clear();
        EXPECT_EQ(size_t(0u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());
        EXPECT_EQ(100, Tracked2::totalStats.destructs);
        EXPECT_EQ(100, Tracked2::totalStats.all());
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

    s.insert(7);
    EXPECT_NE(s.begin(), s.end());
    EXPECT_EQ(int(-1), *s.end());

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
    qc_hash::Set<int> s(128);
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
        EXPECT_EQ(0b10000000'00000000'00000000'00000000u, s.max_slot_count());
    }
    else if constexpr (sizeof(size_t) == 8) {
        EXPECT_EQ(0b10000000'00000000'00000000'00000000'00000000'00000000'00000000'00000000u, s.max_slot_count());
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

TEST(set, iterator) {
    qc_hash::Set<int> s{};
    for (int i{0}; i < 100; ++i) {
        s.insert(i);
    }

    int i{0};
    for (auto it{s.begin()}; it != s.end(); ++it) {
        EXPECT_EQ(i + 1, ++*it);
        EXPECT_EQ(i, --*it);
        ++i;
    }
}

TEST(set, forEachLoop) {
    qc_hash::Set<int> s{};
    for (int i{0}; i < 100; ++i) {
        s.insert(i);
    }

    int i{0};
    for (int & val : s) {
        EXPECT_EQ(i + 1, ++val);
        EXPECT_EQ(i, --val);
        ++i;
    }
}

TEST(set, iteratorConversion) {
    // Just checking for compilation

    qc_hash::Set<int> s{1, 2, 3};

    qc_hash::Set<int>::iterator it1(s.begin());
    qc_hash::Set<int>::const_iterator cit1 = s.cbegin();

    //it1 = cit1; // Should not compile
    static_assert(!std::is_convertible_v<qc_hash::Set<int>::const_iterator, qc_hash::Set<int>::iterator>);
    cit1 = it1;

    ++*it1;
    //++*cit1; // Should not compile
    static_assert(std::is_const_v<std::remove_reference_t<decltype(*cit1)>>);

    qc_hash::Set<int>::iterator it2{it1};
    it2 = it1;
    qc_hash::Set<int>::const_iterator cit2{cit1};
    cit2 = cit1;

    qc_hash::Set<int>::iterator it3{std::move(it1)};
    it3 = std::move(it1);
    qc_hash::Set<int>::const_iterator cit3{std::move(cit1)};
    cit3 = std::move(cit1);

    it1 == cit1;
    cit1 == it1;
}

TEST(set, singleElementInitializerList) {
    qc_hash::Set<int> s{100};
    EXPECT_EQ(1u, s.size());
    EXPECT_EQ(qc_hash::config::minCapacity, s.capacity());
    EXPECT_EQ(100, *s.cbegin());
}

TEST(set, noPreemtiveRehash) {
    qc_hash::Set<int> s{};
    for (int i{0}; i < int(qc_hash::config::minCapacity) - 1; ++i) s.insert(i);
    EXPECT_EQ(qc_hash::config::minCapacity, s.capacity());
    s.emplace(int(qc_hash::config::minCapacity - 1));
    EXPECT_EQ(qc_hash::config::minCapacity, s.capacity());
    s.emplace(int(qc_hash::config::minCapacity - 1));
    EXPECT_EQ(qc_hash::config::minCapacity, s.capacity());
}

struct SetDistStats {
    size_t min, max, median;
    double mean, stdDev;
    std::map<size_t, size_t> histo;
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

TEST(set, stats) {
    constexpr size_t size{8192};

    qc_hash::Set<int> s(size);
    for (int i{0}; i < size; ++i) {
        s.insert(rand());
    }

    const SetDistStats stats{calcStats(s)};
    EXPECT_EQ(0, stats.median);
    EXPECT_NEAR(0.5, stats.mean, 0.1);
    EXPECT_NEAR(1.5, stats.stdDev, 0.1);
}

TEST(set, terminator) {
    qc_hash::Set<int> s{};
    s.insert(0);

    for (int i{0}; i < 4; ++i) {
        const auto it{s.end()};
        EXPECT_EQ(-1, *it);

        s.rehash(2u * s.slot_count());
    }
}

template <typename K, typename T> void testStaticMemory() {
    static constexpr size_t capacity{128u};
    static constexpr size_t slotCount{capacity * 2u};

    MemRecordSet<K> s(capacity);
    s.emplace(K{});
    EXPECT_EQ(sizeof(size_t) * 4u, sizeof(qc_hash::Set<K>));
    EXPECT_EQ((slotCount + 1) * sizeof(K), s.get_allocator().current());

    MemRecordMap<K, T> m(capacity);
    m.emplace(K{}, T{});
    EXPECT_EQ(sizeof(size_t) * 4u, sizeof(qc_hash::Map<K, T>));
    EXPECT_EQ((slotCount + 1) * sizeof(std::pair<K, T>), m.get_allocator().current());
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

    testStaticMemory<Tracked2, Tracked2>();
    testStaticMemory<s16, Tracked2>();
    testStaticMemory<Tracked2, s16>();

    testStaticMemory<s8, std::tuple<s8, s8, s8>>();
}

TEST(set, dynamicMemory) {
    MemRecordSet<int> s(1024u);
    const size_t slotSize{sizeof(int)};

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
    current = 65u * slotSize;
    total += current;
    ++allocations;
    EXPECT_EQ(size_t(64u), s.slot_count());
    EXPECT_EQ(current, s.get_allocator().current());
    EXPECT_EQ(total, s.get_allocator().total());
    EXPECT_EQ(allocations, s.get_allocator().allocations());
    EXPECT_EQ(deallocations, s.get_allocator().deallocations());

    s.emplace(64);
    current = 129u * slotSize;
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
    current = 1025u * slotSize;
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

TEST(set, mapGeneral) {
    qc_hash::Map<Tracked2, Tracked2> m{100};

    Tracked2::resetTotals();
    for (int i{0}; i < 25; ++i) {
        const auto [it, inserted]{m.emplace(Tracked2{i}, i + 100)};
        EXPECT_TRUE(inserted);
        EXPECT_EQ(i, int(it->first.val));
        EXPECT_EQ(i + 100, int(it->second.val));
    }
    EXPECT_EQ(25, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(25, Tracked2::totalStats.destructs);
    EXPECT_EQ(50, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{25}; i < 50; ++i) {
        const auto [it, inserted]{m.emplace(std::piecewise_construct, std::forward_as_tuple(i), std::forward_as_tuple(i + 100))};
        EXPECT_TRUE(inserted);
        EXPECT_EQ(i, int(it->first.val));
        EXPECT_EQ(i + 100, int(it->second.val));
    }
    EXPECT_EQ(25, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(25, Tracked2::totalStats.destructs);
    EXPECT_EQ(50, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{50}; i < 75; ++i) {
        const auto [it, inserted]{m.try_emplace(Tracked2{i}, i + 100)};
        EXPECT_TRUE(inserted);
        EXPECT_EQ(i, int(it->first.val));
        EXPECT_EQ(i + 100, int(it->second.val));
    }
    EXPECT_EQ(25, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(25, Tracked2::totalStats.destructs);
    EXPECT_EQ(50, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{50}; i < 75; ++i) {
        const auto [it, inserted]{m.try_emplace(Tracked2{i}, i + 100)};
        EXPECT_FALSE(inserted);
        EXPECT_EQ(i, int(it->first.val));
    }
    EXPECT_EQ(0, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(25, Tracked2::totalStats.destructs);
    EXPECT_EQ(25, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{75}; i < 100; ++i) {
        const auto [it, inserted]{m.insert(std::pair<Tracked2, Tracked2>{Tracked2{i}, Tracked2{i + 100}})};
        EXPECT_TRUE(inserted);
        EXPECT_EQ(i, int(it->first.val));
        EXPECT_EQ(i + 100, int(it->second.val));
    }
    EXPECT_EQ(100, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(100, Tracked2::totalStats.destructs);
    EXPECT_EQ(200, Tracked2::totalStats.all());

    for (int i{0}; i < 100; ++i) {
        EXPECT_EQ(i + 100, int(m.at(Tracked2{i}).val));
        EXPECT_EQ(i + 100, int(m[Tracked2{i}].val));
    }

    EXPECT_THROW(m.at(Tracked2{100}), std::out_of_range);
    EXPECT_EQ(Tracked2{}, m[Tracked2{100}]);
    m[Tracked2{100}] = Tracked2{200};
    EXPECT_EQ(Tracked2{200}, m[Tracked2{100}]);

    qc_hash::Map<Tracked2, Tracked2> m2{m};
    EXPECT_EQ(m, m2);

    m2[Tracked2{100}].val = 400;
    EXPECT_NE(m, m2);
}
