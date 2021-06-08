#include <chrono>
#include <iomanip>
#include <iostream>
#include <unordered_set>
#include <vector>

#include <qc-core/random.hpp>
#include <qc-core/vector.hpp>

#include <qc-hash/qc-map.hpp>
#include <qc-hash/qc-map-orig.hpp>
#include <qc-hash/qc-map-alt.hpp>

using namespace qc::types;

static u64 now() {
    return std::chrono::nanoseconds(std::chrono::steady_clock::now().time_since_epoch()).count();
}

static void printTime(const s64 nanoseconds) {
    if (nanoseconds < 10000) {
        std::cout << std::setw(4) << nanoseconds << " ns";
        return;
    }

    const s64 microseconds{(nanoseconds + 500) / 1000};
    if (microseconds < 10000) {
        std::cout << std::setw(4) << microseconds << " us";
        return;
    }

    const s64 milliseconds{(microseconds + 500) / 1000};
    if (milliseconds < 10000) {
        std::cout << std::setw(4) << milliseconds << " ms";
        return;
    }

    const s64 seconds{(milliseconds + 500) / 1000};
    std::cout << std::setw(4) << seconds << " s ";
}

static void printFactor(const s64 t1, const s64 t2) {
    const double absFactor{t1 >= t2 ? double(t1) / double(t2) : double(t2) / double(t1)};
    int percent{int(qc::round(absFactor * 100.0)) - 100};
    if (t1 < t2) {
        percent = -percent;
    }
    std::cout << std::setw(6) << percent << " %";
}

struct Result {
    u64 constructionTime{};
    u64 insertionTime{};
    u64 accessTime{};
    u64 iterationTime{};
    u64 erasureTime{};
    u64 refillTime{};
    u64 clearTime{};

    Result & operator+=(const Result & other) {
        constructionTime += other.constructionTime;
        insertionTime += other.insertionTime;
        accessTime += other.accessTime;
        iterationTime += other.iterationTime;
        erasureTime += other.erasureTime;
        refillTime += other.refillTime;
        clearTime += other.clearTime;
        return *this;
    }
};

template <typename S, typename K>
static Result time(const std::vector<K> & keys) {
    static volatile size_t v{};

    const u64 t0{now()};

    // Construction
    S set{};

    const u64 t1{now()};

    // Insertion
    for (const K & key : keys) {
        set.insert(key).second;
    }

    const u64 t2{now()};

    // Access
    for (const K & key : keys) {
        v = v + set.contains(key);
    }

    const u64 t3{now()};

    // Iteration
    for (const auto & key : set) {
        key;
        v = v + 1;
    }

    const u64 t4{now()};

    // Erasure
    for (const auto & key : keys) {
        set.erase(key);
    }

    const u64 t5{now()};

    // Reinsertion
    for (const K & key : keys) {
        set.insert(key).second;
    }

    const u64 t6{now()};

    // Clear
    set.clear();

    const u64 t7{now()};

    return Result{
        .constructionTime = t1 - t0,
        .insertionTime = t2 - t1,
        .accessTime = t3 - t2,
        .iterationTime = t4 - t3,
        .erasureTime = t5 - t4,
        .refillTime = t6 - t5,
        .clearTime = t7 - t6
    };
}

template <typename K, typename S1, typename S2>
static std::pair<Result, Result> compareUnsaturated(const size_t elementCount, const size_t roundCount) {
    qc::Random random{};
    std::vector<K> keys(elementCount);
    Result result1{};
    Result result2{};

    for (size_t round{0u}; round < roundCount; ++round) {
        for (K & key : keys) key = random.next<K>();

        result1 += time<S1>(keys);
        result2 += time<S2>(keys);
    }

    return {result1, result2};
}

static void report(const std::pair<Result, Result> & results) {
    const Result & res1{results.first};
    const Result & res2{results.second};

    std::cout << "  Operation   | S1 Time | S2 Time | % Faster " << std::endl;
    std::cout << "--------------+---------+---------+----------" << std::endl;

    std::cout << " Construction | "; printTime(res1.constructionTime); std::cout << " | "; printTime(res2.constructionTime); std::cout << " | "; printFactor(res1.constructionTime, res2.constructionTime); std::cout << std::endl;
    std::cout << "  Insertion   | "; printTime(res1.   insertionTime); std::cout << " | "; printTime(res2.   insertionTime); std::cout << " | "; printFactor(res1.   insertionTime, res2.   insertionTime); std::cout << std::endl;
    std::cout << "    Access    | "; printTime(res1.      accessTime); std::cout << " | "; printTime(res2.      accessTime); std::cout << " | "; printFactor(res1.      accessTime, res2.      accessTime); std::cout << std::endl;
    std::cout << "  Iteration   | "; printTime(res1.   iterationTime); std::cout << " | "; printTime(res2.   iterationTime); std::cout << " | "; printFactor(res1.   iterationTime, res2.   iterationTime); std::cout << std::endl;
    std::cout << "   Erasure    | "; printTime(res1.     erasureTime); std::cout << " | "; printTime(res2.     erasureTime); std::cout << " | "; printFactor(res1.     erasureTime, res2.     erasureTime); std::cout << std::endl;
    std::cout << "    Refill    | "; printTime(res1.      refillTime); std::cout << " | "; printTime(res2.      refillTime); std::cout << " | "; printFactor(res1.      refillTime, res2.      refillTime); std::cout << std::endl;
    std::cout << "    Clear     | "; printTime(res1.       clearTime); std::cout << " | "; printTime(res2.       clearTime); std::cout << " | "; printFactor(res1.       clearTime, res2.       clearTime); std::cout << std::endl;
}

int main() {
    const bool saturateCache{false};
    const size_t elementCount{1000u};
    const size_t roundCount{10000u};
    using K = size_t;
    using H = qc_hash::config::DefaultHash<K>;
    using S1 = qc_hash_alt::Set<K, H, std::equal_to<K>>;
    using S2 = qc_hash::Set<K, H, std::equal_to<K>>;

    report(compareUnsaturated<K, S1, S2>(elementCount, roundCount));

    return 0;
}
