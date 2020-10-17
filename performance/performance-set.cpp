#include <chrono>
#include <iomanip>
#include <iostream>
#include <unordered_set>
#include <vector>

#include <qc-core/vector.hpp>
#include <qc-core/random.hpp>

#include <qc-hash/qc-map.hpp>

using namespace qc::types;

static u64 now() {
    return std::chrono::nanoseconds(std::chrono::steady_clock::now().time_since_epoch()).count();
}

static void printFactor(double factor) {
    if (factor <= 1.0) {
        std::cout << (1.0f / factor) << "x faster";
    }
    else {
        std::cout << factor << "x slower";
    }
}

template <typename S1, typename S2>
static vec2<u64> compareConstruction(size_t n) {
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

template <typename V, typename S>
static u64 timeInsertion(const std::vector<V> & values, std::vector<S> & sets) {
    u64 then(now());
    for (auto & set : sets) {
        for (const auto & value : values) {
            set.emplace(value);
        }
    }
    return now() - then;
}

template <typename V, typename S1, typename S2>
static vec2<u64> compareInsertion(const std::vector<V> & values, std::vector<S1> & sets1, std::vector<S2> & sets2) {
    return { timeInsertion(values, sets1), timeInsertion(values, sets2) };
}

template <typename V, typename S1, typename S2>
static vec2<u64> compareInsertionSaturated(const std::vector<V> & values, std::vector<S1> & sets1, std::vector<S2> & sets2) {
    u64 t1(0u), t2(0u);

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


template <typename V, typename S>
static u64 timeAccess(const std::vector<V> & values, const std::vector<S> & sets) {
    volatile size_t v{0u};
    u64 then(now());
    for (const auto & set : sets) {
        for (const auto & value : values) {
            v = v + set.count(value);
        }
    }
    return now() - then;
}

template <typename V, typename S1, typename S2>
static vec2<u64> compareAccess(const std::vector<V> & values, const std::vector<S1> & sets1, const std::vector<S2> & sets2) {
    return { timeAccess(values, sets1), timeAccess(values, sets2) };
}

template <typename V, typename S1, typename S2>
static vec2<u64> compareAccessSaturated(const std::vector<V> & values, const std::vector<S1> & sets1, const std::vector<S2> & sets2) {
    volatile size_t v{0u};
    u64 t1(0u), t2(0u);

    for (const auto & value : values) {
        u64 then(now());
        for (const auto & set : sets1) {
            v = v + set.count(value);
        }
        t1 += now() - then;

        then = now();
        for (const auto & set : sets2) {
            v = v + set.count(value);
        }
        t2 += now() - then;
    }

    return { t1, t2 };
}

template <typename V, typename S>
static u64 timeIteration(const std::vector<S> & sets) {
    volatile V v{};
    u64 then(now());
    for (const auto & set : sets) {
        for (const auto & value : set) {
            v = v + value;
        }
    }
    return now() - then;
}

template <typename V, typename S1, typename S2>
static vec2<u64> compareIteration(const S1 & set1, const S2 & set2) {
    return { timeIteration<V>(set1), timeIteration<V>(set2) };
}

template <typename V, typename S>
static u64 timeErasure(const std::vector<V> & values, std::vector<S> & sets) {
    volatile bool v(false);
    u64 then(now());
    for (auto & set : sets) {
        for (const auto & value : values) {
            v = v || set.erase(value);
        }
    }
    return now() - then;
}

template <typename V, typename S1, typename S2>
static vec2<u64> compareErasure(const std::vector<V> & values, S1 & set1, S2 & set2) {
    return { timeErasure(values, set1), timeErasure(values, set2) };
}

template <typename V, typename S1, typename S2>
static vec2<u64> compareErasureSaturated(const std::vector<V> & values, std::vector<S1> & sets1, std::vector<S2> & sets2) {
    volatile bool v(false);
    u64 t1(0u), t2(0u);

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
static Result compareUnsaturated(size_t elementCount, size_t roundCount, size_t groupSize) {
    qc::Random random;
    std::vector<V> values(elementCount);
    for (size_t i{0u}; i < elementCount; ++i) {
        values[i] = random.next<V>();
    }

    Result result{};
    for (size_t round{0u}; round < roundCount; ++round) {
        std::vector<S1> sets1(groupSize);
        std::vector<S2> sets2(groupSize);
        //result.constructionTimes += compareConstruction<S1, S2>(elementCount);
        result.insertionTimes += compareInsertion(values, sets1, sets2);
        result.accessTimes += compareAccess(values, sets1, sets2);
        result.iterationTimes += compareIteration<V>(sets1, sets2);
        result.erasureTimes += compareErasure(values, sets1, sets2);
    }

    return result;
}

template <typename V, typename S1, typename S2>
static Result compareSaturated(size_t elementCount) {
    constexpr size_t l3CacheSize{8u * 1024u * 1024u};
    constexpr size_t cacheLineSize{64u};
    constexpr size_t setCount{l3CacheSize / cacheLineSize};

    qc::Random random;
    std::vector<V> values(elementCount);
    for (size_t i{0u}; i < elementCount; ++i) {
        values[i] = random.next<V>();
    }
    Result result{};

    std::vector<S1> sets1(setCount);
    std::vector<S2> sets2(setCount);

    result.insertionTimes += compareInsertionSaturated(values, sets1, sets2);
    result.accessTimes += compareAccessSaturated(values, sets1, sets2);
    //result.iterationTimes += compareIterationSaturated<V>(sets1, sets2);
    result.erasureTimes += compareErasureSaturated(values, sets1, sets2);

    return result;
}

static void report(const Result & result) {
    double constructionFactor(double(result.constructionTimes.x) / double(result.constructionTimes.y));
    double insertionFactor(double(result.insertionTimes.x) / double(result.insertionTimes.y));
    double accessFactor(double(result.accessTimes.x) / double(result.accessTimes.y));
    double iterationFactor(double(result.iterationTimes.x) / double(result.iterationTimes.y));
    double erasureFactor(double(result.erasureTimes.x) / double(result.erasureTimes.y));

    std::cout << "Construction: "; printFactor(constructionFactor); std::cout << std::endl;
    std::cout << "   Insertion: "; printFactor(   insertionFactor); std::cout << std::endl;
    std::cout << "      Access: "; printFactor(      accessFactor); std::cout << std::endl;
    std::cout << "   Iteration: "; printFactor(   iterationFactor); std::cout << std::endl;
    std::cout << "     Erasure: "; printFactor(     erasureFactor); std::cout << std::endl;
}

int main() {
    using V = intptr_t;
    using H = qc::hash::IdentityHash<V>;
    using S1 = qc::hash::Set<V, H>;
    using S2 = std::unordered_set<V, H>;
    constexpr bool saturateCache(false);
    constexpr size_t elementCount{1000u};
    constexpr size_t roundCount{100u};
    constexpr size_t groupSize{100u};

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Set performance, comparing qc::hash::Set to std::unordered_set..." << std::endl;

    Result result;
    if (saturateCache) {
        result = compareSaturated<V, S1, S2>(elementCount);
    }
    else {
        result = compareUnsaturated<V, S1, S2>(elementCount, roundCount, groupSize);
    }
    report(result);

    std::cout << std::endl;
    std::cout << "Done" << std::endl;
}
