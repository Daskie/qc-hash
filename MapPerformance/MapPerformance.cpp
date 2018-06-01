#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_map>

#include "QHash/Map.hpp"



using nat = intptr_t;



namespace {



unsigned long long now() {
    return std::chrono::nanoseconds(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

void printFactor(double factor) {
    if (factor <= 1.0) {
        std::cout << (1.0f / factor) << "x faster";
    }
    else {
        std::cout << factor << "x slower";
    }
}



template <typename K, typename QH, typename StdH>
double compareInsertion(const std::vector<K> & keys, qc::Map<K, nat, QH> & qMap, std::unordered_map<K, nat, StdH> & stdMap) {
    unsigned long long then(now());
    for (size_t i(0); i < keys.size(); ++i) {
        qMap.emplace(keys[i], i);
    }
    unsigned long long qTime(now() - then);

    then = now();
    for (size_t i(0); i < keys.size(); ++i) {
        stdMap.emplace(keys[i], i);
    }
    unsigned long long stdTime(now() - then);
    
    return double(qTime) / double(stdTime);
}

template <typename K, typename QH, typename StdH>
double compareAccess(const std::vector<K> & keys, qc::Map<K, nat, QH> & qMap, std::unordered_map<K, nat, StdH> & stdMap) {
    volatile nat v(0);

    unsigned long long then(now());
    for (size_t i(0); i < keys.size(); ++i) {
        v += qMap.at(keys[i]);
    }
    unsigned long long qTime(now() - then);

    then = now();
    for (size_t i(0); i < keys.size(); ++i) {
        v += stdMap.at(keys[i]);
    }
    unsigned long long stdTime(now() - then);
    
    return double(qTime) / double(stdTime);
}

template <typename K, typename QH, typename StdH>
double compareIteration(qc::Map<K, nat, QH> & qMap, std::unordered_map<K, nat, StdH> & stdMap) {
    volatile nat v(0);

    unsigned long long then(now());
    for (const auto & value : qMap) {
        v += value.second;
    }
    unsigned long long qTime(now() - then);

    then = now();
    for (const auto & value : stdMap) {
        v += value.second;
    }
    unsigned long long stdTime(now() - then);
    
    return double(qTime) / double(stdTime);
}

template <typename K, typename QH, typename StdH>
double compareErasure(const std::vector<K> & keys, qc::Map<K, nat, QH> & qMap, std::unordered_map<K, nat, StdH> & stdMap) {
    volatile bool v(false);

    unsigned long long then(now());
    for (size_t i(0); i < keys.size(); ++i) {
        v = v || qMap.erase(keys[i]);
    }
    unsigned long long qTime(now() - then);

    then = now();
    for (size_t i(0); i < keys.size(); ++i) {
        v = v || stdMap.erase(keys[i]);
    }
    unsigned long long stdTime(now() - then);
    
    return double(qTime) / double(stdTime);
}



}



int main() {
    constexpr size_t nElements(8192);
    constexpr size_t nRounds(1000);
    
    std::vector<nat> keys(nElements);
    for (nat i(0); i < nElements; ++i) {
        keys[i] = i;
    }
    std::shuffle(keys.begin(), keys.end(), std::default_random_engine());

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Map performance, comparing qc::Map to std::unordered_map..." << std::endl;

    double overallInsertionFactor(0.0);
    double overallAccessFactor(0.0);
    double overallIterationFactor(0.0);
    double overallErasureFactor(0.0);

    for (size_t round(0); round < nRounds; ++round) {
        qc::Map<nat, nat> qMap;
        std::unordered_map<nat, nat> stdMap;
        overallInsertionFactor += compareInsertion(keys, qMap, stdMap);
        overallAccessFactor += compareAccess(keys, qMap, stdMap);
        overallIterationFactor += compareIteration(qMap, stdMap);
        overallErasureFactor += compareErasure(keys, qMap, stdMap);
    }

    overallInsertionFactor /= nRounds;
    overallAccessFactor /= nRounds;
    overallIterationFactor /= nRounds;
    overallErasureFactor /= nRounds;

    std::cout << std::endl;
    
    std::cout << "  Insertion: ";
    printFactor(overallInsertionFactor);
    std::cout << std::endl;
    std::cout << "  Access: ";
    printFactor(overallAccessFactor);
    std::cout << std::endl;
    std::cout << "  Iteration: ";
    printFactor(overallIterationFactor);
    std::cout << std::endl;
    std::cout << "  Erasure: ";
    printFactor(overallErasureFactor);
    std::cout << std::endl;

    std::cout << std::endl;
    std::cout << "Done" << std::endl;
    std::cin.get();
}