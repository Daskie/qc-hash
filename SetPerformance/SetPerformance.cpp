#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_set>

#include "QCore/Vector.hpp"
#include "QCore/Random.hpp"

#include "QHash/Set.hpp"
#include "QHash/Set_chu.hpp"
#include "QHash/Set_sep.hpp"
#include "QHash/FasterSet.h"
#include "QHash/Old.hpp"



using namespace qc::types;



namespace {



u64 now() {
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
vec2<u64> compareConstruction(unat n) {
    u64 then(now());
    S1 * sets1(new S1[n]);
    u64 t1(now() - then);

    then = now();
    S2 * sets2(new S2[n]);
    u64 t2(now() - then);

    delete[] sets1;
    delete[] sets2;

    return { t1, t2 };
}

template <typename V, typename S1, typename S2>
vec2<u64> compareInsertion(const std::vector<V> & values, S1 & set1, S2 & set2) {
    u64 then(now());
    for (const auto & value : values) {
        set1.emplace(value);
    }
    u64 t1(now() - then);

    then = now();
    for (const auto & value : values) {
        set2.emplace(value);
    }
    u64 t2(now() - then);
    
    return { t1, t2 };
}

template <typename V, typename S1, typename S2>
vec2<u64> compareInsertion(const std::vector<V> & values, std::vector<S1> & sets1, std::vector<S2> & sets2) {
    u64 t1(0), t2(0);

    for (const auto & value : values) {
        u64 then(now());
        for (auto & set : sets1) {
            set.emplace(value);
        }
        t1 += now() - then;

        then = now();
        for (auto & set : sets2) {
            set.emplace(value);
        }
        t2 += now() - then;
    }
    
    return { t1, t2 };
}

template <typename V, typename S1, typename S2>
vec2<u64> compareAccess(const std::vector<V> & values, const S1 & set1, const S2 & set2) {
    volatile unat v(0);

    u64 then(now());
    for (const auto & value : values) {
        v += set1.count(value);
    }
    u64 t1(now() - then);

    then = now();
    for (const auto & value : values) {
        v += set2.count(value);
    }
    u64 t2(now() - then);
    
    return { t1, t2 };
}

template <typename V, typename S1, typename S2>
vec2<u64> compareAccess(const std::vector<V> & values, const std::vector<S1> & sets1, const std::vector<S2> & sets2) {
    volatile unat v(0);
    u64 t1(0), t2(0);

    for (const auto & value : values) {
        u64 then(now());
        for (const auto & set : sets1) {
            v += set.count(value);
        }
        t1 += now() - then;

        then = now();
        for (const auto & set : sets2) {
            v += set.count(value);
        }
        t2 += now() - then;
    }
    
    return { t1, t2 };
}

template <typename V, typename S1, typename S2>
vec2<u64> compareIteration(const S1 & set1, const S2 & set2) {
    volatile V v(0);

    u64 then(now());
    for (const auto & value : set1) {
        v += value;
    }
    u64 t1(now() - then);

    then = now();
    for (const auto & value : set2) {
        v += value;
    }
    u64 t2(now() - then);
    
    return { t1, t2 };
}

template <typename V, typename S1, typename S2>
vec2<u64> compareErasure(const std::vector<V> & values, S1 & set1, S2 & set2) {
    volatile bool v(false);

    u64 then(now());
    for (const auto & value : values) {
        v = v || set1.erase(value);
    }
    u64 t1(now() - then);

    then = now();
    for (const auto & value : values) {
        v = v || set2.erase(value);
    }
    u64 t2(now() - then);
    
    return { t1, t2 };
}

template <typename V, typename S1, typename S2>
vec2<u64> compareErasure(const std::vector<V> & values, std::vector<S1> & sets1, std::vector<S2> & sets2) {
    volatile bool v(false);
    u64 t1(0), t2(0);

    for (const auto & value : values) {
        u64 then(now());
        for (auto & set : sets1) {
            v = v || set.erase(value);
        }
        t1 += now() - then;

        then = now();
        for (auto & set : sets2) {
            v = v || set.erase(value);
        }
        t2 += now() - then;
    }

    return { t1, t2 };
}

struct Result {
    vec2<u64> constructionTimes;
    vec2<u64> insertionTimes;
    vec2<u64> accessTimes;
    vec2<u64> iterationTimes;
    vec2<u64> erasureTimes;
};

template <typename V, typename S1, typename S2>
Result compareUnsaturated(unat elementCount, unat roundCount) {
    qc::Random<std::conditional_t<sizeof(nat) <= 4, std::mt19937, std::mt19937_64>> random;
    std::vector<V> values(elementCount);
    for (unat i(0); i < elementCount; ++i) {
        values[i] = random.next<V>();
    }
    Result result{};

    for (unat round(0); round < roundCount; ++round) {
        S1 set1;
        S2 set2;
        //result.constructionTimes += compareConstruction<S1, S2>(elementCount);
        result.insertionTimes += compareInsertion(values, set1, set2);
        result.accessTimes += compareAccess(values, set1, set2);
        result.iterationTimes += compareIteration<V>(set1, set2);
        result.erasureTimes += compareErasure(values, set1, set2);
    }

    return result;
}

template <typename V, typename S1, typename S2>
Result compareSaturated(unat elementCount) {
    constexpr unat k_l3CacheSize(8 * 1024 * 1024);
    constexpr unat k_cacheLineSize(64);
    constexpr unat k_setCount(k_l3CacheSize / k_cacheLineSize);

    qc::Random<std::conditional_t<sizeof(nat) <= 4, std::mt19937, std::mt19937_64>> random;
    std::vector<V> values(elementCount);
    for (unat i(0); i < elementCount; ++i) {
        values[i] = random.next<V>();
    }
    Result result{};

    std::vector<S1> sets1(k_setCount);
    std::vector<S2> sets2(k_setCount);
    
    result.insertionTimes += compareInsertion(values, sets1, sets2);
    result.accessTimes += compareAccess(values, sets1, sets2);
    //result.iterationTimes += compareIteration<V>(sets1, sets2);
    result.erasureTimes += compareErasure(values, sets1, sets2);

    return result;
}

void report(const Result & result) {
    double constructionFactor(double(result.constructionTimes._0) / double(result.constructionTimes._1));
    double insertionFactor(double(result.insertionTimes._0) / double(result.insertionTimes._1));
    double accessFactor(double(result.accessTimes._0) / double(result.accessTimes._1));
    double iterationFactor(double(result.iterationTimes._0) / double(result.iterationTimes._1));
    double erasureFactor(double(result.erasureTimes._0) / double(result.erasureTimes._1));

    std::cout << "Construction: "; printFactor(constructionFactor); std::cout << std::endl;
    std::cout << "   Insertion: "; printFactor(   insertionFactor); std::cout << std::endl;
    std::cout << "      Access: "; printFactor(      accessFactor); std::cout << std::endl;
    std::cout << "   Iteration: "; printFactor(   iterationFactor); std::cout << std::endl;
    std::cout << "     Erasure: "; printFactor(     erasureFactor); std::cout << std::endl;
}



}



int main() {
    using V = qc::u08;
    using H = qc::NoHash<V>;
    using S1 = qc::Set<V, H>;
    using S2 = old::Set<V, H>;
    constexpr bool k_saturateCache(false);
    constexpr unat k_elementCount(1000);
    constexpr unat k_roundCount(10000);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Set performance, comparing qc::Set to std::unordered_set..." << std::endl;

    Result result;
    if (k_saturateCache) {
        result = compareSaturated<V, S1, S2>(k_elementCount);
    }
    else {
        result = compareUnsaturated<V, S1, S2>(k_elementCount, k_roundCount);
    }
    report(result);

    std::cout << std::endl;
    std::cout << "Done" << std::endl;
    std::cin.get();
}