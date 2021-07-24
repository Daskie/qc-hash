#include <array>
#include <chrono>
#include <filesystem>
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

#pragma warning(push)
#pragma warning(disable:4127)
#pragma warning(disable:4458)
#pragma warning(disable:4324)
#include <absl/container/flat_hash_set.h>
#include "robin_hood.h"
#include "flat_hash_map.hpp"
#include "tsl/robin_set.h"
#include "tsl/sparse_set.h"
#pragma warning(pop)

using namespace qc::types;

static const std::vector<std::pair<size_t, size_t>> detailedElementRoundCountsRelease{
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

static const std::vector<std::pair<size_t, size_t>> detailedElementRoundCountsDebug{
    {     10u, 1000u},
    {    100u, 1000u},
    {   1000u, 1000u},
    {  10000u,  100u},
    { 100000u,   10u},
    {1000000u,    3u}
};

static const std::vector<std::pair<size_t, size_t>> & detailedElementRoundCounts{qc::debug ? detailedElementRoundCountsDebug : detailedElementRoundCountsRelease};

static const size_t detailedChartRows{qc::max(detailedElementRoundCountsRelease.size(), detailedElementRoundCountsDebug.size())};

static const std::vector<std::pair<size_t, size_t>> typicalElementRoundCounts{
    {      10u, 1000u},
    {     100u, 1000u},
    {    1000u, 1000u},
    {   10000u,  100u},
    {  100000u,   10u},
    { 1000000u,    5u},
    {10000000u,    3u}
};

enum class Stat : size_t
{
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

class Stats
{
    public:

    double & get(const size_t setI, const size_t elementCount, const Stat stat) {
        _presentSetIndexes.insert(setI);
        _presentElementCounts.insert(elementCount);
        _presentStats.insert(stat);
        return _table[setI][elementCount][stat];
    }

    double & at(const size_t setI, const size_t elementCount, const Stat stat) {
        return const_cast<double &>(const_cast<const Stats *>(this)->at(setI, elementCount, stat));
    }

    const double & at(const size_t setI, const size_t elementCount, const Stat stat) const {
        return _table.at(setI).at(elementCount).at(stat);
    }

    const std::set<size_t> presentSetIndexes() const { return _presentSetIndexes; }

    const std::set<size_t> presentElementCounts() const { return _presentElementCounts; }

    const std::set<Stat> presentStats() const { return _presentStats; }

    private:

    std::map<size_t, std::map<size_t, std::map<Stat, double>>> _table{};
    std::set<size_t> _presentSetIndexes{};
    std::set<size_t> _presentElementCounts{};
    std::set<Stat> _presentStats{};
};

static s64 now()
{
    return std::chrono::nanoseconds(std::chrono::steady_clock::now().time_since_epoch()).count();
}

static void printTime(const s64 nanoseconds, const size_t width)
{
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

static void printFactor(const s64 t1, const s64 t2, const size_t width)
{
    const double absFactor{t1 >= t2 ? double(t1) / double(t2) : double(t2) / double(t1)};
    int percent{int(qc::round(absFactor * 100.0)) - 100};
    if (t1 < t2) {
        percent = -percent;
    }
    std::cout << std::setw(width - 2) << percent << " %";
}

static void reportComparison(const std::vector<std::string> & setNames, const Stats & results, const size_t set1I, const size_t set2I, const size_t elementCount)
{
    const std::string c1Header{std::format("{:d} Elements", elementCount)};
    const std::string c4Header{"% Faster"};

    const std::string name1{setNames[set1I]};
    const std::string name2{setNames[set2I]};

    size_t c1Width{c1Header.size()};
    for (const Stat stat : results.presentStats()) {
        qc::maxify(c1Width, statNames[size_t(stat)].size());
    }
    const size_t c2Width{qc::max(name1.size(), size_t(7u))};
    const size_t c3Width{qc::max(name2.size(), size_t(7u))};
    const size_t c4Width{qc::max(c4Header.size(), size_t(8u))};

    std::cout << std::format(" {:^{}} | {:^{}} | {:^{}} | {:^{}} ", c1Header, c1Width, name1, c2Width, name2, c3Width, c4Header, c4Width) << std::endl;
    std::cout << std::setfill('-') << std::setw(c1Width + 3u) << "+" << std::setw(c2Width + 3u) << "+" << std::setw(c3Width + 3u) << "+" << std::setw(c4Width + 2u) << "" << std::setfill(' ') << std::endl;

    for (const Stat stat : results.presentStats()) {
        const s64 t1{s64(std::round(results.at(set1I, elementCount, stat)) * 1000.0)};
        const s64 t2{s64(std::round(results.at(set2I, elementCount, stat)) * 1000.0)};

        std::cout << std::format(" {:^{}} | ", statNames[size_t(stat)], c1Width);
        printTime(t1, c2Width);
        std::cout << " | ";
        printTime(t2, c3Width);
        std::cout << " | ";
        printFactor(t1, t2, c4Width);
        std::cout << std::endl;
    }
}

static void printOpsChartable(const std::vector<std::string> & setNames, const Stats & results, std::ofstream & ofs)
{
    for (const Stat stat : results.presentStats()) {
        ofs << statNames[size_t(stat)] << ','; for (const std::string & name : setNames) ofs << name << ','; ofs << std::endl;

        size_t lineCount{0u};
        for (const size_t elementCount : results.presentElementCounts()) {
            ofs << elementCount << ',';
            for (const size_t setI : results.presentSetIndexes()) {
                ofs << results.at(setI, elementCount, stat) << ',';
            }
            ofs << std::endl;
            ++lineCount;
        }
        for (; lineCount < detailedChartRows; ++lineCount) {
            ofs << std::endl;
        }
    }
}

static void printTypicalChartable(const std::vector<std::string> & setNames, const Stats & results, std::ofstream & ofs)
{
    for (const size_t elementCount : results.presentElementCounts()) {
        ofs << elementCount << ",Insert,Access,Iterate,Erase" << std::endl;

        for (const size_t setI : results.presentSetIndexes()) {
            ofs << setNames[setI];
            ofs << ',' << results.at(setI, elementCount, Stat::insertReserved);
            ofs << ',' << results.at(setI, elementCount, Stat::accessPresent);
            ofs << ',' << results.at(setI, elementCount, Stat::iterateFull);
            ofs << ',' << results.at(setI, elementCount, Stat::erase);
            ofs << std::endl;
        }
    }
}

template <typename S, typename K>
static void time(const size_t setI, const std::vector<K> & presentKeys, const std::vector<K> & nonpresentKeys, Stats & results)
{
    static volatile size_t v{};

    const std::span<const K> firstHalfPresentKeys{&presentKeys[0], presentKeys.size() / 2};
    const std::span<const K> secondHalfPresentKeys{&presentKeys[presentKeys.size() / 2], presentKeys.size() / 2};
    const double invElementCount{1.0 / double(presentKeys.size())};
    const double invHalfElementCount{invElementCount * 2.0};

    alignas(S) std::byte backingMemory[sizeof(S)];
    S * setPtr;

    std::array<double, size_t(Stat::_n)> stats{};

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
            // Important to actually use the value as to load the memory
            v = v + size_t(key);
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
            // Important to actually use the value as to load the memory
            v = v + size_t(key);
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
            // Important to actually use the value as to load the memory
            v = v + size_t(key);
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

    for (size_t i{}; i < stats.size(); ++i) {
        results.get(setI, presentKeys.size(), Stat(i)) += stats[i];
    }
}

template <typename S, typename K>
static void timeTypical(const size_t setI, const std::vector<K> & keys, Stats & results)
{
    static volatile size_t v{};

    S set{};
    set.reserve(keys.size());

    const s64 t0{now()};

    // Insert
    for (const K & key : keys) {
        set.insert(key);
    }

    const s64 t1{now()};

    // Access
    for (const K & key : keys) {
        v = v + set.count(key);
    }

    const s64 t2{now()};

    // Iterate
    for (const K & key : set) {
        // Important to actually use the value as to load the memory
        v = v + size_t(key);
    }

    const s64 t3{now()};

    // Erase
    for (const K & key : keys) {
        set.erase(key);
    }

    const s64 t4{now()};

    results.get(setI, keys.size(), Stat::insertReserved) += double(t1 - t0);
    results.get(setI, keys.size(), Stat::accessPresent) += double(t2 - t1);
    results.get(setI, keys.size(), Stat::iterateFull) += double(t3 - t2);
    results.get(setI, keys.size(), Stat::erase) += double(t4 - t3);
}

template <typename K, typename Set, typename RecordAllocatorSet, typename... SetPairs>
static void timeSets(const size_t setI, const std::vector<K> & presentKeys, const std::vector<K> & absentKeys, Stats & results)
{
    if constexpr (!std::is_same_v<Set, void>) {
        time<Set>(setI, presentKeys, absentKeys, results);
    }

    if constexpr (sizeof...(SetPairs) != 0u) {
        timeSets<K, SetPairs...>(setI + 1u, presentKeys, absentKeys, results);
    }
}

template <typename K, typename Set, typename RecordAllocatorSet, typename... SetPairs>
static void timeSetsTypical(const size_t setI, const std::vector<K> & keys, Stats & results)
{
    if constexpr (!std::is_same_v<Set, void>) {
        timeTypical<Set>(setI, keys, results);
    }

    if constexpr (sizeof...(SetPairs) != 0u) {
        timeSetsTypical<K, SetPairs...>(setI + 1u, keys, results);
    }
}

template <typename K, typename Set, typename RecordAllocatorSet, typename... SetPairs>
static void compareMemory(const size_t setI, const std::vector<K> & keys, Stats & results)
{
    if constexpr (!std::is_same_v<Set, void>) {
        results.get(setI, keys.size(), Stat::objectSize) = sizeof(Set);
        results.get(setI, keys.size(), Stat::iteratorSize) = sizeof(typename Set::iterator);
    }

    if constexpr (!std::is_same_v<RecordAllocatorSet, void>) {
        const RecordAllocatorSet set{keys.cbegin(), keys.cend()};
        results.get(setI, keys.size(), Stat::memoryOverhead) = double(set.get_allocator().stats().current - keys.size() * sizeof(K)) / double(keys.size());
    }

    if constexpr (sizeof...(SetPairs) != 0u) {
        compareMemory<K, SetPairs...>(setI + 1u, keys, results);
    }
}

template <typename K, typename... SetPairs>
static void compareDetailedSized(const size_t elementCount, const size_t roundCount, qc::Random & random, Stats & results)
{
    const double invRoundCount{1.0 / double(roundCount)};

    std::vector<K> presentKeys(elementCount);
    std::vector<K> absentKeys(elementCount);
    for (K & key : presentKeys) key = random.next<K>();

    for (size_t round{0u}; round < roundCount; ++round) {
        std::swap(presentKeys, absentKeys);
        for (K & key : presentKeys) key = random.next<K>();

        timeSets<K, SetPairs...>(0u, presentKeys, absentKeys, results);
    }

    for (const size_t setI : results.presentSetIndexes()) {
        for (const Stat stat : results.presentStats()) {
            results.at(setI, elementCount, stat) *= invRoundCount;
        }
    }

    compareMemory<K, SetPairs...>(0u, presentKeys, results);
}

template <typename K, typename... SetPairs>
static void compareDetailed(Stats & results)
{
    qc::Random random{size_t(std::chrono::steady_clock::now().time_since_epoch().count())};

    for (const auto [elementCount, roundCount] : detailedElementRoundCounts) {
        if (elementCount > std::numeric_limits<qc::utype<K>>::max()) {
            break;
        }

        std::cout << "Comparing " << roundCount << " rounds of " << elementCount << " elements...";

        compareDetailedSized<K, SetPairs...>(elementCount, roundCount, random, results);

        std::cout << " done" << std::endl;
    }
}

template <typename K, typename... SetPairs>
static void compareTypicalSized(const size_t elementCount, const size_t roundCount, qc::Random & random, Stats & results)
{
    std::vector<K> keys(elementCount);

    for (size_t round{0u}; round < roundCount; ++round) {
        for (K & key : keys) key = random.next<K>();
        timeSetsTypical<K, SetPairs...>(0u, keys, results);
    }

    const double invRoundCount{1.0 / double(roundCount)};
    for (const size_t setI : results.presentSetIndexes()) {
        for (const Stat stat : results.presentStats()) {
            results.at(setI, elementCount, stat) *= invRoundCount;
        }
    }
}

template <typename K, typename... SetPairs>
static void compareTypical(Stats & results)
{
    qc::Random random{size_t(std::chrono::steady_clock::now().time_since_epoch().count())};

    for (const auto [elementCount, roundCount] : typicalElementRoundCounts) {
        if (elementCount > std::numeric_limits<qc::utype<K>>::max()) {
            break;
        }

        std::cout << "Comparing " << roundCount << " rounds of " << elementCount << " elements...";

        compareTypicalSized<K, SetPairs...>(elementCount, roundCount, random, results);

        std::cout << " done" << std::endl;
    }
}

template <typename K, typename... SetPairs>
static void compare(const std::vector<std::string> & setNames)
{
    static const std::filesystem::path outFilePath{"out.txt"};

    if (sizeof...(SetPairs) / 2 != setNames.size()) {
        std::cout << "Wrong number of names!" << std::endl;
        throw std::exception{};
    }

    // 1-vs-1
    if constexpr (false) {
        static_assert(sizeof...(SetPairs) == 4);
        Stats results{};
        compareDetailed<K, SetPairs...>(results);
        std::cout << std::endl;
        for (const auto[elementCount, roundCount] : detailedElementRoundCounts) {
            reportComparison(setNames, results, 1, 0, elementCount);
            std::cout << std::endl;
        }
    }
    // Detailed
    else if constexpr (true) {
        Stats results{};
        compareDetailed<K, SetPairs...>(results);
        std::ofstream ofs{outFilePath};
        printOpsChartable(setNames, results, ofs);
        std::cout << "Wrote results to " << outFilePath << std::endl;
    }
    // Typical
    else {
        Stats results{};
        compareTypical<K, SetPairs...>(results);
        std::ofstream ofs{outFilePath};
        printTypicalChartable(setNames, results, ofs);
        std::cout << "Wrote results to " << outFilePath << std::endl;
    }
}

int main()
{
    using K = u64;

    compare<K,
        qc::hash::RawSet<K>,
        qc::hash::RawSet<K, qc::hash::RawSet<K>::hasher, qc::hash::RawSet<K>::key_equal, qc::memory::RecordAllocator<K>>,
        //qc::hash_alt::Set<K>,
        //qc::hash_alt::Set<K, qc::hash_alt::Set<K>::hasher, qc::memory::RecordAllocator<K>>
        std::unordered_set<K>,
        std::unordered_set<K, std::unordered_set<K>::hasher, std::unordered_set<K>::key_equal, qc::memory::RecordAllocator<K>>,
        std::conditional_t<sizeof(size_t) == 8, absl::flat_hash_set<K>, void>,
        std::conditional_t<sizeof(size_t) == 8, absl::flat_hash_set<K, absl::flat_hash_set<K>::hasher, absl::flat_hash_set<K>::key_equal, qc::memory::RecordAllocator<K>>, void>,
        robin_hood::unordered_set<K>,
        void,
        ska::flat_hash_set<K>,
        ska::flat_hash_set<K, ska::flat_hash_set<K>::hasher, ska::flat_hash_set<K>::key_equal, qc::memory::RecordAllocator<K>>,
        tsl::robin_set<K>,
        tsl::robin_set<K, tsl::robin_set<K>::hasher, tsl::robin_set<K>::key_equal, qc::memory::RecordAllocator<K>>,
        tsl::sparse_set<K>,
        tsl::sparse_set<K, tsl::sparse_set<K>::hasher, tsl::sparse_set<K>::key_equal, qc::memory::RecordAllocator<K>>
        //qc_hash_flat::Set<K>,
        //qc_hash_flat::Set<K, qc_hash_flat::Set<K>::hasher, qc_hash_flat::Set<K>::key_equal, qc::memory::RecordAllocator<K>>
        //qc_hash_chunk::Set<K>,
        //qc_hash_chunk::Set<K, qc_hash_chunk::Set<K>::hasher, qc_hash_chunk::Set<K>::key_equal, qc::memory::RecordAllocator<K>>
        //qc_hash_orig::Set<K>,
        //qc_hash_orig::Set<K, qc_hash_orig::Set<K>::hasher, qc_hash_orig::Set<K>::key_equal, qc::memory::RecordAllocator<K>>
    >({
       "qc::hash::Set",
       //"qc::hash::AltSet",
       "std::unordered_set",
       "absl::flat_hash_set",
       "robin_hood::unordered_set",
       "ska::flat_hash_set",
       "tsl::robin_set",
       "tsl::sparse_hash_set",
       //"qc::hash::FlatSet",
       //"qc::hash::ChunkSet",
       //"qc::hash::OrigSet",
    });

    return 0;
}
