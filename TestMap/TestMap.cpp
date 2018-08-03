#include <SDKDDKVer.h>
#include <vector>
#include <unordered_map>

#include "CppUnitTest.h"

#include "QHash/Map.hpp"



using namespace Microsoft::VisualStudio::CppUnitTestFramework;



TEST_CLASS(Map) {

public:
        
    TEST_METHOD(DefaultConstructor) {
        qc::Map<int, int> m;
        Assert::AreEqual(qc::config::map::defNBuckets, m.bucket_count());
        Assert::IsTrue(m.size() >= 0);
    }

    TEST_METHOD(CapacityConstructor) {
        Assert::AreEqual(size_t(   1), qc::Map<int, int>(   0).bucket_count());
        Assert::AreEqual(size_t(   1), qc::Map<int, int>(   1).bucket_count());
        Assert::AreEqual(size_t(   4), qc::Map<int, int>(   3).bucket_count());
        Assert::AreEqual(size_t(1024), qc::Map<int, int>(1000).bucket_count());
    }

    TEST_METHOD(CopyConstructor) {
        qc::Map<int, int> m1;
        for (int i(0); i < 64; ++i) m1.emplace(i, i);
        qc::Map<int, int> m2(m1);
        Assert::IsTrue(m2 == m1);
    }

    TEST_METHOD(MoveConstructor) {
        qc::Map<int, int> m1;
        for (int i(0); i < 64; ++i) m1.emplace(i, i);
        qc::Map<int, int> ref(m1);
        qc::Map<int, int> m2(std::move(m1));
        Assert::IsTrue(m2 == ref);
        Assert::IsTrue(m1.empty());
    }

    TEST_METHOD(RangeConstructor) {
        std::vector<std::pair<const int, int>> values{
            { 0, 5 },
            { 1, 4 },
            { 2, 3 },
            { 3, 2 },
            { 4, 1 },
            { 5, 0 }
        };
        qc::Map<int, int> m(values.cbegin(), values.cend());
        Assert::AreEqual(size_t(6), m.size());
        Assert::AreEqual(size_t(8), m.bucket_count());
        for (int i = 0; i < 6; ++i) {
            Assert::AreEqual(5 - i, m.at(i));
        }
    }

    TEST_METHOD(ValuesConstructor) {
        qc::Map<int, int> m({
            { 0, 5 },
            { 1, 4 },
            { 2, 3 },
            { 3, 2 },
            { 4, 1 },
            { 5, 0 }
        });
        Assert::AreEqual(size_t(6), m.size());
        Assert::AreEqual(size_t(8), m.bucket_count());
        for (int i = 0; i < 6; ++i) {
            Assert::AreEqual(5 - i, m.at(i));
        }
    }

    TEST_METHOD(CopyAssignment) {
        qc::Map<int, int> m1;
        for (int i(0); i < 64; ++i) m1.emplace(i, i);
        qc::Map<int, int> m2; m2 = m1;
        Assert::IsTrue(m2 == m1);
    }

    TEST_METHOD(MoveAssignment) {
        qc::Map<int, int> m1;
        for (int i(0); i < 64; ++i) m1.emplace(i, i);
        qc::Map<int, int> ref(m1);
        qc::Map<int, int> m2; m2 = std::move(m1);
        Assert::IsTrue(m2 == ref);
        Assert::IsTrue(m1.empty());
    }

    TEST_METHOD(ValuesAssignment) {
        qc::Map<int, int> m; m = {
            { 0, 5 },
            { 1, 4 },
            { 2, 3 },
            { 3, 2 },
            { 4, 1 },
            { 5, 0 }
        };
        Assert::AreEqual(size_t(6), m.size());
        Assert::AreEqual(size_t(8), m.bucket_count());
        for (int i = 0; i < 6; ++i) {
            Assert::AreEqual(5 - i, m.at(i));
        }
    }

    TEST_METHOD(Swap) {
        qc::Map<int, int> m1{{ 1, 1 }, { 2, 2 }, { 3, 3 }};
        qc::Map<int, int> m2{{ 4, 4 }, { 5, 5 }, { 6, 6 }};
        qc::Map<int, int> m3(m1);
        qc::Map<int, int> m4(m2);
        m3.swap(m4);
        Assert::IsTrue(m3 == m2);
        Assert::IsTrue(m4 == m1);
        std::swap(m3, m4);
        Assert::IsTrue(m3 == m1);
        Assert::IsTrue(m4 == m2);
    }
    
    TEST_METHOD(InsertLRef) {
        qc::Map<int, int> m;
        for (int i = 0; i < 128; ++i) {
            std::pair<int, int> value{ 127 - i, i };
            auto res(m.insert(value));
            Assert::AreEqual(127 - i, res.first->first);
            Assert::AreEqual(i, res.first->second);
            Assert::IsTrue(res.second);
            res = m.insert(value);
            Assert::AreEqual(127 - i, res.first->first);
            Assert::AreEqual(i, res.first->second);
            Assert::IsFalse(res.second);
        }
        Assert::AreEqual(size_t(128), m.size());
    }

    TEST_METHOD(InsertRRef) {
        qc::Map<std::string, std::string> m;
        std::pair<std::string, std::string> value{ "key", "element" };
        auto res(m.insert(std::move(value)));
        Assert::AreEqual(std::string("key"), res.first->first);
        Assert::AreEqual(std::string("element"), res.first->second);
        Assert::IsTrue(res.second);
        Assert::IsTrue(value.first.empty());
        Assert::IsTrue(value.second.empty());
    }

    TEST_METHOD(InsertRange) {
        qc::Map<int, int> m;
        std::vector<std::pair<int, int>> values;
        for (int i(0); i < 128; ++i) values.push_back({ 127 - i, i });
        m.insert(values.cbegin(), values.cend());
        Assert::AreEqual(size_t(128), m.size());
        for (int i(0); i < 128; ++i) {
            auto it(m.find(127 - i));
            Assert::AreEqual(127 - i, it->first);
            Assert::AreEqual(i, it->second);
        }
    }

    TEST_METHOD(InsertValues) {
        qc::Map<int, int> m;        
        m.insert({
            { 0, 5 },
            { 1, 4 },
            { 2, 3 },
            { 3, 2 },
            { 4, 1 },
            { 5, 0 }
        });
        Assert::AreEqual(size_t(6), m.size());
        for (int i = 0; i < 6; ++i) {
            Assert::AreEqual(5 - i, m.at(i));
        }
    }
    
    TEST_METHOD(Emplace) {
        qc::Map<std::string, std::string> m;
        std::string key("key"), element("element");
        auto res(m.emplace(std::move(key), std::move(element)));
        Assert::AreEqual(std::string("key"), res.first->first);
        Assert::AreEqual(std::string("element"), res.first->second);
        Assert::IsTrue(res.second);
        Assert::IsTrue(key.empty());
        Assert::IsTrue(element.empty());
        key = "key"; element = "different";
        res = m.emplace(std::move(key), std::move(element));
        Assert::IsFalse(res.second);
        Assert::AreEqual(std::string("element"), std::string(m.at("key")));
    }

    TEST_METHOD(NoPreemtiveRehash) {
        qc::Map<int, int> m;
        m.rehash(1);
        Assert::AreEqual(size_t(1), m.bucket_count());
        m.emplace(0, 0);
        Assert::AreEqual(size_t(1), m.bucket_count());
        m.emplace(0, 1);
        Assert::AreEqual(size_t(1), m.bucket_count());
    }

    TEST_METHOD(At) {
        qc::Map<int, int> m;
        for (int i(0); i < 128; ++i) {
            m.emplace(i, 127 - i);
        }
        for (int i(0); i < 128; ++i) {
            Assert::AreEqual(127 - i, m.at(i));
        }
    }

    TEST_METHOD(Find) {
        qc::Map<int, int> m;
        for (int i(0); i < 128; ++i) {
            m.emplace(i, 127 - i);
        }
        for (int i(0); i < 128; ++i) {
            auto it(m.find(i));
            Assert::AreEqual(i, it->first);
            Assert::AreEqual(127 - i, it->second);
        }
        Assert::IsTrue(m.find(128) == m.end());
    }

    TEST_METHOD(FindElement) {
        qc::Map<int, int> m;
        for (int i(0); i < 128; ++i) {
            m.emplace(i, 127 - i);
        }
        for (int i(0); i < 128; ++i) {
            auto it(m.find_e(i));
            Assert::AreEqual(127 - i, it->first);
            Assert::AreEqual(i, it->second);
        }
        Assert::IsTrue(m.find_e(128) == m.end());
    }

    TEST_METHOD(Access) {
        qc::Map<int, int> m;
        for (int i(0); i < 128; ++i) {
            m[i] = 127 - i;
        }
        Assert::AreEqual(size_t(128), m.size());
        for (int i(0); i < 128; ++i) {
            Assert::AreEqual(127 - i, m[i]);
        }
        Assert::AreEqual(0, m[128]);
    }

    TEST_METHOD(EraseKey) {
        qc::Map<int, int> m;
        for (int i(0); i < 128; ++i) {
            m.emplace(i, 127 - i);
        }
        Assert::IsFalse(m.erase(128));
        for (int i(0); i < 128; ++i) {
            Assert::IsTrue(m.erase(i));
            Assert::AreEqual(size_t(128 - i - 1), m.size());
        }
    }

    TEST_METHOD(EraseIterator) {
        qc::Map<int, int> m;
        for (int i(0); i < 128; ++i) {
            m.emplace(i, 127 - i);
        }
        Assert::IsTrue(m.erase(m.end()) == m.end());
        auto it(m.begin());
        for (int i(0); i < 128; ++i) {
            it = m.erase(it);
            Assert::AreEqual(size_t(128 - i - 1), m.size());
        }
    }

    TEST_METHOD(EraseRange) {
        qc::Map<int, int> m;
        for (int i(0); i < 128; ++i) {
            m.emplace(i, 127 - i);
        }
        auto end(m.begin());
        end = m.erase(m.begin(), end);
        Assert::IsTrue(end == m.begin());
        Assert::AreEqual(size_t(128), m.size());
        for (int i(0); i < 64; ++i) ++end;
        end = m.erase(m.begin(), end);
        Assert::AreEqual(size_t(64), m.size());
        end = m.erase(m.begin(), m.end());
        Assert::IsTrue(end == m.end());
        Assert::AreEqual(size_t(0), m.size());
    }

    TEST_METHOD(Count) {
        qc::Map<int, int> m;
        for (int i(0); i < 128; ++i) {
            m.emplace(i, 127 - i);
        }
        for (int i(0); i < 128; ++i) {
            Assert::IsTrue(m.count(i));
        }
        Assert::IsFalse(m.count(128));
    }

    TEST_METHOD(Rehash) {
        qc::Map<int, int> m(16);
        for (int i(0); i < 16; ++i) {
            m.emplace(i, i);
        }
        Assert::AreEqual(size_t(16), m.bucket_count());
        m.emplace(16, 16);
        Assert::AreEqual(size_t(32), m.bucket_count());
        for (int i(17); i < 128; ++i) {
            m.emplace(i, i);
        }
        Assert::AreEqual(size_t(128), m.bucket_count());

        m.rehash(500);
        Assert::AreEqual(size_t(512), m.bucket_count());
        Assert::AreEqual(size_t(128), m.size());
        for (int i = 0; i < 128; ++i) {
            Assert::AreEqual(i, m.at(i));
        }

        m.rehash(10);
        Assert::AreEqual(size_t(16), m.bucket_count());
        Assert::AreEqual(size_t(128), m.size());
        for (int i = 0; i < 128; ++i) {
            Assert::AreEqual(i, m.at(i));
        }

        m.clear();
        m.rehash(1);
        Assert::AreEqual(size_t(1), m.bucket_count());
        m.emplace(0, 0);
        Assert::AreEqual(size_t(1), m.bucket_count());
        m.emplace(1, 1);
        Assert::AreEqual(size_t(2), m.bucket_count());
        m.emplace(2, 2);
        Assert::AreEqual(size_t(4), m.bucket_count());
        m.emplace(3, 3);
        Assert::AreEqual(size_t(4), m.bucket_count());
        m.emplace(4, 4);
        Assert::AreEqual(size_t(8), m.bucket_count());
    }

    TEST_METHOD(Reserve) {
        qc::Map<int, int> m(16);
        Assert::AreEqual(size_t(16), m.bucket_count());
        m.reserve(17);
        Assert::AreEqual(size_t(32), m.bucket_count());
        m.reserve(16);
        Assert::AreEqual(size_t(32), m.bucket_count());
    }

    TEST_METHOD(Clear) {
        qc::Map<int, int> m(16);
        m.clear();
        Assert::AreEqual(size_t(0), m.size());
        Assert::AreEqual(size_t(16), m.bucket_count());
        for (int i(0); i < 128; ++i) {
            m.emplace(i, i);
        }
        Assert::AreEqual(size_t(128), m.size());
        Assert::AreEqual(size_t(128), m.bucket_count());
        m.clear();
        Assert::AreEqual(size_t(0), m.size());
        Assert::AreEqual(size_t(128), m.bucket_count());
    }

    TEST_METHOD(Equality) {
        qc::Map<int, int> m1, m2, m3;
        for (int i(0); i < 128; ++i) {
            m1.emplace(i, i);
            m3.emplace(i, 127 - i);
        }
        m2 = m1;
        Assert::IsTrue(m2 == m1);
        Assert::IsFalse(m2 != m1);
        Assert::IsFalse(m3 == m1);
        Assert::IsTrue(m3 != m1);
    }
    
    TEST_METHOD(ReferenceNature) {
        qc::Map<int, int> m;
        m.emplace(0, 0);
        Assert::AreEqual(0, m.at(0));
        ++m.at(0);
        Assert::AreEqual(1, m.at(0));
        int * p = &m[0];
        ++(*p);
        Assert::AreEqual(2, m.at(0));
    }

    TEST_METHOD(OutOfRangeException) {
        qc::Map<int, int> m;
        Assert::ExpectException<std::out_of_range>([&m](){ m.at(0); });
    }

    TEST_METHOD(Iterator) {
        struct Test { int v; };

        qc::Map<int, Test> m;
        for (int i(0); i < 64; ++i) {
            m.emplace_h(i, i, Test{ i });
        }
        m.rehash(8);
        int i(0);
        for (auto it = m.begin(); it != m.end(); ++it) {
            Assert::AreEqual(i % 8 * 8 + i / 8, it->second.v);
            Assert::AreEqual(it->second.v, (*it).second.v);
            Assert::AreEqual(it->second.v, it->first);
            Assert::AreEqual(it->second.v, int(it.hash()));
            it->second.v *= 2;
            ++i;
        }

        const qc::Map<int, Test> & cm(m);
        i = 0;
        for (auto it = cm.cbegin(); it != cm.cend(); ++it) {
            Assert::AreEqual(2 * (i % 8 * 8 + i / 8), it->second.v);
            Assert::AreEqual(it->second.v, (*it).second.v);
            Assert::AreEqual(it->second.v, it->first * 2);
            Assert::AreEqual(it->second.v, int(it.hash() * 2));
            //it->second.v *= 2;
            ++i;
        }

        // Just checking for compilation
        qc::Map<int, Test>::iterator it1 = m.begin();
        qc::Map<int, Test>::const_iterator cit1 = m.cbegin();
        it1 = cit1;
        cit1 = it1;
        qc::Map<int, Test>::iterator mit2(it1);
        mit2 = it1;
        qc::Map<int, Test>::const_iterator cit2(cit1);
        cit2 = cit1;
        qc::Map<int, Test>::iterator mit3(std::move(it1));
        mit3 = std::move(it1);
        qc::Map<int, Test>::const_iterator cit3(std::move(cit1));
        cit3 = std::move(cit1);
    }

    TEST_METHOD(ForEachLoop) {
        qc::Map<int, int> m;
        for (int i(0); i < 128; ++i) {
            m.emplace_h(i, i, 127 - i);
        }
        int i(0);
        for (const auto & v : m) {
            Assert::AreEqual(i, v.first);
            Assert::AreEqual(127 - i, v.second);
            ++i;
        }
    }

    TEST_METHOD(MapMap) {
        qc::Map<int, qc::Map<int, int>> mm;

        for (int i(0); i < 100; ++i) {
            for (int j(0); j < 100; ++j) {
                mm[i][j] = i * j;
            }
        }
        for (int i(0); i < 100; ++i) {
            for (int j(0); j < 100; ++j) {
                Assert::AreEqual(i * j, mm.at(i).at(j));
                mm.at(i).erase(j);
            }
            mm.erase(i);
        }
    }

    struct MapStats {
        size_t min, max, median;
        double mean, stddev;
        std::unordered_map<size_t, size_t> histo;
    };

    template <typename K, typename E>
    typename MapStats calcStats(const qc::Map<K, E> & map) {
        size_t min(map.bucket_size(0));
        size_t max(map.bucket_size(0));

        std::unordered_map<size_t, size_t> sizeCounts;
        size_t total(0);
        for (size_t i(0); i < map.bucket_count(); ++i) {
            size_t bucket_size(map.bucket_size(i));
            ++sizeCounts[bucket_size];
            if (bucket_size < min) min = bucket_size;
            else if (bucket_size > max) max = bucket_size;
            total += bucket_size;
        }
        double mean(double(total) / double(map.bucket_count()));

        double stddev(0.0);
        for (size_t i(0); i < map.bucket_count(); ++i) {
            double val(map.bucket_size(i) - mean);
            stddev += val * val;
        }
        stddev /= double(map.bucket_count());
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

    /*void printHisto(const MapStats & stats) {
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

        qc::Map<int, int> m1(k_size);
        qc::Map<int, int> m2(k_size);
        for (int i = 0; i < k_size; ++i) {
            m1.emplace_h(i, i, i);
            m2.emplace(i, i);
        }
        m1.rehash(k_size / 8);
        m2.rehash(k_size / 8);

        MapStats stats1(calcStats(m1));
        Assert::AreEqual(size_t(8), stats1.min);
        Assert::AreEqual(size_t(8), stats1.max);
        Assert::AreEqual(0.0, stats1.stddev, 1.0e-6);

        MapStats stats2(calcStats(m2));
        Assert::AreEqual(8.0, stats2.mean, 1.0e-6);
        Assert::AreEqual(2.85, stats2.stddev, 0.1);
    }
};