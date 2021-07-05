#include <array>
#include <chrono>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <span>
#include <unordered_set>
#include <vector>

#include <qc-core/memory.hpp>
#include <qc-core/random.hpp>
#include <qc-core/vector.hpp>

#include <qc-hash/qc-map-chunk.hpp>
#include <qc-hash/qc-map-flat.hpp>
#include <qc-hash/qc-map-orig.hpp>
#include <qc-hash/qc-map-alt.hpp>
#include <qc-hash/qc-map.hpp>

#include <absl/container/flat_hash_set.h>

#include "robin_hood.h"

#include "flat_hash_map.hpp"

#include "tsl/sparse_set.h"

using namespace qc::types;

static const std::vector<std::pair<size_t, size_t>> elementRoundCounts{
    {       5u, 1000u},
    {      10u, 1000u},
    {      25u, 1000u},
    {      50u, 1000u},
    {     100u, 1000u},
    {     250u, 1000u},
    {     500u, 1000u},
    {    1000u, 1000u},
    {    2500u,  400u},
    {    5000u,  200u},
    {   10000u,  100u},
    {   25000u,   40u},
    {   50000u,   20u},
    {  100000u,   10u},
    {  250000u,   10u},
    {  500000u,   10u},
    { 1000000u,    5u},
    { 2500000u,    5u},
    { 5000000u,    5u},
    {10000000u,    3u}
};

static s64 now() {
    return std::chrono::nanoseconds(std::chrono::steady_clock::now().time_since_epoch()).count();
}

static void printTime(const s64 nanoseconds, const size_t width) {
    if (nanoseconds < 10000) {
        std::cout << std::setw(width - 3) << nanoseconds << " ns";
        return;
    }

    const s64 microseconds{(nanoseconds + 500) / 1000};
    if (microseconds < 10000) {
        std::cout << std::setw(width - 3) << microseconds << " us";
        return;
    }

    const s64 milliseconds{(microseconds + 500) / 1000};
    if (milliseconds < 10000) {
        std::cout << std::setw(width - 3) << milliseconds << " ms";
        return;
    }

    const s64 seconds{(milliseconds + 500) / 1000};
    std::cout << std::setw(width - 3) << seconds << " s ";
}

static void printFactor(const s64 t1, const s64 t2, const size_t width) {
    const double absFactor{t1 >= t2 ? double(t1) / double(t2) : double(t2) / double(t1)};
    int percent{int(qc::round(absFactor * 100.0)) - 100};
    if (t1 < t2) {
        percent = -percent;
    }
    std::cout << std::setw(width - 2) << percent << " %";
}

enum class Stat : size_t {
    objectSize,
    iteratorSize,
    memoryOverhead,
    construct,
    insert,
    insertReserved,
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
    destruction,
    _n
};

static const std::array<std::string, size_t(Stat::_n)> statNames{
    "ObjectSize",
    "IteratorSize",
    "MemoryOverhead",
    "Construct",
    "Insert",
    "InsertReserved",
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
    "LoneEnd",
    "Destruction"
};

using Stats = std::array<double, size_t(Stat::_n)>;

static Stats & operator+=(Stats & stats1, const Stats & stats2) {
    for (size_t i{}; i < stats1.size(); ++i) {
        stats1[i] += stats2[i];
    }
    return stats1;
}

template <typename S, typename K>
static Stats time(const std::vector<K> & presentKeys, const std::vector<K> & nonpresentKeys) {
    static volatile size_t v{};

    const std::span<const K> firstHalfPresentKeys{&presentKeys[0], presentKeys.size() / 2};
    const std::span<const K> secondHalfPresentKeys{&presentKeys[presentKeys.size() / 2], presentKeys.size() / 2};
    const double invElementCount{1.0 / double(presentKeys.size())};
    const double invHalfElementCount{invElementCount * 2.0};

    alignas(S) std::byte backingMemory[sizeof(S)];
    S * setPtr;
    Stats stats{};

    // Construct
    {
        const s64 t0{now()};

        setPtr = new(backingMemory) S{};

        stats[size_t(Stat::construct)] += double(now() - t0);
    }

    S & set{*setPtr};

    // Insert to full capacity
    {
        const s64 t0{now()};

        for (const K & key : presentKeys) {
            set.insert(key);
        }

        stats[size_t(Stat::insert)] += double(now() - t0) * invElementCount;
    }

    // Full capacity insert present elements
    {
        const s64 t0{now()};

        for (const K & key : presentKeys) {
            set.insert(key);
        }

        stats[size_t(Stat::insertPresent)] += double(now() - t0) * invElementCount;
    }

    // Full capacity access present elements
    {
        const s64 t0{now()};

        for (const K & key : presentKeys) {
            v = v + set.count(key);
        }

        stats[size_t(Stat::accessPresent)] += double(now() - t0) * invElementCount;
    }

    // Full capacity access absent elements
    {
        const s64 t0{now()};

        for (const K & key : nonpresentKeys) {
            v = v + set.count(key);
        }

        stats[size_t(Stat::accessAbsent)] += double(now() - t0) * invElementCount;
    }

    // Full capacity iteration
    {
        const s64 t0{now()};

        for (const K & key : set) {
            key;
            v = v + 1;
        }

        stats[size_t(Stat::iterateFull)] += double(now() - t0) * invElementCount;
    }

    // Full capacity erase absent elements
    {
        const s64 t0{now()};

        for (const K & key : nonpresentKeys) {
            set.erase(key);
        }

        stats[size_t(Stat::eraseAbsent)] += double(now() - t0) * invElementCount;
    }

    // Half erasure
    {
        const s64 t0{now()};

        for (const K & key : secondHalfPresentKeys) {
            set.erase(key);
        }

        stats[size_t(Stat::erase)] += double(now() - t0) * invHalfElementCount;
    }

    // Half capacity iteration
    {
        const s64 t0{now()};

        for (const K & key : set) {
            key;
            v = v + 1;
        }

        stats[size_t(Stat::iterateHalf)] += double(now() - t0) * invHalfElementCount;
    }

    // Erase remaining elements
    {
        const s64 t0{now()};

        for (const K & key : firstHalfPresentKeys) {
            set.erase(key);
        }

        stats[size_t(Stat::erase)] += double(now() - t0) * invHalfElementCount;
    }

    // Empty access
    {
        const s64 t0{now()};

        for (const K & key : presentKeys) {
            v = v + set.count(key);
        }

        stats[size_t(Stat::accessEmpty)] += double(now() - t0) * invElementCount;
    }

    // Empty iteration
    {
        const s64 t0{now()};

        for (const K & key : set) {
            key;
            v = v + 1;
        }

        stats[size_t(Stat::iterateEmpty)] += double(now() - t0);
    }

    set.insert(presentKeys.front());

    // Single element begin
    {
        const s64 t0{now()};

        volatile const auto it{set.cbegin()};

        stats[size_t(Stat::loneBegin)] += double(now() - t0);
    }

    // Single element end
    {
        const s64 t0{now()};

        volatile const auto it{set.cend()};

        stats[size_t(Stat::loneEnd)] += double(now() - t0);
    }

    set.erase(presentKeys.front());

    // Reinsertion
    {
        const s64 t0{now()};

        for (const K & key : presentKeys) {
            set.insert(key);
        }

        stats[size_t(Stat::refill)] += double(now() - t0) * invElementCount;
    }

    // Clear
    {
        const s64 t0{now()};

        set.clear();

        stats[size_t(Stat::clear)] += double(now() - t0) * invElementCount;
    }

    // Reserved insertion
    {
        set.reserve(presentKeys.size());

        const s64 t0{now()};

        for (const K & key : presentKeys) {
            set.insert(key);
        }

        stats[size_t(Stat::insertReserved)] += double(now() - t0) * invElementCount;
    }

    // Destruct
    {
        const s64 t0{now()};

        set.~S();

        stats[size_t(Stat::destruction)] += double(now() - t0) * invElementCount;
    }

    return stats;
}

static void reportComparison(const std::vector<std::string> & setNames, const std::vector<std::map<size_t, Stats>> & setStats, const size_t set1I, const size_t set2I, const size_t elementCount) {
    static const std::string c1Header{std::format("{:d} Elements", elementCount)};
    static const std::string c4Header{"% Faster"};

    const std::string name1{setNames[set1I]};
    const std::string name2{setNames[set2I]};
    const Stats & stats1{setStats[set1I].at(1000u)};
    const Stats & stats2{setStats[set2I].at(1000u)};

    size_t c1Width{c1Header.size()};
    for (size_t opI{0u}; opI < size_t(Stat::_n); ++opI) {
        qc::maxify(c1Width, statNames[opI].size());
    }
    const size_t c2Width{qc::max(name1.size(), size_t(7u))};
    const size_t c3Width{qc::max(name2.size(), size_t(7u))};
    const size_t c4Width{qc::max(c4Header.size(), size_t(8u))};

    std::cout << std::format(" {:^{}} | {:^{}} | {:^{}} | {:^{}} ", c1Header, c1Width, name1, c2Width, name2, c3Width, c4Header, c4Width) << std::endl;
    std::cout << std::setfill('-') << std::setw(c1Width + 3u) << "+" << std::setw(c2Width + 3u) << "+" << std::setw(c3Width + 3u) << "+" << std::setw(c4Width + 2u) << "" << std::setfill(' ') << std::endl;
    for (size_t opI{0u}; opI < size_t(Stat::_n); ++opI) {
        const s64 t1{s64(std::round(stats1[opI] * 1000.0))};
        const s64 t2{s64(std::round(stats2[opI] * 1000.0))};

        std::cout << std::format(" {:^{}} | ", statNames[opI], c1Width);
        printTime(t1, c2Width);
        std::cout << " | ";
        printTime(t2, c3Width);
        std::cout << " | ";
        printFactor(t1, t2, c4Width);
        std::cout << std::endl;
    }
}

static std::vector<std::map<size_t, std::vector<double>>> reorganizeStats(const std::vector<std::map<size_t, Stats>> & setStats) {
    std::vector<std::map<size_t, std::vector<double>>> newStats((size_t(Stat::_n)));

    for (size_t statI{0u}; statI < size_t(Stat::_n); ++statI) {
        std::map<size_t, std::vector<double>> & statMap{newStats[statI]};

        for (size_t setI{0u}; setI < setStats.size(); ++setI) {
            for (const auto & [elementCount, stats] : setStats[setI]) {
                statMap[elementCount].push_back(stats[statI]);
            }
        }
    }

    return newStats;
}

static void printChartable(const std::vector<std::string> & setNames, const std::vector<std::map<size_t, Stats>> & setStats, std::ofstream & ofs) {

    const std::vector<std::map<size_t, std::vector<double>>> stats{reorganizeStats(setStats)};

    for (size_t statI{0u}; statI < size_t(Stat::_n); ++statI) {
        const std::map<size_t, std::vector<double>> & statMap{stats[statI]};

        ofs << statNames[statI] << ','; for (const std::string & name : setNames) ofs << name << ','; ofs << std::endl;

        size_t lineCount{0u};
        for (const auto & [elementCount, setTimes] : statMap) {
            ofs << elementCount << ',';
            for (const double time : setTimes) {
                ofs << time << ',';
            }
            ofs << std::endl;
            ++lineCount;
        }
        for (; lineCount < elementRoundCounts.size(); ++lineCount) {
            ofs << std::endl;
        }
    }
}

template <typename K, typename Set, typename RecordAllocatorSet, typename... SetPairs>
static void timeSets(std::vector<Stats> & setStats, const size_t setI, const std::vector<K> & presentKeys, const std::vector<K> & absentKeys) {
    setStats[setI] += time<Set>(presentKeys, absentKeys);

    if constexpr (sizeof...(SetPairs) != 0u) {
        timeSets<K, SetPairs...>(setStats, setI + 1u, presentKeys, absentKeys);
    }
}

template <typename K, typename Set, typename RecordAllocatorSet, typename... SetPairs>
static void compareMemory(std::vector<Stats> & setStats, const size_t setI, const std::vector<K> & keys) {
    Stats & stats{setStats[setI]};
    stats[size_t(Stat::objectSize)] = sizeof(Set);
    stats[size_t(Stat::iteratorSize)] = sizeof(typename Set::iterator);

    if constexpr (std::is_same_v<RecordAllocatorSet, void>) {
        stats[size_t(Stat::memoryOverhead)] = std::numeric_limits<double>::quiet_NaN();
    }
    else {
        const RecordAllocatorSet set{keys.cbegin(), keys.cend()};
        stats[size_t(Stat::memoryOverhead)] = double(set.get_allocator().stats().current - keys.size() * sizeof(K)) / double(keys.size());
    }

    if constexpr (sizeof...(SetPairs) != 0u) {
        compareMemory<K, SetPairs...>(setStats, setI + 1u, keys);
    }
}

template <typename K, typename... SetPairs, typename E>
static std::vector<Stats> compareSized(const size_t elementCount, const size_t roundCount, qc::Random<E> & random) {
    const double invRoundCount{1.0 / double(roundCount)};

    std::vector<K> presentKeys(elementCount);
    std::vector<K> absentKeys(elementCount);
    for (K & key : presentKeys) key = random.next<K>();
    std::vector<Stats> setStats(sizeof...(SetPairs) / 2);

    for (size_t round{0u}; round < roundCount; ++round) {
        std::swap(presentKeys, absentKeys);
        for (K & key : presentKeys) key = random.next<K>();

        timeSets<K, SetPairs...>(setStats, 0u, presentKeys, absentKeys);
    }

    for (Stats & stats : setStats) {
        for (double & stat : stats) {
            stat *= invRoundCount;
        }
    }

    compareMemory<K, SetPairs...>(setStats, 0u, presentKeys);

    return setStats;
}

template <typename K, typename... SetPairs>
static std::vector<std::map<size_t, Stats>> compare() {
    qc::Random random{u64(std::chrono::steady_clock::now().time_since_epoch().count())};
    std::vector<std::map<size_t, Stats>> setStats(sizeof...(SetPairs) / 2);

    for (const auto [elementCount, roundCount] : elementRoundCounts) {
        if (elementCount > std::numeric_limits<qc::utype<sizeof(K)>>::max()) {
            break;
        }

        std::cout << "Comparing " << roundCount << " rounds of " << elementCount << " elements...";

        const std::vector<Stats> elementCountStats{compareSized<K, SetPairs...>(elementCount, roundCount, random)};
        for (size_t i{0u}; i < setStats.size(); ++i) {
            setStats[i][elementCount] = elementCountStats[i];
        }

        std::cout << " done" << std::endl;
    }

    return setStats;
}

int main() {
    using K = u64;

    static const std::string outFileName{"out.txt"};

    std::vector<std::string> setNames{
        "qc::hash::Set",
        //"qc::hash::AltSet",
        "absl::flat_hash_set",
        "robin_hood::unordered_set",
        "ska::flat_hash_set",
        "tsl::sparse_hash_set",
        //"qc::hash::FlatSet",
        //"qc::hash::ChunkSet",
        //"qc::hash::OrigSet",
        "std::unordered_set",
    };

    std::vector<std::map<size_t, Stats>> setStats{compare<K,
        qc::hash::Set<K>,
        qc::hash::Set<K, qc::hash::Set<K>::hasher, qc::memory::RecordAllocator<K>>,
        //qc::hash_alt::Set<K>,
        //qc::hash_alt::Set<K, qc::hash_alt::Set<K>::hasher, qc::memory::RecordAllocator<K>>,
        absl::flat_hash_set<K>,
        absl::flat_hash_set<K, absl::flat_hash_set<K>::hasher, absl::flat_hash_set<K>::key_equal, qc::memory::RecordAllocator<K>>,
        robin_hood::unordered_set<K>,
        void,
        ska::flat_hash_set<K>,
        ska::flat_hash_set<K, ska::flat_hash_set<K>::hasher, ska::flat_hash_set<K>::key_equal, qc::memory::RecordAllocator<K>>,
        tsl::sparse_set<K>,
        tsl::sparse_set<K, tsl::sparse_set<K>::hasher, tsl::sparse_set<K>::key_equal, qc::memory::RecordAllocator<K>>,
        //qc_hash_flat::Set<K>,
        //qc_hash_flat::Set<K, qc_hash_flat::Set<K>::hasher, qc_hash_flat::Set<K>::key_equal, qc::memory::RecordAllocator<K>>
        //qc_hash_chunk::Set<K>,
        //qc_hash_chunk::Set<K, qc_hash_chunk::Set<K>::hasher, qc_hash_chunk::Set<K>::key_equal, qc::memory::RecordAllocator<K>>
        //qc_hash_orig::Set<K>,
        //qc_hash_orig::Set<K, qc_hash_orig::Set<K>::hasher, qc_hash_orig::Set<K>::key_equal, qc::memory::RecordAllocator<K>>
        std::unordered_set<K>,
        std::unordered_set<K, std::unordered_set<K>::hasher, std::unordered_set<K>::key_equal, qc::memory::RecordAllocator<K>>
    >()};

    if (setStats.size() != setNames.size()) {
        std::cout << "Names not the same length as stats!" << std::endl;
        throw std::exception{};
    }

    std::cout << std::endl;

    if constexpr (false) {
        for (const auto [elementCount, roundCount] : elementRoundCounts) {
            reportComparison(setNames, setStats, 4, 0, elementCount);
            std::cout << std::endl;
        }
    }
    else {
        std::ofstream ofs{outFileName};
        printChartable(setNames, setStats, ofs);
        std::cout << "Wrote results to " << outFileName << std::endl;
    }

    absl::flat_hash_set<int> s;
    auto it = s.begin();
    ++it;

    return 0;
}
