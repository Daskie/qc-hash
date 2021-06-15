#include <chrono>
#include <format>
#include <iomanip>
#include <iostream>
#include <span>
#include <unordered_set>
#include <vector>

#include <qc-core/random.hpp>
#include <qc-core/vector.hpp>

#include <qc-hash/qc-map-chunk.hpp>
#include <qc-hash/qc-map-flat.hpp>
#include <qc-hash/qc-map-orig.hpp>
#include <qc-hash/qc-map-alt.hpp>
#include <qc-hash/qc-map.hpp>

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

enum class Operation : size_t {
    construct,
    insert,
    insertPresent,
    accessPresent,
    accessAbsent,
    accessEmpty,
    iterateFull,
    iterateHalf,
    iterateEmpty,
    erase,
    eraseAbsent,
    refill,
    clear,
    loneBegin,
    loneEnd,
    _n
};

static const std::array<std::string, size_t(Operation::_n)> operationNames{
    "Construct",
    "Insert",
    "InsertPresent",
    "AccessPresent",
    "AccessAbsent",
    "AccessEmpty",
    "IterateFull",
    "IterateHalf",
    "iterateEmpty",
    "Erase",
    "EraseAbsent",
    "Refill",
    "Clear",
    "LoneBegin",
    "LoneEnd"
};

static Operation & operator++(Operation & op) {
    return op = Operation(size_t(op) + 1u);
}

using Times = std::array<s64, size_t(Operation::_n)>;

static Times & operator+=(Times & times1, const Times & times2) {
    for (size_t i{}; i < times1.size(); ++i) {
        times1[i] += times2[i];
    }
    return times1;
}

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

    return {
        t1 - t0,   // construct
        t2 - t1,   // insert
        t3 - t2,   // insertPresent
        t4 - t3,   // accessPresent
        t5 - t4,   // accessAbsent
        t11 - t10, // accessEmpty
        t6 - t5,   // iterateFull
        t9 - t8,   // iterateHalf
        t12 - t11, // iterateEmpty
        (t8 - t7) + (t10 - t9), // erase
        t7 - t6,   // eraseAbsent
        t17 - t16, // refill
        t18 - t17, // clear
        t14 - t13, // loneBegin
        t15 - t14  // loneEnd
    };
}

static void reportComparison(const std::pair<std::string, Times> & setTimes1, const std::pair<std::string, Times> & setTimes2) {
    static const std::string c1Header{"Operation"};
    static const std::string c4Header{"% Faster"};

    size_t c1Width{c1Header.size()};
    for (Operation op{}; op < Operation::_n; ++op) {
        qc::maxify(c1Width, operationNames[size_t(op)].size());
    }
    const size_t c2Width{qc::max(setTimes1.first.size(), size_t(7u))};
    const size_t c3Width{qc::max(setTimes2.first.size(), size_t(7u))};
    const size_t c4Width{qc::max(c4Header.size(), size_t(8u))};

    std::cout << std::format(" {:^{}} | {:^{}} | {:^{}} | {:^{}} ", "Operation", c1Width, setTimes1.first, c2Width, setTimes2.first, c3Width, "% Faster", c4Width) << std::endl;
    std::cout << "---------------+---------+---------+----------" << std::endl;
    for (Operation op{}; op < Operation::_n; ++op) {
        const s64 t1{setTimes1.second[size_t(op)]};
        const s64 t2{setTimes2.second[size_t(op)]};

        std::cout << std::format(" {:^{}} | ", operationNames[size_t(op)], c1Width);
        printTime(t1);
        std::cout << " | ";
        printTime(t2);
        std::cout << " | ";
        printFactor(t1, t2);
        std::cout << std::endl;
    }
}

static void printChartable(const std::vector<std::pair<std::string, Times>> & stats) {
    std::cout << ','; for (const auto & [name, times] : stats) std::cout << name << ','; std::cout << std::endl;
    for (Operation op{}; op < Operation::_n; ++op) {
        std::cout << operationNames[size_t(op)] << ','; for (const auto & [name, times] : stats) std::cout << times[size_t(op)] << ','; std::cout << std::endl;
    }
}

int main() {
    const size_t elementCount{1000u};
    const size_t roundCount{1000u};
    using K = size_t;
    //using H = qc_hash::config::DefaultHash<K>;
    using S1 = std::unordered_set<K>;
    using S2 = qc_hash_orig::Set<K>;
    using S3 = qc_hash_chunk::Set<K>;
    using S4 = qc_hash_flat::Set<K>;
    using S5 = qc_hash::Set<K>;

    qc::Random random{};
    std::vector<K> presentKeys(elementCount);
    std::vector<K> nonpresentKeys(elementCount);
    for (K & key : presentKeys) key = random.next<K>();

    std::vector<std::pair<std::string, Times>> setTimes{
        {"std::unordered_set", {}},
        {"qc::hash::OrigSet", {}},
        {"qc::hash::ChunkSet", {}},
        {"qc::hash::FlatSet", {}},
        {"qc::hash::Set", {}}
    };

    for (size_t round{0u}; round < roundCount; ++round) {
        std::swap(presentKeys, nonpresentKeys);
        for (K & key : presentKeys) key = random.next<K>();

        setTimes[0].second += time<S1>(presentKeys, nonpresentKeys);
        setTimes[1].second += time<S2>(presentKeys, nonpresentKeys);
        setTimes[2].second += time<S3>(presentKeys, nonpresentKeys);
        setTimes[3].second += time<S4>(presentKeys, nonpresentKeys);
        setTimes[4].second += time<S5>(presentKeys, nonpresentKeys);
    }

    //reportComparison(setTimes[3], setTimes[4]);
    printChartable(setTimes);

    return 0;
}
