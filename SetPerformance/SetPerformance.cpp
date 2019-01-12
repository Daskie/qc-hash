#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_set>

#include "QCore/Utils.hpp"

#include "QHash/Set.hpp"
#include "QHash/FastSet.hpp"
#include "QHash/FasterSet.h"



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



template <typename S1, typename S2>
double compareConstruction(size_t n) {
    unsigned long long then(now());
    volatile auto s1s(std::make_unique<S1[]>(n));
    unsigned long long t1(now() - then);

    then = now();
    volatile auto s2s(std::make_unique<S2[]>(n));
    unsigned long long t2(now() - then);
    
    return double(t1) / double(t2);
}

template <typename V, typename S1, typename S2>
double compareInsertion(const std::vector<V> & values, S1 & s1, S2 & s2) {
    unsigned long long then(now());
    for (size_t i(0); i < values.size(); ++i) {
        s1.emplace(values[i]);
    }
    unsigned long long t1(now() - then);

    then = now();
    for (size_t i(0); i < values.size(); ++i) {
        s2.emplace(values[i]);
    }
    unsigned long long t2(now() - then);
    
    return double(t1) / double(t2);
}

template <typename V, typename S1, typename S2>
double compareAccess(const std::vector<V> & values, const S1 & s1, const S2 & s2) {
    volatile size_t v(0);

    unsigned long long then(now());
    for (size_t i(0); i < values.size(); ++i) {
        v += s1.count(values[i]);
    }
    unsigned long long t1(now() - then);

    then = now();
    for (size_t i(0); i < values.size(); ++i) {
        v += s2.count(values[i]);
    }
    unsigned long long t2(now() - then);
    
    return double(t1) / double(t2);
}

template <typename V, typename S1, typename S2>
double compareIteration(const S1 & s1, const S2 & s2) {
    volatile V v(0);

    unsigned long long then(now());
    for (const auto & value : s1) {
        v += value;
    }
    unsigned long long t1(now() - then);

    then = now();
    for (const auto & value : s2) {
        v += value;
    }
    unsigned long long t2(now() - then);
    
    return double(t1) / double(t2);
}

template <typename V, typename S1, typename S2>
double compareErasure(const std::vector<V> & values, S1 & s1, S2 & s2) {
    volatile bool v(false);

    unsigned long long then(now());
    for (size_t i(0); i < values.size(); ++i) {
        v = v || s1.erase(values[i]);
    }
    unsigned long long t1(now() - then);

    then = now();
    for (size_t i(0); i < values.size(); ++i) {
        v = v || s2.erase(values[i]);
    }
    unsigned long long t2(now() - then);
    
    return double(t1) / double(t2);
}



}



int main() {
    using V = qc::unat;
    using H = qc::Hash<V>;
    using S1 = qc::Set<V, H>;
    using S2 = std::unordered_set<V, H>;
    constexpr size_t k_elements(1000);
    constexpr size_t k_rounds(10000);

    std::vector<V> values(k_elements);
    for (size_t i(0); i < k_elements; ++i) {
        values[i] = qc::rand<V>();
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Set performance, comparing qc::Set to std::unordered_set..." << std::endl;

    double overallConstructionFactor(0.0);
    double overallInsertionFactor(0.0);
    double overallAccessFactor(0.0);
    double overallIterationFactor(0.0);
    double overallErasureFactor(0.0);

    for (size_t round(0); round < k_rounds; ++round) {
        S1 s1;
        S2 s2;
        //overallConstructionFactor += compareConstruction<S1, S2>(k_elements);
        overallInsertionFactor += compareInsertion(values, s1, s2);
        overallAccessFactor += compareAccess(values, s1, s2);
        overallIterationFactor += compareIteration<V>(s1, s2);
        overallErasureFactor += compareErasure(values, s1, s2);
    }

    overallInsertionFactor /= k_rounds;
    overallAccessFactor /= k_rounds;
    overallIterationFactor /= k_rounds;
    overallErasureFactor /= k_rounds;

    std::cout << std::endl;
    
    //std::cout << "  Construction: ";
    //printFactor(overallConstructionFactor);
    //std::cout << std::endl;
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