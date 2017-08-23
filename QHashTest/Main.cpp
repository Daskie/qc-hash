#include <string>
#include <iostream>
#include <unordered_map>
#include <iomanip>

#include "QHash/Map.hpp"

using std::string;
using std::cout;
using std::endl;
using std::unordered_map;

using namespace qmu;

namespace {

Map<int, int> f_map1;
Map<int, string> f_map2;
Map<int, char> f_map4;

void setupMaps() {
    for (int i = 0; i < 10; ++i) {
        f_map1.insert_h(i, i);
        f_map2.insert_h(i, "" + i);
    }
    for (int i = 0; i < 1000; ++i) {
        f_map4.insert_h(i, i % 256);
    }
}

bool testDefaultConstructor() {
    Map<int, int> m;
    if (m.nSlots() != config::map::defNSlots || m.size() != 0) return false;	
    return true;
}

bool testConstructor() {
    cout << "small..." << endl;
    Map<int, int> m1(3);
    if (m1.nSlots() != 4 || m1.size() != 0) return false;

    cout << "large..." << endl;
    Map<int, int> m2(1000);
    if (m2.nSlots() != 1024 || m2.size() != 0) return false;

    cout << "zero..." << endl;
    Map<int, int> m3(0);
    if (m3.nSlots() != 1 || m3.size() != 0) return false;

    cout << "negative..." << endl;
    Map<int, int> m4(-1);
    if (m4.nSlots() != 1 || m4.size() != 0) return false;

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
    if (h1.nSlots() != 0 || h1.size() != 0) return false;

    cout << "string..." << endl;
    Map<int, string> h2(f_map2);
    Map<int, string> m2(std::move(h2));
    if (m2 != f_map2) return false;
    if (h2.nSlots() != 0 || h2.size() != 0) return false;

    cout << "large..." << endl;
    Map<int, char> h4(f_map4);
    Map<int, char> m4(std::move(h4));
    if (m4 != f_map4) return false;
    if (h4.nSlots() != 0 || h4.size() != 0) return false;

    return true;
}

bool testMoveAssignment() {
    cout << "int..." << endl;
    Map<int, int> h1(f_map1);
    Map<int, int> m1;
    m1 = std::move(h1);
    if (m1 != f_map1) return false;
    if (h1.nSlots() != 0 || h1.size() != 0) return false;

    cout << "string..." << endl;
    Map<int, string> h2(f_map2);
    Map<int, string> m2;
    m2 = std::move(h2);
    if (m2 != f_map2) return false;
    if (h2.nSlots() != 0 || h2.size() != 0) return false;

    cout << "large..." << endl;
    Map<int, char> h4(f_map4);
    Map<int, char> m4;
    m4 = std::move(h4);
    if (m4 != f_map4) return false;
    if (h4.nSlots() != 0 || h4.size() != 0) return false;

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

bool testRangeConstructor() {
    std::vector<std::pair<const int, int>> pairs;
    for (int i(0); i < 100; ++i) {
        pairs.push_back({ i, 100 - i });
    }

    Map<int, int> m(pairs.cbegin(), pairs.cend());
    if (m.nSlots() != 128 || m.size() != 100) return false;

    for (int i(0); i < 100; ++i) {
        if (m.at(i) != 100 - i) return false;
    }

    return true;
}

bool testDestructor(nat n) {
    Map<int, long long> * m1 = new Map<int, long long>(n);
    for (int i = 0; i < n; ++i) {
        m1->insert_h(i, i);
    }
    delete m1;
    return true;
}

bool testInsert() {
    int arr[128];
    for (int i = 0; i < 128; ++i) {
        arr[i] = i;
    }

    cout << "int..." << endl;
    Map<int, int> m1;
    for (int i = 0; i < 128; ++i) {
        auto res = m1.insert(arr[i], arr[i]);
        if (*res.first != arr[i] || !res.second) return false;
        res = m1.insert(arr[i], arr[i]);
        if (*res.first != arr[i] || res.second) return false;
    }
    if (m1.size() != 128) return false;

    cout << "string..." << endl;
    Map<string, int> m2;
    for (int i = 0; i < 128; ++i) {
        auto res = m2.insert(string(1, char(i)), arr[i]);
        if (*res.first != arr[i] || !res.second) return false;
        res = m2.insert(string(1, char(i)), arr[i]);
        if (*res.first != arr[i] || res.second) return false;
    }
    if (m2.size() != 128) return false;

    cout << "initializer list..." << endl;
    Map<int, int> m3;
    for (int i = 0; i < 128; i += 2) {
        m3.insert(std::initializer_list<std::pair<int, int>>{ { i, arr[i] }, { i + 1, arr[i + 1] } });
        if (m3.at(i) != arr[i] || m3.at(i + 1) != arr[i + 1]) return false;
    }
    if (m3.size() != 128) return false;

    Map<int, int> m33;
    int ints[]{ 0, 1, 2, 3 };
    std::initializer_list<std::pair<const int &, const int &>> crpairs{ { ints[0], ints[1] }, { ints[2], ints[3] } };
    m33.insert(crpairs);
    if (m33.at(0) != 1 || m33.at(2) != 3) return false;

    cout << "range..." << endl;
    Map<int, int> m4;
    std::vector<std::pair<int, int>> vec;
    for (int i(0); i < 128; ++i) vec.push_back({ i, i });
    m4.insert(vec.cbegin(), vec.cend());
    for (int i(0); i < 128; ++i) {
        if (m4.at(i) != i) return false;
    }
    if (m4.size() != 128) return false;

    return true;
}

bool testEmplace() {
    Map<nat, std::unique_ptr<nat>> m;

    m.emplace(0, std::make_unique<nat>(7_n));

    if (*m.at(0) != 7) return false;
    if (*m[0] != 7) return false;
    if (**m.begin() != 7) return false;
    if (**m.cbegin() != 7) return false;

    std::unique_ptr<nat> np(m.erase(0));
    if (*np != 7) return false;

    return true;
}

bool testAt() {
    int arr[128];
    Map<int, int> m1;
    for (int i = 0; i < 128; ++i) {
        arr[i] = i;
        m1.insert(i, arr[i]);
    }

    cout << "int..." << endl;
    for (int i = 0; i < 128; ++i) {
        if (m1.at(i) != i) return false;
    }

    cout << "string..." << endl;
    Map<string, int> m2;
    int x = 777;
    m2.insert(string("okay"), x);
    if (m2.at(string("okay")) != 777) return false;

    return true;
}

bool testFind() {
    int arr[128];
    Map<int, int> m1;
    for (int i = 0; i < 128; ++i) {
        arr[i] = i;
        m1.insert(i, arr[i]);
    }
    const Map<int, int> & m1_c(m1);

    cout << "int..." << endl;
    for (int i = 0; i < 128; ++i) {
        auto res(m1.find(i));
        if (res.hash() != hash(i) || res.element() != i) return false;
        if (res != m1_c.find(i)) return false;
    }

    cout << "string..." << endl;
    Map<string, int> m2;
    const Map<string, int> & m2_c(m2);
    m2.insert(string("okay"), 777);
    auto res(m2.find(string("okay")));
    if (res.hash() != hash(string("okay")) || res.element() != 777) return false;
    if (res != m2_c.find(string("okay"))) return false;
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
        m1.insert(i, arr[i]);
    }

    cout << "int..." << endl;
    for (int i = 0; i < 64; ++i) {
        if (m1.erase(i) != i) return false;
    }
    if (m1.size() != 64) return false;

    cout << "string..." << endl;
    Map<string, int> m2;
    m2.insert(string("okay"), 777);
    if (m2.erase(string("okay")) != 777) return false;
    if (m2.size() != 0) return false;

    return true;
}

bool testCount() {
    int arr[128];
    Map<int, int> m1;
    for (int i = 0; i < 128; ++i) {
        arr[i] = i;
        m1.insert(i, arr[i]);
    }

    cout << "int..." << endl;
    for (int i = 0; i < 128; ++i) {
        if (!m1.count(i)) return false;
    }

    cout << "string..." << endl;
    Map<string, int> m2;
    int x = 777;
    m2.insert(string("okay"), x);
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
        auto res(m1.findElement(arr[i]));
        if (res != m1.end()) return false;
        if (res != m1.findElement(arr[i])) return false;
        m1.insert(i, arr[i]);
    }
    for (int i = 0; i < 10; ++i) {
        auto res(m1.findElement(arr[i]));
        if (res == m1.end()) return false;
        if (res != m1.findElement(arr[i])) return false;
        m1.erase(i);
    }
    for (int i = 0; i < 10; ++i) {
        auto res(m1.findElement(arr[i]));
        if (res != m1.end()) return false;
        if (res != m1.findElement(arr[i])) return false;
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
        m1.insert(i, arr[i]);
    }
    if (m1.nSlots() != 16) return false;
    m1.insert(16, arr[16]);
    if (m1.nSlots() != 32) return false;
    for (int i = 17; i < 128; ++i) {
        m1.insert(i, arr[i]);
    }
    if (m1.nSlots() != 128) return false;

    cout << "larger..." << endl;
    m1.rehash(500);
    if (m1.nSlots() != 512 || m1.size() != 128) return false;
    for (int i = 0; i < 128; ++i) {
        if (m1.at(i) != arr[i]) return false;
    }

    cout << "smaller..." << endl;
    m1.rehash(10);
    if (m1.nSlots() != 16 || m1.size() != 128) return false;
    for (int i = 0; i < 128; ++i) {
        if (m1.at(i) != arr[i]) return false;
    }

    cout << "bonus..." << endl;
    Map<int, int> m2(1);
    if (m2.nSlots() != 1) return false;
    m2.insert(0, 0);
    if (m2.nSlots() != 1) return false;
    m2.insert(1, 1);
    if (m2.nSlots() != 2) return false;
    m2.insert(2, 2);
    if (m2.nSlots() != 4) return false;
    m2.insert(3, 3);
    if (m2.nSlots() != 4) return false;
    m2.insert(4, 4);
    if (m2.nSlots() != 8) return false;

    return true;
}

bool testClear() {
    int arr[128];

    Map<int, int> m1;
    for (int i = 0; i < 128; ++i) {
        m1.insert_h(i, arr[i]);
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
        m1.insert_h(i, arr[i]);
    }

    cout << "equality..." << endl;
    Map<int, int> m2(m1);
    if (m2 != m1) return false;

    cout << "inequality..." << endl;
    m2.erase_h(0);
    if (m2 == m1) return false;

    return true;
}

bool testFixed() {
    Map<int, int> m1(1, false);
    m1.insert(0, 0);
    m1.insert(1, 1);
    if (m1.nSlots() != 2) return false;

    Map<int, int> m2(1, true);
    m2.insert(0, 0);
    m2.insert(1, 1);
    if (m2.nSlots() != 1) return false;

    return true;
}

bool testReferenceNature() {
    Map<int, int> m1;
    m1.insert(777, 7);
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
    try {
        m1.erase(0);
        return false;
    }
    catch (const std::out_of_range &) {}

    return true;
}

bool testPrecisions() {
    cout << "x32..." << endl;
    Map<int, int, 4> m1;
    m1.insert(99, 7);
    if (m1.at(99) != 7) return false;
    if (!m1.at_h(hash<4, int>(99))) return false;

    cout << "x64..." << endl;
    Map<int, int, 8> m2;
    m2.insert(99, 7);
    if (m2.at(99) != 7) return false;
    if (!m2.count_h(hash<8, int>(99))) return false;

    return true;
}

bool testIterator() {
    struct Test { int v; };
    cout << "standard..." << endl;
    Map<int, Test> m1;
    for (int i = 0; i < 64; ++i) {
        m1.insert_h(i, { i });
    }
    m1.rehash(8);
    int i = 0;
    for (auto it = m1.begin(); it; ++it) {
        if (it->v != i % 8 * 8 + i / 8) return false;
        if ((*it).v != it->v) return false;
        if (it.element().v != it->v) return false;
        if (it.hash() != i % 8 * 8 + i / 8) return false;
        (*it).v *= 2;
        ++i;
    }

    cout << "const..." << endl;
    const Map<int, Test> * mp = &m1;
    i = 0;
    for (auto it = mp->cbegin(); it; ++it) {
        if (it->v != 2 * (i % 8 * 8 + i / 8)) return false;
        if ((*it).v != it->v) return false;
        //(*it).v *= 2;
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
    Map<nat, nat> m5;
    for (const auto & e : m5) {}
    for (nat i(0); i < 64; ++i) {
        m5.insert_h(i, i);
    }
    m5.rehash(8);
    i = 0;
    for (auto & e : m5) {
        if (e != i % 8 * 8 + i / 8) return false;
        e *= 2;
        if (e != 2 * (i % 8 * 8 + i / 8)) return false;
        ++i;
    }

    return true;
}

struct MapStats {
    nat min, max, median;
    double mean, stddev;
    std::vector<nat> histo;
};

template <typename K, typename E, nat t_p>
typename MapStats calcStats(const Map<K, E, t_p> & map) {
    std::vector<nat> slotSizes;
    for (nat i(0); i < map.nSlots(); ++i) {
        slotSizes.push_back(map.slotSize(i));
    }

    nat min = slotSizes[0];
    nat max = slotSizes[0];
    nat median = slotSizes[0];
    double mean = (double)slotSizes[0];
    double stddev = 0.0;

    nat total = 0;
    for (nat i = 0; i < map.nSlots(); ++i) {
        if (slotSizes[i] < min) {
            min = slotSizes[i];
        }
        else if (slotSizes[i] > max) {
            max = slotSizes[i];
        }

        total += slotSizes[i];
    }
    mean = (double)total / map.nSlots();

    std::vector<nat> sizeCounts(max - min + 1, 0);
    for (nat i = 0; i < map.nSlots(); ++i) {
        ++sizeCounts[slotSizes[i] - min];

        stddev += (slotSizes[i] - mean) * (slotSizes[i] - mean);
    }
    stddev /= map.nSlots();
    stddev = std::sqrt(stddev);

    median = min;
    for (nat i = 1; i < max - min + 1; ++i) {
        if (sizeCounts[i] > sizeCounts[median - min]) {
            median = i + min;
        }
    }

    return {
        min, max, median,
        mean, stddev,
        sizeCounts
    };
}

void printHisto(const MapStats & stats) {
    nat sizeDigits = stats.max ? (nat)log10(stats.max) + 1 : 1;
    nat maxCount = stats.histo[stats.median - stats.min];
    nat countDigits = maxCount ? (nat)log10(maxCount) + 1 : 1;
    nat maxLength = 80 - sizeDigits - countDigits - 5; // 5 is for "[][]" & \n
    nat length;
    for (nat i = stats.min; i < stats.max + 1; ++i) {
        std::cout << "[";
        std::cout << std::setw(sizeDigits);
        std::cout << i << "][";
        std::cout << std::setw(countDigits);
        std::cout << stats.histo[i - stats.min];
        std::cout << "]";
        length = nat((double)maxLength * stats.histo[i - stats.min] / maxCount + 0.5f);
        for (nat j = 0; j < length; ++j) {
            std::cout << '-';
        }
        std::cout << endl;
    }
}

bool testStats() {
    constexpr nat size = 8192;

    int arr[size];
    for (int i = 0; i < size; ++i) {
        arr[i] = i;
    }
    Map<int, int> m1(size / 16, true);
    Map<int, int> m2(size / 16, true);
    for (int i = 0; i < size; ++i) {
        m1.insert(i, arr[i]);
        m2.insert_h(hashv(&i, 1), arr[i]);
    }

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

    cout << "Testing Fixed..." << endl << endl;
    if (!testFixed()) {
        cout << "Fixed Test Failed!" << endl;
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

    cout << "Testing Precisions..." << endl << endl;
    if (!testPrecisions()) {
        cout << "Precisions Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Iterator..." << endl << endl;
    if (!testIterator()) {
        cout << "Iterator Test Failed!" << endl;
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