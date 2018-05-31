#include <string>
#include <iostream>
#include <unordered_map>
#include <map>
#include <iomanip>

#include "QHash/Map.hpp"
#include "QCore/Core.hpp"

using std::string;
using std::cout;
using std::endl;
using std::unordered_map;

using namespace qc;

namespace {

Map<int, int> f_map1;
Map<int, string> f_map2;
Map<int, char> f_map4;

void setupMaps() {
    for (int i = 0; i < 10; ++i) {
        f_map1.emplace_h(i, i, i);
        f_map2.emplace_h(i, i, "" + i);
    }
    for (int i = 0; i < 1000; ++i) {
        f_map4.emplace_h(i, i, i % 256);
    }
}

bool testDefaultConstructor() {
    Map<int, int> m;
    if (m.nBuckets() != config::map::defNBuckets || m.size() != 0) return false;	
    return true;
}

bool testConstructor() {
    cout << "small..." << endl;
    Map<int, int> m1(3);
    if (m1.nBuckets() != 4 || m1.size() != 0) return false;

    cout << "large..." << endl;
    Map<int, int> m2(1000);
    if (m2.nBuckets() != 1024 || m2.size() != 0) return false;

    cout << "zero..." << endl;
    Map<int, int> m3(0);
    if (m3.nBuckets() != 1 || m3.size() != 0) return false;

    return true;
}

bool testCopyConstructor() {
    cout << "int..." << endl;
    Map<int, int> m1(f_map1);
    if (m1 != f_map1) return false;

    cout << "string..." << endl;
    Map<int, string> m2(f_map2);
    if (m2 != f_map2) return false;

    cout << "large..." << endl;
    Map<int, char> m4(f_map4);
    if (m4 != f_map4) return false;

    return true;
}

bool testCopyAssignment() {
    cout << "int..." << endl;
    Map<int, int> m1;
    m1 = f_map1;
    if (m1 != f_map1) return false;

    cout << "string..." << endl;
    Map<int, string> m2;
    m2 = f_map2;
    if (m2 != f_map2) return false;

    cout << "large..." << endl;
    Map<int, char> m4;
    m4 = f_map4;
    if (m4 != f_map4) return false;

    return true;
}

bool testMoveConstructor() {
    cout << "int..." << endl;
    Map<int, int> h1(f_map1);
    Map<int, int> m1(std::move(h1));
    if (m1 != f_map1) return false;
    if (h1.nBuckets() != 0 || h1.size() != 0) return false;

    cout << "string..." << endl;
    Map<int, string> h2(f_map2);
    Map<int, string> m2(std::move(h2));
    if (m2 != f_map2) return false;
    if (h2.nBuckets() != 0 || h2.size() != 0) return false;

    cout << "large..." << endl;
    Map<int, char> h4(f_map4);
    Map<int, char> m4(std::move(h4));
    if (m4 != f_map4) return false;
    if (h4.nBuckets() != 0 || h4.size() != 0) return false;

    return true;
}

bool testMoveAssignment() {
    cout << "int..." << endl;
    Map<int, int> h1(f_map1);
    Map<int, int> m1;
    m1 = std::move(h1);
    if (m1 != f_map1) return false;
    if (h1.nBuckets() != 0 || h1.size() != 0) return false;

    cout << "string..." << endl;
    Map<int, string> h2(f_map2);
    Map<int, string> m2;
    m2 = std::move(h2);
    if (m2 != f_map2) return false;
    if (h2.nBuckets() != 0 || h2.size() != 0) return false;

    cout << "large..." << endl;
    Map<int, char> h4(f_map4);
    Map<int, char> m4;
    m4 = std::move(h4);
    if (m4 != f_map4) return false;
    if (h4.nBuckets() != 0 || h4.size() != 0) return false;

    return true;
}

bool testPairsConstructor() {
    cout << "multiple pairs..." << endl;
    Map<int, int> m1(std::initializer_list<std::pair<int, int>>{
        { 0, 5 },
        { 1, 4 },
        { 2, 3 },
        { 3, 2 },
        { 4, 1 },
        { 5, 0 }
    });
    for (int i = 0; i < 6; ++i) {
        if (m1.at(i) != 5 - i) return false;
    }

    return true;
}

bool testPairsAssignment() {
    cout << "multiple pairs..." << endl;
    std::initializer_list<std::pair<int, int>> pairs{
        { 0, 5 },
        { 1, 4 },
        { 2, 3 },
        { 3, 2 },
        { 4, 1 },
        { 5, 0 }
    };
    Map<int, int> m1;
    m1 = pairs;
    for (int i = 0; i < 6; ++i) {
        if (m1.at(i) != 5 - i) return false;
    }

    return true;
}

bool testRangeConstructor() {
    std::vector<std::pair<const int, int>> pairs;
    for (int i(0); i < 100; ++i) {
        pairs.push_back({ i, 100 - i });
    }

    Map<int, int> m(pairs.cbegin(), pairs.cend());
    if (m.nBuckets() != 128 || m.size() != 100) return false;

    for (int i(0); i < 100; ++i) {
        if (m.at(i) != 100 - i) return false;
    }

    return true;
}

bool testDestructor(int n) {
    Map<int, long long> * m1 = new Map<int, long long>(n);
    for (int i = 0; i < n; ++i) {
        m1->emplace_h(i, i, i);
    }
    delete m1;
    return true;
}

bool testSwap() {
    Map<int, int> m1{{1, 1}, {2, 2}, {3, 3}};
    Map<int, int> m2{{4, 4}, {5, 5}, {6, 6}};
    Map<int, int> m3(m1);
    Map<int, int> m4(m2);
    m3.swap(m4);
    return m3 == m2 && m4 == m1;
    std::swap(m3, m4);
    return m3 == m1 && m4 == m2;
}

bool testInsert() {
    cout << "lref..." << endl;
    Map<int, int> m1;
    for (int i = 0; i < 128; ++i) {
        std::pair<int, int> val{ 127 - i, i };
        auto res = m1.insert(val);
        if (res.first->first != 127 - i || res.first->second != i || !res.second) return false;
        res = m1.insert(val);
        if (res.first->first != 127 - i || res.first->second != i || res.second) return false;
    }
    if (m1.size() != 128) return false;
    
    cout << "rref..." << endl;
    Map<int, int> m2;
    for (int i = 0; i < 128; ++i) {
        auto res = m2.insert({ 127 - i, i });
        if (res.first->first != 127 - i || res.first->second != i || !res.second) return false;
        res = m2.insert({ 127 - i, i });
        if (res.first->first != 127 - i || res.first->second != i || res.second) return false;
    }
    if (m2.size() != 128) return false;

    cout << "initializer list..." << endl;
    Map<int, int> m3;
    for (int i = 0; i < 128; i += 2) {
        m3.insert(std::initializer_list<std::pair<int, int>>{ { 127 - i, i }, { 127 - i + 1, i + 1 } });
        if (m3.find(127 - i)->first != 127 - i || m3.find(127 - i)->second != i ||
            m3.find(127 - i + 1)->first != 127 - i + 1 || m3.find(127 - i + 1)->second != i + 1) return false;
    }
    if (m3.size() != 128) return false;

    cout << "range..." << endl;
    Map<int, int> m4;
    std::vector<std::pair<int, int>> vec;
    for (int i(0); i < 128; ++i) vec.push_back({ 127 - i, i });
    m4.insert(vec.cbegin(), vec.cend());
    for (int i(0); i < 128; ++i) {
        if (m4.find(127 - i)->first != 127 - i || m4.find(127 - i)->second != i) return false;
    }
    if (m4.size() != 128) return false;

    return true;
}

bool testEmplace() {

    cout << "rref value..." << endl;
    Map<int, std::unique_ptr<int>> m1;
    m1.emplace(1, std::make_unique<int>(1));

    if (*m1.at(1) != 1) return false;

    cout << "No unnecessary rehash" << endl;
    Map<int, int> m2;
    m2.rehash(1);
    if (m2.nBuckets() != 1) return false;
    m2.emplace(0, 0);
    if (m2.nBuckets() != 1) return false;
    m2.emplace(0, 1);
    if (m2.nBuckets() != 1) return false;
    if (m2.at(0) != 0) return false;

    return true;
}

bool testAt() {
    int arr[128];
    Map<int, int> m1;
    for (int i = 0; i < 128; ++i) {
        arr[i] = i;
        m1.emplace(i, arr[i]);
    }

    cout << "int..." << endl;
    for (int i = 0; i < 128; ++i) {
        if (m1.at(i) != i) return false;
    }

    cout << "string..." << endl;
    Map<string, int> m2;
    int x = 777;
    m2.emplace(string("okay"), x);
    if (m2.at(string("okay")) != 777) return false;

    return true;
}

bool testFind() {
    int arr[128];
    Map<int, int> m1;
    for (int i = 0; i < 128; ++i) {
        arr[i] = i;
        m1.emplace(i, arr[i]);
    }
    const Map<int, int> & m1_c(m1);

    cout << "int..." << endl;
    for (int i = 0; i < 128; ++i) {
        auto res(m1.find(i));
        if (res.hash() != Hash<int>()(i) || res.element() != i) return false;
        if (res != m1_c.find(i)) return false;
    }

    cout << "string..." << endl;
    Map<string, int> m2;
    const Map<string, int> & m2_c(m2);
    m2.emplace(string("okay"), 777);
    auto res(m2.cfind(string("okay")));
    if (res.hash() != Hash<string>()(string("okay")) || res.element() != 777) return false;
    if (res != m2_c.cfind(string("okay"))) return false;
    if (m2.at(string("okay")) != 777) return false;

    return true;
}

bool testAccess() {
    int arr[128];
    Map<int, int> m1;
    for (int i = 0; i < 128; ++i) {
        arr[i] = i;
        m1[i] = arr[i];
    }

    cout << "int..." << endl;
    for (int i = 0; i < 128; ++i) {
        if (m1[i] != i) return false;
    }

    cout << "string..." << endl;
    Map<string, int> m2;
    int x = 777;
    m2["okay"] = x;
    if (m2["okay"] != 777) return false;

    return true;
}

bool testErase() {
    int arr[128];
    for (int i = 0; i < 128; ++i) {
        arr[i] = i;
    }

    Map<int, int> m1;
    for (int i = 0; i < 128; ++i) {
        m1.emplace(i, arr[i]);
    }

    cout << "int..." << endl;
    for (int i = 0; i < 64; ++i) {
        if (!m1.erase(i)) return false;
        if (m1.size() != 127 - i) return false;
    }

    cout << "string..." << endl;
    Map<string, int> m2;
    m2.emplace(string("okay"), 777);
    if (!m2.erase(string("okay"))) return false;
    if (m2.size() != 0) return false;

    cout << "iterator..." << endl;
    Map<int, int> m3;
    for (int i(0); i < 32; ++i) {
        m3[i] = i;
    }
    auto it(m3.begin());
    while (it != m3.end()) {
        auto next(m3.erase(it));
        if (next == it) return false;
        it = next;
    }
    if (m3.size() != 0) return false;

    cout << "iterator range..." << endl;
    Map<int, int> m4;
    for (int i(0); i < 32; ++i) {
        m4[i] = i;
    }
    auto end(m4.begin());
    for (int i(0); i < 16; ++i) ++end;
    end = m4.erase(m4.begin(), end);
    if (end != m4.begin()) return false;
    if (m4.size() != 16) return false;
    end = m4.erase(m4.begin(), m4.end());
    if (end != m4.end()) return false;
    if (m4.size() != 0) return false;

    return true;
}

bool testCount() {
    int arr[128];
    Map<int, int> m1;
    for (int i = 0; i < 128; ++i) {
        arr[i] = i;
        m1.emplace(i, arr[i]);
    }

    cout << "int..." << endl;
    for (int i = 0; i < 128; ++i) {
        if (!m1.count(i)) return false;
    }

    cout << "string..." << endl;
    Map<string, int> m2;
    int x = 777;
    m2.emplace(string("okay"), x);
    if (!m2.count(string("okay"))) return false;

    return true;
}

bool testFindElement() {
    int arr[128];
    for (int i = 0; i < 128; ++i) {
        arr[i] = i;
    }

    Map<int, int> m1;

    for (int i = 0; i < 10; ++i) {
        auto res(m1.find_e(arr[i]));
        if (res != m1.end()) return false;
        if (res != m1.find_e(arr[i])) return false;
        m1.emplace(i, arr[i]);
    }
    for (int i = 0; i < 10; ++i) {
        auto res(m1.find_e(arr[i]));
        if (res == m1.end()) return false;
        if (res != m1.find_e(arr[i])) return false;
        m1.erase(i);
    }
    for (int i = 0; i < 10; ++i) {
        auto res(m1.find_e(arr[i]));
        if (res != m1.end()) return false;
        if (res != m1.find_e(arr[i])) return false;
    }

    return true;
}

bool testRehash() {
    int arr[128];
    for (int i = 0; i < 128; ++i) {
        arr[i] = i;
    }

    cout << "powers of two..." << endl;
    Map<int, int> m1(16);
    for (int i = 0; i < 16; ++i) {
        m1.emplace(i, arr[i]);
    }
    if (m1.nBuckets() != 16) return false;
    m1.emplace(16, arr[16]);
    if (m1.nBuckets() != 32) return false;
    for (int i = 17; i < 128; ++i) {
        m1.emplace(i, arr[i]);
    }
    if (m1.nBuckets() != 128) return false;

    cout << "larger..." << endl;
    m1.rehash(500);
    if (m1.nBuckets() != 512 || m1.size() != 128) return false;
    for (int i = 0; i < 128; ++i) {
        if (m1.at(i) != arr[i]) return false;
    }

    cout << "smaller..." << endl;
    m1.rehash(10);
    if (m1.nBuckets() != 16 || m1.size() != 128) return false;
    for (int i = 0; i < 128; ++i) {
        if (m1.at(i) != arr[i]) return false;
    }

    cout << "bonus..." << endl;
    Map<int, int> m2(1);
    if (m2.nBuckets() != 1) return false;
    m2.emplace(0, 0);
    if (m2.nBuckets() != 1) return false;
    m2.emplace(1, 1);
    if (m2.nBuckets() != 2) return false;
    m2.emplace(2, 2);
    if (m2.nBuckets() != 4) return false;
    m2.emplace(3, 3);
    if (m2.nBuckets() != 4) return false;
    m2.emplace(4, 4);
    if (m2.nBuckets() != 8) return false;

    return true;
}

bool testReserve() {
    Map<int, int> m1;
    unat nBuckets(m1.nBuckets());
    m1.reserve(nBuckets + 1);
    if (m1.nBuckets() <= nBuckets) return false;
    nBuckets = m1.nBuckets();
    m1.reserve(0);
    if (m1.nBuckets() != nBuckets) return false;

    return true;
}

bool testClear() {
    int arr[128];

    Map<int, int> m1;
    for (int i = 0; i < 128; ++i) {
        m1.emplace_h(i, i, arr[i]);
    }

    cout << "standard..." << endl;
    m1.clear();
    if (m1.size() != 0) return false;

    cout << "empty..." << endl;
    m1.clear();
    if (m1.size() != 0) return false;

    return true;
}

bool testEquality() {
    int arr[128];
    for (int i = 0; i < 128; ++i) {
        arr[i] = i;
    }
    
    Map<int, int> m1;
    for (int i = 0; i < 128; ++i) {
        m1.emplace_h(i, i, arr[i]);
    }

    cout << "equality..." << endl;
    Map<int, int> m2(m1);
    if (m2 != m1) return false;

    cout << "inequality..." << endl;
    m2.erase_h(0);
    if (m2 == m1) return false;

    return true;
}

bool testReferenceNature() {
    Map<int, int> m1;
    m1.emplace(777, 7);
    m1.at(777) = 8;
    if (m1.at(777) != 8) {
        return false;
    }
    int * ip = &m1.at(777);
    *ip = 9;
    if (m1.at(777) != 9) {
        return false;
    }

    return true;
}

bool testErrorThrows() {
    Map<int, int> m1;

    cout << "out_of_range..." << endl;
    try {
        m1.at(0);
        return false;
    }
    catch (const std::out_of_range &) {}

    return true;
}

bool testIterator() {
    struct Test { int v; };
    cout << "standard..." << endl;
    Map<int, Test> m1;
    for (int i = 0; i < 64; ++i) {
        m1.emplace_h(i, i, Test{ i });
    }
    m1.rehash(8);
    int i = 0;
    for (auto it = m1.begin(); it != m1.end(); ++it) {
        if (it->second.v != i % 8 * 8 + i / 8) return false;
        if ((*it).second.v != it->second.v) return false;
        if (it.element().v != it->second.v) return false;
        if (it.hash() != it->second.v) return false;
        if (it.key() != it->second.v) return false;
        (*it).second.v *= 2;
        ++i;
    }

    cout << "const..." << endl;
    const Map<int, Test> * mp = &m1;
    i = 0;
    for (auto it = mp->cbegin(); it != mp->cend(); ++it) {
        if (it->second.v != 2 * (i % 8 * 8 + i / 8)) return false;
        if ((*it).second.v != it->second.v) return false;
        if (it.element().v != it->second.v) return false;
        if (it.hash() * 2 != it->second.v) return false;
        if (it.key() * 2 != it->second.v) return false;
        //(*it).second.v *= 2;
        ++i;
    }

    cout << "conversion..." << endl;
    Map<int, Test>::iterator mit1 = m1.begin();
    Map<int, Test>::const_iterator cit1 = m1.cbegin();
    mit1 = cit1;

    Map<int, Test>::iterator mit2(mit1);
    mit2 = mit1;
    Map<int, Test>::const_iterator cit2(cit1);
    cit2 = cit1;
    Map<int, Test>::iterator mit3(std::move(mit1));
    mit3 = std::move(mit1);
    Map<int, Test>::const_iterator cit3(std::move(cit1));
    cit3 = std::move(cit1);

    cout << "for each loop..." << endl;
    Map<int, int> m5;
    for (const auto & e : m5) {}
    for (int i(0); i < 64; ++i) {
        m5.emplace_h(i, i, i);
    }
    m5.rehash(8);
    i = 0;
    for (auto & e : m5) {
        if (e.second != i % 8 * 8 + i / 8) return false;
        if (e.first != e.second) return false;
        e.second *= 2;
        if (e.second != 2 * (i % 8 * 8 + i / 8)) return false;
        ++i;
    }

    return true;
}

bool testMapMap() {
    Map<int, Map<int, int>> map;

    for (int i(0); i < 100; ++i) {
        for (int j(0); j < 100; ++j) {
            map[i][j] = i * j;
        }
    }
    for (int i(0); i < 100; ++i) {
        for (int j(0); j < 100; ++j) {
            if (map[i][j] != i * j) return false;
            map[i].erase(j);
        }
        map.erase(i);
    }

    return true;
}

bool testPointerHash() {
    int n(sizeof(nat));

    Map<int *, int> m1(n);
    int x; int * p(&x);
    for (int i(0); i < n; ++i) {
        m1[p++] = i;
    }
    for (int i(0); i < n; ++i) {
        if (m1.bucketSize(i) != 1) return false;
    }
    
    Map<int, int> m2(n);
    for (int i(0); i < n; ++i) {
        m2[int(unat(p++))] = i;
    }
    if (m2.bucketSize(0) != n) return false;
    for (int i(1); i < n; ++i) {
        if (m2.bucketSize(i) != 0) return false;
    }

    return true;
}

struct MapStats {
    unat min, max, median;
    double mean, stddev;
    std::map<unat, unat> histo;
};

template <typename K, typename E>
typename MapStats calcStats(const Map<K, E> & map) {
    unat min(map.bucketSize(0));
    unat max(map.bucketSize(0));

    std::map<unat, unat> sizeCounts;
    unat total(0);
    for (unat i(0); i < map.nBuckets(); ++i) {
        unat bucketSize(map.bucketSize(i));
        ++sizeCounts[bucketSize];
        if (bucketSize < min) min = bucketSize;
        else if (bucketSize > max) max = bucketSize;
        total += bucketSize;
    }
    double mean(double(total) / double(map.nBuckets()));

    double stddev(0.0);
    for (unat i = 0; i < map.nBuckets(); ++i) {
        double val(map.bucketSize(i) - mean);
        stddev += val * val;
    }
    stddev /= double(map.nBuckets());
    stddev = std::sqrt(stddev);

    unat median(0);
    unat medianVal(0);
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

void printHisto(const MapStats & stats) {
    int sizeDigits = stats.max ? (int)log10(stats.max) + 1 : 1;
    unat maxCount = stats.histo.at(stats.median);
    int countDigits = maxCount ? (int)log10(maxCount) + 1 : 1;
    int maxLength = 80 - sizeDigits - countDigits - 5; // 5 is for "[][]" & \n
    int length;
    for (auto bucketSize : stats.histo) {
        cout << "[";
        cout << std::setw(sizeDigits);
        cout << bucketSize.first << "][";
        cout << std::setw(countDigits);
        cout << bucketSize.second;
        cout << "]";
        length = int((double)maxLength * bucketSize.second / maxCount + 0.5f);
        for (int j = 0; j < length; ++j) {
            cout << '-';
        }
        cout << endl;
    }
}

bool testStats() {
    constexpr int size = 8192;

    int arr[size];
    for (int i = 0; i < size; ++i) {
        arr[i] = i;
    }
    Map<int, int> m1(size);
    Map<int, int> m2(size);
    for (int i = 0; i < size; ++i) {
        m1.emplace(i, arr[i]);
        m2.emplace_h(hash(&i, sizeof(int)), i, arr[i]);
    }
    m1.rehash(1024);
    m2.rehash(1024);

    cout << "standard..." << endl;
    MapStats stats1 = calcStats(m1);
    cout << "min:" << stats1.min << ", ";
    cout << "max:" << stats1.max << ", ";
    cout << "median:" << stats1.median << ", ";
    cout << "mean:" << stats1.mean << ", ";
    cout << "stddev:" << stats1.stddev << endl;
    printHisto(stats1);

    cout << "array..." << endl;
    MapStats stats2 = calcStats(m2);
    cout << "min:" << stats2.min << ", ";
    cout << "max:" << stats2.max << ", ";
    cout << "median:" << stats2.median << ", ";
    cout << "mean:" << stats2.mean << ", ";
    cout << "stddev:" << stats2.stddev << endl;
    printHisto(stats2);

    return true;
}

bool runTests() {

    cout << "Testing Default Constructor..." << endl << endl;
    if (!testDefaultConstructor()) {
        cout << "Default Constructor Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Constructor..." << endl << endl;
    if (!testConstructor()) {
        cout << "Constructor Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Copy Constructor..." << endl << endl;
    if (!testCopyConstructor()) {
        cout << "Copy Constructor Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Copy Assignment..." << endl << endl;
    if (!testCopyAssignment()) {
        cout << "Copy Assignment Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Move Constructor..." << endl << endl;
    if (!testMoveConstructor()) {
        cout << "Move Constructor Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Move Assignment..." << endl << endl;
    if (!testMoveAssignment()) {
        cout << "Move Assignment Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Pairs Constructor..." << endl << endl;
    if (!testPairsConstructor()) {
        cout << "Pairs Constructor Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Pairs Assignment..." << endl << endl;
    if (!testPairsConstructor()) {
        cout << "Pairs Assignment Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Range Constructor..." << endl << endl;
    if (!testRangeConstructor()) {
        cout << "Range Constructor Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Destructor..." << endl << endl;
    if (!testDestructor(10000)) {
        cout << "Destructor Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Swap..." << endl << endl;
    if (!testSwap()) {
        cout << "Swap Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Access..." << endl << endl;
    if (!testAccess()) {
        cout << "Access Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Insert..." << endl << endl;
    if (!testInsert()) {
        cout << "Insert Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Emplace..." << endl << endl;
    if (!testEmplace()) {
        cout << "Emplace Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing At..." << endl << endl;
    if (!testAt()) {
        cout << "At Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Find..." << endl << endl;
    if (!testFind()) {
        cout << "Find Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Access..." << endl << endl;
    if (!testAccess()) {
        cout << "Access Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Erase..." << endl << endl;
    if (!testErase()) {
        cout << "Erase Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Count..." << endl << endl;
    if (!testCount()) {
        cout << "Count Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Find Element..." << endl << endl;
    if (!testFindElement()) {
        cout << "Find Element Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Rehash..." << endl << endl;
    if (!testRehash()) {
        cout << "Rehash Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Clear..." << endl << endl;
    if (!testClear()) {
        cout << "Clear Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Equality..." << endl << endl;
    if (!testEquality()) {
        cout << "Equality Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Reference Nature..." << endl << endl;
    if (!testReferenceNature()) {
        cout << "Reference Nature Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Error Throws..." << endl << endl;
    if (!testErrorThrows()) {
        cout << "Error Throws Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Iterator..." << endl << endl;
    if (!testIterator()) {
        cout << "Iterator Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Map Map..." << endl << endl;
    if (!testMapMap()) {
        cout << "Map Map Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Pointer Hash..." << endl << endl;
    if (!testMapMap()) {
        cout << "Pointer Hash Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Stats..." << endl << endl;
    if (!testStats()) {
        cout << "Stats Test Failed!" << endl;
        return false;
    }
    cout << endl;

    return true;
}

}

#include <unordered_set>
int main() {
    setupMaps();

    if (runTests()) {
        cout << "All Tests Passed" << endl;
    }
    else {
        cout << "Testing Failed" << endl;
    }

    cout << endl;
    std::cin.get();
    return 0;
}