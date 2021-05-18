#include <chrono>
#include <iomanip>
#include <iostream>
#include <unordered_set>
#include <vector>

#include <qc-core/vector.hpp>
#include <qc-core/random.hpp>

#include <qc-hash/qc-map.hpp>
//#include <qc-hash/qc-map-orig.hpp>
//#include <qc-hash/qc-map-alt.hpp>

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

template <typename K, typename S>
static u64 timeInsertion(const std::vector<K> & values, std::vector<S> & sets) {
    u64 then(now());
    for (auto & set : sets) {
        for (const auto & value : values) {
            set.emplace(value);
        }
    }
    return now() - then;
}

template <typename K, typename S1, typename S2>
static vec2<u64> compareInsertion(const std::vector<K> & values, std::vector<S1> & sets1, std::vector<S2> & sets2) {
    return { timeInsertion(values, sets1), timeInsertion(values, sets2) };
}

template <typename K, typename S1, typename S2>
static vec2<u64> compareInsertionSaturated(const std::vector<K> & values, std::vector<S1> & sets1, std::vector<S2> & sets2) {
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


template <typename K, typename S>
static u64 timeAccess(const std::vector<K> & values, const std::vector<S> & sets) {
    volatile size_t v{0u};
    u64 then(now());
    for (const auto & set : sets) {
        for (const auto & value : values) {
            v = v + set.count(value);
        }
    }
    return now() - then;
}

template <typename K, typename S1, typename S2>
static vec2<u64> compareAccess(const std::vector<K> & values, const std::vector<S1> & sets1, const std::vector<S2> & sets2) {
    return { timeAccess(values, sets1), timeAccess(values, sets2) };
}

template <typename K, typename S1, typename S2>
static vec2<u64> compareAccessSaturated(const std::vector<K> & values, const std::vector<S1> & sets1, const std::vector<S2> & sets2) {
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

template <typename K, typename S>
static u64 timeIteration(const std::vector<S> & sets) {
    volatile K v{};
    u64 then(now());
    for (const auto & set : sets) {
        for (const auto & value : set) {
            v = v + value;
        }
    }
    return now() - then;
}

template <typename K, typename S1, typename S2>
static vec2<u64> compareIteration(const S1 & set1, const S2 & set2) {
    return { timeIteration<K>(set1), timeIteration<K>(set2) };
}

template <typename K, typename S1, typename S2>
static vec2<u64> compareIterationSaturated(const std::vector<S1> & sets1, const std::vector<S2> & sets2) {
    volatile K v{};
    u64 t1{0u}, t2{0u};

    auto sets1It{sets1.cbegin()};
    auto sets2It{sets2.cbegin()};
    for (; sets1It != sets1.cend() && sets2It != sets2.cend(); ++sets1It, ++sets2It) {
        {
            const u64 then{now()};
            for (const auto & value : *sets1It) {
                v = v + value;
            }
            t1 += now() - then;
        }
        {
            const u64 then{now()};
            for (const auto & value : *sets2It) {
                v = v + value;
            }
            t2 += now() - then;
        }
    }

    return {t1, t2};
}

template <typename K, typename S>
static u64 timeErasure(const std::vector<K> & values, std::vector<S> & sets) {
    volatile bool v(false);
    u64 then(now());
    for (auto & set : sets) {
        for (const auto & value : values) {
            v = v || set.erase(value);
        }
    }
    return now() - then;
}

template <typename K, typename S1, typename S2>
static vec2<u64> compareErasure(const std::vector<K> & values, S1 & set1, S2 & set2) {
    return { timeErasure(values, set1), timeErasure(values, set2) };
}

template <typename K, typename S1, typename S2>
static vec2<u64> compareErasureSaturated(const std::vector<K> & values, std::vector<S1> & sets1, std::vector<S2> & sets2) {
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
    vec2<u64> insertionTimes;
    vec2<u64> accessTimes;
    vec2<u64> iterationTimes;
    vec2<u64> erasureTimes;
    vec2<u64> refillTimes;
};

template <typename K, typename S1, typename S2>
static Result compareUnsaturated(size_t elementCount, size_t roundCount, size_t groupSize) {
    qc::Random random;
    std::vector<K> values(elementCount);
    for (size_t i{0u}; i < elementCount; ++i) {
        values[i] = random.next<K>();
    }

    Result result{};
    for (size_t round{0u}; round < roundCount; ++round) {
        std::vector<S1> sets1(groupSize);
        std::vector<S2> sets2(groupSize);
        result.insertionTimes += compareInsertion(values, sets1, sets2);
        result.accessTimes += compareAccess(values, sets1, sets2);
        result.iterationTimes += compareIteration<K>(sets1, sets2);
        result.erasureTimes += compareErasure(values, sets1, sets2);
        result.refillTimes += compareInsertion(values, sets1, sets2);
    }

    return result;
}

template <typename K, typename S1, typename S2>
static Result compareSaturated(const size_t elementCount) {
    const size_t minTotalMemToUse{1024u * 1024u * 1024u}; // 1 GB
    const size_t minMemPerSet{qc::min(sizeof(S1), sizeof(S2)) + sizeof(K) * elementCount};
    const size_t setCount{(minTotalMemToUse + minMemPerSet - 1u) / minMemPerSet / 2u};

    qc::Random random{};
    std::vector<K> values(elementCount);
    for (size_t i{0u}; i < elementCount; ++i) {
        values[i] = random.next<K>();
    }

    std::vector<S1> sets1(setCount);
    std::vector<S2> sets2(setCount);
    Result result{};

    result.insertionTimes += compareInsertionSaturated(values, sets1, sets2);
    result.accessTimes += compareAccessSaturated(values, sets1, sets2);
    result.iterationTimes += compareIterationSaturated<K>(sets1, sets2);
    result.erasureTimes += compareErasureSaturated(values, sets1, sets2);
    result.refillTimes += compareInsertionSaturated(values, sets1, sets2);

    return result;
}

static void report(const Result & result) {
    const dvec2 insertionSeconds{dvec2(result.insertionTimes) * 1.0e-9};
    const dvec2 accessSeconds{dvec2(result.accessTimes) * 1.0e-9};
    const dvec2 iterationSeconds{dvec2(result.iterationTimes) * 1.0e-9};
    const dvec2 erasureSeconds{dvec2(result.erasureTimes) * 1.0e-9};
    const dvec2 refillSeconds{dvec2(result.refillTimes) * 1.0e-9};

    const double insertionFactor{insertionSeconds.y / insertionSeconds.x};
    const double accessFactor{accessSeconds.y / accessSeconds.x};
    const double iterationFactor{iterationSeconds.y / iterationSeconds.x};
    const double erasureFactor{erasureSeconds.y / erasureSeconds.x};
    const double refillFactor{refillSeconds.y / refillSeconds.x};

    std::cout << "Insertion    : " << insertionSeconds.y    << "s vs " << insertionSeconds.x    << "s, or "; printFactor(insertionFactor   ); std::cout << std::endl;
    std::cout << "Access       : " << accessSeconds.y       << "s vs " << accessSeconds.x       << "s, or "; printFactor(accessFactor      ); std::cout << std::endl;
    std::cout << "Iteration    : " << iterationSeconds.y    << "s vs " << iterationSeconds.x    << "s, or "; printFactor(iterationFactor   ); std::cout << std::endl;
    std::cout << "Erasure      : " << erasureSeconds.y      << "s vs " << erasureSeconds.x      << "s, or "; printFactor(erasureFactor     ); std::cout << std::endl;
    std::cout << "Refill       : " << refillSeconds.y       << "s vs " << refillSeconds.x       << "s, or "; printFactor(refillFactor      ); std::cout << std::endl;
}

int main() {
    using K = size_t;
    using H = qc_hash::config::DefaultHash<K>;
    using S1 = std::unordered_set<K, H>;
    using S2 = qc_hash::Set<K, H>;
    const bool saturateCache{true};
    const size_t elementCount{1000u};
    const size_t roundCount{1000u};
    const size_t groupSize{100u};

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Set performance - comparing S2 against S1..." << std::endl;

    Result result;
    if (saturateCache) {
        result = compareSaturated<K, S1, S2>(elementCount);
    }
    else {
        result = compareUnsaturated<K, S1, S2>(elementCount, roundCount, groupSize);
    }
    report(result);

    std::cout << std::endl;
    std::cout << "Done" << std::endl;
}
