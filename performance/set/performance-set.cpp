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

static Stat & operator++(Stat & op) {
    return op = Stat(size_t(op) + 1u);
}

using Stats = std::array<s64, size_t(Stat::_n)>;

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

    alignas(S) std::byte backingMemory[sizeof(S)];
    S * setPtr;
    Stats stats{};

    // Construct
    {
        const s64 t0{now()};

        setPtr = new(backingMemory) S{};

        stats[size_t(Stat::construct)] += now() - t0;
    }

    S & set{*setPtr};

    // Insert to full capacity
    {
        const s64 t0{now()};

        for (const K & key : presentKeys) {
            set.insert(key);
        }

        stats[size_t(Stat::insert)] += now() - t0;
    }

    // Full capacity insert present elements
    {
        const s64 t0{now()};

        for (const K & key : presentKeys) {
            set.insert(key);
        }

        stats[size_t(Stat::insertPresent)] += now() - t0;
    }

    // Full capacity access present elements
    {
        const s64 t0{now()};

        for (const K & key : presentKeys) {
            v = v + set.contains(key);
        }

        stats[size_t(Stat::accessPresent)] += now() - t0;
    }

    // Full capacity access absent elements
    {
        const s64 t0{now()};

        for (const K & key : nonpresentKeys) {
            v = v + set.contains(key);
        }

        stats[size_t(Stat::accessAbsent)] += now() - t0;
    }

    // Full capacity iteration
    {
        const s64 t0{now()};

        for (const K & key : set) {
            key;
            v = v + 1;
        }

        stats[size_t(Stat::iterateFull)] += now() - t0;
    }

    // Full capacity erase absent elements
    {
        const s64 t0{now()};

        for (const K & key : nonpresentKeys) {
            set.erase(key);
        }

        stats[size_t(Stat::eraseAbsent)] += now() - t0;
    }

    // Half erasure
    {
        const s64 t0{now()};

        for (const K & key : secondHalfPresentKeys) {
            set.erase(key);
        }

        stats[size_t(Stat::erase)] += now() - t0;
    }

    // Half capacity iteration
    {
        const s64 t0{now()};

        for (const K & key : set) {
            key;
            v = v + 1;
        }

        stats[size_t(Stat::iterateHalf)] += now() - t0;
    }

    // Erase remaining elements
    {
        const s64 t0{now()};

        for (const K & key : firstHalfPresentKeys) {
            set.erase(key);
        }

        stats[size_t(Stat::erase)] += now() - t0;
    }

    // Empty access
    {
        const s64 t0{now()};

        for (const K & key : presentKeys) {
            v = v + set.contains(key);
        }

        stats[size_t(Stat::accessEmpty)] += now() - t0;
    }

    // Empty iteration
    {
        const s64 t0{now()};

        for (const K & key : set) {
            key;
            v = v + 1;
        }

        stats[size_t(Stat::iterateEmpty)] += now() - t0;
    }

    set.insert(presentKeys.front());

    // Single element begin
    {
        const s64 t0{now()};

        volatile const auto it{set.cbegin()};

        stats[size_t(Stat::loneBegin)] += now() - t0;
    }

    // Single element end
    {
        const s64 t0{now()};

        volatile const auto it{set.cend()};

        stats[size_t(Stat::loneEnd)] += now() - t0;
    }

    set.erase(presentKeys.front());

    // Reinsertion
    {
        const s64 t0{now()};

        for (const K & key : presentKeys) {
            set.insert(key).second;
        }

        stats[size_t(Stat::refill)] += now() - t0;
    }

    // Clear
    {
        const s64 t0{now()};

        set.clear();

        stats[size_t(Stat::clear)] += now() - t0;
    }

    // Reserved insertion
    {
        set.reserve(presentKeys.size());

        const s64 t0{now()};

        for (const K & key : presentKeys) {
            set.insert(key);
        }

        stats[size_t(Stat::insertReserved)] += now() - t0;
    }

    // Destruct
    {
        const s64 t0{now()};

        set.~S();

        stats[size_t(Stat::destruction)] += now() - t0;
    }

    return stats;
}

static void reportComparison(const std::pair<std::string, Stats> & setStats1, const std::pair<std::string, Stats> & setStats2) {
    static const std::string c1Header{"Stat"};
    static const std::string c4Header{"% Faster"};

    size_t c1Width{c1Header.size()};
    for (Stat op{}; op < Stat::_n; ++op) {
        qc::maxify(c1Width, statNames[size_t(op)].size());
    }
    const size_t c2Width{qc::max(setStats1.first.size(), size_t(7u))};
    const size_t c3Width{qc::max(setStats2.first.size(), size_t(7u))};
    const size_t c4Width{qc::max(c4Header.size(), size_t(8u))};

    std::cout << std::format(" {:^{}} | {:^{}} | {:^{}} | {:^{}} ", c1Header, c1Width, setStats1.first, c2Width, setStats2.first, c3Width, c4Header, c4Width) << std::endl;
    std::cout << "---------------+---------+---------+----------" << std::endl;
    for (Stat op{}; op < Stat::_n; ++op) {
        const s64 t1{setStats1.second[size_t(op)]};
        const s64 t2{setStats2.second[size_t(op)]};

        std::cout << std::format(" {:^{}} | ", statNames[size_t(op)], c1Width);
        printTime(t1, c2Width);
        std::cout << " | ";
        printTime(t2, c3Width);
        std::cout << " | ";
        printFactor(t1, t2, c4Width);
        std::cout << std::endl;
    }
}

static void printChartable(const std::vector<std::pair<std::string, Stats>> & setStats) {
    std::cout << ','; for (const auto & [name, stats] : setStats) std::cout << name << ','; std::cout << std::endl;
    for (Stat stat{}; stat < Stat::_n; ++stat) {
        std::cout << statNames[size_t(stat)] << ','; for (const auto & [name, stats] : setStats) std::cout << stats[size_t(stat)] << ','; std::cout << std::endl;
    }
}

int main() {
    const size_t elementCount{1000u};
    const size_t roundCount{5000u};
    using K = size_t;
    using S1 = std::unordered_set<K>;
    using S2 = qc_hash_orig::Set<K>;
    using S3 = qc_hash_chunk::Set<K>;
    using S4 = qc_hash_flat::Set<K>;
    //using S5 = qc_hash_alt::Set<K>;
    using S6 = qc::hash::Set<K>;

    qc::Random random{};
    std::vector<K> presentKeys(elementCount);
    std::vector<K> nonpresentKeys(elementCount);
    for (K & key : presentKeys) key = random.next<K>();

    std::vector<std::pair<std::string, Stats>> setStats{
        {"std::unordered_set", {sizeof(S1), sizeof(S1::iterator)}},
        {"qc::hash::OrigSet",  {sizeof(S2), sizeof(S2::iterator)}},
        {"qc::hash::ChunkSet", {sizeof(S3), sizeof(S3::iterator)}},
        {"qc::hash::FlatSet",  {sizeof(S4), sizeof(S4::iterator)}},
        //{"qc::hash::AltSet",   {sizeof(S5), sizeof(S5::iterator)}},
        {"qc::hash::Set",      {sizeof(S6), sizeof(S6::iterator)}}
    };

    for (size_t round{0u}; round < roundCount; ++round) {
        std::swap(presentKeys, nonpresentKeys);
        for (K & key : presentKeys) key = random.next<K>();

        size_t statsI{0u};
        setStats[statsI++].second += time<S1>(presentKeys, nonpresentKeys);
        setStats[statsI++].second += time<S2>(presentKeys, nonpresentKeys);
        setStats[statsI++].second += time<S3>(presentKeys, nonpresentKeys);
        setStats[statsI++].second += time<S4>(presentKeys, nonpresentKeys);
        //setStats[statsI++].second += time<S5>(presentKeys, nonpresentKeys);
        setStats[statsI++].second += time<S6>(presentKeys, nonpresentKeys);
    }

    //reportComparison(setStats[4], setStats[5]);
    printChartable(setStats);

    return 0;
}
