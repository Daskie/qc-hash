#include <unordered_map>
#include <vector>

#include "CppUnitTest.h"

#include <qc-core/memory.hpp>

#include "qc-map.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

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

TEST_CLASS(Set) {

    public:

    struct Tracker {
        static int &  defConstructs() { static int  defConstructs(0); return  defConstructs; }
        static int &  valConstructs() { static int  valConstructs(0); return  valConstructs; }
        static int & copyConstructs() { static int copyConstructs(0); return copyConstructs; }
        static int & moveConstructs() { static int moveConstructs(0); return moveConstructs; }
        static int &    copyAssigns() { static int    copyAssigns(0); return    copyAssigns; }
        static int &    moveAssigns() { static int    moveAssigns(0); return    moveAssigns; }
        static int &      destructs() { static int      destructs(0); return      destructs; }

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
        Tracker & operator=(const Tracker & other) { i = other.i; ++copyAssigns(); }
        Tracker & operator=(Tracker && other) noexcept { i = other.i; ++moveAssigns(); return *this; }
        ~Tracker() { ++destructs(); }
        friend bool operator==(const Tracker & t1, const Tracker & t2) { return t1.i == t2.i; }
    };

    TEST_METHOD(DefaultConstructor) {
        qc::hash::Set<int> s;
        Assert::AreEqual(qc::hash::config::minCapacity, s.capacity());
        Assert::AreEqual(size_t(0u), s.size());
    }

    TEST_METHOD(CapacityConstructor) {
        Assert::AreEqual(size_t(  16u), qc::hash::Set<int>(   0u).capacity());
        Assert::AreEqual(size_t(  16u), qc::hash::Set<int>(   1u).capacity());
        Assert::AreEqual(size_t(  16u), qc::hash::Set<int>(  16u).capacity());
        Assert::AreEqual(size_t(  32u), qc::hash::Set<int>(  17u).capacity());
        Assert::AreEqual(size_t(  32u), qc::hash::Set<int>(  32u).capacity());
        Assert::AreEqual(size_t(  64u), qc::hash::Set<int>(  33u).capacity());
        Assert::AreEqual(size_t(  64u), qc::hash::Set<int>(  64u).capacity());
        Assert::AreEqual(size_t(1024u), qc::hash::Set<int>(1000u).capacity());
    }

    TEST_METHOD(CopyConstructor) {
        qc::hash::Set<int> s1;
        for (int i(0); i < 128; ++i) s1.emplace(i);
        qc::hash::Set<int> s2(s1);
        Assert::IsTrue(s2 == s1);
    }

    TEST_METHOD(MoveConstructor) {
        qc::hash::Set<int> s1;
        for (int i(0); i < 128; ++i) s1.emplace(i);
        qc::hash::Set<int> ref(s1);
        qc::hash::Set<int> s2(std::move(s1));
        Assert::IsTrue(s2 == ref);
        Assert::IsTrue(s1.empty());
    }

    TEST_METHOD(RangeConstructor) {
        std::vector<int> values{
             0,  1,  2,  3,  4,
             5,  6,  7,  8,  9,
            10, 11, 12, 13, 14,
            15, 16, 17, 18, 19
        };
        qc::hash::Set<int> s(values.cbegin(), values.cend());
        Assert::AreEqual(size_t(20u), s.size());
        Assert::AreEqual(size_t(32u), s.capacity());
        for (int i(0); i < 20; ++i) {
            Assert::IsTrue(s.contains(i));
        }
    }

    TEST_METHOD(InitializerListConstructor) {
        qc::hash::Set<int> s({
             0,  1,  2,  3,  4,
             5,  6,  7,  8,  9,
            10, 11, 12, 13, 14,
            15, 16, 17, 18, 19
        });
        Assert::AreEqual(size_t(20u), s.size());
        Assert::AreEqual(size_t(32u), s.capacity());
        for (int i(0); i < 20; ++i) {
            Assert::IsTrue(s.contains(i));
        }
    }

    TEST_METHOD(CopyAssignment) {
        qc::hash::Set<int> s1;
        for (int i(0); i < 128; ++i) s1.emplace(i);
        qc::hash::Set<int> s2;
        s2 = s1;
        Assert::IsTrue(s2 == s1);

        qc::hash::Set<std::string> s3;
        for (int i(0); i < 128; ++i) s3.emplace(std::to_string(i));
        qc::hash::Set<std::string> s4;
        s4 = s3;
        Assert::IsTrue(s4 == s3);
    }

    TEST_METHOD(MoveAssignment) {
        qc::hash::Set<int> s1;
        for (int i(0); i < 128; ++i) s1.emplace(i);
        qc::hash::Set<int> ref(s1);
        qc::hash::Set<int> s2;
        s2 = std::move(s1);
        Assert::IsTrue(ref == s2);
        Assert::IsTrue(s1.empty());
    }

    TEST_METHOD(ValuesAssignment) {
        qc::hash::Set<int> s; s = { 0, 1, 2, 3, 4, 5 };
        Assert::AreEqual(size_t(6u), s.size());
        Assert::AreEqual(qc::hash::config::minCapacity, s.capacity());
        for (int i(0); i < 6; ++i) {
            Assert::IsTrue(s.count(i));
        }
    }

    TEST_METHOD(Clear) {
        qc::hash::Set<int> s1;
        for (int i(0); i < 128; ++i) s1.emplace(i);
        Assert::AreEqual(size_t(128u), s1.size());
        Assert::AreEqual(size_t(128u), s1.capacity());
        s1.clear();
        Assert::AreEqual(size_t(0u), s1.size());
        Assert::AreEqual(size_t(128u), s1.capacity());

        qc::hash::Set<std::string> s2;
        for (int i(0); i < 128; ++i) s2.emplace(std::to_string(i));
        Assert::AreEqual(size_t(128u), s2.size());
        Assert::AreEqual(size_t(128u), s2.capacity());
        s2.clear();
        Assert::AreEqual(size_t(0u), s2.size());
        Assert::AreEqual(size_t(128u), s2.capacity());
    }

    TEST_METHOD(InsertLRef) {
        qc::hash::Set<int> s;
        for (int i(0); i < 128; ++i) {
            auto res1(s.insert(i));
            Assert::IsTrue(res1.first != s.end());
            Assert::AreEqual(i, *res1.first);
            Assert::IsTrue(res1.second);
            auto res2(s.insert(i));
            Assert::IsTrue(res2.first == res1.first);
            Assert::IsFalse(res2.second);
        }
        Assert::AreEqual(size_t(128u), s.size());
    }

    TEST_METHOD(InsertRRef) {
        qc::hash::Set<std::string> s;
        std::string value("value");
        auto res(s.insert(std::move(value)));
        Assert::IsTrue(res.first != s.end());
        Assert::AreEqual(std::string("value"), *res.first);
        Assert::IsTrue(res.second);
        Assert::IsTrue(value.empty());
    }

    TEST_METHOD(InsertRange) {
        qc::hash::Set<int> s;
        std::vector<int> values;
        for (int i(0); i < 128; ++i) values.push_back(i);
        s.insert(values.cbegin(), values.cend());
        Assert::AreEqual(size_t(128u), s.size());
        for (int i(0); i < 128; ++i) {
            Assert::IsTrue(s.contains(i));
        }
    }

    TEST_METHOD(InsertValues) {
        qc::hash::Set<int> s;
        s.insert({ 0, 1, 2, 3, 4, 5 });
        Assert::AreEqual(size_t(6u), s.size());
        for (int i(0); i < 6; ++i) {
            Assert::IsTrue(s.contains(i));
        }
    }

    TEST_METHOD(Emplace) {
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
        for (int i(0); i < 128; ++i) {
            auto [it, res](s.emplace(i));
            Assert::IsTrue(it != s.cend());
            Assert::IsTrue(res);
        }
        for (int i(0); i < 128; ++i) {
            Assert::IsTrue(s.contains(A(i)));
        }
    }

    TEST_METHOD(TryEmplace) {
        Tracker::reset();

        qc::hash::Map<Tracker, Tracker> m(64u);
        Assert::AreEqual(0, Tracker::total());
        m.try_emplace(Tracker(0), 0);
        Assert::AreEqual(4, Tracker::total());
        Assert::AreEqual(2, Tracker::valConstructs());
        Assert::AreEqual(1, Tracker::moveConstructs());
        Assert::AreEqual(1, Tracker::destructs());
        m.try_emplace(Tracker(0), 1);
        Assert::AreEqual(6, Tracker::total());
        Assert::AreEqual(3, Tracker::valConstructs());
        Assert::AreEqual(1, Tracker::moveConstructs());
        Assert::AreEqual(2, Tracker::destructs());
        Assert::AreEqual(0, m[Tracker(0)].i);
    }

    TEST_METHOD(EraseValue) {
        qc::hash::Set<int> s;
        for (int i(0); i < 128; ++i) {
            s.emplace(i);
        }
        Assert::AreEqual(size_t(128u), s.capacity());
        Assert::IsFalse(s.erase(128));
        int i(0);
        for (int j(0); j < 95; ++j, ++i) {
            Assert::IsTrue(s.erase(i));
            Assert::AreEqual(size_t(128u - i - 1u), s.size());
            Assert::AreEqual(size_t(128u), s.capacity());
        }
        for (int j(0); j < 16; ++j, ++i) {
            Assert::IsTrue(s.erase(i));
            Assert::AreEqual(size_t(128u - i - 1u), s.size());
            Assert::AreEqual(size_t(64u), s.capacity());
        }
        for (int j(0); j < 8; ++j, ++i) {
            Assert::IsTrue(s.erase(i));
            Assert::AreEqual(size_t(128u - i - 1u), s.size());
            Assert::AreEqual(size_t(32u), s.capacity());
        }
        for (int j(0); j < 9; ++j, ++i) {
            Assert::IsTrue(s.erase(i));
            Assert::AreEqual(size_t(128u - i - 1u), s.size());
            Assert::AreEqual(size_t(16u), s.capacity());
        }
        Assert::IsTrue(s.empty());

        s.reserve(1024u);
        s.insert({ 1, 2, 3, 4, 5, 6, 7 });
        Assert::AreEqual(size_t(1024u), s.capacity());
        s.erase(0);
        Assert::AreEqual(size_t(1024u), s.capacity());
        s.erase(1);
        Assert::AreEqual(size_t(512u), s.capacity());
        s.erase(2);
        Assert::AreEqual(size_t(256u), s.capacity());
        s.erase(3);
        Assert::AreEqual(size_t(128u), s.capacity());
        s.erase(4);
        Assert::AreEqual(size_t(64u), s.capacity());
        s.erase(5);
        Assert::AreEqual(size_t(32u), s.capacity());
        s.erase(6);
        Assert::AreEqual(size_t(16u), s.capacity());
        s.erase(7);
        Assert::AreEqual(size_t(16u), s.capacity());
    }

    TEST_METHOD(EraseIterator) {
        qc::hash::Set<int> s;
        for (int i(0); i < 128; ++i) {
            s.emplace(i);
        }
        Assert::AreEqual(size_t(128), s.capacity());
        Assert::IsTrue(s.erase(s.cend()) == s.end());
        int i(0);
        for (int j(0); j < 95; ++j, ++i) {
            auto it(s.erase(s.find(i)));
            Assert::IsTrue(s.end() == it);
            Assert::AreEqual(size_t(128u - i - 1u), s.size());
            Assert::AreEqual(size_t(128u), s.capacity());
        }
        for (int j(0); j < 16; ++j, ++i) {
            auto it(s.erase(s.find(i)));
            Assert::IsTrue(s.end() == it);
            Assert::AreEqual(size_t(128u - i - 1u), s.size());
            Assert::AreEqual(size_t(64u), s.capacity());
        }
        for (int j(0); j < 8; ++j, ++i) {
            auto it(s.erase(s.find(i)));
            Assert::IsTrue(s.end() == it);
            Assert::AreEqual(size_t(128u - i - 1u), s.size());
            Assert::AreEqual(size_t(32u), s.capacity());
        }
        for (int j(0); j < 9; ++j, ++i) {
            auto it(s.erase(s.find(i)));
            Assert::IsTrue(s.end() == it);
            Assert::AreEqual(size_t(128u - i - 1u), s.size());
            Assert::AreEqual(size_t(16u), s.capacity());
        }
        Assert::IsTrue(s.empty());

        s.reserve(1024u);
        s.emplace(0);
        Assert::AreEqual(size_t(1024u), s.capacity());
        s.erase(s.cend());
        Assert::AreEqual(size_t(1024u), s.capacity());
        s.erase(s.cbegin());
        Assert::AreEqual(size_t(512u), s.capacity());
    }

    TEST_METHOD(EraseRange) {
        qc::hash::Set<int, qc::hash::IdentityHash<int>> s;
        for (int i(0); i < 128; ++i) {
            s.emplace(i);
        }
        Assert::AreEqual(size_t(128u), s.size());
        Assert::AreEqual(size_t(128u), s.capacity());
        auto it(s.erase(s.end(), s.cend()));
        Assert::IsTrue(it == s.end());
        Assert::AreEqual(size_t(128u), s.size());
        Assert::AreEqual(size_t(128u), s.capacity());
        it = s.begin();
        for (int i(0); i < 48; ++i, ++it);
        it = s.erase(s.begin(), it);
        Assert::IsTrue(it == s.end());
        it = s.begin();
        for (int i(0); i < 32; ++i, ++it);
        it = s.erase(it, s.end());
        Assert::IsTrue(it == s.end());
        Assert::AreEqual(size_t(32u), s.size());
        Assert::AreEqual(size_t(32u), s.capacity());
        for (int i(0); i < 32; ++i) {
            Assert::IsTrue(s.contains(48 + i));
        }
        it = s.erase(s.cbegin(), s.cend());
        Assert::IsTrue(it == s.end());
        Assert::IsTrue(s.empty());
        Assert::AreEqual(qc::hash::config::minCapacity, s.capacity());

        s.reserve(1024u);
        s.emplace(0);
        Assert::AreEqual(size_t(1024u), s.capacity());
        s.erase(s.cbegin(), s.cbegin());
        Assert::AreEqual(size_t(1024u), s.capacity());
        s.erase(s.cbegin(), s.cend());
        Assert::AreEqual(size_t(16u), s.capacity());
    }

    TEST_METHOD(Access) {
        qc::hash::Map<int, int> m;
        for (int i(0); i < 100; ++i) {
            m[i] = i;
        }
        for (int i(0); i < 100; ++i) {
            Assert::AreEqual(i, m[i]);
        }
        m.clear();
        for (int i(0); i < 100; ++i) {
            m[i];
        }
        for (int i(0); i < 100; ++i) {
            Assert::AreEqual(0, m[i]);
        }
    }

    TEST_METHOD(Find) {
        qc::hash::Set<int> s;
        for (int i(0); i < 128; ++i) {
            s.emplace(i);
        }
        for (int i(0); i < 128; ++i) {
            auto it(s.find(i));
            Assert::AreEqual(i, *it);
        }
        Assert::IsTrue(s.find(128) == s.end());
    }

    TEST_METHOD(Swap) {
        qc::hash::Set<int> s1{ 1, 2, 3 };
        qc::hash::Set<int> s2{ 4, 5, 6 };
        qc::hash::Set<int> s3(s1);
        qc::hash::Set<int> s4(s2);
        Assert::IsTrue(s3 == s1);
        Assert::IsTrue(s4 == s2);
        s3.swap(s4);
        Assert::IsTrue(s3 == s2);
        Assert::IsTrue(s4 == s1);
        std::swap(s3, s4);
        Assert::IsTrue(s3 == s1);
        Assert::IsTrue(s4 == s2);

        auto it1(s1.cbegin());
        auto it2(s2.cbegin());
        auto it3(it1);
        auto it4(it2);
        Assert::IsTrue(it3 == it1);
        Assert::IsTrue(it4 == it2);
        std::swap(it1, it2);
        Assert::IsTrue(it4 == it1);
        Assert::IsTrue(it3 == it2);
    }

    TEST_METHOD(NoPreemtiveRehash) {
        qc::hash::Set<int> s;
        for (int i(0); i < qc::hash::config::minCapacity - 1; ++i) s.emplace(i);
        Assert::AreEqual(qc::hash::config::minCapacity, s.capacity());
        s.emplace(int(qc::hash::config::minCapacity - 1));
        Assert::AreEqual(qc::hash::config::minCapacity, s.capacity());
        s.emplace(int(qc::hash::config::minCapacity - 1));
        Assert::AreEqual(qc::hash::config::minCapacity, s.capacity());
    }

    TEST_METHOD(Rehash) {
        qc::hash::Set<int> s;
        Assert::AreEqual(qc::hash::config::minBucketCount, s.bucket_count());
        s.rehash(0u);
        Assert::AreEqual(qc::hash::config::minBucketCount, s.bucket_count());
        s.rehash(1u);
        Assert::AreEqual(qc::hash::config::minBucketCount, s.bucket_count());
        for (int i(0); i < 16; ++i) {
            s.emplace(i);
        }
        Assert::AreEqual(size_t(32u), s.bucket_count());
        s.emplace(16);
        Assert::AreEqual(size_t(64u), s.bucket_count());
        for (int i(17); i < 128; ++i) {
            s.emplace(i);
        }
        Assert::AreEqual(size_t(256u), s.bucket_count());
        s.rehash(500u);
        Assert::AreEqual(size_t(512u), s.bucket_count());
        Assert::AreEqual(size_t(128u), s.size());
        for (int i = 0; i < 128; ++i) {
            Assert::IsTrue(s.contains(i));
        }
        s.rehash(10u);
        Assert::AreEqual(size_t(256u), s.bucket_count());
        for (int i = 0; i < 128; ++i) {
            Assert::IsTrue(s.contains(i));
        }
        s.clear();
        Assert::AreEqual(size_t(256u), s.bucket_count());
        s.rehash(0u);
        Assert::AreEqual(qc::hash::config::minBucketCount, s.bucket_count());
    }

    TEST_METHOD(Equality) {
        qc::hash::Set<int> s1, s2, s3;
        for (int i(0); i < 128; ++i) {
            s1.emplace(i);
            s3.emplace(i + 128);
        }
        s2 = s1;
        Assert::IsTrue(s2 == s1);
        Assert::IsTrue(s3 != s1);
    }

    TEST_METHOD(Iterator) {
        struct A {
            int x;
            A(int x) : x(x) {}
            bool operator==(const A & other) const { return x == other.x; }
        };

        qc::hash::Set<A, qc::hash::IdentityHash<A>> s;
        for (int i(0); i < 128; ++i) {
            s.emplace(i);
        }
        int i(0);
        for (auto it(s.begin()); it != s.end(); ++it) {
            Assert::AreEqual(i, it->x);
            Assert::AreEqual(it->x, (*it).x);
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

    TEST_METHOD(ForEachLoop) {
        qc::hash::Set<int, qc::hash::IdentityHash<int>> s;
        for (int i(0); i < 128; ++i) {
            s.emplace(i);
        }
        int i(0);
        for (const auto & v : s) {
            Assert::AreEqual(i, v);
            ++i;
        }
    }

    TEST_METHOD(Circuity) {
        qc::hash::Set<int, qc::hash::IdentityHash<int>> s(128u);
        for (int i(0); i < 32; ++i) {
            s.emplace(224 + i * 256);
        }
        for (int i(32); i < 64; ++i) {
            s.emplace(224 + i * 256);
        }
        for (int i(0); i < 32; ++i) {
            s.emplace(192 + i * 256);
        }
        for (int i(32); i < 64; ++i) {
            s.emplace(192 + i * 256);
        }
        Assert::AreEqual(size_t(256u), s.bucket_count());
        Assert::AreEqual(size_t(64u), s.bucket_size(224u));
        Assert::AreEqual(size_t(64u), s.bucket_size(192u));
        auto it(s.begin());
        for (int i(0); it != s.end() && i < 32; ++it, ++i) {
            Assert::AreEqual(224 + (32 + i) * 256, *it);
        }
        for (int i(0); it != s.end() && i < 32; ++it, ++i) {
            Assert::AreEqual(224 + i * 256, *it);
        }
        for (int i(0); it != s.end() && i < 32; ++it, ++i) {
            Assert::AreEqual(192 + i * 256, *it);
        }
        for (int i(0); it != s.end() && i < 32; ++it, ++i) {
            Assert::AreEqual(192 + (32 + i) * 256, *it);
        }

        for (int i(0); i < 32; ++i) {
            s.erase(224 + i * 256);
        }
        for (int i(0); i < 32; ++i) {
            s.erase(192 + i * 256);
        }
        Assert::AreEqual(size_t(32u), s.bucket_size(224u));
        Assert::AreEqual(size_t(32u), s.bucket_size(192u));
        it = s.begin();
        for (int i(0); it != s.end() && i < 32; ++it, ++i) {
            Assert::AreEqual(192 + (i + 32) * 256, *it);
        }
        for (int i(0); it != s.end() && i < 32; ++it, ++i) {
            Assert::AreEqual(224 + (i + 32) * 256, *it);
        }
    }

    TEST_METHOD(Reordering) {
        qc::hash::Set<int, qc::hash::IdentityHash<int>> s(128u);
        for (int i(0); i < 128; ++i) {
            s.emplace(i * 256);
        }
        int j(0);
        for (const auto & v : s) {
            Assert::AreEqual(j * 256, v);
            ++j;
        }

        for (int i(0); i < 64; ++i) {
            auto it(s.begin());
            int v(*it);
            s.erase(it);
            s.emplace(v);
        }
        Assert::AreEqual(size_t(256u), s.bucket_count());
        Assert::AreEqual(size_t(128u), s.size());
        auto it(s.begin());
        for (int i(64); it != s.end() && i < 128; ++it, ++i) {
            Assert::AreEqual(i * 256, *it);
        }
        for (int i(0); it != s.end() && i < 64; ++it, ++i) {
            Assert::AreEqual(i * 256, *it);
        }
    }

    struct SetStats {
        size_t min, max, median;
        double mean, stddev;
        std::unordered_map<size_t, size_t> histo;
    };

    template <typename V, typename H>
    typename SetStats calcStats(const qc::hash::Set<V, H> & set) {
        size_t min(~size_t(0u));
        size_t max(0u);

        std::unordered_map<size_t, size_t> histo;
        size_t total(0u);
        for (size_t i(0u); i < set.bucket_count(); ++i) {
            size_t size(set.bucket_size(i));
            ++histo[size];
            if (size < min) min = size;
            else if (size > max) max = size;
            total += size;
        }

        double mean(double(total) / double(set.bucket_count()));

        double stddev(0.0);
        for (size_t i(0u); i < set.bucket_count(); ++i) {
            double diff(double(set.bucket_size(i)) - mean);
            stddev += diff * diff;
        }
        stddev /= double(set.bucket_count());
        stddev = std::sqrt(stddev);

        size_t median(0u);
        size_t medianVal(0u);
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

    TEST_METHOD(Stats) {
        constexpr int size(8192);

        qc::hash::Set<int, qc::hash::IdentityHash<int>> s1(size);
        qc::hash::Set<int, qc::hash::Hash<int>> s2(size);
        for (int i(0); i < size; ++i) {
            s1.emplace(i);
            s2.emplace(i);
        }

        SetStats stats1(calcStats(s1));
        Assert::AreEqual(size_t(size), stats1.histo.at(0));
        Assert::AreEqual(size_t(size), stats1.histo.at(1));
        Assert::AreEqual(0.5, stats1.mean, 1.0e-6);
        Assert::AreEqual(0.5, stats1.stddev, 1.0e-6);

        SetStats stats2(calcStats(s2));
        Assert::AreEqual(0.5, stats2.mean, 1.0e-6);
        Assert::AreEqual(0.7, stats2.stddev, 0.1);
    }

    TEST_METHOD(Terminator) {
        struct Entry {
            int val;
            unsigned int dist;
        };

        qc::hash::Set<int> s;
        s.insert(0);
        for (int i(0); i < 5; ++i) {
            Assert::AreEqual(std::numeric_limits<unsigned int>::max(), reinterpret_cast<const Entry *>(reinterpret_cast<const size_t &>(s.end()))->dist);
            s.rehash(2u * s.bucket_count());
        }
    }

    template <typename K, typename T> using RecordMap = qc::hash::Map<K, T, qc::hash::Hash<K>, std::equal_to<K>, qc::core::RecordAllocator<std::conditional_t<std::is_same_v<T, void>, K, std::pair<K, T>>>>;
    template <typename K> using RecordSet = qc::hash::Set<K, qc::hash::Hash<K>, std::equal_to<K>, qc::core::RecordAllocator<K>>;

    TEST_METHOD(Memory) {
        Assert::AreEqual(size_t(sizeof(size_t) * 4u), sizeof(qc::hash::Set<int>));

        size_t bucketSize(sizeof(int) * 2u);
        RecordSet<int> s(1024u);

        size_t current(0u), total(0u), allocations(0u), deallocations(0u);
        Assert::AreEqual(current, s.get_allocator().current());
        Assert::AreEqual(total, s.get_allocator().total());
        Assert::AreEqual(allocations, s.get_allocator().allocations());
        Assert::AreEqual(deallocations, s.get_allocator().deallocations());

        s.rehash(64u);
        Assert::AreEqual(current, s.get_allocator().current());
        Assert::AreEqual(total, s.get_allocator().total());
        Assert::AreEqual(allocations, s.get_allocator().allocations());
        Assert::AreEqual(deallocations, s.get_allocator().deallocations());

        for (int i(0); i < 32; ++i) s.emplace(i);
        current = (64u + 1u) * bucketSize;
        total += current;
        ++allocations;
        Assert::AreEqual(size_t(64u), s.bucket_count());
        Assert::AreEqual(current, s.get_allocator().current());
        Assert::AreEqual(total, s.get_allocator().total());
        Assert::AreEqual(allocations, s.get_allocator().allocations());
        Assert::AreEqual(deallocations, s.get_allocator().deallocations());

        s.emplace(64);
        current = (128u + 1u) * bucketSize;
        total += current;
        ++allocations;
        ++deallocations;
        Assert::AreEqual(current, s.get_allocator().current());
        Assert::AreEqual(total, s.get_allocator().total());
        Assert::AreEqual(allocations, s.get_allocator().allocations());
        Assert::AreEqual(deallocations, s.get_allocator().deallocations());

        s.clear();
        Assert::AreEqual(current, s.get_allocator().current());
        Assert::AreEqual(total, s.get_allocator().total());
        Assert::AreEqual(allocations, s.get_allocator().allocations());
        Assert::AreEqual(deallocations, s.get_allocator().deallocations());

        s.rehash(1024u);
        current = (1024u + 1u) * bucketSize;
        total += current;
        ++allocations;
        ++deallocations;
        Assert::AreEqual(current, s.get_allocator().current());
        Assert::AreEqual(total, s.get_allocator().total());
        Assert::AreEqual(allocations, s.get_allocator().allocations());
        Assert::AreEqual(deallocations, s.get_allocator().deallocations());

        s.emplace(0);
        Assert::AreEqual(current, s.get_allocator().current());
        Assert::AreEqual(total, s.get_allocator().total());
        Assert::AreEqual(allocations, s.get_allocator().allocations());
        Assert::AreEqual(deallocations, s.get_allocator().deallocations());

        s.erase(s.cbegin(), s.cend());
        current = (32u + 1u) * bucketSize;
        total += current;
        ++allocations;
        ++deallocations;
        Assert::AreEqual(current, s.get_allocator().current());
        Assert::AreEqual(total, s.get_allocator().total());
        Assert::AreEqual(allocations, s.get_allocator().allocations());
        Assert::AreEqual(deallocations, s.get_allocator().deallocations());
    }

    template <typename K, typename T>
    int memoryUsagePer() {
        RecordMap<K, T> m({{}});
        return int(m.get_allocator().current() / (m.bucket_count() + 1u));
    }

    TEST_METHOD(BucketSize) {
        struct s24 {
            char _1, _2, _3;
            bool operator==(const s24 & other) const { return _1 == other._1 && _2 == other._2 && _3 == other._3; }
        };

        Assert::AreEqual( 2, memoryUsagePer<s08, void>());
        Assert::AreEqual( 4, memoryUsagePer<s16, void>());
        Assert::AreEqual( 8, memoryUsagePer<s32, void>());
        Assert::AreEqual(16, memoryUsagePer<s64, void>());
        Assert::AreEqual( 4, memoryUsagePer<s24, void>());

        Assert::AreEqual( 3, memoryUsagePer<s08, s08>());
        Assert::AreEqual( 4, memoryUsagePer<s08, s16>());
        Assert::AreEqual( 8, memoryUsagePer<s08, s32>());
        Assert::AreEqual(16, memoryUsagePer<s08, s64>());
        Assert::AreEqual( 5, memoryUsagePer<s08, s24>());

        Assert::AreEqual( 4, memoryUsagePer<s16, s08>());
        Assert::AreEqual( 6, memoryUsagePer<s16, s16>());
        Assert::AreEqual( 8, memoryUsagePer<s16, s32>());
        Assert::AreEqual(16, memoryUsagePer<s16, s64>());
        Assert::AreEqual( 6, memoryUsagePer<s16, s24>());

        Assert::AreEqual( 8, memoryUsagePer<s32, s08>());
        Assert::AreEqual( 8, memoryUsagePer<s32, s16>());
        Assert::AreEqual(12, memoryUsagePer<s32, s32>());
        Assert::AreEqual(16, memoryUsagePer<s32, s64>());
        Assert::AreEqual( 8, memoryUsagePer<s32, s24>());

        Assert::AreEqual(16, memoryUsagePer<s64, s08>());
        Assert::AreEqual(16, memoryUsagePer<s64, s16>());
        Assert::AreEqual(16, memoryUsagePer<s64, s32>());
        Assert::AreEqual(24, memoryUsagePer<s64, s64>());
        Assert::AreEqual(16, memoryUsagePer<s64, s24>());

        Assert::AreEqual( 5, memoryUsagePer<s24, s08>());
        Assert::AreEqual( 6, memoryUsagePer<s24, s16>());
        Assert::AreEqual( 8, memoryUsagePer<s24, s32>());
        Assert::AreEqual(16, memoryUsagePer<s24, s64>());
        Assert::AreEqual( 7, memoryUsagePer<s24, s24>());
    }

    template <typename T, typename K>
    std::pair<int, int> bucketAndDistSizes() {
        using Types = qc::hash::_Types<T, K>;
        return { int(sizeof(Types::Bucket)), int(sizeof(Types::Dist)) };
    }

    TEST_METHOD(BucketStruct) {
        struct s24 {
            char _1, _2, _3;
            bool operator==(const s24 & other) const { return _1 == other._1 && _2 == other._2 && _3 == other._3; }
        };

        Assert::IsTrue(std::pair<int, int>( 2, 1) == bucketAndDistSizes<u08, void>());
        Assert::IsTrue(std::pair<int, int>( 4, 2) == bucketAndDistSizes<u16, void>());
        Assert::IsTrue(std::pair<int, int>( 8, 4) == bucketAndDistSizes<u32, void>());
        Assert::IsTrue(std::pair<int, int>(16, 8) == bucketAndDistSizes<u64, void>());
        Assert::IsTrue(std::pair<int, int>( 4, 1) == bucketAndDistSizes<s24, void>());

        Assert::IsTrue(std::pair<int, int>( 3, 1) == bucketAndDistSizes<u08, u08>());
        Assert::IsTrue(std::pair<int, int>( 4, 1) == bucketAndDistSizes<u08, u16>());
        Assert::IsTrue(std::pair<int, int>( 8, 2) == bucketAndDistSizes<u08, u32>());
        Assert::IsTrue(std::pair<int, int>(16, 4) == bucketAndDistSizes<u08, u64>());
        Assert::IsTrue(std::pair<int, int>( 5, 1) == bucketAndDistSizes<u08, s24>());

        Assert::IsTrue(std::pair<int, int>( 4, 1) == bucketAndDistSizes<u16, u08>());
        Assert::IsTrue(std::pair<int, int>( 6, 2) == bucketAndDistSizes<u16, u16>());
        Assert::IsTrue(std::pair<int, int>( 8, 2) == bucketAndDistSizes<u16, u32>());
        Assert::IsTrue(std::pair<int, int>(16, 4) == bucketAndDistSizes<u16, u64>());
        Assert::IsTrue(std::pair<int, int>( 6, 1) == bucketAndDistSizes<u16, s24>());

        Assert::IsTrue(std::pair<int, int>( 8, 2) == bucketAndDistSizes<u32, u08>());
        Assert::IsTrue(std::pair<int, int>( 8, 2) == bucketAndDistSizes<u32, u16>());
        Assert::IsTrue(std::pair<int, int>(12, 4) == bucketAndDistSizes<u32, u32>());
        Assert::IsTrue(std::pair<int, int>(16, 4) == bucketAndDistSizes<u32, u64>());
        Assert::IsTrue(std::pair<int, int>( 8, 1) == bucketAndDistSizes<u32, s24>());

        Assert::IsTrue(std::pair<int, int>(16, 4) == bucketAndDistSizes<u64, u08>());
        Assert::IsTrue(std::pair<int, int>(16, 4) == bucketAndDistSizes<u64, u16>());
        Assert::IsTrue(std::pair<int, int>(16, 4) == bucketAndDistSizes<u64, u32>());
        Assert::IsTrue(std::pair<int, int>(24, 8) == bucketAndDistSizes<u64, u64>());
        Assert::IsTrue(std::pair<int, int>(16, 4) == bucketAndDistSizes<u64, s24>());

        Assert::IsTrue(std::pair<int, int>( 5, 1) == bucketAndDistSizes<s24, u08>());
        Assert::IsTrue(std::pair<int, int>( 6, 1) == bucketAndDistSizes<s24, u16>());
        Assert::IsTrue(std::pair<int, int>( 8, 1) == bucketAndDistSizes<s24, u32>());
        Assert::IsTrue(std::pair<int, int>(16, 4) == bucketAndDistSizes<s24, u64>());
        Assert::IsTrue(std::pair<int, int>( 7, 1) == bucketAndDistSizes<s24, s24>());
    }

    TEST_METHOD(Sensitivity) {
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

    TEST_METHOD(CopyAversion) {
        Tracker::reset();

        qc::hash::Map<Tracker, Tracker> m;
        Assert::IsFalse(Tracker::copies());
        for (int i(0); i < 100; ++i) {
            m.emplace(i, i);
        }
        Assert::IsFalse(Tracker::copies());
        qc::hash::Map<Tracker, Tracker> m2(std::move(m));
        Assert::IsFalse(Tracker::copies());
        m = std::move(m2);
        Assert::IsFalse(Tracker::copies());
        m.erase(m.cbegin(), m.cend());
        Assert::IsFalse(Tracker::copies());
    }

    TEST_METHOD(SetAsMap) {
        qc::hash::Set<int> s;
        // These should all fail to compile with error about not being for sets
        //s.at(0);
        //s[0];
        //s.emplace(0, 0);
        //s.emplace(std::piecewise_construct, std::make_tuple(0), std::make_tuple(0));
        //s.try_emplace(0, 0);
    }

};
