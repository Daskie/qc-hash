#include <SDKDDKVer.h>
#include <vector>
#include <unordered_map>

#include "CppUnitTest.h"

#include "QHash/Set.hpp"



using namespace Microsoft::VisualStudio::CppUnitTestFramework;



TEST_CLASS(Set) {

public:
        
    TEST_METHOD(DefaultConstructor) {
        qc::Set<int> s;
        Assert::AreEqual(qc::config::set::defInitCapacity * 2, s.bucket_count());
        Assert::AreEqual(size_t(0), s.size());
    }

    TEST_METHOD(CapacityConstructor) {
        Assert::AreEqual(size_t(   8), qc::Set<int>(   0).bucket_count());
        Assert::AreEqual(size_t(   8), qc::Set<int>(   1).bucket_count());
        Assert::AreEqual(size_t(   8), qc::Set<int>(   2).bucket_count());
        Assert::AreEqual(size_t(   8), qc::Set<int>(   4).bucket_count());
        Assert::AreEqual(size_t(  16), qc::Set<int>(   7).bucket_count());
        Assert::AreEqual(size_t(  16), qc::Set<int>(   8).bucket_count());
        Assert::AreEqual(size_t(2048), qc::Set<int>(1000).bucket_count());
    }

    TEST_METHOD(CopyConstructor) {
        qc::Set<int> s1;
        for (int i(0); i < 128; ++i) s1.emplace(i);
        qc::Set<int> s2(s1);
        Assert::IsTrue(s2 == s1);
    }

    TEST_METHOD(MoveConstructor) {
        qc::Set<int> s1;
        for (int i(0); i < 128; ++i) s1.emplace(i);
        qc::Set<int> ref(s1);
        qc::Set<int> s2(std::move(s1));
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
        qc::Set<int> s(values.cbegin(), values.cend());
        Assert::AreEqual(size_t(20), s.size());
        Assert::AreEqual(size_t(64), s.bucket_count());
        for (int i = 0; i < 20; ++i) {
            Assert::IsTrue(s.contains(i));
        }
    }

    TEST_METHOD(InitializerListConstructor) {
        qc::Set<int> s({
             0,  1,  2,  3,  4,
             5,  6,  7,  8,  9,
            10, 11, 12, 13, 14,
            15, 16, 17, 18, 19
        });
        Assert::AreEqual(size_t(20), s.size());
        Assert::AreEqual(size_t(64), s.bucket_count());
        for (int i = 0; i < 20; ++i) {
            Assert::IsTrue(s.contains(i));
        }
    }

    TEST_METHOD(CopyAssignment) {
        qc::Set<int> s1;
        for (int i(0); i < 128; ++i) s1.emplace(i);
        qc::Set<int> s2;
        s2 = s1;
        Assert::IsTrue(s2 == s1);

        qc::Set<std::string> s3;
        for (int i(0); i < 128; ++i) s3.emplace(std::to_string(i));
        qc::Set<std::string> s4;
        s4 = s3;
        Assert::IsTrue(s4 == s3);
    }

    TEST_METHOD(MoveAssignment) {
        qc::Set<int> s1;
        for (int i(0); i < 128; ++i) s1.emplace(i);
        qc::Set<int> ref(s1);
        qc::Set<int> s2;
        s2 = std::move(s1);
        Assert::IsTrue(ref == s2);
        Assert::IsTrue(s1.empty());
    }

    TEST_METHOD(ValuesAssignment) {
        qc::Set<int> s; s = { 0, 1, 2, 3, 4, 5 };
        Assert::AreEqual(size_t(6), s.size());
        Assert::AreEqual(size_t(16), s.bucket_count());
        for (int i = 0; i < 6; ++i) {
            Assert::IsTrue(s.count(i));
        }
    }

    TEST_METHOD(Clear) {
        qc::Set<int> s1;
        for (int i(0); i < 128; ++i) s1.emplace(i);
        Assert::AreEqual(size_t(128), s1.size());
        Assert::AreEqual(size_t(256), s1.bucket_count());
        s1.clear();
        Assert::AreEqual(size_t(0), s1.size());
        Assert::AreEqual(size_t(256), s1.bucket_count());

        qc::Set<std::string> s2;
        for (int i(0); i < 128; ++i) s2.emplace(std::to_string(i));
        Assert::AreEqual(size_t(128), s2.size());
        Assert::AreEqual(size_t(256), s2.bucket_count());
        s2.clear();
        Assert::AreEqual(size_t(0), s2.size());
        Assert::AreEqual(size_t(256), s2.bucket_count());
    }
    
    TEST_METHOD(InsertLRef) {
        qc::Set<int> s;
        for (int i = 0; i < 128; ++i) {
            auto res1(s.insert(i));
            Assert::IsTrue(res1.first != s.end());
            Assert::AreEqual(i, *res1.first);
            Assert::IsTrue(res1.second);
            auto res2(s.insert(i));
            Assert::IsTrue(res2.first == res1.first);
            Assert::IsFalse(res2.second);
        }
        Assert::AreEqual(size_t(128), s.size());
    }

    TEST_METHOD(InsertRRef) {
        qc::Set<std::string> s;
        std::string value("value");
        auto res(s.insert(std::move(value)));
        Assert::IsTrue(res.first != s.end());
        Assert::AreEqual(std::string("value"), *res.first);
        Assert::IsTrue(res.second);
        Assert::IsTrue(value.empty());
    }

    TEST_METHOD(InsertRange) {
        qc::Set<int> s;
        std::vector<int> values;
        for (int i(0); i < 128; ++i) values.push_back(i);
        s.insert(values.cbegin(), values.cend());
        Assert::AreEqual(size_t(128), s.size());
        for (int i(0); i < 128; ++i) {
            Assert::IsTrue(s.contains(i));
        }
    }

    TEST_METHOD(InsertValues) {
        qc::Set<int> s;        
        s.insert({ 0, 1, 2, 3, 4, 5 });
        Assert::AreEqual(size_t(6), s.size());
        for (int i(0); i < 6; ++i) {
            Assert::IsTrue(s.contains(i));
        }
    }
    
    TEST_METHOD(Emplace) {
        struct A {
            int x;
            A(int x) : x(x) {}
            A(const A & other) = delete;
            A(A && other) : x(other.x) { other.x = 0; }
            ~A() { x = 0; }
            A & operator=(const A & other) = delete;
            A & operator=(A && other) { x = other.x; other.x = 0; return *this; }
            bool operator==(const A & other) const { return x == other.x; }
        };

        qc::Set<A> s;
        for (int i(0); i < 128; ++i) {
            auto [it, res](s.emplace(i));
            Assert::IsTrue(it != s.cend());
            Assert::IsTrue(res);
        }
        for (int i(0); i < 128; ++i) {
            Assert::IsTrue(s.contains(A(i)));
        }
    }

    TEST_METHOD(EraseValue) {
        qc::Set<int> s;
        for (int i(0); i < 128; ++i) {
            s.emplace(i);
        }
        Assert::IsFalse(s.erase(128));
        for (int i(0); i < 128; ++i) {
            Assert::IsTrue(s.erase(i));
            Assert::AreEqual(size_t(128 - i - 1), s.size());
        }
    }

    TEST_METHOD(EraseIterator) {
        qc::Set<int> s;
        for (int i(0); i < 128; ++i) {
            s.emplace(i);
        }
        Assert::IsTrue(s.erase(s.end()) == s.end());
        auto it(s.begin());
        for (int i(0); i < 128; ++i) {
            it = s.erase(it);
            Assert::AreEqual(size_t(128 - i - 1), s.size());
        }
    }

    TEST_METHOD(Swap) {
        qc::Set<int> s1{ 1, 2, 3 };
        qc::Set<int> s2{ 4, 5, 6 };
        qc::Set<int> s3(s1);
        qc::Set<int> s4(s2);
        s3.swap(s4);
        Assert::IsTrue(s3 == s2);
        Assert::IsTrue(s4 == s1);
        std::swap(s3, s4);
        Assert::IsTrue(s3 == s1);
        Assert::IsTrue(s4 == s2);
    }

    TEST_METHOD(Find) {
        qc::Set<int> s;
        for (int i(0); i < 128; ++i) {
            s.emplace(i);
        }
        for (int i(0); i < 128; ++i) {
            auto it(s.find(i));
            Assert::AreEqual(i, *it);
        }
        Assert::IsTrue(s.find(128) == s.end());
    }

    TEST_METHOD(NoPreemtiveRehash) {
        qc::Set<int> s{ 0, 1, 2 };
        Assert::AreEqual(size_t(8), s.bucket_count());
        s.emplace(3);
        Assert::AreEqual(size_t(8), s.bucket_count());
        s.emplace(3);
        Assert::AreEqual(size_t(8), s.bucket_count());
    }

    TEST_METHOD(Rehash) {
        qc::Set<int> s(16);
        for (int i(0); i < 16; ++i) {
            s.emplace(i);
        }
        Assert::AreEqual(size_t(32), s.bucket_count());
        s.emplace(16);
        Assert::AreEqual(size_t(64), s.bucket_count());
        for (int i(17); i < 128; ++i) {
            s.emplace(i);
        }
        Assert::AreEqual(size_t(256), s.bucket_count());
        s.rehash(500);
        Assert::AreEqual(size_t(512), s.bucket_count());
        Assert::AreEqual(size_t(128), s.size());
        for (int i = 0; i < 128; ++i) {
            Assert::IsTrue(s.contains(i));
        }
        s.rehash(10);
        Assert::AreEqual(size_t(512), s.bucket_count());
        s.rehash(256);
        Assert::AreEqual(size_t(256), s.bucket_count());
        for (int i = 0; i < 128; ++i) {
            Assert::IsTrue(s.contains(i));
        }
        s.clear();
        s.rehash(0);
        Assert::AreEqual(size_t(8), s.bucket_count());
    }

    TEST_METHOD(Reserve) {
        qc::Set<int> s(40);
        s.emplace(0);
        Assert::AreEqual(size_t(128), s.bucket_count());
        s.reserve(9);
        Assert::AreEqual(size_t(128), s.bucket_count());
        s.reserve(64);
        Assert::AreEqual(size_t(128), s.bucket_count());
        s.reserve(65);
        Assert::AreEqual(size_t(256), s.bucket_count());
    }

    TEST_METHOD(Equality) {
        qc::Set<int> s1, s2, s3;
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

        qc::Set<A, qc::NoHash<A>> s;
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
        qc::Set<int> t;
        qc::Set<int>::iterator it1(t.begin());
        qc::Set<int>::const_iterator cit1 = t.cbegin();
        it1 = cit1;
        cit1 = it1;
        qc::Set<int>::iterator mit2(it1);
        mit2 = it1;
        qc::Set<int>::const_iterator cit2(cit1);
        cit2 = cit1;
        qc::Set<int>::iterator mit3(std::move(it1));
        mit3 = std::move(it1);
        qc::Set<int>::const_iterator cit3(std::move(cit1));
        cit3 = std::move(cit1);
    }

    TEST_METHOD(ForEachLoop) {
        qc::Set<int, qc::NoHash<int>> s;
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
        qc::Set<int, qc::NoHash<int>> s(128);
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
        Assert::AreEqual(size_t(256), s.bucket_count());
        Assert::AreEqual(size_t(64), s.bucket_size(224));
        Assert::AreEqual(size_t(64), s.bucket_size(192));
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
        Assert::AreEqual(size_t(32), s.bucket_size(224));
        Assert::AreEqual(size_t(32), s.bucket_size(192));
        it = s.begin();
        for (int i(0); it != s.end() && i < 32; ++it, ++i) {
            Assert::AreEqual(192 + (i + 32) * 256, *it);
        }
        for (int i(0); it != s.end() && i < 32; ++it, ++i) {
            Assert::AreEqual(224 + (i + 32) * 256, *it);
        }
    }

    TEST_METHOD(Reordering) {
        qc::Set<int, qc::NoHash<int>> s(128);
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
        Assert::AreEqual(size_t(256), s.bucket_count());
        Assert::AreEqual(size_t(128), s.size());
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
    typename SetStats calcStats(const qc::Set<V, H> & set) {
        size_t min(-1);
        size_t max(0);

        std::unordered_map<size_t, size_t> histo;
        size_t total(0);
        for (size_t i(0); i < set.bucket_count(); ++i) {
            size_t size(set.bucket_size(i));
            ++histo[size];
            if (size < min) min = size;
            else if (size > max) max = size;
            total += size;
        }

        double mean(double(total) / double(set.bucket_count()));

        double stddev(0.0);
        for (size_t i(0); i < set.bucket_count(); ++i) {
            double diff(double(set.bucket_size(i)) - mean);
            stddev += diff * diff;
        }
        stddev /= double(set.bucket_count());
        stddev = std::sqrt(stddev);

        size_t median(0);
        size_t medianVal(0);
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
        constexpr int k_size(8192);

        qc::Set<int, qc::NoHash<int>> s1(k_size);
        qc::Set<int> s2(k_size);
        for (int i(0); i < k_size; ++i) {
            s1.emplace(i);
            s2.emplace(i);
        }

        SetStats stats1(calcStats(s1));
        Assert::AreEqual(size_t(k_size), stats1.histo.at(0));
        Assert::AreEqual(size_t(k_size), stats1.histo.at(1));
        Assert::AreEqual(0.5, stats1.mean, 1.0e-6);
        Assert::AreEqual(0.5, stats1.stddev, 1.0e-6);

        SetStats stats2(calcStats(s2));
        Assert::AreEqual(0.5, stats2.mean, 1.0e-6);
        Assert::AreEqual(0.7, stats2.stddev, 0.1);
    }
};