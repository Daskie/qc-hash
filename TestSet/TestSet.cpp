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
        Assert::AreEqual(qc::config::set::defNBuckets, s.bucket_count());
        Assert::IsTrue(s.size() >= 0);
    }

    TEST_METHOD(CapacityConstructor) {
        Assert::AreEqual(size_t(   1), qc::Set<int>(   0).bucket_count());
        Assert::AreEqual(size_t(   1), qc::Set<int>(   1).bucket_count());
        Assert::AreEqual(size_t(   4), qc::Set<int>(   3).bucket_count());
        Assert::AreEqual(size_t(1024), qc::Set<int>(1000).bucket_count());
    }

    TEST_METHOD(CopyConstructor) {
        qc::Set<int> s1;
        for (int i(0); i < 64; ++i) s1.emplace(i);
        qc::Set<int> s2(s1);
        Assert::IsTrue(s2 == s1);
    }

    TEST_METHOD(MoveConstructor) {
        qc::Set<int> s1;
        for (int i(0); i < 64; ++i) s1.emplace(i);
        qc::Set<int> ref(s1);
        qc::Set<int> s2(std::move(s1));
        Assert::IsTrue(s2 == ref);
        Assert::IsTrue(s1.empty());
    }

    TEST_METHOD(RangeConstructor) {
        std::vector<int> values{ 0, 1, 2, 3, 4, 5 };
        qc::Set<int> s(values.cbegin(), values.cend());
        Assert::AreEqual(size_t(6), s.size());
        Assert::AreEqual(size_t(8), s.bucket_count());
        for (int i = 0; i < 6; ++i) {
            Assert::IsTrue(s.count(i));
        }
    }

    TEST_METHOD(ValuesConstructor) {
        qc::Set<int> s({ 0, 1, 2, 3, 4, 5 });
        Assert::AreEqual(size_t(6), s.size());
        Assert::AreEqual(size_t(8), s.bucket_count());
        for (int i = 0; i < 6; ++i) {
            Assert::IsTrue(s.count(i));
        }
    }

    TEST_METHOD(CopyAssignment) {
        qc::Set<int> s1;
        for (int i(0); i < 64; ++i) s1.emplace(i);
        qc::Set<int> s2; s2 = s1;
        Assert::IsTrue(s2 == s1);
    }

    TEST_METHOD(MoveAssignment) {
        qc::Set<int> s1;
        for (int i(0); i < 64; ++i) s1.emplace(i);
        qc::Set<int> ref(s1);
        qc::Set<int> s2; s2 = std::move(s1);
        Assert::IsTrue(ref == s2);
        Assert::IsTrue(s1.empty());
    }

    TEST_METHOD(ValuesAssignment) {
        qc::Set<int> s; s = { 0, 1, 2, 3, 4, 5 };
        Assert::AreEqual(size_t(6), s.size());
        Assert::AreEqual(size_t(8), s.bucket_count());
        for (int i = 0; i < 6; ++i) {
            Assert::IsTrue(s.count(i));
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
    
    TEST_METHOD(InsertLRef) {
        qc::Set<int> s;
        for (int i = 0; i < 128; ++i) {
            auto res(s.insert(i));
            Assert::AreEqual(i, *res.first);
            Assert::IsTrue(res.second);
            res = s.insert(i);
            Assert::IsFalse(res.second);
        }
        Assert::AreEqual(size_t(128), s.size());
    }

    TEST_METHOD(InsertRRef) {
        qc::Set<std::string> s;
        std::string value("value");
        auto res(s.insert(std::move(value)));
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
            Assert::IsTrue(s.count(i));
        }
    }

    TEST_METHOD(InsertValues) {
        qc::Set<int> s;        
        s.insert({ 0, 1, 2, 3, 4, 5 });
        Assert::AreEqual(size_t(6), s.size());
        for (int i = 0; i < 6; ++i) {
            Assert::IsTrue(s.count(i));
        }
    }
    
    TEST_METHOD(Emplace) {
        qc::Set<std::string> s;
        std::string value("value");
        auto res(s.emplace(std::move(value)));
        Assert::AreEqual(std::string("value"), *res.first);
        Assert::IsTrue(res.second);
        Assert::IsTrue(value.empty());
        value = "value";
        res = s.emplace(std::move(value));
        Assert::IsFalse(res.second);
    }

    TEST_METHOD(NoPreemtiveRehash) {
        qc::Set<int> s;
        s.rehash(1);
        Assert::AreEqual(size_t(1), s.bucket_count());
        s.emplace(0);
        Assert::AreEqual(size_t(1), s.bucket_count());
        s.emplace(0);
        Assert::AreEqual(size_t(1), s.bucket_count());
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

    TEST_METHOD(EraseKey) {
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

    TEST_METHOD(EraseRange) {
        qc::Set<int> s;
        for (int i(0); i < 128; ++i) {
            s.emplace(i);
        }
        auto end(s.begin());
        end = s.erase(s.begin(), end);
        Assert::IsTrue(end == s.begin());
        Assert::AreEqual(size_t(128), s.size());
        for (int i(0); i < 64; ++i) ++end;
        end = s.erase(s.begin(), end);
        Assert::AreEqual(size_t(64), s.size());
        end = s.erase(s.begin(), s.end());
        Assert::IsTrue(end == s.end());
        Assert::AreEqual(size_t(0), s.size());
    }

    TEST_METHOD(Count) {
        qc::Set<int> s;
        for (int i(0); i < 128; ++i) {
            s.emplace(i);
        }
        for (int i(0); i < 128; ++i) {
            Assert::IsTrue(s.count(i));
        }
        Assert::IsFalse(s.count(128));
    }

    TEST_METHOD(Rehash) {
        qc::Set<int> s(16);
        for (int i(0); i < 16; ++i) {
            s.emplace(i);
        }
        Assert::AreEqual(size_t(16), s.bucket_count());
        s.emplace(16);
        Assert::AreEqual(size_t(32), s.bucket_count());
        for (int i(17); i < 128; ++i) {
            s.emplace(i);
        }
        Assert::AreEqual(size_t(128), s.bucket_count());

        s.rehash(500);
        Assert::AreEqual(size_t(512), s.bucket_count());
        Assert::AreEqual(size_t(128), s.size());
        for (int i = 0; i < 128; ++i) {
            Assert::IsTrue(s.count(i));
        }

        s.rehash(10);
        Assert::AreEqual(size_t(16), s.bucket_count());
        Assert::AreEqual(size_t(128), s.size());
        for (int i = 0; i < 128; ++i) {
            Assert::IsTrue(s.count(i));
        }

        s.clear();
        s.rehash(1);
        Assert::AreEqual(size_t(1), s.bucket_count());
        s.emplace(0);
        Assert::AreEqual(size_t(1), s.bucket_count());
        s.emplace(1);
        Assert::AreEqual(size_t(2), s.bucket_count());
        s.emplace(2);
        Assert::AreEqual(size_t(4), s.bucket_count());
        s.emplace(3);
        Assert::AreEqual(size_t(4), s.bucket_count());
        s.emplace(4);
        Assert::AreEqual(size_t(8), s.bucket_count());
    }

    TEST_METHOD(Reserve) {
        qc::Set<int> s(16);
        Assert::AreEqual(size_t(16), s.bucket_count());
        s.reserve(17);
        Assert::AreEqual(size_t(32), s.bucket_count());
        s.reserve(16);
        Assert::AreEqual(size_t(32), s.bucket_count());
    }

    TEST_METHOD(Clear) {
        qc::Set<int> s(16);
        s.clear();
        Assert::AreEqual(size_t(0), s.size());
        Assert::AreEqual(size_t(16), s.bucket_count());
        for (int i(0); i < 128; ++i) {
            s.emplace(i);
        }
        Assert::AreEqual(size_t(128), s.size());
        Assert::AreEqual(size_t(128), s.bucket_count());
        s.clear();
        Assert::AreEqual(size_t(0), s.size());
        Assert::AreEqual(size_t(128), s.bucket_count());
    }

    TEST_METHOD(Equality) {
        qc::Set<int> s1, s2, s3;
        for (int i(0); i < 128; ++i) {
            s1.emplace(i);
            s3.emplace(i + 128);
        }
        s2 = s1;
        Assert::IsTrue(s2 == s1);
        Assert::IsFalse(s2 != s1);
        Assert::IsFalse(s3 == s1);
        Assert::IsTrue(s3 != s1);
    }

    TEST_METHOD(Iterator) {
        struct Test { int v; };

        qc::Set<Test> s;
        for (int i(0); i < 64; ++i) {
            s.emplace_h(i, Test{ i });
        }
        s.rehash(8);
        int i(0);
        for (auto it = s.begin(); it != s.end(); ++it) {
            Assert::AreEqual(i % 8 * 8 + i / 8, it->v);
            Assert::AreEqual(it->v, (*it).v);
            Assert::AreEqual(it->v, int(it.hash()));
            it->v *= 2;
            ++i;
        }

        const qc::Set<Test> & cs(s);
        i = 0;
        for (auto it = cs.cbegin(); it != cs.cend(); ++it) {
            Assert::AreEqual(2 * (i % 8 * 8 + i / 8), it->v);
            Assert::AreEqual(it->v, (*it).v);
            Assert::AreEqual(it->v, int(it.hash() * 2));
            //it->second.v *= 2;
            ++i;
        }

        // Just checking for compilation
        qc::Set<Test>::iterator it1 = s.begin();
        qc::Set<Test>::const_iterator cit1 = s.cbegin();
        it1 = cit1;
        cit1 = it1;
        qc::Set<Test>::iterator mit2(it1);
        mit2 = it1;
        qc::Set<Test>::const_iterator cit2(cit1);
        cit2 = cit1;
        qc::Set<Test>::iterator mit3(std::move(it1));
        mit3 = std::move(it1);
        qc::Set<Test>::const_iterator cit3(std::move(cit1));
        cit3 = std::move(cit1);
    }

    TEST_METHOD(ForEachLoop) {
        qc::Set<int> s;
        for (int i(0); i < 128; ++i) {
            s.emplace_h(i, i);
        }
        int i(0);
        for (const auto & v : s) {
            Assert::AreEqual(i, v);
            ++i;
        }
    }

    struct SetStats {
        size_t min, max, median;
        double mean, stddev;
        std::unordered_map<size_t, size_t> histo;
    };

    template <typename V>
    typename SetStats calcStats(const qc::Set<V> & set) {
        size_t min(set.bucket_size(0));
        size_t max(set.bucket_size(0));

        std::unordered_map<size_t, size_t> sizeCounts;
        size_t total(0);
        for (size_t i(0); i < set.bucket_count(); ++i) {
            size_t bucket_size(set.bucket_size(i));
            ++sizeCounts[bucket_size];
            if (bucket_size < min) min = bucket_size;
            else if (bucket_size > max) max = bucket_size;
            total += bucket_size;
        }
        double mean(double(total) / double(set.bucket_count()));

        double stddev(0.0);
        for (size_t i(0); i < set.bucket_count(); ++i) {
            double val(set.bucket_size(i) - mean);
            stddev += val * val;
        }
        stddev /= double(set.bucket_count());
        stddev = std::sqrt(stddev);

        size_t median(0);
        size_t medianVal(0);
        for (const auto & sizeCount : sizeCounts) {
            if (sizeCount.second > medianVal) {
                median = sizeCount.first;
                medianVal = sizeCount.second;
            }
        }

        return {
            min, max, median,
            mean, stddev,
            sizeCounts
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
        constexpr int k_size = 8192;

        qc::Set<int> s1(k_size);
        qc::Set<int> s2(k_size);
        for (int i = 0; i < k_size; ++i) {
            s1.emplace(i);
            s2.emplace_h(qc::hash(&i, sizeof(int)), i);
        }
        s1.rehash(k_size / 8);
        s2.rehash(k_size / 8);

        SetStats stats1(calcStats(s1));
        Assert::AreEqual(size_t(8), stats1.min);
        Assert::AreEqual(size_t(8), stats1.max);
        Assert::AreEqual(0.0, stats1.stddev, 1.0e-6);

        SetStats stats2(calcStats(s2));
        Assert::AreEqual(8.0, stats2.mean, 1.0e-6);
        Assert::AreEqual(2.85, stats2.stddev, 0.1);
    }
};