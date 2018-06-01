#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_set>

#include "QHash/Set.hpp"



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



template <typename V, typename QH, typename StdH>
double compareInsertion(const std::vector<V> & values, qc::Set<V, QH> & qSet, std::unordered_set<V, StdH> & stdSet) {
    unsigned long long then(now());
    for (size_t i(0); i < values.size(); ++i) {
        qSet.emplace(values[i]);
    }
    unsigned long long qTime(now() - then);

    then = now();
    for (size_t i(0); i < values.size(); ++i) {
        stdSet.emplace(values[i]);
    }
    unsigned long long stdTime(now() - then);
    
    return double(qTime) / double(stdTime);
}

template <typename V, typename QH, typename StdH>
double compareAccess(const std::vector<V> & values, qc::Set<V, QH> & qSet, std::unordered_set<V, StdH> & stdSet) {
    volatile size_t v(0);

    unsigned long long then(now());
    for (size_t i(0); i < values.size(); ++i) {
        v += qSet.count(values[i]);
    }
    unsigned long long qTime(now() - then);

    then = now();
    for (size_t i(0); i < values.size(); ++i) {
        v += stdSet.count(values[i]);
    }
    unsigned long long stdTime(now() - then);
    
    return double(qTime) / double(stdTime);
}

template <typename QH, typename StdH>
double compareIteration(qc::Set<nat, QH> & qSet, std::unordered_set<nat, StdH> & stdSet) {
    volatile nat v(0);

    unsigned long long then(now());
    for (const auto & value : qSet) {
        v += value;
    }
    unsigned long long qTime(now() - then);

    then = now();
    for (const auto & value : stdSet) {
        v += value;
    }
    unsigned long long stdTime(now() - then);
    
    return double(qTime) / double(stdTime);
}

template <typename V, typename QH, typename StdH>
double compareErasure(const std::vector<V> & values, qc::Set<V, QH> & qSet, std::unordered_set<V, StdH> & stdSet) {
    volatile bool v(false);

    unsigned long long then(now());
    for (size_t i(0); i < values.size(); ++i) {
        v = v || qSet.erase(values[i]);
    }
    unsigned long long qTime(now() - then);

    then = now();
    for (size_t i(0); i < values.size(); ++i) {
        v = v || stdSet.erase(values[i]);
    }
    unsigned long long stdTime(now() - then);
    
    return double(qTime) / double(stdTime);
}



}



int main() {
    constexpr size_t nElements(8192);
    constexpr size_t nRounds(1000);
    
    std::vector<nat> values(nElements);
    for (nat i(0); i < nElements; ++i) {
        values[i] = i;
    }
    std::shuffle(values.begin(), values.end(), std::default_random_engine());

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Set performance, comparing qc::Set to std::unordered_set..." << std::endl;

    double overallInsertionFactor(0.0);
    double overallAccessFactor(0.0);
    double overallIterationFactor(0.0);
    double overallErasureFactor(0.0);

    for (size_t round(0); round < nRounds; ++round) {
        qc::Set<nat> qSet;
        std::unordered_set<nat> stdSet;
        overallInsertionFactor += compareInsertion(values, qSet, stdSet);
        overallAccessFactor += compareAccess(values, qSet, stdSet);
        overallIterationFactor += compareIteration(qSet, stdSet);
        overallErasureFactor += compareErasure(values, qSet, stdSet);
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