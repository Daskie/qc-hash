#include <cmath>

#include <unordered_map>
#include <vector>

#include <gtest/gtest.h>

#include <qc-core/memory.hpp>

#include <qc-hash/qc-map.hpp>

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

TEST(set, defaultConstructor) {
    qc::hash::Set<int> s;
    EXPECT_EQ(qc::hash::config::minCapacity, s.capacity());
    EXPECT_EQ(size_t(0u), s.size());
}

TEST(set, capacityConstructor) {
    EXPECT_EQ(size_t(  16u), qc::hash::Set<int>(   0u).capacity());
    EXPECT_EQ(size_t(  16u), qc::hash::Set<int>(   1u).capacity());
    EXPECT_EQ(size_t(  16u), qc::hash::Set<int>(  16u).capacity());
    EXPECT_EQ(size_t(  32u), qc::hash::Set<int>(  17u).capacity());
    EXPECT_EQ(size_t(  32u), qc::hash::Set<int>(  32u).capacity());
    EXPECT_EQ(size_t(  64u), qc::hash::Set<int>(  33u).capacity());
    EXPECT_EQ(size_t(  64u), qc::hash::Set<int>(  64u).capacity());
    EXPECT_EQ(size_t(1024u), qc::hash::Set<int>(1000u).capacity());
}

TEST(set, copyConstructor) {
    qc::hash::Set<int> s1;
    for (int i{0}; i < 128; ++i) s1.emplace(i);
    qc::hash::Set<int> s2(s1);
    EXPECT_EQ(s1, s2);
}

TEST(set, moveConstructor) {
    qc::hash::Set<int> s1;
    for (int i{0}; i < 128; ++i) s1.emplace(i);
    qc::hash::Set<int> ref(s1);
    qc::hash::Set<int> s2(std::move(s1));
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
    qc::hash::Set<int> s(values.cbegin(), values.cend());
    EXPECT_EQ(size_t(20u), s.size());
    EXPECT_EQ(size_t(32u), s.capacity());
    for (int i{0}; i < 20; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
}

TEST(set, initializerListConstructor) {
    qc::hash::Set<int> s({
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
    qc::hash::Set<int> s1;
    for (int i{0}; i < 128; ++i) s1.emplace(i);
    qc::hash::Set<int> s2;
    s2 = s1;
    EXPECT_EQ(s1, s2);

    qc::hash::Set<std::string> s3;
    for (int i{0}; i < 128; ++i) s3.emplace(std::to_string(i));
    qc::hash::Set<std::string> s4;
    s4 = s3;
    EXPECT_EQ(s3, s4);
}

TEST(set, moveAssignment) {
    qc::hash::Set<int> s1;
    for (int i{0}; i < 128; ++i) s1.emplace(i);
    qc::hash::Set<int> ref(s1);
    qc::hash::Set<int> s2;
    s2 = std::move(s1);
    EXPECT_EQ(ref, s2);
    EXPECT_TRUE(s1.empty());
}

TEST(set, valuesAssignment) {
    qc::hash::Set<int> s; s = { 0, 1, 2, 3, 4, 5 };
    EXPECT_EQ(size_t(6u), s.size());
    EXPECT_EQ(qc::hash::config::minCapacity, s.capacity());
    for (int i{0}; i < 6; ++i) {
        EXPECT_TRUE(s.count(i));
    }
}

TEST(set, clear) {
    qc::hash::Set<int> s1;
    for (int i{0}; i < 128; ++i) s1.emplace(i);
    EXPECT_EQ(size_t(128u), s1.size());
    EXPECT_EQ(size_t(128u), s1.capacity());
    s1.clear();
    EXPECT_EQ(size_t(0u), s1.size());
    EXPECT_EQ(size_t(128u), s1.capacity());

    qc::hash::Set<std::string> s2;
    for (int i{0}; i < 128; ++i) s2.emplace(std::to_string(i));
    EXPECT_EQ(size_t(128u), s2.size());
    EXPECT_EQ(size_t(128u), s2.capacity());
    s2.clear();
    EXPECT_EQ(size_t(0u), s2.size());
    EXPECT_EQ(size_t(128u), s2.capacity());
}

TEST(set, insertLRef) {
    qc::hash::Set<int> s;
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
    qc::hash::Set<std::string> s;
    std::string value("value");
    auto res(s.insert(std::move(value)));
    EXPECT_NE(s.end(), res.first);
    EXPECT_EQ(std::string("value"), *res.first);
    EXPECT_TRUE(res.second);
    EXPECT_TRUE(value.empty());
}

TEST(set, insertRange) {
    qc::hash::Set<int> s;
    std::vector<int> values;
    for (int i{0}; i < 128; ++i) values.push_back(i);
    s.insert(values.cbegin(), values.cend());
    EXPECT_EQ(size_t(128u), s.size());
    for (int i{0}; i < 128; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
}

TEST(set, insertValues) {
    qc::hash::Set<int> s;
    s.insert({ 0, 1, 2, 3, 4, 5 });
    EXPECT_EQ(size_t(6u), s.size());
    for (int i{0}; i < 6; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
}

TEST(set, emplace) {
    struct A {
        int x;
        A(int x) : x(x) {}
        A(const A & other) = delete;
        A(A && other) noexcept : x(other.x) { other.x = 0; }
        ~A() { x = 0; }
        A & operator=(const A & other) = delete;
        A & operator=(A && other) noexcept { x = other.x; other.x = 0; return *this; }
        bool operator==(const A & other) const { return x == other.x; }
    };

    qc::hash::Set<A> s;
    for (int i{0}; i < 128; ++i) {
        auto [it, res](s.emplace(i));
        EXPECT_NE(s.cend(), it);
        EXPECT_TRUE(res);
    }
    for (int i{0}; i < 128; ++i) {
        EXPECT_TRUE(s.contains(A(i)));
    }
}

TEST(set, tryEmplace) {
    Tracker::reset();

    qc::hash::Map<Tracker, Tracker> m(64u);
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
    qc::hash::Set<int> s;
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
        EXPECT_EQ(size_t(64u), s.capacity());
    }
    for (int j{0}; j < 8; ++j, ++i) {
        EXPECT_TRUE(s.erase(i));
        EXPECT_EQ(size_t(128u - i - 1u), s.size());
        EXPECT_EQ(size_t(32u), s.capacity());
    }
    for (int j{0}; j < 9; ++j, ++i) {
        EXPECT_TRUE(s.erase(i));
        EXPECT_EQ(size_t(128u - i - 1u), s.size());
        EXPECT_EQ(size_t(16u), s.capacity());
    }
    EXPECT_TRUE(s.empty());

    s.reserve(1024u);
    s.insert({ 1, 2, 3, 4, 5, 6, 7 });
    EXPECT_EQ(size_t(1024u), s.capacity());
    s.erase(0);
    EXPECT_EQ(size_t(1024u), s.capacity());
    s.erase(1);
    EXPECT_EQ(size_t(512u), s.capacity());
    s.erase(2);
    EXPECT_EQ(size_t(256u), s.capacity());
    s.erase(3);
    EXPECT_EQ(size_t(128u), s.capacity());
    s.erase(4);
    EXPECT_EQ(size_t(64u), s.capacity());
    s.erase(5);
    EXPECT_EQ(size_t(32u), s.capacity());
    s.erase(6);
    EXPECT_EQ(size_t(16u), s.capacity());
    s.erase(7);
    EXPECT_EQ(size_t(16u), s.capacity());
}

TEST(set, eraseIterator) {
    qc::hash::Set<int> s;
    for (int i{0}; i < 128; ++i) {
        s.emplace(i);
    }
    EXPECT_EQ(size_t(128), s.capacity());
    EXPECT_EQ(s.end(), s.erase(s.cend()));
    int i{0};
    for (int j{0}; j < 95; ++j, ++i) {
        auto it(s.erase(s.find(i)));
        EXPECT_EQ(s.end(), it);
        EXPECT_EQ(size_t(128u - i - 1u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());
    }
    for (int j{0}; j < 16; ++j, ++i) {
        auto it(s.erase(s.find(i)));
        EXPECT_EQ(s.end(), it);
        EXPECT_EQ(size_t(128u - i - 1u), s.size());
        EXPECT_EQ(size_t(64u), s.capacity());
    }
    for (int j{0}; j < 8; ++j, ++i) {
        auto it(s.erase(s.find(i)));
        EXPECT_EQ(s.end(), it);
        EXPECT_EQ(size_t(128u - i - 1u), s.size());
        EXPECT_EQ(size_t(32u), s.capacity());
    }
    for (int j{0}; j < 9; ++j, ++i) {
        auto it(s.erase(s.find(i)));
        EXPECT_EQ(s.end(), it);
        EXPECT_EQ(size_t(128u - i - 1u), s.size());
        EXPECT_EQ(size_t(16u), s.capacity());
    }
    EXPECT_TRUE(s.empty());

    s.reserve(1024u);
    s.emplace(0);
    EXPECT_EQ(size_t(1024u), s.capacity());
    s.erase(s.cend());
    EXPECT_EQ(size_t(1024u), s.capacity());
    s.erase(s.cbegin());
    EXPECT_EQ(size_t(512u), s.capacity());
}

TEST(set, eraseRange) {
    qc::hash::Set<int, qc::hash::IdentityHash<int>> s;
    for (int i{0}; i < 128; ++i) {
        s.emplace(i);
    }
    EXPECT_EQ(size_t(128u), s.size());
    EXPECT_EQ(size_t(128u), s.capacity());
    auto it(s.erase(s.end(), s.cend()));
    EXPECT_EQ(s.end(), it);
    EXPECT_EQ(size_t(128u), s.size());
    EXPECT_EQ(size_t(128u), s.capacity());
    it = s.begin();
    for (int i{0}; i < 48; ++i, ++it);
    it = s.erase(s.begin(), it);
    EXPECT_EQ(s.end(), it);
    it = s.begin();
    for (int i{0}; i < 32; ++i, ++it);
    it = s.erase(it, s.end());
    EXPECT_EQ(s.end(), it);
    EXPECT_EQ(size_t(32u), s.size());
    EXPECT_EQ(size_t(32u), s.capacity());
    for (int i{0}; i < 32; ++i) {
        EXPECT_TRUE(s.contains(48 + i));
    }
    it = s.erase(s.cbegin(), s.cend());
    EXPECT_EQ(s.end(), it);
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(qc::hash::config::minCapacity, s.capacity());

    s.reserve(1024u);
    s.emplace(0);
    EXPECT_EQ(size_t(1024u), s.capacity());
    s.erase(s.cbegin(), s.cbegin());
    EXPECT_EQ(size_t(1024u), s.capacity());
    s.erase(s.cbegin(), s.cend());
    EXPECT_EQ(size_t(16u), s.capacity());
}

TEST(set, access) {
    qc::hash::Map<int, int> m;
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
    qc::hash::Set<int> s;
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
    qc::hash::Set<int> s1{ 1, 2, 3 };
    qc::hash::Set<int> s2{ 4, 5, 6 };
    qc::hash::Set<int> s3(s1);
    qc::hash::Set<int> s4(s2);
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
    qc::hash::Set<int> s;
    for (int i{0}; i < int(qc::hash::config::minCapacity) - 1; ++i) s.emplace(i);
    EXPECT_EQ(qc::hash::config::minCapacity, s.capacity());
    s.emplace(int(qc::hash::config::minCapacity - 1));
    EXPECT_EQ(qc::hash::config::minCapacity, s.capacity());
    s.emplace(int(qc::hash::config::minCapacity - 1));
    EXPECT_EQ(qc::hash::config::minCapacity, s.capacity());
}

TEST(set, rehash) {
    qc::hash::Set<int> s;
    EXPECT_EQ(qc::hash::config::minBucketCount, s.bucket_count());
    s.rehash(0u);
    EXPECT_EQ(qc::hash::config::minBucketCount, s.bucket_count());
    s.rehash(1u);
    EXPECT_EQ(qc::hash::config::minBucketCount, s.bucket_count());
    for (int i{0}; i < 16; ++i) {
        s.emplace(i);
    }
    EXPECT_EQ(size_t(32u), s.bucket_count());
    s.emplace(16);
    EXPECT_EQ(size_t(64u), s.bucket_count());
    for (int i{17}; i < 128; ++i) {
        s.emplace(i);
    }
    EXPECT_EQ(size_t(256u), s.bucket_count());
    s.rehash(500u);
    EXPECT_EQ(size_t(512u), s.bucket_count());
    EXPECT_EQ(size_t(128u), s.size());
    for (int i = 0; i < 128; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
    s.rehash(10u);
    EXPECT_EQ(size_t(256u), s.bucket_count());
    for (int i = 0; i < 128; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
    s.clear();
    EXPECT_EQ(size_t(256u), s.bucket_count());
    s.rehash(0u);
    EXPECT_EQ(qc::hash::config::minBucketCount, s.bucket_count());
}

TEST(set, equality) {
    qc::hash::Set<int> s1, s2, s3;
    for (int i{0}; i < 128; ++i) {
        s1.emplace(i);
        s3.emplace(i + 128);
    }
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

    qc::hash::Set<A, qc::hash::IdentityHash<A>> s;
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
    qc::hash::Set<int> t;
    qc::hash::Set<int>::iterator it1(t.begin());
    qc::hash::Set<int>::const_iterator cit1 = t.cbegin();
    //it1 = cit1;
    cit1 = it1;
    qc::hash::Set<int>::iterator it2(it1);
    it2 = it1;
    qc::hash::Set<int>::const_iterator cit2(cit1);
    cit2 = cit1;
    qc::hash::Set<int>::iterator it3(std::move(it1));
    it3 = std::move(it1);
    qc::hash::Set<int>::const_iterator cit3(std::move(cit1));
    cit3 = std::move(cit1);
    it1 == cit1;
    cit1 == it1;
}

TEST(set, forEachLoop) {
    qc::hash::Set<int, qc::hash::IdentityHash<int>> s;
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
    qc::hash::Set<int, qc::hash::IdentityHash<int>> s(128u);
    for (int i{0}; i < 32; ++i) {
        s.emplace(224 + i * 256);
    }
    for (int i{32}; i < 64; ++i) {
        s.emplace(224 + i * 256);
    }
    for (int i{0}; i < 32; ++i) {
        s.emplace(192 + i * 256);
    }
    for (int i{32}; i < 64; ++i) {
        s.emplace(192 + i * 256);
    }
    EXPECT_EQ(size_t(256u), s.bucket_count());
    EXPECT_EQ(size_t(64u), s.bucket_size(224u));
    EXPECT_EQ(size_t(64u), s.bucket_size(192u));
    auto it(s.begin());
    for (int i{0}; it != s.end() && i < 32; ++it, ++i) {
        EXPECT_EQ(224 + (32 + i) * 256, *it);
    }
    for (int i{0}; it != s.end() && i < 32; ++it, ++i) {
        EXPECT_EQ(224 + i * 256, *it);
    }
    for (int i{0}; it != s.end() && i < 32; ++it, ++i) {
        EXPECT_EQ(192 + i * 256, *it);
    }
    for (int i{0}; it != s.end() && i < 32; ++it, ++i) {
        EXPECT_EQ(192 + (32 + i) * 256, *it);
    }

    for (int i{0}; i < 32; ++i) {
        s.erase(224 + i * 256);
    }
    for (int i{0}; i < 32; ++i) {
        s.erase(192 + i * 256);
    }
    EXPECT_EQ(size_t(32u), s.bucket_size(224u));
    EXPECT_EQ(size_t(32u), s.bucket_size(192u));
    it = s.begin();
    for (int i{0}; it != s.end() && i < 32; ++it, ++i) {
        EXPECT_EQ(192 + (i + 32) * 256, *it);
    }
    for (int i{0}; it != s.end() && i < 32; ++it, ++i) {
        EXPECT_EQ(224 + (i + 32) * 256, *it);
    }
}

TEST(set, reordering) {
    qc::hash::Set<int, qc::hash::IdentityHash<int>> s(128u);
    for (int i{0}; i < 128; ++i) {
        s.emplace(i * 256);
    }
    int j{0};
    for (const auto & v : s) {
        EXPECT_EQ(j * 256, v);
        ++j;
    }

    for (int i{0}; i < 64; ++i) {
        auto it(s.begin());
        int v{*it};
        s.erase(it);
        s.emplace(v);
    }
    EXPECT_EQ(size_t(256u), s.bucket_count());
    EXPECT_EQ(size_t(128u), s.size());
    auto it(s.begin());
    for (int i{64}; it != s.end() && i < 128; ++it, ++i) {
        EXPECT_EQ(i * 256, *it);
    }
    for (int i{0}; it != s.end() && i < 64; ++it, ++i) {
        EXPECT_EQ(i * 256, *it);
    }
}

struct SetStats {
    size_t min, max, median;
    double mean, stddev;
    std::unordered_map<size_t, size_t> histo;
};

template <typename V, typename H>
SetStats calcStats(const qc::hash::Set<V, H> & set) {
    size_t min{~size_t(0u)};
    size_t max{0u};

    std::unordered_map<size_t, size_t> histo;
    size_t total{0u};
    for (size_t i{0u}; i < set.bucket_count(); ++i) {
        size_t size{set.bucket_size(i)};
        ++histo[size];
        if (size < min) min = size;
        else if (size > max) max = size;
        total += size;
    }

    double mean(double(total) / double(set.bucket_count()));

    double stddev(0.0);
    for (size_t i{0u}; i < set.bucket_count(); ++i) {
        double diff(double(set.bucket_size(i)) - mean);
        stddev += diff * diff;
    }
    stddev /= double(set.bucket_count());
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
    constexpr int size{8192};

    qc::hash::Set<int, qc::hash::IdentityHash<int>> s1(size);
    qc::hash::Set<int, qc::hash::Hash<int>> s2(size);
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

    qc::hash::Set<int> s;
    s.insert(0);
    for (int i{0}; i < 5; ++i) {
        const auto it{s.end()};
        EXPECT_EQ(std::numeric_limits<unsigned int>::max(), reinterpret_cast<const Entry *>(reinterpret_cast<const size_t &>(it))->dist);
        s.rehash(2u * s.bucket_count());
    }
}

template <typename K, typename T> using RecordMap = qc::hash::Map<K, T, qc::hash::Hash<K>, std::equal_to<K>, qc::RecordAllocator<std::conditional_t<std::is_same_v<T, void>, K, std::pair<K, T>>>>;
template <typename K> using RecordSet = qc::hash::Set<K, qc::hash::Hash<K>, std::equal_to<K>, qc::RecordAllocator<K>>;

TEST(set, memory) {
    EXPECT_EQ(size_t(sizeof(size_t) * 4u), sizeof(qc::hash::Set<int>));

    size_t bucketSize{sizeof(int) * 2u};
    RecordSet<int> s(1024u);

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
    current = (64u + 1u) * bucketSize;
    total += current;
    ++allocations;
    EXPECT_EQ(size_t(64u), s.bucket_count());
    EXPECT_EQ(current, s.get_allocator().current());
    EXPECT_EQ(total, s.get_allocator().total());
    EXPECT_EQ(allocations, s.get_allocator().allocations());
    EXPECT_EQ(deallocations, s.get_allocator().deallocations());

    s.emplace(64);
    current = (128u + 1u) * bucketSize;
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
    current = (1024u + 1u) * bucketSize;
    total += current;
    ++allocations;
    ++deallocations;
    EXPECT_EQ(current, s.get_allocator().current());
    EXPECT_EQ(total, s.get_allocator().total());
    EXPECT_EQ(allocations, s.get_allocator().allocations());
    EXPECT_EQ(deallocations, s.get_allocator().deallocations());

    s.emplace(0);
    EXPECT_EQ(current, s.get_allocator().current());
    EXPECT_EQ(total, s.get_allocator().total());
    EXPECT_EQ(allocations, s.get_allocator().allocations());
    EXPECT_EQ(deallocations, s.get_allocator().deallocations());

    s.erase(s.cbegin(), s.cend());
    current = (32u + 1u) * bucketSize;
    total += current;
    ++allocations;
    ++deallocations;
    EXPECT_EQ(current, s.get_allocator().current());
    EXPECT_EQ(total, s.get_allocator().total());
    EXPECT_EQ(allocations, s.get_allocator().allocations());
    EXPECT_EQ(deallocations, s.get_allocator().deallocations());
}

template <typename K, typename T>
int memoryUsagePer() {
    RecordMap<K, T> m({{}});
    return int(m.get_allocator().current() / (m.bucket_count() + 1u));
}

TEST(set, bucketSize) {
    struct s24 {
        char _1, _2, _3;
        bool operator==(const s24 & other) const { return _1 == other._1 && _2 == other._2 && _3 == other._3; }
    };

    EXPECT_EQ( 2, (memoryUsagePer<s08, void>()));
    EXPECT_EQ( 4, (memoryUsagePer<s16, void>()));
    EXPECT_EQ( 8, (memoryUsagePer<s32, void>()));
    EXPECT_EQ(16, (memoryUsagePer<s64, void>()));
    EXPECT_EQ( 4, (memoryUsagePer<s24, void>()));

    EXPECT_EQ( 3, (memoryUsagePer<s08, s08>()));
    EXPECT_EQ( 4, (memoryUsagePer<s08, s16>()));
    EXPECT_EQ( 8, (memoryUsagePer<s08, s32>()));
    EXPECT_EQ(16, (memoryUsagePer<s08, s64>()));
    EXPECT_EQ( 5, (memoryUsagePer<s08, s24>()));

    EXPECT_EQ( 4, (memoryUsagePer<s16, s08>()));
    EXPECT_EQ( 6, (memoryUsagePer<s16, s16>()));
    EXPECT_EQ( 8, (memoryUsagePer<s16, s32>()));
    EXPECT_EQ(16, (memoryUsagePer<s16, s64>()));
    EXPECT_EQ( 6, (memoryUsagePer<s16, s24>()));

    EXPECT_EQ( 8, (memoryUsagePer<s32, s08>()));
    EXPECT_EQ( 8, (memoryUsagePer<s32, s16>()));
    EXPECT_EQ(12, (memoryUsagePer<s32, s32>()));
    EXPECT_EQ(16, (memoryUsagePer<s32, s64>()));
    EXPECT_EQ( 8, (memoryUsagePer<s32, s24>()));

    EXPECT_EQ(16, (memoryUsagePer<s64, s08>()));
    EXPECT_EQ(16, (memoryUsagePer<s64, s16>()));
    EXPECT_EQ(16, (memoryUsagePer<s64, s32>()));
    EXPECT_EQ(24, (memoryUsagePer<s64, s64>()));
    EXPECT_EQ(16, (memoryUsagePer<s64, s24>()));

    EXPECT_EQ( 5, (memoryUsagePer<s24, s08>()));
    EXPECT_EQ( 6, (memoryUsagePer<s24, s16>()));
    EXPECT_EQ( 8, (memoryUsagePer<s24, s32>()));
    EXPECT_EQ(16, (memoryUsagePer<s24, s64>()));
    EXPECT_EQ( 7, (memoryUsagePer<s24, s24>()));
}

template <typename T, typename K>
std::pair<int, int> bucketAndDistSizes() {
    using Types = qc::hash::_Types<T, K>;
    return { int(sizeof(typename Types::Bucket)), int(sizeof(typename Types::Dist)) };
}

TEST(set, bucketStruct) {
    struct s24 {
        char _1, _2, _3;
        bool operator==(const s24 & other) const { return _1 == other._1 && _2 == other._2 && _3 == other._3; }
    };

    EXPECT_EQ((std::pair<int, int>( 2, 1)), (bucketAndDistSizes<u08, void>()));
    EXPECT_EQ((std::pair<int, int>( 4, 2)), (bucketAndDistSizes<u16, void>()));
    EXPECT_EQ((std::pair<int, int>( 8, 4)), (bucketAndDistSizes<u32, void>()));
    EXPECT_EQ((std::pair<int, int>(16, 8)), (bucketAndDistSizes<u64, void>()));
    EXPECT_EQ((std::pair<int, int>( 4, 1)), (bucketAndDistSizes<s24, void>()));

    EXPECT_EQ((std::pair<int, int>( 3, 1)), (bucketAndDistSizes<u08, u08>()));
    EXPECT_EQ((std::pair<int, int>( 4, 1)), (bucketAndDistSizes<u08, u16>()));
    EXPECT_EQ((std::pair<int, int>( 8, 2)), (bucketAndDistSizes<u08, u32>()));
    EXPECT_EQ((std::pair<int, int>(16, 4)), (bucketAndDistSizes<u08, u64>()));
    EXPECT_EQ((std::pair<int, int>( 5, 1)), (bucketAndDistSizes<u08, s24>()));

    EXPECT_EQ((std::pair<int, int>( 4, 1)), (bucketAndDistSizes<u16, u08>()));
    EXPECT_EQ((std::pair<int, int>( 6, 2)), (bucketAndDistSizes<u16, u16>()));
    EXPECT_EQ((std::pair<int, int>( 8, 2)), (bucketAndDistSizes<u16, u32>()));
    EXPECT_EQ((std::pair<int, int>(16, 4)), (bucketAndDistSizes<u16, u64>()));
    EXPECT_EQ((std::pair<int, int>( 6, 1)), (bucketAndDistSizes<u16, s24>()));

    EXPECT_EQ((std::pair<int, int>( 8, 2)), (bucketAndDistSizes<u32, u08>()));
    EXPECT_EQ((std::pair<int, int>( 8, 2)), (bucketAndDistSizes<u32, u16>()));
    EXPECT_EQ((std::pair<int, int>(12, 4)), (bucketAndDistSizes<u32, u32>()));
    EXPECT_EQ((std::pair<int, int>(16, 4)), (bucketAndDistSizes<u32, u64>()));
    EXPECT_EQ((std::pair<int, int>( 8, 1)), (bucketAndDistSizes<u32, s24>()));

    EXPECT_EQ((std::pair<int, int>(16, 4)), (bucketAndDistSizes<u64, u08>()));
    EXPECT_EQ((std::pair<int, int>(16, 4)), (bucketAndDistSizes<u64, u16>()));
    EXPECT_EQ((std::pair<int, int>(16, 4)), (bucketAndDistSizes<u64, u32>()));
    EXPECT_EQ((std::pair<int, int>(24, 8)), (bucketAndDistSizes<u64, u64>()));
    EXPECT_EQ((std::pair<int, int>(16, 4)), (bucketAndDistSizes<u64, s24>()));

    EXPECT_EQ((std::pair<int, int>( 5, 1)), (bucketAndDistSizes<s24, u08>()));
    EXPECT_EQ((std::pair<int, int>( 6, 1)), (bucketAndDistSizes<s24, u16>()));
    EXPECT_EQ((std::pair<int, int>( 8, 1)), (bucketAndDistSizes<s24, u32>()));
    EXPECT_EQ((std::pair<int, int>(16, 4)), (bucketAndDistSizes<s24, u64>()));
    EXPECT_EQ((std::pair<int, int>( 7, 1)), (bucketAndDistSizes<s24, s24>()));
}

TEST(set, sensitivity) {
    struct Sensitive {
        Sensitive() = delete;
        Sensitive(const Sensitive &) = delete;
        Sensitive(Sensitive &&) = default;
        Sensitive & operator=(const Sensitive &) = delete;
        Sensitive & operator=(Sensitive &&) = default;
    };

    qc::hash::Set<Sensitive> s;
    qc::hash::Map<Sensitive, Sensitive> m;
}

TEST(set, copyAversion) {
    Tracker::reset();

    qc::hash::Map<Tracker, Tracker> m;
    EXPECT_FALSE(Tracker::copies());
    for (int i{0}; i < 100; ++i) {
        m.emplace(i, i);
    }
    EXPECT_FALSE(Tracker::copies());
    qc::hash::Map<Tracker, Tracker> m2(std::move(m));
    EXPECT_FALSE(Tracker::copies());
    m = std::move(m2);
    EXPECT_FALSE(Tracker::copies());
    m.erase(m.cbegin(), m.cend());
    EXPECT_FALSE(Tracker::copies());
}

TEST(set, asMap) {
    qc::hash::Set<int> s;
    // These should all fail to compile with error about not being for sets
    //s.at(0);
    //s[0];
    //s.emplace(0, 0);
    //s.emplace(std::piecewise_construct, std::make_tuple(0), std::make_tuple(0));
    //s.try_emplace(0, 0);
}
