#include <array>
#include <chrono>
#include <format>
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
            v = v + set.contains(key);
        }

        stats[size_t(Stat::accessPresent)] += double(now() - t0) * invElementCount;
    }

    // Full capacity access absent elements
    {
        const s64 t0{now()};

        for (const K & key : nonpresentKeys) {
            v = v + set.contains(key);
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
            v = v + set.contains(key);
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

//static void reportComparison(const std::pair<std::string, Stats> & setStats1, const std::pair<std::string, Stats> & setStats2) {
//    static const std::string c1Header{"Stat"};
//    static const std::string c4Header{"% Faster"};
//
//    size_t c1Width{c1Header.size()};
//    for (Stat op{}; op < Stat::_n; ++op) {
//        qc::maxify(c1Width, statNames[size_t(op)].size());
//    }
//    const size_t c2Width{qc::max(setStats1.first.size(), size_t(7u))};
//    const size_t c3Width{qc::max(setStats2.first.size(), size_t(7u))};
//    const size_t c4Width{qc::max(c4Header.size(), size_t(8u))};
//
//    std::cout << std::format(" {:^{}} | {:^{}} | {:^{}} | {:^{}} ", c1Header, c1Width, setStats1.first, c2Width, setStats2.first, c3Width, c4Header, c4Width) << std::endl;
//    std::cout << "---------------+---------+---------+----------" << std::endl;
//    for (Stat op{}; op < Stat::_n; ++op) {
//        const s64 t1{setStats1.second[size_t(op)]};
//        const s64 t2{setStats2.second[size_t(op)]};
//
//        std::cout << std::format(" {:^{}} | ", statNames[size_t(op)], c1Width);
//        printTime(t1, c2Width);
//        std::cout << " | ";
//        printTime(t2, c3Width);
//        std::cout << " | ";
//        printFactor(t1, t2, c4Width);
//        std::cout << std::endl;
//    }
//}

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

static void printChartable(const std::vector<std::string> & setNames, const std::vector<std::map<size_t, Stats>> & setStats) {

    const std::vector<std::map<size_t, std::vector<double>>> stats{reorganizeStats(setStats)};

    for (size_t statI{0u}; statI < size_t(Stat::_n); ++statI) {
        const std::map<size_t, std::vector<double>> & statMap{stats[statI]};

        std::cout << statNames[statI] << ','; for (const std::string & name : setNames) std::cout << name << ','; std::cout << std::endl;

        size_t lineCount{0u};
        for (const auto & [elementCount, setTimes] : statMap) {
            std::cout << elementCount << ',';
            for (const double time : setTimes) {
                std::cout << time << ',';
            }
            std::cout << std::endl;
            ++lineCount;
        }
        for (; lineCount < elementRoundCounts.size(); ++lineCount) {
            std::cout << std::endl;
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
    const RecordAllocatorSet set{keys.cbegin(), keys.cend()};
    stats[size_t(Stat::memoryOverhead)] = double(set.get_allocator().current() - keys.size() * sizeof(K)) / double(keys.size());

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
    using K = u8;

    std::vector<std::string> setNames{
        "qc::hash::Set",
        //"qc::hash::AltSet",
        "qc::hash::FlatSet",
        "qc::hash::ChunkSet",
        "qc::hash::OrigSet",
        "std::unordered_set"
    };

    std::vector<std::map<size_t, Stats>> setStats{compare<K,
        qc::hash::Set<K>,
        qc::hash::Set<K, qc::hash::Set<K>::hasher, qc::memory::RecordAllocator<K>>,
        //qc::hash_alt::Set<K>,
        //qc::hash_alt::Set<K, qc::hash_alt::Set<K>::hasher, qc::memory::RecordAllocator<K>>,
        qc_hash_flat::Set<K>,
        qc_hash_flat::Set<K, qc_hash_flat::Set<K>::hasher, qc_hash_flat::Set<K>::key_equal, qc::memory::RecordAllocator<K>>,
        qc_hash_chunk::Set<K>,
        qc_hash_chunk::Set<K, qc_hash_chunk::Set<K>::hasher, qc_hash_chunk::Set<K>::key_equal, qc::memory::RecordAllocator<K>>,
        qc_hash_orig::Set<K>,
        qc_hash_orig::Set<K, qc_hash_orig::Set<K>::hasher, qc_hash_orig::Set<K>::key_equal, qc::memory::RecordAllocator<K>>,
        std::unordered_set<K>,
        std::unordered_set<K, std::unordered_set<K>::hasher, std::unordered_set<K>::key_equal, qc::memory::RecordAllocator<K>>
    >()};

    if (setStats.size() != setNames.size()) {
        throw std::exception{};
    }

    std::cout << std::endl;

    //reportComparison(setStats[4], setStats[5]);
    printChartable(setNames, setStats);

    return 0;
}
