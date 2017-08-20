#include <string>
#include <iostream>
#include <unordered_map>

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
    for (int i = 0; i < 1000000; ++i) {
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
    Map<int, int> m1(10);
    if (m1.nSlots() != 10 || m1.size() != 0) return false;

    cout << "huge..." << endl;
    Map<int, int> m2(100000);
    if (m2.nSlots() != 100000 || m2.size() != 0) return false;

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
    if (&m1 == &f_map1 || !m1.equals(f_map1)) return false;

    cout << "string..." << endl;
    Map<int, string> m2(f_map2);
    if (&m2 == &f_map2 || !m2.equals(f_map2)) return false;

    cout << "huge..." << endl;
    Map<int, char> m4(f_map4);
    if (&m4 == &f_map4 || !m4.equals(f_map4)) return false;

    return true;
}

bool testCopyAssignment() {
    cout << "int..." << endl;
    Map<int, int> m1;
    m1 = f_map1;
    if (&m1 == &f_map1 || !m1.equals(f_map1)) return false;

    cout << "string..." << endl;
    Map<int, string> m2;
    m2 = f_map2;
    if (&m2 == &f_map2 || !m2.equals(f_map2)) return false;

    cout << "huge..." << endl;
    Map<int, char> m4;
    m4 = f_map4;
    if (&m4 == &f_map4 || !m4.equals(f_map4)) return false;

    return true;
}

bool testMoveConstructor() {
    cout << "int..." << endl;
    Map<int, int> h1(f_map1);
    Map<int, int> m1(std::move(h1));
    if (&m1 == &f_map1 || !m1.equals(f_map1)) return false;
    if (h1.nSlots() != 0 || h1.size() != 0) return false;

    cout << "string..." << endl;
    Map<int, string> h2(f_map2);
    Map<int, string> m2(std::move(h2));
    if (&m2 == &f_map2 || !m2.equals(f_map2)) return false;
    if (h2.nSlots() != 0 || h2.size() != 0) return false;

    cout << "huge..." << endl;
    Map<int, char> h4(f_map4);
    Map<int, char> m4(std::move(h4));
    if (&m4 == &f_map4 || !m4.equals(f_map4)) return false;
    if (h4.nSlots() != 0 || h4.size() != 0) return false;

    return true;
}

bool testMoveAssignment() {
    cout << "int..." << endl;
    Map<int, int> h1(f_map1);
    Map<int, int> m1;
    m1 = std::move(h1);
    if (&m1 == &f_map1 || !m1.equals(f_map1)) return false;
    if (h1.nSlots() != 0 || h1.size() != 0) return false;

    cout << "string..." << endl;
    Map<int, string> h2(f_map2);
    Map<int, string> m2;
    m2 = std::move(h2);
    if (&m2 == &f_map2 || !m2.equals(f_map2)) return false;
    if (h2.nSlots() != 0 || h2.size() != 0) return false;

    cout << "huge..." << endl;
    Map<int, char> h4(f_map4);
    Map<int, char> m4;
    m4 = std::move(h4);
    if (&m4 == &f_map4 || !m4.equals(f_map4)) return false;
    if (h4.nSlots() != 0 || h4.size() != 0) return false;

    return true;
}

bool testVariadicConstructor() {
    cout << "uniform..." << endl;
    Map<int, int> m1(std::initializer_list<std::pair<const int &, const int &>>{
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

    /*cout << "varying..." << endl;
    Map<int> m2( std::initializer_list<std::pair<const int &, const int &>>{
        { 7, 0 },
        { 'd', 2 },
        { 9.9, 3 },
        { 7.0f, 4 },
        { std::string("five"), 5 }
    });
    if (m2.at(7) != 0 ||
        m2.at('d') != 2 ||
        m2.at(9.9) != 3 ||
        m2.at(7.0f) != 4 ||
        m2.at(std::string("five")) != 5)
        return false;*/

    cout << "single pair..." << endl;
    Map<int, int> m3(std::initializer_list<std::pair<const int &, const int &>>{ { 77, 777 } });
    if (m3.at(77) != 777) return false;

    return true;
}

bool testDestructor() {
    Map<int, long long> * m1 = new Map<int, long long>(1000);
    for (int i = 0; i < 10000; ++i) {
        m1->insert_h(i, i);
    }
    delete m1;
    return true;
}

bool testInsert() {
    int arr[100];
    for (int i = 0; i < 100; ++i) {
        arr[i] = i;
    }

    cout << "int..." << endl;
    Map<int, int> m1;
    for (int i = 0; i < 100; ++i) {
        auto res = m1.insert(arr[i], arr[i]);
        if (*res.first != arr[i] || !res.second) return false;
        res = m1.insert(arr[i], arr[i]);
        if (*res.first != arr[i] || res.second) return false;
    }
    if (m1.size() != 100) return false;

    cout << "string..." << endl;
    Map<string, int> m2;
    for (int i = 0; i < 100; ++i) {
        auto res = m2.insert(string(1, char(i)), arr[i]);
        if (*res.first != arr[i] || !res.second) return false;
        res = m2.insert(string(1, char(i)), arr[i]);
        if (*res.first != arr[i] || res.second) return false;
    }
    if (m2.size() != 100) return false;

    cout << "initializer list..." << endl;
    Map<int, int> m3;
    for (int i = 0; i < 100; i += 2) {
        m3.insert({ { i, arr[i] }, { i + 1, arr[i + 1] } });
        if (m1.at(i) != arr[i] || m3.at(i + 1) != arr[i + 1]) return false;
    }
    if (m3.size() != 100) return false;

    return true;
}

bool testAt() {
    int arr[100];
    Map<int, int> m1;
    for (int i = 0; i < 100; ++i) {
        arr[i] = i;
        m1.insert(i, arr[i]);
    }

    cout << "int..." << endl;
    for (int i = 0; i < 100; ++i) {
        if (m1.at(i) != i) return false;
    }

    cout << "string..." << endl;
    Map<string, int> m2;
    int x = 777;
    m2.insert(string("okay"), x);
    if (m2.at(string("okay")) != 777) return false;

    return true;
}

bool testIteratorAccess() {
    int arr[100];
    Map<int, int> m1;
    for (int i = 0; i < 100; ++i) {
        arr[i] = i;
        m1.insert(i, arr[i]);
    }
    const Map<int, int> & m1_c(m1);

    cout << "int..." << endl;
    for (int i = 0; i < 100; ++i) {
        auto res(m1.iterator(i));
        if (res.hashKey() != hash(i) || res.element() != i) return false;
        if (res != m1_c.citerator(i)) return false;
    }

    cout << "string..." << endl;
    Map<string, int> m2;
    const Map<string, int> & m2_c(m2);
    m2.insert(string("okay"), 777);
    auto res(m2.iterator(string("okay")));
    if (res.hashKey() != hash(string("okay")) || res.element() != 777) return false;
    if (res != m2_c.citerator(string("okay"))) return false;
    if (m2.at(string("okay")) != 777) return false;

    return true;
}

bool testAccess() {
    int arr[100];
    Map<int, int> m1;
    for (int i = 0; i < 100; ++i) {
        arr[i] = i;
        m1[i] = arr[i];
    }

    cout << "int..." << endl;
    for (int i = 0; i < 100; ++i) {
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
    int arr[100];
    for (int i = 0; i < 100; ++i) {
        arr[i] = i;
    }

    Map<int, int> m1;
    for (int i = 0; i < 100; ++i) {
        m1.insert(i, arr[i]);
    }

    cout << "int..." << endl;
    for (int i = 0; i < 50; ++i) {
        if (m1.erase(i) != i) return false;
    }
    if (m1.size() != 50) return false;

    cout << "string..." << endl;
    Map<string, int> m2;
    m2.insert(string("okay"), 777);
    if (m2.erase(string("okay")) != 777) return false;
    if (m2.size() != 0) return false;

    return true;
}

bool testCount() {
    int arr[100];
    Map<int, int> m1;
    for (int i = 0; i < 100; ++i) {
        arr[i] = i;
        m1.insert(i, arr[i]);
    }

    cout << "int..." << endl;
    for (int i = 0; i < 100; ++i) {
        if (!m1.count(i)) return false;
    }

    cout << "string..." << endl;
    Map<string, int> m2;
    int x = 777;
    m2.insert(string("okay"), x);
    if (!m2.count(string("okay"))) return false;

    return true;
}

bool testFind() {
    int arr[100];
    for (int i = 0; i < 100; ++i) {
        arr[i] = i;
    }

    Map<int, int> m1;

    for (int i = 0; i < 10; ++i) {
        auto res(m1.find(arr[i]));
        if (res != m1.end()) return false;
        if (res != m1.cfind(arr[i])) return false;
        m1.insert(i, arr[i]);
    }
    for (int i = 0; i < 10; ++i) {
        auto res(m1.find(arr[i]));
        if (res == m1.end()) return false;
        if (res != m1.cfind(arr[i])) return false;
        m1.erase(i);
    }
    for (int i = 0; i < 10; ++i) {
        auto res(m1.find(arr[i]));
        if (res != m1.end()) return false;
        if (res != m1.cfind(arr[i])) return false;
    }

    return true;
}

bool testRehash() {
    int arr[100];
    for (int i = 0; i < 100; ++i) {
        arr[i] = i;
    }

    cout << "powers of two..." << endl;
    Map<int, int> m1(10);
    for (int i = 0; i < 10; ++i) {
        m1.insert(i, arr[i]);
    }
    if (m1.nSlots() != 10) return false;
    m1.insert(10, arr[10]);
    if (m1.nSlots() != 16) return false;
    for (int i = 11; i < 100; ++i) {
        m1.insert(i, arr[i]);
    }
    if (m1.nSlots() != 128) return false;

    cout << "larger..." << endl;
    m1.rehash(500);
    if (m1.nSlots() != 500 || m1.size() != 100) return false;
    for (int i = 0; i < 100; ++i) {
        if (m1.at(i) != arr[i]) return false;
    }

    cout << "smaller..." << endl;
    m1.rehash(10);
    if (m1.nSlots() != 10 || m1.size() != 100) return false;
    for (int i = 0; i < 100; ++i) {
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
    m2.clear();
    m2.rehash(1);
    m2.factor(3);
    if (m2.nSlots() != 1) return false;
    m2.insert(0, 0);
    if (m2.nSlots() != 1) return false;
    m2.insert(1, 1);
    if (m2.nSlots() != 1) return false;
    m2.insert(2, 2);
    if (m2.nSlots() != 1) return false;
    m2.insert(3, 3);
    if (m2.nSlots() != 2) return false;
    m2.insert(4, 4);
    if (m2.nSlots() != 2) return false;
    m2.insert(5, 5);
    if (m2.nSlots() != 2) return false;
    m2.insert(6, 6);
    if (m2.nSlots() != 4) return false;
    m2.clear();
    m2.rehash(1);
    m2.factor(1.0 / 3.0);
    if (m2.nSlots() != 1) return false;
    m2.insert(0, 0);
    if (m2.nSlots() != 4) return false;
    m2.insert(1, 1);
    if (m2.nSlots() != 8) return false;
    m2.insert(2, 2);
    if (m2.nSlots() != 16) return false;
    m2.insert(3, 3);
    if (m2.nSlots() != 16) return false;
    m2.insert(4, 4);
    if (m2.nSlots() != 16) return false;
    m2.insert(5, 5);
    if (m2.nSlots() != 32) return false;

    return true;
}

bool testClear() {
    int arr[100];

    Map<int, int> m1;
    for (int i = 0; i < 100; ++i) {
        m1.insert_h(i, arr[i]);
    }

    cout << "standard..." << endl;
    m1.clear();
    if (m1.size() != 0) return false;
    for (int i = 0; i < 100; ++i) {
        if (m1.find(arr[i])) return false;
    }

    cout << "empty..." << endl;
    m1.clear();
    if (m1.size() != 0) return false;

    return true;
}

bool testEquals() {
    int arr[100];
    for (int i = 0; i < 100; ++i) {
        arr[i] = i;
    }
    
    Map<int, int> m1;
    for (int i = 0; i < 100; ++i) {
        m1.insert_h(i, arr[i]);
    }

    cout << "equality..." << endl;
    Map<int, int> m2(m1);
    if (!m2.equals(m1)) return false;

    cout << "inequality..." << endl;
    m2.erase_h(0);
    if (m2.equals(m1)) return false;

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

bool testSeedNature() {
    Map<int, int> m1;

    for (int i = 0; i < 100; ++i) {
        m1.seed(i * i);
        if (!m1.insert(0, i).second) return false;
    }
    try {
        for (int i = 0; i < 100; ++i) {
            m1.seed(i * i);
            if (m1.at(0) != i) {
                return false;
            }
        }
    }
    catch (std::out_of_range ex) {
        return false;
    }

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

    cout << "x128..." << endl;
    Map<int, int, 16> m3;
    m3.insert(99, 7);
    if (m3.at(99) != 7) return false;
    if (!m3.count_h(hash<16, int>(99))) return false;

    return true;
}

bool testPrintContents() {
    int arr[100];
    for (int i = 0; i < 100; ++i) {
        arr[i] = i;
    }

    cout << "standard..." << endl;
    Map<int, int> m1;
    for (int i = 0; i < 30; ++i) {
        m1.insert_h(i, arr[i]);
    }
    cout << "value..." << endl;
    m1.printContents(cout, true, false, false);
    cout << "key..." << endl;
    m1.printContents(cout, false, true, false);
    cout << "address..." << endl;
    m1.printContents(cout, false, false, true);
    cout << "value & key..." << endl;
    m1.printContents(cout, true, true, false);
    cout << "key & address..." << endl;
    m1.printContents(cout, false, true, true);
    cout << "value & address..." << endl;
    m1.printContents(cout, true, false, true);
    cout << "all..." << endl;
    m1.printContents(cout, true, true, true);

    cout << "empty..." << endl;
    Map<int, int> m2;
    m2.printContents(cout, true, true, true);

    cout << "too many elements..." << endl;
    Map<int, int> m3;
    for (int i = 0; i < 100; ++i) {
        m3.insert_h(i, arr[i]);
    }
    m3.printContents(cout, true, true, true);

    cout << "too many slots..." << endl;
    Map<int, int> m4(100);
    for (unsigned int i = 0; i < 100; ++i) {
        m4.insert_h(i, arr[i]);
    }
    m4.printContents(cout, true, true, true);

    return true;
}

bool testIterator() {
    struct Test { int v; };
    cout << "standard..." << endl;
    Map<int, Test> m1;
    for (int i = 0; i < 100; ++i) {
        m1.insert_h(i, { i });
    }
    m1.rehash(10);
    int i = 0;
    for (auto it = m1.begin(); it; ++it) {
        if (it->v != i % 10 * 10 + i / 10) return false;
        if ((*it).v != it->v) return false;
        if (it.element().v != it->v) return false;
        if (it.hashKey() != i % 10 * 10 + i / 10) return false;
        (*it).v *= 2;
        ++i;
    }

    cout << "const..." << endl;
    const Map<int, Test> * mp = &m1;
    i = 0;
    for (auto it = mp->cbegin(); it; ++it) {
        if (it->v != 2 * (i % 10 * 10 + i / 10)) return false;
        if ((*it).v != it->v) return false;
        //(*it).v *= 2;
        ++i;
    }

    cout << "conversion..." << endl;
    Map<int, Test>::Iterator mit1 = m1.begin();
    Map<int, Test>::CIterator cit1 = m1.cbegin();
    mit1 = cit1;

    Map<int, Test>::Iterator mit2(mit1);
    mit2 = mit1;
    Map<int, Test>::CIterator cit2(cit1);
    cit2 = cit1;
    Map<int, Test>::Iterator mit3(std::move(mit1));
    mit3 = std::move(mit1);
    Map<int, Test>::CIterator cit3(std::move(cit1));
    cit3 = std::move(cit1);

    return true;
}

bool testStats() {
    int arr[10000];
    for (int i = 0; i < 10000; ++i) {
        arr[i] = i;
    }
    Map<int, int> m1(1000, true);
    Map<int, int> m2(1000, true);
    for (int i = 0; i < 10000; ++i) {
        m1.insert(i, arr[i]);
        m2.insert_h(hashv(&i, 1), arr[i]);
    }

    cout << "standard..." << endl;
    Map<int, int>::MapStats stats1 = m1.stats();
    cout << "min:" << stats1.min << ", ";
    cout << "max:" << stats1.max << ", ";
    cout << "median:" << stats1.median << ", ";
    cout << "mean:" << stats1.mean << ", ";
    cout << "stddev:" << stats1.stddev << endl;
    Map<int, int>::printHisto(stats1, cout);

    cout << "array..." << endl;
    Map<int, int>::MapStats stats2 = m2.stats();
    cout << "min:" << stats2.min << ", ";
    cout << "max:" << stats2.max << ", ";
    cout << "median:" << stats2.median << ", ";
    cout << "mean:" << stats2.mean << ", ";
    cout << "stddev:" << stats2.stddev << endl;
    Map<int, int>::printHisto(stats2, cout);

    /*
    cout << "huge..." << endl;
    Map<char> m3(1000000, true);
    for (int i = 0; i < 10000000; ++i) {
        m3.insert(i, 0);
    }
    Map<char>::MapStats stats3 = m3.stats();
    cout << "min:" << stats3.min << ", ";
    cout << "max:" << stats3.max << ", ";
    cout << "median:" << stats3.median << ", ";
    cout << "mean:" << stats3.mean << ", ";
    cout << "stddev:" << stats3.stddev << endl;
    Map<char>::printHisto(stats3, cout);
    */

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

    cout << "Testing Variadic Constructor..." << endl << endl;
    if (!testVariadicConstructor()) {
        cout << "Variadic Constructor Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Destructor..." << endl << endl;
    if (!testDestructor()) {
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

    cout << "Testing At..." << endl << endl;
    if (!testAt()) {
        cout << "At Test Failed!" << endl;
        return false;
    }
    cout << endl;

    cout << "Testing Iterator Access..." << endl << endl;
    if (!testIteratorAccess()) {
        cout << "Iterator Access Test Failed!" << endl;
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

    cout << "Testing Find..." << endl << endl;
    if (!testFind()) {
        cout << "Find Test Failed!" << endl;
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

    cout << "Testing Equals..." << endl << endl;
    if (!testEquals()) {
        cout << "Equals Test Failed!" << endl;
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

    cout << "Testing Seed Nature..." << endl << endl;
    if (!testSeedNature()) {
        cout << "Seed Nature Test Failed!" << endl;
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

    cout << "Testing PrintContents..." << endl << endl;
    if (!testPrintContents()) {
        cout << "PrintContents Test Failed!" << endl;
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
    system("pause");
    return 0;
}