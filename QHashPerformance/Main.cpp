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

    PerfPoint() :
        t1(0.0), t2(0.0), n(0)
    {}

    PerfPoint(double t1, double t2, nat n) :
        t1(t1), t2(t2), n(n)
    {}

    friend PerfPoint operator+(const PerfPoint & p1, const PerfPoint & p2) {
        return PerfPoint(p1.t1 + p2.t1, p1.t2 + p2.t2, p1.n + p2.n);
    }

    friend PerfPoint & operator+=(PerfPoint & p1, const PerfPoint & p2) {
        p1 = p1 + p2;
        return p1;
    }

    friend std::ostream & operator<<(std::ostream & os, const PerfPoint & p) {
        if (p.t1 < p.t2) {
            os << "+" << (p.t2 / p.t1) << "x";
        }
        else if (p.t1 > p.t2) {
            os << "-" << (p.t1 / p.t2) << "x";
        }
        else {
            os << " " << 1.0 << "x";
        }
        return os;
    }
    
};



template <nat N>
PerfPoint runHashComparison(nat nBytes) {
    struct T { u08 data[N]; };

    nat n(nBytes / N);

    std::unique_ptr<T[]> arr(std::make_unique<T[]>(n));

    int * ints(reinterpret_cast<int *>(arr.get()));
    nat nInts(n * N / sizeof(int));
    for (nat i(0); i < nInts; ++i) {
        ints[i] = std::rand();
    }

    volatile nat x1;
    double then(now());
    for (nat i(0); i < n; ++i) {
        x1 = hash(arr[i]);
    }
    double t1(now() - then);

    volatile size_t x2;
    double t2;
    if constexpr (N <= 8) {
        const precision_ut<N> * keys(reinterpret_cast<const precision_ut<N> *>(arr.get()));
        std::hash<precision_ut<N>> stdHash;
        then = now();
        for (nat i(0); i < n; ++i) {
            x2 = stdHash(keys[i]);
        }
        t2 = now() - then;
    }
    if constexpr (N > 8) {
        const char * chars(reinterpret_cast<const char *>(arr.get()));
        std::vector<std::string> strs; strs.reserve(n);
        for (nat i(0); i < n; ++i) {
            strs.emplace_back(chars + i * sizeof(T), sizeof(T));
        }

        std::hash<std::string> stdHash;
        then = now();
        for (nat i(0); i < n; ++i) {
            x2 = stdHash(strs[i]);
        }
        t2 = now() - then;
    }

    return PerfPoint(t1, t2, n);
}



template <nat N>
PerfPoint testHashNPerf(nat nHashBytes, nat nTrials) {
    std::cout << "  " << std::setw(8) << N << " byte key... ";
    PerfPoint p;
    for (nat i(0); i < nTrials; ++i) {
        p += runHashComparison<N>(nHashBytes);
    }
    std::cout << p << std::endl;
    
    return p;
}

void testHashPerformance() {
    static constexpr nat nHashBytes(10000000);
    static constexpr nat nTrials(10);

    std::cout << std::fixed << std::setprecision(2);

    std::cout << "  " << "Total bytes: " << nHashBytes << ", Number of trials: " << nTrials << std::endl;
    std::cout << std::endl;

    PerfPoint overallP;

    overallP += testHashNPerf<1>(nHashBytes, nTrials);
    overallP += testHashNPerf<2>(nHashBytes, nTrials);
    overallP += testHashNPerf<4>(nHashBytes, nTrials);
    overallP += testHashNPerf<8>(nHashBytes, nTrials);
    overallP += testHashNPerf<16>(nHashBytes, nTrials);
    overallP += testHashNPerf<32>(nHashBytes, nTrials);
    overallP += testHashNPerf<64>(nHashBytes, nTrials);
    overallP += testHashNPerf<nHashBytes>(nHashBytes, nTrials);

    std::cout << std::endl;
    std::cout << "  " << "Overall: " << overallP << std::endl;
}



template <typename K>
PerfPoint runMapInsertComparison(const std::vector<K> & keys, Map<K, nat> & m1, std::unordered_map<K, nat> & m2) {
    double then(now());
    for (nat i(0); i < nat(keys.size()); ++i) {
        m1.insert(keys[i], i);
    }
    double t1(now() - then);

    then = now();
    for (nat i(0); i < nat(keys.size()); ++i) {
        m2.insert({ keys[i], i });
    }
    double t2(now() - then);
    
    return PerfPoint(t1, t2, keys.size());
}

template <typename K>
PerfPoint runMapAtComparison(const std::vector<K> & keys, Map<K, nat> & m1, std::unordered_map<K, nat> & m2) {
    double then(now());
    for (nat i(0); i < nat(keys.size()); ++i) {
        m1.at(keys[i]);
    }
    double t1(now() - then);

    then = now();
    for (nat i(0); i < nat(keys.size()); ++i) {
        m2.at(keys[i]);
    }
    double t2(now() - then);
    
    return PerfPoint(t1, t2, keys.size());
}

template <typename K>
PerfPoint runMapIteratorComparison(const std::vector<K> & keys, Map<K, nat> & m1, std::unordered_map<K, nat> & m2) {
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
    
    return PerfPoint(t1, t2, keys.size());
}

template <typename K>
PerfPoint runMapCountComparison(const std::vector<K> & keys, Map<K, nat> & m1, std::unordered_map<K, nat> & m2) {
    double then(now());
    volatile nat x1;
    for (nat i(0); i < nat(keys.size()); ++i) {
        x1 = m1.count(keys[i]);
    }
    double t1(now() - then);

    then = now();
    volatile nat x2;
    for (nat i(0); i < nat(keys.size()); ++i) {
        x2 = m2.count(keys[i]);
    }
    double t2(now() - then);
    
    return PerfPoint(t1, t2, keys.size());
}

template <typename K>
PerfPoint runMapEraseComparison(const std::vector<K> & keys, Map<K, nat> & m1, std::unordered_map<K, nat> & m2) {
    double then(now());
    volatile nat x1;
    for (nat i(0); i < nat(keys.size()); ++i) {
        x1 = m1.erase(keys[i]);
    }
    double t1(now() - then);

    then = now();
    volatile nat x2;
    for (nat i(0); i < nat(keys.size()); ++i) {
        x2 = m2.erase(keys[i]);
    }
    double t2(now() - then);
    
    return PerfPoint(t1, t2, keys.size());
}

void testMapPerformance() {
    static constexpr nat nElements(1000000);
    
    std::vector<nat> natKeys(nElements);
    std::vector<std::string> strKeys(nElements);
    std::hash<nat> stdHash;
    for (nat i(0); i < nElements; ++i) {
        natKeys[i] = i;
        strKeys[i] = std::to_string(stdHash(i));
    }

    Map<nat, nat> natMap1;
    std::unordered_map<nat, nat> natMap2;
    Map<std::string, nat> strMap1;
    std::unordered_map<std::string, nat> strMap2;

    std::cout << std::fixed << std::setprecision(2);

    std::cout << "  " << "Number of elements: " << nElements << std::endl;

    std::cout << std::endl;

    PerfPoint overallNatP, overallStrP;
    PerfPoint natP, strP;

    std::cout << "  " << "  Insert nat key... ";
    natP = runMapInsertComparison(natKeys, natMap1, natMap2);
    std::cout << natP << ", string key... ";
    strP = runMapInsertComparison(strKeys, strMap1, strMap2);
    std::cout << strP << std::endl;
    overallNatP += natP;
    overallStrP += strP;

    std::cout << "  " << "      At nat key... ";
    natP = runMapAtComparison(natKeys, natMap1, natMap2);
    std::cout << natP << ", string key... ";
    strP = runMapAtComparison(strKeys, strMap1, strMap2);
    std::cout << strP << std::endl;
    overallNatP += natP;
    overallStrP += strP;

    std::cout << "  " << "Iterator nat key... ";
    natP = runMapIteratorComparison(natKeys, natMap1, natMap2);
    std::cout << natP << ", string key... ";
    strP = runMapIteratorComparison(strKeys, strMap1, strMap2);
    std::cout << strP << std::endl;
    overallNatP += natP;
    overallStrP += strP;

    std::cout << "  " << "   Count nat key... ";
    natP = runMapCountComparison(natKeys, natMap1, natMap2);
    std::cout << natP << ", string key... ";
    strP = runMapCountComparison(strKeys, strMap1, strMap2);
    std::cout << strP << std::endl;
    overallNatP += natP;
    overallStrP += strP;

    std::cout << "  " << "   Erase nat key... ";
    natP = runMapEraseComparison(natKeys, natMap1, natMap2);
    std::cout << natP << ", string key... ";
    strP = runMapEraseComparison(strKeys, strMap1, strMap2);
    std::cout << strP << std::endl;
    overallNatP += natP;
    overallStrP += strP;

    std::cout << std::endl;
    
    std::cout << "  " << "Overall nat key: " << overallNatP << ", string key: " << overallStrP << std::endl;
}



int main() {
    std::cout << "Hash Performance..." << std::endl;
    std::cout << std::endl;
    testHashPerformance();

    std::cout << std::endl;

    std::cout << "Map Performance..." << std::endl;
    std::cout << std::endl;
    testMapPerformance();

    std::cin.get();
    return 0;
}