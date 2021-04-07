// First include in order to test its own includes
#include <qc-hash/qc-map.hpp>

#include <cmath>

#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

#include <qc-core/core.hpp>
#include <qc-core/memory.hpp>

using namespace qc::types;

struct Tracker {
    static int &  defConstructs() { static int  defConstructs{0}; return  defConstructs; }
    static int &  valConstructs() { static int  valConstructs{0}; return  valConstructs; }
    static int & copyConstructs() { static int copyConstructs{0}; return copyConstructs; }
    static int & moveConstructs() { static int moveConstructs{0}; return moveConstructs; }
    static int &    copyAssigns() { static int    copyAssigns{0}; return    copyAssigns; }
    static int &    moveAssigns() { static int    moveAssigns{0}; return    moveAssigns; }
    static int &      destructs() { static int      destructs{0}; return      destructs; }

    static int constructs() { return defConstructs() + valConstructs() + copyConstructs() + moveConstructs(); }
    static int assigns() { return copyAssigns() + moveAssigns(); }
    static int copies() { return copyConstructs() + copyAssigns(); }
    static int moves() { return moveConstructs() + moveAssigns(); }
    static int total() { return constructs() + assigns() + destructs(); }

    static void reset() { defConstructs() = valConstructs() = copyConstructs() = moveConstructs() = copyAssigns() = moveAssigns() = destructs() = 0; }

    int i;
    Tracker(int i) : i(i) {
        ++valConstructs();
    }
    Tracker() : i() { ++defConstructs(); }
    Tracker(const Tracker & other) : i(other.i) { ++copyConstructs(); }
    Tracker(Tracker && other) noexcept : i(other.i) {
        ++moveConstructs();
    }
    Tracker & operator=(const Tracker & other) { i = other.i; ++copyAssigns(); return *this; }
    Tracker & operator=(Tracker && other) noexcept { i = other.i; ++moveAssigns(); return *this; }
    ~Tracker() { ++destructs(); }
    friend bool operator==(const Tracker & t1, const Tracker & t2) { return t1.i == t2.i; }
};

struct TrackerHash {
    size_t operator()(const Tracker & tracker) const {
        return size_t(tracker.i);
    }
};

TEST(set, defaultConstructor) {
    qc_hash::Set<int> s;
    EXPECT_EQ(qc_hash::config::minCapacity, s.capacity());
    EXPECT_EQ(size_t(0u), s.size());
}

TEST(set, capacityConstructor) {
    EXPECT_EQ(size_t(  16u), qc_hash::Set<int>(   0u).capacity());
    EXPECT_EQ(size_t(  16u), qc_hash::Set<int>(   1u).capacity());
    EXPECT_EQ(size_t(  16u), qc_hash::Set<int>(  16u).capacity());
    EXPECT_EQ(size_t(  32u), qc_hash::Set<int>(  17u).capacity());
    EXPECT_EQ(size_t(  32u), qc_hash::Set<int>(  32u).capacity());
    EXPECT_EQ(size_t(  64u), qc_hash::Set<int>(  33u).capacity());
    EXPECT_EQ(size_t(  64u), qc_hash::Set<int>(  64u).capacity());
    EXPECT_EQ(size_t(1024u), qc_hash::Set<int>(1000u).capacity());
}

TEST(set, copyConstructor) {
    qc_hash::Set<int> s1{};
    for (int i{0}; i < 128; ++i) s1.emplace(i);
    qc_hash::Set<int> s2{s1};
    EXPECT_EQ(s1, s2);
}

TEST(set, moveConstructor) {
    qc_hash::Set<int> s1;
    for (int i{0}; i < 128; ++i) s1.emplace(i);
    qc_hash::Set<int> ref(s1);
    qc_hash::Set<int> s2(std::move(s1));
    EXPECT_EQ(ref, s2);
    EXPECT_TRUE(s1.empty());
}

TEST(set, rangeConstructor) {
    std::vector<int> values{
         0,  1,  2,  3,  4,
         5,  6,  7,  8,  9,
        10, 11, 12, 13, 14,
        15, 16, 17, 18, 19
    };
    qc_hash::Set<int> s(values.cbegin(), values.cend());
    EXPECT_EQ(size_t(20u), s.size());
    EXPECT_EQ(size_t(32u), s.capacity());
    for (int i{0}; i < 20; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
}

TEST(set, initializerListConstructor) {
    qc_hash::Set<int> s({
         0,  1,  2,  3,  4,
         5,  6,  7,  8,  9,
        10, 11, 12, 13, 14,
        15, 16, 17, 18, 19
    });
    EXPECT_EQ(size_t(20u), s.size());
    EXPECT_EQ(size_t(32u), s.capacity());
    for (int i{0}; i < 20; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
}

TEST(set, copyAssignment) {
    qc_hash::Set<int> s1;
    for (int i{0}; i < 128; ++i) s1.emplace(i);
    qc_hash::Set<int> s2;
    s2 = s1;
    EXPECT_EQ(s1, s2);

    qc_hash::Set<std::string> s3{};
    for (int i{0}; i < 128; ++i) s3.emplace(std::to_string(i));
    qc_hash::Set<std::string> s4{};
    s4 = s3;
    EXPECT_EQ(s3, s4);

    s1 = s1;
    EXPECT_EQ(s1, s1);

    qc_hash::Set<int> s5;
    s1 = s5;
    EXPECT_EQ(s5, s1);
}

TEST(set, moveAssignment) {
    qc_hash::Set<int> s1;
    for (int i{0}; i < 128; ++i) s1.emplace(i);
    qc_hash::Set<int> ref(s1);
    qc_hash::Set<int> s2;
    s2 = std::move(s1);
    EXPECT_EQ(ref, s2);
    EXPECT_TRUE(s1.empty());

    s1 = std::move(s1);
    EXPECT_EQ(s1, s1);

    qc_hash::Set<int> s3;
    s2 = std::move(s3);
    EXPECT_EQ(s3, s2);
}

TEST(set, valuesAssignment) {
    qc_hash::Set<int> s; s = { 0, 1, 2, 3, 4, 5 };
    EXPECT_EQ(size_t(6u), s.size());
    EXPECT_EQ(qc_hash::config::minCapacity, s.capacity());
    for (int i{0}; i < 6; ++i) {
        EXPECT_TRUE(s.count(i));
    }
}

TEST(set, clear) {
    qc_hash::Set<int> s1;
    for (int i{0}; i < 128; ++i) s1.emplace(i);
    EXPECT_EQ(size_t(128u), s1.size());
    EXPECT_EQ(size_t(128u), s1.capacity());
    s1.clear();
    EXPECT_EQ(size_t(0u), s1.size());
    EXPECT_EQ(size_t(128u), s1.capacity());

    qc_hash::Set<std::string> s2;
    for (int i{0}; i < 128; ++i) s2.emplace(std::to_string(i));
    EXPECT_EQ(size_t(128u), s2.size());
    EXPECT_EQ(size_t(128u), s2.capacity());
    s2.clear();
    EXPECT_EQ(size_t(0u), s2.size());
    EXPECT_EQ(size_t(128u), s2.capacity());
}

TEST(set, insertLRef) {
    qc_hash::Set<int> s;
    for (int i{0}; i < 128; ++i) {
        auto res1(s.insert(i));
        EXPECT_NE(s.end(), res1.first);
        EXPECT_EQ(i, *res1.first);
        EXPECT_TRUE(res1.second);
        auto res2(s.insert(i));
        EXPECT_EQ(res1.first, res2.first);
        EXPECT_FALSE(res2.second);
    }
    EXPECT_EQ(size_t(128u), s.size());
}

TEST(set, insertRRef) {
    qc_hash::Set<std::string> s;
    std::string value("value");
    auto res(s.insert(std::move(value)));
    EXPECT_NE(s.end(), res.first);
    EXPECT_EQ(std::string("value"), *res.first);
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(value.empty());
}

TEST(set, insertRange) {
    qc_hash::Set<int> s;
    std::vector<int> values;
    for (int i{0}; i < 128; ++i) values.push_back(i);
    s.insert(values.cbegin(), values.cend());
    EXPECT_EQ(size_t(128u), s.size());
    for (int i{0}; i < 128; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
}

TEST(set, insertValues) {
    qc_hash::Set<int> s;
    s.insert({ 0, 1, 2, 3, 4, 5 });
    EXPECT_EQ(size_t(6u), s.size());
    for (int i{0}; i < 6; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
}

TEST(set, emplace) {
    struct Uncopiable {
        int x;
        Uncopiable(int x) : x(x) {}
        Uncopiable(const Uncopiable & other) = delete;
        Uncopiable(Uncopiable && other) noexcept : x(other.x) { other.x = 0; }
        ~Uncopiable() { x = 0; }
        Uncopiable & operator=(const Uncopiable & other) = delete;
        Uncopiable & operator=(Uncopiable && other) noexcept { x = other.x; other.x = 0; return *this; }
        bool operator==(const Uncopiable & other) const { return x == other.x; }
    };

    struct UncopiableHash {
        size_t operator()(const Uncopiable & a) const {
            return size_t(a.x);
        }
    };

    qc_hash::Set<Uncopiable, UncopiableHash> s;
    for (int i{0}; i < 128; ++i) {
        auto [it, res](s.emplace(i));
        EXPECT_NE(s.cend(), it);
        EXPECT_TRUE(res);
    }
    for (int i{0}; i < 128; ++i) {
        EXPECT_TRUE(s.contains(Uncopiable(i)));
    }
}

TEST(set, tryEmplace) {
    Tracker::reset();

    qc_hash::Map<Tracker, Tracker, TrackerHash> m(64u);
    EXPECT_EQ(0, Tracker::total());
    m.try_emplace(Tracker(0), 0);
    EXPECT_EQ(4, Tracker::total());
    EXPECT_EQ(2, Tracker::valConstructs());
    EXPECT_EQ(1, Tracker::moveConstructs());
    EXPECT_EQ(1, Tracker::destructs());
    m.try_emplace(Tracker(0), 1);
    EXPECT_EQ(6, Tracker::total());
    EXPECT_EQ(3, Tracker::valConstructs());
    EXPECT_EQ(1, Tracker::moveConstructs());
    EXPECT_EQ(2, Tracker::destructs());
    EXPECT_EQ(0, m[Tracker(0)].i);
}

TEST(set, eraseValue) {
    qc_hash::Set<int> s;
    for (int i{0}; i < 128; ++i) {
        s.emplace(i);
    }
    EXPECT_EQ(size_t(128u), s.capacity());
    EXPECT_FALSE(s.erase(128));
    int i{0};
    for (int j{0}; j < 95; ++j, ++i) {
        EXPECT_TRUE(s.erase(i));
        EXPECT_EQ(size_t(128u - i - 1u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());
    }
    for (int j{0}; j < 16; ++j, ++i) {
        EXPECT_TRUE(s.erase(i));
        EXPECT_EQ(size_t(128u - i - 1u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());
    }
    for (int j{0}; j < 8; ++j, ++i) {
        EXPECT_TRUE(s.erase(i));
        EXPECT_EQ(size_t(128u - i - 1u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());
    }
    for (int j{0}; j < 9; ++j, ++i) {
        EXPECT_TRUE(s.erase(i));
        EXPECT_EQ(size_t(128u - i - 1u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());
    }
    EXPECT_TRUE(s.empty());

    s.reserve(1024u);
    for (int i{0}; i <= 128; ++i) s.insert(i);
    EXPECT_EQ(size_t(1024u), s.capacity());
    s.erase(128);
    EXPECT_EQ(size_t(1024u), s.capacity());
}

TEST(set, eraseIterator) {
    qc_hash::Set<int> s;
    for (int i{0}; i < 128; ++i) {
        s.emplace(i);
    }
    EXPECT_EQ(size_t(128), s.capacity());
    int i{0};
    for (int j{0}; j < 95; ++j, ++i) {
        s.erase(s.find(i));
        EXPECT_EQ(size_t(128u - i - 1u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());
    }
    for (int j{0}; j < 16; ++j, ++i) {
        s.erase(s.find(i));
        EXPECT_EQ(size_t(128u - i - 1u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());
    }
    for (int j{0}; j < 8; ++j, ++i) {
        s.erase(s.find(i));
        EXPECT_EQ(size_t(128u - i - 1u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());
    }
    for (int j{0}; j < 9; ++j, ++i) {
        s.erase(s.find(i));
        EXPECT_EQ(size_t(128u - i - 1u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());
    }
    EXPECT_TRUE(s.empty());

    s.reserve(1024u);
    s.emplace(0);
    EXPECT_EQ(size_t(1024u), s.capacity());
    s.erase(s.begin());
    EXPECT_EQ(size_t(1024u), s.capacity());
}

std::vector<std::pair<int, int>> toElementVector(const qc_hash::Set<int> & s) {
    std::vector<std::pair<int, int>> elements;
    elements.reserve(s.size());

    const size_t slotCount{s.capacity() * 2u};
    for (size_t slotI{0u}; slotI < slotCount; ++slotI) {
        std::pair<u8, int> element{s._elementAt(slotI)};
        if (element.first) {
            elements.emplace_back(element.second, element.first);
        }
    }

    return elements;
}

TEST(set, bucketShifts) {
    qc_hash::Set<int> s(16u);
    std::vector<std::pair<int, int>> expected;

    for (int i{0u}; i < 4; ++i) s.insert(i * 32 + 0); // 0, 32, 64, 96
    expected = {{0, 1}, {32, 2}, {64, 3}, {96, 4}};
    EXPECT_EQ(expected, toElementVector(s));

    for (int i{0u}; i < 4; ++i) s.insert(i * 32 + 16); // 16, 48, 80, 112
    expected = {{0, 1}, {32, 2}, {64, 3}, {96, 4}, /****/ {16, 1}, {48, 2}, {80, 3}, {112, 4}};
    EXPECT_EQ(expected, toElementVector(s));

    for (int i{0u}; i < 4; ++i) s.insert(i * 32 + 30); // 30, 62, 94, 126
    expected = {{94, 3}, {126, 4}, /**/ {64, 3}, {96, 4}, {0, 5}, {32, 6}, /****/ {16, 1}, {48, 2}, {80, 3}, {112, 4}, /****/ {30, 1}, {62, 2}};
    EXPECT_EQ(expected, toElementVector(s));

    for (int i{0u}; i < 4; ++i) s.insert(i * 32 + 31); // 31, 63, 95, 127
    expected = {{94, 3}, {126, 4}, /**/ {31, 4}, {63, 5}, {95, 6}, {127, 7}, /**/ {64, 7}, {96, 8}, {0, 9}, {32, 10}, /****/ {16, 1}, {48, 2}, {80, 3}, {112, 4}, /****/ {30, 1}, {62, 2}};
    EXPECT_EQ(expected, toElementVector(s));

    s.erase(30);
    expected = {{94, 3}, /**/ {127, 3}, {31, 4}, {63, 5}, {95, 6}, /**/ {32, 6}, {64, 7}, {96, 8}, {0, 9}, /****/ {16, 1}, {48, 2}, {80, 3}, {112, 4}, /****/ {126, 1}, {62, 2}};
    EXPECT_EQ(expected, toElementVector(s));

    s.erase(62);
    expected = {/**/ {95, 2}, {127, 3}, {31, 4}, {63, 5}, /**/ {0, 5}, {32, 6}, {64, 7}, {96, 8}, /****/ {16, 1}, {48, 2}, {80, 3}, {112, 4}, /****/ {126, 1}, {94, 2}};
    EXPECT_EQ(expected, toElementVector(s));

    s.erase(94);
    expected = {{95, 2}, {127, 3}, {31, 4}, /**/ {96, 4}, {0, 5}, {32, 6}, {64, 7}, /****/ {16, 1}, {48, 2}, {80, 3}, {112, 4}, /****/ {126, 1}, /**/ {63, 1}};
    EXPECT_EQ(expected, toElementVector(s));

    s.erase(126);
    expected = {{95, 2}, {127, 3}, {31, 4}, /**/ {96, 4}, {0, 5}, {32, 6}, {64, 7}, /**/ {16, 1}, {48, 2}, {80, 3}, {112, 4}, /****/ {63, 1}};
    EXPECT_EQ(expected, toElementVector(s));
}

TEST(set, access) {
    qc_hash::Map<int, int> m;
    for (int i{0}; i < 100; ++i) {
        m[i] = i;
    }
    for (int i{0}; i < 100; ++i) {
        EXPECT_EQ(i, m[i]);
    }
    m.clear();
    for (int i{0}; i < 100; ++i) {
        m[i];
    }
    for (int i{0}; i < 100; ++i) {
        EXPECT_EQ(0, m[i]);
    }
}

TEST(set, find) {
    qc_hash::Set<int> s;
    EXPECT_EQ(s.end(), s.find(0));

    for (int i{0}; i < 128; ++i) {
        s.emplace(i);
    }
    for (int i{0}; i < 128; ++i) {
        auto it(s.find(i));
        EXPECT_EQ(i, *it);
    }
    EXPECT_EQ(s.end(), s.find(128));
}

TEST(set, swap) {
    qc_hash::Set<int> s1{ 1, 2, 3 };
    qc_hash::Set<int> s2{ 4, 5, 6 };
    qc_hash::Set<int> s3(s1);
    qc_hash::Set<int> s4(s2);
    EXPECT_EQ(s1, s3);
    EXPECT_EQ(s2, s4);
    s3.swap(s4);
    EXPECT_EQ(s2, s3);
    EXPECT_EQ(s1, s4);
    std::swap(s3, s4);
    EXPECT_EQ(s1, s3);
    EXPECT_EQ(s2, s4);

    auto it1(s1.cbegin());
    auto it2(s2.cbegin());
    auto it3(it1);
    auto it4(it2);
    EXPECT_EQ(it1, it3);
    EXPECT_EQ(it2, it4);
    std::swap(it1, it2);
    EXPECT_EQ(it1, it4);
    EXPECT_EQ(it2, it3);
}

TEST(set, noPreemtiveRehash) {
    qc_hash::Set<int> s;
    for (int i{0}; i < int(qc_hash::config::minCapacity) - 1; ++i) s.emplace(i);
    EXPECT_EQ(qc_hash::config::minCapacity, s.capacity());
    s.emplace(int(qc_hash::config::minCapacity - 1));
    EXPECT_EQ(qc_hash::config::minCapacity, s.capacity());
    s.emplace(int(qc_hash::config::minCapacity - 1));
    EXPECT_EQ(qc_hash::config::minCapacity, s.capacity());
}

TEST(set, rehash) {
    qc_hash::Set<int> s;
    EXPECT_EQ(qc_hash::config::minSlotCount, s.slot_count());
    s.rehash(0u);
    EXPECT_EQ(qc_hash::config::minSlotCount, s.slot_count());
    s.rehash(1u);
    EXPECT_EQ(qc_hash::config::minSlotCount, s.slot_count());
    for (int i{0}; i < 16; ++i) {
        s.emplace(i);
    }
    EXPECT_EQ(size_t(32u), s.slot_count());
    s.emplace(16);
    EXPECT_EQ(size_t(64u), s.slot_count());
    for (int i{17}; i < 128; ++i) {
        s.emplace(i);
    }
    EXPECT_EQ(size_t(256u), s.slot_count());
    s.rehash(500u);
    EXPECT_EQ(size_t(512u), s.slot_count());
    EXPECT_EQ(size_t(128u), s.size());
    for (int i = 0; i < 128; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
    s.rehash(10u);
    EXPECT_EQ(size_t(256u), s.slot_count());
    for (int i = 0; i < 128; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
    s.clear();
    EXPECT_EQ(size_t(256u), s.slot_count());
    s.rehash(0u);
    EXPECT_EQ(qc_hash::config::minSlotCount, s.slot_count());
}

TEST(set, equality) {
    qc_hash::Set<int> s1, s2, s3;
    for (int i{0}; i < 128; ++i) {
        s1.emplace(i);
        s3.emplace(i + 128);
    }
    EXPECT_NE(s1, s2);
    s2 = s1;
    EXPECT_EQ(s1, s2);
    EXPECT_NE(s1, s3);
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
    EXPECT_EQ(size_t(10u), s.bucket_size(59u));
    EXPECT_EQ(size_t(10u), s.bucket_size(54u));

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

    EXPECT_EQ(size_t(5u), s.bucket_size(59u));
    EXPECT_EQ(size_t(5u), s.bucket_size(54u));

    it = s.begin();
    for (int i{0}; it != s.end() && i < 5; ++it, ++i) {
        EXPECT_EQ(54 + (9 - i) * 64, *it);
    }
    for (int i{0}; it != s.end() && i < 5; ++it, ++i) {
        EXPECT_EQ(59 + (5 + i) * 64, *it);
    }
}

struct SetStats {
    size_t min, max, median;
    double mean, stddev;
    std::unordered_map<size_t, size_t> histo;
};

template <typename V, typename H>
SetStats calcStats(const qc_hash::Set<V, H> & set) {
    size_t min{~size_t(0u)};
    size_t max{0u};

    std::unordered_map<size_t, size_t> histo;
    size_t total{0u};
    for (size_t i{0u}; i < set.slot_count(); ++i) {
        size_t size{set.bucket_size(i)};
        ++histo[size];
        if (size < min) min = size;
        else if (size > max) max = size;
        total += size;
    }

    double mean(double(total) / double(set.slot_count()));

    double stddev(0.0);
    for (size_t i{0u}; i < set.slot_count(); ++i) {
        double diff(double(set.bucket_size(i)) - mean);
        stddev += diff * diff;
    }
    stddev /= double(set.slot_count());
    stddev = std::sqrt(stddev);

    size_t median{0u};
    size_t medianVal{0u};
    for (const auto & count : histo) {
        if (count.second > medianVal) {
            median = count.first;
            medianVal = count.second;
        }
    }

    return {
        min, max, median,
        mean, stddev,
        std::move(histo)
    };
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
    struct MurmurHash {
        size_t operator()(const int v) const {
            return qc_hash::murmur3::hash(&v, sizeof(int));
        }
    };

    constexpr int size{8192};

    qc_hash::Set<int> s1(size);
    qc_hash::Set<int, MurmurHash> s2(size);
    for (int i{0}; i < size; ++i) {
        s1.emplace(i);
        s2.emplace(i);
    }

    SetStats stats1(calcStats(s1));
    EXPECT_EQ(size_t(size), stats1.histo.at(0));
    EXPECT_EQ(size_t(size), stats1.histo.at(1));
    EXPECT_NEAR(0.5, stats1.mean, 1.0e-6);
    EXPECT_NEAR(0.5, stats1.stddev, 1.0e-6);

    SetStats stats2(calcStats(s2));
    EXPECT_NEAR(0.5, stats2.mean, 1.0e-6);
    EXPECT_NEAR(0.7, stats2.stddev, 0.1);
}

TEST(set, terminator) {
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
template <typename K> using RecordSet = qc_hash::Set<K, qc_hash::Hash<K>, std::equal_to<K>, qc::RecordAllocator<K>>;

template <typename K, typename T> void testStaticMemory() {
    static constexpr size_t capacity{128u};
    static constexpr size_t slotCount{capacity * 2u};

    RecordSet<K> s(capacity);
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
    RecordSet<int> s(1024u);
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

TEST(set, bucketSize) {
    qc_hash::Set<int> s;
    EXPECT_EQ(0u, s.bucket_size(0u));
    s.insert(0);
    EXPECT_EQ(1u, s.bucket_size(0u));
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
}

TEST(set, asMap) {
    qc_hash::Set<int> s;
    // These should all fail to compile with error about not being for sets
    //s.at(0);
    //s[0];
    //s.emplace(0, 0);
    //s.emplace(std::piecewise_construct, std::make_tuple(0), std::make_tuple(0));
    //s.try_emplace(0, 0);
}

TEST(set, maxDist) {
    qc_hash::Set<int> s(256u);

    for (int i{0}; i < 254; ++i) {
        s.insert(i * 512);
    }

    EXPECT_EQ(254u, s.size());
    EXPECT_EQ(512u, s.slot_count());
    EXPECT_EQ(254u, s.bucket_size(0u));

    auto it{s.cbegin()};
    for (int i{0}; i < 254; ++i, ++it) {
        EXPECT_EQ(i * 512, *it);
    }

    EXPECT_TRUE(s.erase(253 * 512));
    EXPECT_FALSE(s.contains(253 * 512));
    EXPECT_EQ(253u, s.size());
}
