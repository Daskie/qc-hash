#include <iostream>
#include <string>
#include <chrono>
#include <unordered_map>
#include <iomanip>
#include <memory>

#include "QHash/Map.hpp"
#include "QMU/Time.hpp"



using namespace qmu;
   


struct PerfPoint {
        
    double t1, t2;
    nat n;
    double p;

    PerfPoint() :
        t1(0.0), t2(0.0), n(0), p(1.0)
    {}

    PerfPoint(double t1, double t2, nat n) :
        t1(t1), t2(t2), n(n), p(t1 / t2)
    {}
    
};



struct PerfPrint {

    double p;

    PerfPrint(double p) : p(p) {}        

    friend std::ostream & operator<<(std::ostream & os, const PerfPrint & p) {
        if (p.p < 1.0) {
            os << "+" << (1.0 / p.p) << "x";
        }
        else if (p.p > 1.0) {
            os << "-" << p.p << "x";
        }
        else {
            os << " " << p.p << "x";
        }
        return os;
    }

};



enum class HashK { s08, s16, s32, s64, array, N};

constexpr nat k_nHashKs = static_cast<nat>(HashK::N);



struct HashPerf {

    PerfPoint points[k_nHashKs];
    double avg;

    PerfPoint & at(HashK k) {
        return points[static_cast<nat>(k)];
    }

    void calcAvg() {
        avg = 0.0;
        for (nat k(0); k < static_cast<nat>(HashK::N); ++k) {
            avg += points[k].p;
        }
        avg /= k_nHashKs;
    }

    void report() {
        calcAvg();

        std::cout << std::fixed << std::setprecision(2);

        std::cout << "Hash Performance" << std::endl;
        std::cout << std::endl;
        std::cout << "  K [  s08]: " << PerfPrint(at(HashK::s08).p) << std::endl;
        std::cout << "  K [  s16]: " << PerfPrint(at(HashK::s16).p) << std::endl;
        std::cout << "  K [  s32]: " << PerfPrint(at(HashK::s32).p) << std::endl;
        std::cout << "  K [  s64]: " << PerfPrint(at(HashK::s64).p) << std::endl;
        std::cout << "  K [array]: " << PerfPrint(at(HashK::array).p) << std::endl;

        std::cout << std::endl << std::endl;

        std::cout << "Hash Performance Overall: " << PerfPrint(avg) << std::endl;
    }

};



template <typename T>
PerfPoint testHashPerfPrim(nat n) {
    nat nInts(n * sizeof(T) / sizeof(int));
    n = nInts * sizeof(int) / sizeof(T);

    std::unique_ptr<T[]> keys(std::make_unique<T[]>(n));
    int * arr(reinterpret_cast<int *>(keys.get()));
    for (nat i(0); i < nInts; ++i) {
        arr[i] = std::rand();
    }

    volatile nat x1;
    double then(now());
    for (nat i(0); i < n; ++i) {
        x1 = hash(keys[i], 0);
    }
    double t1(now() - then);

    std::hash<T> stdHash;
    volatile size_t x2;
    then = now();
    for (nat i(0); i < n; ++i) {
        x2 = stdHash(keys[i]);
    }
    double t2(now() - then);

    return PerfPoint(t1, t2, n);
}

PerfPoint testHashPerfArray(nat n) {
    n = n / sizeof(int) * sizeof(int);

    std::string str(n, 0);
    int * arr(reinterpret_cast<int *>(const_cast<char *>(str.c_str())));

    for (nat i(0); i < n / static_cast<nat>(sizeof(int)); ++i) {
        arr[i] = std::rand();
    }

    volatile nat x1;
    double then(now());
    x1 = hash(str, 0);
    double t1(now() - then);

    std::hash<std::string> stdHash;
    volatile size_t x2;
    then = now();
    x2 = stdHash(str);
    double t2(now() - then);

    return PerfPoint(t1, t2, n);
}



void testHashPerformance() {
    HashPerf perf;

    perf.at(HashK::s08) = testHashPerfPrim<s08>(10000000);
    perf.at(HashK::s16) = testHashPerfPrim<s16>(10000000);
    perf.at(HashK::s32) = testHashPerfPrim<s32>(10000000);
    perf.at(HashK::s64) = testHashPerfPrim<s64>(10000000);
    perf.at(HashK::array) = testHashPerfArray(10000000);

    perf.report();
}



enum class MapF { insert, at, iterator, count, erase, N };
enum class MapK { s08, s16, s32, s64, string, N};

constexpr nat k_nMapFs = static_cast<nat>(MapF::N);
constexpr nat k_nMapKs = static_cast<nat>(MapK::N);

struct MapPerf {

    PerfPoint points[k_nMapFs][k_nMapKs];
    double funcAvgs[k_nMapFs];
    double keyAvgs[k_nMapKs];
    double overallAvg;

    PerfPoint & at(MapF f, MapK k) {
        return points[static_cast<nat>(f)][static_cast<nat>(k)];
    }

    void calcAvgs() {
        std::memset(funcAvgs, 0, k_nMapFs * sizeof(double));
        std::memset(keyAvgs, 0, k_nMapKs * sizeof(double));
        overallAvg = 0.0;

        for (nat f(0); f < k_nMapFs; ++f) {
            for (nat k(0); k < k_nMapKs; ++k) {
                funcAvgs[f] += points[f][k].p / k_nMapKs;
                keyAvgs[k] += points[f][k].p / k_nMapFs;
                overallAvg += points[f][k].p / (k_nMapFs * k_nMapKs);
            }
        }
    }

    void report() {
        calcAvgs();

        std::cout << std::fixed << std::setprecision(2);

        std::cout << "Map Function Performance" << std::endl;
        std::cout << std::endl;
        std::cout << "  Insert: " << PerfPrint(funcAvgs[static_cast<nat>(MapF::insert)]) << std::endl;
        std::cout << "    K [   s08]: " << PerfPrint(at(MapF::insert, MapK::s08).p) << std::endl;
        std::cout << "    K [   s16]: " << PerfPrint(at(MapF::insert, MapK::s16).p) << std::endl;
        std::cout << "    K [   s32]: " << PerfPrint(at(MapF::insert, MapK::s32).p) << std::endl;
        std::cout << "    K [   s64]: " << PerfPrint(at(MapF::insert, MapK::s64).p) << std::endl;
        std::cout << "    K [string]: " << PerfPrint(at(MapF::insert, MapK::string).p) << std::endl;
        std::cout << std::endl;
        std::cout << "  At: " << PerfPrint(funcAvgs[static_cast<nat>(MapF::at)]) << std::endl;
        std::cout << "    K [   s08]: " << PerfPrint(at(MapF::at, MapK::s08).p) << std::endl;
        std::cout << "    K [   s16]: " << PerfPrint(at(MapF::at, MapK::s16).p) << std::endl;
        std::cout << "    K [   s32]: " << PerfPrint(at(MapF::at, MapK::s32).p) << std::endl;
        std::cout << "    K [   s64]: " << PerfPrint(at(MapF::at, MapK::s64).p) << std::endl;
        std::cout << "    K [string]: " << PerfPrint(at(MapF::at, MapK::string).p) << std::endl;
        std::cout << std::endl;
        std::cout << "  Iterator: " << PerfPrint(funcAvgs[static_cast<nat>(MapF::iterator)]) << std::endl;
        std::cout << "    K [   s08]: " << PerfPrint(at(MapF::iterator, MapK::s08).p) << std::endl;
        std::cout << "    K [   s16]: " << PerfPrint(at(MapF::iterator, MapK::s16).p) << std::endl;
        std::cout << "    K [   s32]: " << PerfPrint(at(MapF::iterator, MapK::s32).p) << std::endl;
        std::cout << "    K [   s64]: " << PerfPrint(at(MapF::iterator, MapK::s64).p) << std::endl;
        std::cout << "    K [string]: " << PerfPrint(at(MapF::iterator, MapK::string).p) << std::endl;
        std::cout << std::endl;
        std::cout << "  Count: " << PerfPrint(funcAvgs[static_cast<nat>(MapF::count)]) << std::endl;
        std::cout << "    K [   s08]: " << PerfPrint(at(MapF::count, MapK::s08).p) << std::endl;
        std::cout << "    K [   s16]: " << PerfPrint(at(MapF::count, MapK::s16).p) << std::endl;
        std::cout << "    K [   s32]: " << PerfPrint(at(MapF::count, MapK::s32).p) << std::endl;
        std::cout << "    K [   s64]: " << PerfPrint(at(MapF::count, MapK::s64).p) << std::endl;
        std::cout << "    K [string]: " << PerfPrint(at(MapF::count, MapK::string).p) << std::endl;
        std::cout << std::endl;
        std::cout << "  Erase: " << PerfPrint(funcAvgs[static_cast<nat>(MapF::erase)]) << std::endl;
        std::cout << "    K [   s08]: " << PerfPrint(at(MapF::erase, MapK::s08).p) << std::endl;
        std::cout << "    K [   s16]: " << PerfPrint(at(MapF::erase, MapK::s16).p) << std::endl;
        std::cout << "    K [   s32]: " << PerfPrint(at(MapF::erase, MapK::s32).p) << std::endl;
        std::cout << "    K [   s64]: " << PerfPrint(at(MapF::erase, MapK::s64).p) << std::endl;
        std::cout << "    K [string]: " << PerfPrint(at(MapF::erase, MapK::string).p) << std::endl;

        std::cout << std::endl << std::endl;

        std::cout << "Map Key Performance" << std::endl;
        std::cout << std::endl;
        std::cout << "  S08: " << PerfPrint(keyAvgs[static_cast<nat>(MapK::s08)]) << std::endl;
        std::cout << "    F [  access]: " << PerfPrint(at(MapF::  insert, MapK::s08).p) << std::endl;
        std::cout << "    F [      at]: " << PerfPrint(at(MapF::      at, MapK::s08).p) << std::endl;
        std::cout << "    F [iterator]: " << PerfPrint(at(MapF::iterator, MapK::s08).p) << std::endl;
        std::cout << "    F [   count]: " << PerfPrint(at(MapF::   count, MapK::s08).p) << std::endl;
        std::cout << "    F [   erase]: " << PerfPrint(at(MapF::   erase, MapK::s08).p) << std::endl;
        std::cout << std::endl;
        std::cout << "  S16: " << PerfPrint(keyAvgs[static_cast<nat>(MapK::s16)]) << std::endl;
        std::cout << "    F [  access]: " << PerfPrint(at(MapF::  insert, MapK::s16).p) << std::endl;
        std::cout << "    F [      at]: " << PerfPrint(at(MapF::      at, MapK::s16).p) << std::endl;
        std::cout << "    F [iterator]: " << PerfPrint(at(MapF::iterator, MapK::s16).p) << std::endl;
        std::cout << "    F [   count]: " << PerfPrint(at(MapF::   count, MapK::s16).p) << std::endl;
        std::cout << "    F [   erase]: " << PerfPrint(at(MapF::   erase, MapK::s16).p) << std::endl;
        std::cout << std::endl;
        std::cout << "  S32: " << PerfPrint(keyAvgs[static_cast<nat>(MapK::s32)]) << std::endl;
        std::cout << "    F [  access]: " << PerfPrint(at(MapF::  insert, MapK::s32).p) << std::endl;
        std::cout << "    F [      at]: " << PerfPrint(at(MapF::      at, MapK::s32).p) << std::endl;
        std::cout << "    F [iterator]: " << PerfPrint(at(MapF::iterator, MapK::s32).p) << std::endl;
        std::cout << "    F [   count]: " << PerfPrint(at(MapF::   count, MapK::s32).p) << std::endl;
        std::cout << "    F [   erase]: " << PerfPrint(at(MapF::   erase, MapK::s32).p) << std::endl;
        std::cout << std::endl;
        std::cout << "  S64: " << PerfPrint(keyAvgs[static_cast<nat>(MapK::s64)]) << std::endl;
        std::cout << "    F [  access]: " << PerfPrint(at(MapF::  insert, MapK::s64).p) << std::endl;
        std::cout << "    F [      at]: " << PerfPrint(at(MapF::      at, MapK::s64).p) << std::endl;
        std::cout << "    F [iterator]: " << PerfPrint(at(MapF::iterator, MapK::s64).p) << std::endl;
        std::cout << "    F [   count]: " << PerfPrint(at(MapF::   count, MapK::s64).p) << std::endl;
        std::cout << "    F [   erase]: " << PerfPrint(at(MapF::   erase, MapK::s64).p) << std::endl;
        std::cout << std::endl;
        std::cout << "  String: " << PerfPrint(keyAvgs[static_cast<nat>(MapK::string)]) << std::endl;
        std::cout << "    F [  access]: " << PerfPrint(at(MapF::  insert, MapK::string).p) << std::endl;
        std::cout << "    F [      at]: " << PerfPrint(at(MapF::      at, MapK::string).p) << std::endl;
        std::cout << "    F [iterator]: " << PerfPrint(at(MapF::iterator, MapK::string).p) << std::endl;
        std::cout << "    F [   count]: " << PerfPrint(at(MapF::   count, MapK::string).p) << std::endl;
        std::cout << "    F [   erase]: " << PerfPrint(at(MapF::   erase, MapK::string).p) << std::endl;

        std::cout << std::endl << std::endl;

        std::cout << "Map Performance Overall: " << PerfPrint(overallAvg) << std::endl;
    }

};



template <typename T> struct Generator;
template <> struct Generator<s08> {
    s08 v = 0;
    void next() { ++v; }
    void reset() { v = 0; }
};

template <>
struct Generator<s16> {
    s16 v = 0;
    void next() { ++v; }
    void reset() { v = 0; }
};

template <>
struct Generator<s32> {
    s32 v = 0;
    void next() { ++v; }
    void reset() { v = 0; }
};

template <>
struct Generator<s64> {
    s64 v = 0;
    void next() { ++v; }
    void reset() { v = 0; }
};

template <>
struct Generator<std::string> {
    u64 i = 0;
    std::string v;
    void next() { v = std::string(reinterpret_cast<char *>(&++i), 8); }
    void reset() { i = 0; v = std::string(); }
};



template <typename T>
PerfPoint testMapPerfInsert(nat n, Map<nat> & m1, std::unordered_map<T, nat> & m2) {
    Generator<T> g;

    double then(now());
    for (nat i(0); i < n; ++i) {
        m1.insert(g.v, i);
        g.next();
    }
    double t1(now() - then);

    g.reset();

    then = now();
    for (nat i(0); i < n; ++i) {
        m2.insert({ g.v, i });
        g.next();
    }
    double t2(now() - then);
    
    return PerfPoint(t1, t2, n);
}

template <typename T>
PerfPoint testMapPerfAt(nat n, Map<nat> & m1, std::unordered_map<T, nat> & m2) {
    Generator<T> g;

    double then(now());
    for (nat i(0); i < n; ++i) {
        m1.at(g.v);
        g.next();
    }
    double t1(now() - then);

    g.reset();

    then = now();
    for (nat i(0); i < n; ++i) {
        m2.at(g.v);
        g.next();
    }
    double t2(now() - then);
    
    return PerfPoint(t1, t2, n);
}

template <typename T>
PerfPoint testMapPerfIterator(nat n, Map<nat> & m1, std::unordered_map<T, nat> & m2) {
    double then(now());
    volatile int x1(0);
    for (auto it(m1.begin()); it != m1.end(); ++it) {
        ++x1;
    }
    double t1(now() - then);

    then = now();
    volatile int x2(0);
    for (auto it = m2.begin(); it != m2.end(); ++it) {
        ++x2;
    }
    double t2(now() - then);
    
    return PerfPoint(t1, t2, n);
}

template <typename T>
PerfPoint testMapPerfCount(nat n, Map<nat> & m1, std::unordered_map<T, nat> & m2) {
    Generator<T> g;

    double then(now());
    volatile nat x1;
    for (nat i(0); i < n; ++i) {
        x1 = m1.count(g.v);
        g.next();
    }
    double t1(now() - then);

    g.reset();

    then = now();
    volatile nat x2;
    for (nat i(0); i < n; ++i) {
        x2 = m2.count(g.v);
        g.next();
    }
    double t2(now() - then);
    
    return PerfPoint(t1, t2, n);
}

template <typename T>
PerfPoint testMapPerfErase(nat n, Map<nat> & m1, std::unordered_map<T, nat> & m2) {
    n = min(n, m1.size(), static_cast<nat>(m2.size()));
    Generator<T> g;

    double then(now());
    volatile nat x1;
    for (nat i(0); i < n; ++i) {
        x1 = m1.erase(g.v);
        g.next();
    }
    double t1(now() - then);

    g.reset();

    then = now();
    volatile nat x2;
    for (nat i(0); i < n; ++i) {
        x2 = m2.erase(g.v);
        g.next();
    }
    double t2(now() - then);
    
    return PerfPoint(t1, t2, n);
}

void testMapPerfS08(MapPerf & perf, nat n) {
    Map<nat> m1;
    std::unordered_map<s08, nat> m2;
    perf.at(MapF::  insert, MapK::s08) =   testMapPerfInsert(n, m1, m2);
    perf.at(MapF::      at, MapK::s08) =       testMapPerfAt(n, m1, m2);
    perf.at(MapF::iterator, MapK::s08) = testMapPerfIterator(n, m1, m2);
    perf.at(MapF::   count, MapK::s08) =    testMapPerfCount(n, m1, m2);
    perf.at(MapF::   erase, MapK::s08) =    testMapPerfErase(n, m1, m2);
}

void testMapPerfS16(MapPerf & perf, nat n) {
    Map<nat> m1;
    std::unordered_map<s16, nat> m2;
    perf.at(MapF::  insert, MapK::s16) =   testMapPerfInsert(n, m1, m2);
    perf.at(MapF::      at, MapK::s16) =       testMapPerfAt(n, m1, m2);
    perf.at(MapF::iterator, MapK::s16) = testMapPerfIterator(n, m1, m2);
    perf.at(MapF::   count, MapK::s16) =    testMapPerfCount(n, m1, m2);
    perf.at(MapF::   erase, MapK::s16) =    testMapPerfErase(n, m1, m2);
}

void testMapPerfS32(MapPerf & perf, nat n) {
    Map<nat> m1;
    std::unordered_map<s32, nat> m2;
    perf.at(MapF::  insert, MapK::s32) =   testMapPerfInsert(n, m1, m2);
    perf.at(MapF::      at, MapK::s32) =       testMapPerfAt(n, m1, m2);
    perf.at(MapF::iterator, MapK::s32) = testMapPerfIterator(n, m1, m2);
    perf.at(MapF::   count, MapK::s32) =    testMapPerfCount(n, m1, m2);
    perf.at(MapF::   erase, MapK::s32) =    testMapPerfErase(n, m1, m2);
}

void testMapPerfS64(MapPerf & perf, nat n) {
    Map<nat> m1;
    std::unordered_map<s64, nat> m2;
    perf.at(MapF::  insert, MapK::s64) =   testMapPerfInsert(n, m1, m2);
    perf.at(MapF::      at, MapK::s64) =       testMapPerfAt(n, m1, m2);
    perf.at(MapF::iterator, MapK::s64) = testMapPerfIterator(n, m1, m2);
    perf.at(MapF::   count, MapK::s64) =    testMapPerfCount(n, m1, m2);
    perf.at(MapF::   erase, MapK::s64) =    testMapPerfErase(n, m1, m2);
}

void testMapPerfString(MapPerf & perf, nat n) {
    Map<nat> m1;
    std::unordered_map<std::string, nat> m2;
    perf.at(MapF::  insert, MapK::string) =   testMapPerfInsert(n, m1, m2);
    perf.at(MapF::      at, MapK::string) =       testMapPerfAt(n, m1, m2);
    perf.at(MapF::iterator, MapK::string) = testMapPerfIterator(n, m1, m2);
    perf.at(MapF::   count, MapK::string) =    testMapPerfCount(n, m1, m2);
    perf.at(MapF::   erase, MapK::string) =    testMapPerfErase(n, m1, m2);
}

void testMapPerformance() {
    MapPerf perf;

    testMapPerfS08(perf, 1000000);
    testMapPerfS16(perf, 1000000);
    testMapPerfS32(perf, 1000000);
    testMapPerfS64(perf, 1000000);
    testMapPerfString(perf, 1000000);

    perf.report();
}

int main() {
    testHashPerformance();
    testMapPerformance();

    std::cin.get();
    return 0;
}