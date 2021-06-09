#include <chrono>
#include <iomanip>
#include <iostream>
#include <span>
#include <unordered_set>
#include <vector>

#include <qc-core/random.hpp>
#include <qc-core/vector.hpp>

#include <qc-hash/qc-map.hpp>
#include <qc-hash/qc-map-orig.hpp>
#include <qc-hash/qc-map-alt.hpp>

using namespace qc::types;

static s64 now() {
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

struct Times {
    s64 construct{};
    s64 insert{};
    s64 insertPresent{};
    s64 accessPresent{};
    s64 accessAbsent{};
    s64 accessEmpty{};
    s64 iterateFull{};
    s64 iterateHalf{};
    s64 iterateEmpty{};
    s64 erase{};
    s64 eraseAbsent{};
    s64 refill{};
    s64 clear{};
    s64 loneBegin{};
    s64 loneEnd{};

    Times & operator+=(const Times & other) {
        construct += other.construct;
        insert += other.insert;
        insertPresent += other.insertPresent;
        accessPresent += other.accessPresent;
        accessAbsent += other.accessAbsent;
        accessEmpty += other.accessEmpty;
        iterateFull += other.iterateFull;
        iterateHalf += other.iterateHalf;
        iterateEmpty += other.iterateEmpty;
        erase += other.erase;
        eraseAbsent += other.eraseAbsent;
        refill += other.refill;
        clear += other.clear;
        loneBegin += other.loneBegin;
        loneEnd += other.loneEnd;
        return *this;
    }
};

template <typename S, typename K>
static Times time(const std::vector<K> & presentKeys, const std::vector<K> & nonpresentKeys) {
    static volatile size_t v{};

    const std::span<const K> firstHalfPresentKeys{&presentKeys[0], presentKeys.size() / 2};
    const std::span<const K> secondHalfPresentKeys{&presentKeys[presentKeys.size() / 2], presentKeys.size() / 2};

    const s64 t0{now()};

    // Construct
    S set{};

    const s64 t1{now()};

    // Insert to full capacity
    for (const K & key : presentKeys) {
        set.insert(key);
    }

    const s64 t2{now()};

    // Full capacity insert present elements
    for (const K & key : presentKeys) {
        set.insert(key);
    }

    const s64 t3{now()};

    // Full capacity access present elements
    for (const K & key : presentKeys) {
        v = v + set.contains(key);
    }

    const s64 t4{now()};

    // Full capacity access absent elements
    for (const K & key : nonpresentKeys) {
        v = v + set.contains(key);
    }

    const s64 t5{now()};

    // Full capacity iteration
    for (const K & key : set) {
        key;
        v = v + 1;
    }

    const s64 t6{now()};

    // Full capacity erase absent elements
    for (const K & key : nonpresentKeys) {
        set.erase(key);
    }

    const s64 t7{now()};

    // Half erasure
    for (const K & key : secondHalfPresentKeys) {
        set.erase(key);
    }

    const s64 t8{now()};

    // Half capacity iteration
    for (const K & key : set) {
        key;
        v = v + 1;
    }

    const s64 t9{now()};

    // Erase remaining elements
    for (const K & key : firstHalfPresentKeys) {
        set.erase(key);
    }

    const s64 t10{now()};

    // Empty access
    for (const K & key : presentKeys) {
        v = v + set.contains(key);
    }

    const s64 t11{now()};

    // Empty iteration
    for (const K & key : set) {
        key;
        v = v + 1;
    }

    const s64 t12{now()};

    set.insert(presentKeys.front());

    const s64 t13{now()};

    // Single element begin
    const auto bIt{set.cbegin()};

    const s64 t14{now()};

    // Single element end
    const auto eIt{set.cend()};

    const s64 t15{now()};

    v = v + (bIt == eIt);
    set.erase(presentKeys.front());

    const s64 t16{now()};

    // Reinsertion
    for (const K & key : presentKeys) {
        set.insert(key).second;
    }

    const s64 t17{now()};

    // Clear
    set.clear();

    const s64 t18{now()};

    return Times{
        .construct = t1 - t0,
        .insert = t2 - t1,
        .insertPresent = t3 - t2,
        .accessPresent = t4 - t3,
        .accessAbsent = t5 - t4,
        .accessEmpty = t11 - t10,
        .iterateFull = t6 - t5,
        .iterateHalf = t9 - t8,
        .iterateEmpty = t12 - t11,
        .erase = (t8 - t7) + (t10 - t9),
        .eraseAbsent = t7 - t6,
        .refill = t17 - t16,
        .clear = t18 - t17,
        .loneBegin = t14 - t13,
        .loneEnd = t15 - t14
    };
}

template <typename K, typename S1, typename S2>
static std::pair<Times, Times> compareUnsaturated(const size_t elementCount, const size_t roundCount) {
    qc::Random random{};
    std::vector<K> presentKeys(elementCount);
    std::vector<K> nonpresentKeys(elementCount);
    for (K & key : presentKeys) key = random.next<K>();
    Times result1{};
    Times result2{};

    for (size_t round{0u}; round < roundCount; ++round) {
        std::swap(presentKeys, nonpresentKeys);
        for (K & key : presentKeys) key = random.next<K>();

        result1 += time<S1>(presentKeys, nonpresentKeys);
        result2 += time<S2>(presentKeys, nonpresentKeys);
    }

    return {result1, result2};
}

static void report(const std::pair<Times, Times> & results) {
    const Times & res1{results.first};
    const Times & res2{results.second};

    std::cout << "   Operation   | S1 Time | S2 Time | % Faster " << std::endl;
    std::cout << "---------------+---------+---------+----------" << std::endl;
    std::cout << "   Construct   | "; printTime(res1.construct    ); std::cout << " | "; printTime(res2.construct    ); std::cout << " | "; printFactor(res1.construct    , res2.construct    ); std::cout << std::endl;
    std::cout << "    Insert     | "; printTime(res1.insert       ); std::cout << " | "; printTime(res2.insert       ); std::cout << " | "; printFactor(res1.insert       , res2.insert       ); std::cout << std::endl;
    std::cout << " InsertPresent | "; printTime(res1.insertPresent); std::cout << " | "; printTime(res2.insertPresent); std::cout << " | "; printFactor(res1.insertPresent, res2.insertPresent); std::cout << std::endl;
    std::cout << " AccessPresent | "; printTime(res1.accessPresent); std::cout << " | "; printTime(res2.accessPresent); std::cout << " | "; printFactor(res1.accessPresent, res2.accessPresent); std::cout << std::endl;
    std::cout << " AccessAbsent  | "; printTime(res1.accessAbsent ); std::cout << " | "; printTime(res2.accessAbsent ); std::cout << " | "; printFactor(res1.accessAbsent , res2.accessAbsent ); std::cout << std::endl;
    std::cout << "  AccessEmpty  | "; printTime(res1.accessEmpty  ); std::cout << " | "; printTime(res2.accessEmpty  ); std::cout << " | "; printFactor(res1.accessEmpty  , res2.accessEmpty  ); std::cout << std::endl;
    std::cout << "  IterateFull  | "; printTime(res1.iterateFull  ); std::cout << " | "; printTime(res2.iterateFull  ); std::cout << " | "; printFactor(res1.iterateFull  , res2.iterateFull  ); std::cout << std::endl;
    std::cout << "  IterateHalf  | "; printTime(res1.iterateHalf  ); std::cout << " | "; printTime(res2.iterateHalf  ); std::cout << " | "; printFactor(res1.iterateHalf  , res2.iterateHalf  ); std::cout << std::endl;
    std::cout << " IterateEmpty  | "; printTime(res1.iterateEmpty ); std::cout << " | "; printTime(res2.iterateEmpty ); std::cout << " | "; printFactor(res1.iterateEmpty , res2.iterateEmpty ); std::cout << std::endl;
    std::cout << "     Erase     | "; printTime(res1.erase        ); std::cout << " | "; printTime(res2.erase        ); std::cout << " | "; printFactor(res1.erase        , res2.erase        ); std::cout << std::endl;
    std::cout << "  EraseAbsent  | "; printTime(res1.eraseAbsent  ); std::cout << " | "; printTime(res2.eraseAbsent  ); std::cout << " | "; printFactor(res1.eraseAbsent  , res2.eraseAbsent  ); std::cout << std::endl;
    std::cout << "    Refill     | "; printTime(res1.refill       ); std::cout << " | "; printTime(res2.refill       ); std::cout << " | "; printFactor(res1.refill       , res2.refill       ); std::cout << std::endl;
    std::cout << "     Clear     | "; printTime(res1.clear        ); std::cout << " | "; printTime(res2.clear        ); std::cout << " | "; printFactor(res1.clear        , res2.clear        ); std::cout << std::endl;
    std::cout << "   LoneBegin   | "; printTime(res1.loneBegin    ); std::cout << " | "; printTime(res2.loneBegin    ); std::cout << " | "; printFactor(res1.loneBegin    , res2.loneBegin    ); std::cout << std::endl;
    std::cout << "    LoneEnd    | "; printTime(res1.loneEnd      ); std::cout << " | "; printTime(res2.loneEnd      ); std::cout << " | "; printFactor(res1.loneEnd      , res2.loneEnd      ); std::cout << std::endl;
}

int main() {
    const bool saturateCache{false};
    const size_t elementCount{10000u};
    const size_t roundCount{1000u};
    using K = size_t;
    using H = qc_hash::config::DefaultHash<K>;
    using S1 = std::unordered_set<K, H>;
    using S2 = qc_hash::Set<K, H>;

    report(compareUnsaturated<K, S1, S2>(elementCount, roundCount));

    return 0;
}
