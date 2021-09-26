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

#include <qc-hash/qc-map.hpp>

#pragma warning(push)
#pragma warning(disable: 4127 4458 4324 4293 4309 4305 4244)
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include "extern/robin_hood.h"
#include "extern/flat_hash_map.hpp"
#include "extern/tsl/robin_map.h"
#include "extern/tsl/robin_set.h"
#include "extern/tsl/sparse_map.h"
#include "extern/tsl/sparse_set.h"
#pragma warning(pop)

using namespace qc::types;

template <typename C> concept IsMap = !std::is_same_v<typename C::mapped_type, void>;

static const std::vector<std::pair<size_t, size_t>> detailedElementRoundCountsRelease{
    {         5u, 200'000u},
    {        10u, 100'000u},
    {        25u,  40'000u},
    {        50u,  20'000u},
    {       100u,  10'000u},
    {       250u,   4'000u},
    {       500u,   2'000u},
    {     1'000u,   1'000u},
    {     2'500u,     400u},
    {     5'000u,     200u},
    {    10'000u,     100u},
    {    25'000u,      40u},
    {    50'000u,      20u},
    {   100'000u,      10u},
    {   250'000u,      10u},
    {   500'000u,      10u},
    { 1'000'000u,       5u},
    { 2'500'000u,       5u},
    { 5'000'000u,       5u},
    {10'000'000u,       3u}
};

static const std::vector<std::pair<size_t, size_t>> detailedElementRoundCountsDebug{
    {       10u, 100'000u},
    {      100u,  10'000u},
    {    1'000u,   1'000u},
    {   10'000u,     100u},
    {  100'000u,      10u},
    {1'000'000u,       3u}
};

static const std::vector<std::pair<size_t, size_t>> & detailedElementRoundCounts{qc::debug ? detailedElementRoundCountsDebug : detailedElementRoundCountsRelease};

static const size_t detailedChartRows{qc::max(detailedElementRoundCountsRelease.size(), detailedElementRoundCountsDebug.size())};

static const std::vector<std::pair<size_t, size_t>> typicalElementRoundCounts{
    {        10u, 1'000'000u},
    {       100u,   100'000u},
    {     1'000u,    10'000u},
    {    10'000u,     1'000u},
    {   100'000u,       100u},
    { 1'000'000u,        10u},
    {10'000'000u,         3u}
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

template <size_t size> struct Trivial;

template <size_t size> requires (size <= 8)
struct Trivial<size>
{
    typename qc::sized<size>::utype val;
};

template <size_t size> requires (size > 8)
struct Trivial<size>
{
    std::array<u64, size / 8> val;
};

static_assert(std::is_trivial_v<Trivial<1>>);
static_assert(std::is_trivial_v<Trivial<8>>);
static_assert(std::is_trivial_v<Trivial<64>>);

template <size_t size>
class Complex : public Trivial<size>
{
    public:

    constexpr Complex() : Trivial<size>{} {}

    constexpr Complex(const Complex & other) = default;

    constexpr Complex(Complex && other) noexcept :
        Trivial<size>{std::exchange(other.val, {})}
    {}

    Complex & operator=(const Complex &) = delete;

    Complex && operator=(Complex && other) noexcept {
        Trivial::val = std::exchange(other.val, {});
    }

    constexpr ~Complex() noexcept {}
};

static_assert(!std::is_trivial_v<Complex<1>>);
static_assert(!std::is_trivial_v<Complex<8>>);
static_assert(!std::is_trivial_v<Complex<64>>);

template <size_t size> requires (size <= sizeof(size_t))
struct qc::hash::RawHash<Trivial<size>>
{
    constexpr size_t operator()(const Trivial<size> k) const noexcept
    {
        return k.val;
    }
};

template <size_t size> requires (size <= sizeof(size_t))
struct qc::hash::RawHash<Complex<size>>
{
    constexpr size_t operator()(const Complex<size> & k) const noexcept
    {
        return k.val;
    }
};

class Stats
{
    public:

    double & get(const size_t containerI, const size_t elementCount, const Stat stat)
    {
        _presentContainerIndices.insert(containerI);
        _presentElementCounts.insert(elementCount);
        _presentStats.insert(stat);
        return _table[containerI][elementCount][stat];
    }

    double & at(const size_t containerI, const size_t elementCount, const Stat stat)
    {
        return const_cast<double &>(const_cast<const Stats *>(this)->at(containerI, elementCount, stat));
    }

    const double & at(const size_t containerI, const size_t elementCount, const Stat stat) const
    {
        return _table.at(containerI).at(elementCount).at(stat);
    }

    const std::set<size_t> presentContainerIndices() const { return _presentContainerIndices; }

    const std::set<size_t> presentElementCounts() const { return _presentElementCounts; }

    const std::set<Stat> presentStats() const { return _presentStats; }

    void setContainerNames(const std::vector<std::string> & containerNames)
    {
        if (containerNames.size() != _presentContainerIndices.size()) throw std::exception{};
        _containerNames = containerNames;
    }

    template <typename... ContainerInfos>
    void setContainerNames()
    {
        _containerNames.clear();
        (_containerNames.push_back(ContainerInfos::name), ...);
    }

    const std::string & containerName(const size_t containerI) const
    {
        return _containerNames.at(containerI);
    }

    private:

    std::map<size_t, std::map<size_t, std::map<Stat, double>>> _table{};
    std::set<size_t> _presentContainerIndices{};
    std::set<size_t> _presentElementCounts{};
    std::set<Stat> _presentStats{};
    std::vector<std::string> _containerNames{};
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

#pragma warning(suppress: 4505)
static void reportComparison(const Stats & results, const size_t container1I, const size_t container2I, const size_t elementCount)
{
    const std::string c1Header{std::format("{:d} Elements", elementCount)};
    const std::string c4Header{"% Faster"};

    const std::string name1{results.containerName(container1I)};
    const std::string name2{results.containerName(container2I)};

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
        const s64 t1{s64(std::round(results.at(container1I, elementCount, stat)))};
        const s64 t2{s64(std::round(results.at(container2I, elementCount, stat)))};

        std::cout << std::format(" {:^{}} | ", statNames[size_t(stat)], c1Width);
        printTime(t1, c2Width);
        std::cout << " | ";
        printTime(t2, c3Width);
        std::cout << " | ";
        printFactor(t1, t2, c4Width);
        std::cout << std::endl;
    }
}

#pragma warning(suppress: 4505)
static void printOpsChartable(const Stats & results, std::ostream & ofs)
{
    for (const Stat stat : results.presentStats()) {
        ofs << statNames[size_t(stat)] << ','; for (const size_t containerI : results.presentContainerIndices()) ofs << results.containerName(containerI) << ','; ofs << std::endl;

        size_t lineCount{0u};
        for (const size_t elementCount : results.presentElementCounts()) {
            ofs << elementCount << ',';
            for (const size_t containerI : results.presentContainerIndices()) {
                ofs << results.at(containerI, elementCount, stat) << ',';
            }
            ofs << std::endl;
            ++lineCount;
        }
        for (; lineCount < detailedChartRows; ++lineCount) {
            ofs << std::endl;
        }
    }
}

static void printTypicalChartable(const Stats & results, std::ostream & ofs)
{
    for (const size_t elementCount : results.presentElementCounts()) {
        ofs << elementCount << ",Insert,Access,Iterate,Erase" << std::endl;

        for (const size_t containerI : results.presentContainerIndices()) {
            ofs << results.containerName(containerI);
            ofs << ',' << results.at(containerI, elementCount, Stat::insertReserved);
            ofs << ',' << results.at(containerI, elementCount, Stat::accessPresent);
            ofs << ',' << results.at(containerI, elementCount, Stat::iterateFull);
            ofs << ',' << results.at(containerI, elementCount, Stat::erase);
            ofs << std::endl;
        }
    }
}

template <typename Container, typename K>
static void time(const size_t containerI, const std::span<const K> presentKeys, const std::span<const K> absentKeys, Stats & results)
{
    static_assert(std::is_same_v<K, typename Container::key_type>);

    static constexpr bool isSet{!IsMap<Container>};

    static volatile size_t v{};

    const std::span<const K> firstHalfPresentKeys{&presentKeys[0], presentKeys.size() / 2};
    const std::span<const K> secondHalfPresentKeys{&presentKeys[presentKeys.size() / 2], presentKeys.size() / 2};
    const double invElementCount{1.0 / double(presentKeys.size())};
    const double invHalfElementCount{invElementCount * 2.0};

    alignas(Container) std::byte backingMemory[sizeof(Container)];
    Container * containerPtr;

    std::array<double, size_t(Stat::_n)> stats{};

    // Construct
    {
        const s64 t0{now()};

        containerPtr = new (backingMemory) Container{};

        stats[size_t(Stat::construct)] += double(now() - t0);
    }

    Container & container{*containerPtr};

    // Insert to full capacity
    {
        const s64 t0{now()};

        for (const K & key : presentKeys) {
            if constexpr (isSet) {
                container.emplace(key);
            }
            else {
                container.emplace(key, typename Container::mapped_type{});
            }
        }

        stats[size_t(Stat::insert)] += double(now() - t0) * invElementCount;
    }

    // Full capacity insert present elements
    {
        const s64 t0{now()};

        for (const K & key : presentKeys) {
            if constexpr (isSet) {
                container.emplace(key);
            }
            else {
                container.emplace(key, typename Container::mapped_type{});
            }
        }

        stats[size_t(Stat::insertPresent)] += double(now() - t0) * invElementCount;
    }

    // Full capacity access present elements
    {
        const s64 t0{now()};

        for (const K & key : presentKeys) {
            v = v + container.count(reinterpret_cast<const typename Container::key_type &>(key));
        }

        stats[size_t(Stat::accessPresent)] += double(now() - t0) * invElementCount;
    }

    // Full capacity access absent elements
    {
        const s64 t0{now()};

        for (const K & key : absentKeys) {
            v = v + container.count(key);
        }

        stats[size_t(Stat::accessAbsent)] += double(now() - t0) * invElementCount;
    }

    // Full capacity iteration
    {
        const s64 t0{now()};

        for (const auto & element : container) {
            // Important to actually use the value as to load the memory
            if constexpr (isSet) {
                v = v + reinterpret_cast<const size_t &>(element);
            }
            else {
                v = v + reinterpret_cast<const size_t &>(element.first);
            }
        }

        stats[size_t(Stat::iterateFull)] += double(now() - t0) * invElementCount;
    }

    // Full capacity erase absent elements
    {
        const s64 t0{now()};

        for (const K & key : absentKeys) {
            container.erase(key);
        }

        stats[size_t(Stat::eraseAbsent)] += double(now() - t0) * invElementCount;
    }

    // Half erasure
    {
        const s64 t0{now()};

        for (const K & key : secondHalfPresentKeys) {
            container.erase(key);
        }

        stats[size_t(Stat::erase)] += double(now() - t0) * invHalfElementCount;
    }

    // Half capacity iteration
    {
        const s64 t0{now()};

        for (const auto & element : container) {
            // Important to actually use the value as to load the memory
            if constexpr (isSet) {
                v = v + reinterpret_cast<const size_t &>(element);
            }
            else {
                v = v + reinterpret_cast<const size_t &>(element.first);
            }
        }

        stats[size_t(Stat::iterateHalf)] += double(now() - t0) * invHalfElementCount;
    }

    // Erase remaining elements
    {
        const s64 t0{now()};

        for (const K & key : firstHalfPresentKeys) {
            container.erase(key);
        }

        stats[size_t(Stat::erase)] += double(now() - t0) * invHalfElementCount;
    }

    // Empty access
    {
        const s64 t0{now()};

        for (const K & key : presentKeys) {
            v = v + container.count(key);
        }

        stats[size_t(Stat::accessEmpty)] += double(now() - t0) * invElementCount;
    }

    // Empty iteration
    {
        const s64 t0{now()};

        for (const auto & element : container) {
            // Important to actually use the value as to load the memory
            if constexpr (isSet) {
                v = v + reinterpret_cast<const size_t &>(element);
            }
            else {
                v = v + reinterpret_cast<const size_t &>(element.first);
            }
        }

        stats[size_t(Stat::iterateEmpty)] += double(now() - t0);
    }

    // Insert single element
    if constexpr (isSet) {
        container.emplace(presentKeys.front());
    }
    else {
        container.emplace(presentKeys.front(), typename Container::mapped_type{});
    }

    // Single element begin
    {
        const s64 t0{now()};

        volatile const auto it{container.cbegin()};

        stats[size_t(Stat::loneBegin)] += double(now() - t0);
    }

    // Single element end
    {
        const s64 t0{now()};

        volatile const auto it{container.cend()};

        stats[size_t(Stat::loneEnd)] += double(now() - t0);
    }

    // Erase single element
    container.erase(presentKeys.front());

    // Reinsertion
    {
        const s64 t0{now()};

        for (const K & key : presentKeys) {
            if constexpr (isSet) {
                container.emplace(key);
            }
            else {
                container.emplace(key, typename Container::mapped_type{});
            }
        }

        stats[size_t(Stat::refill)] += double(now() - t0) * invElementCount;
    }

    // Clear
    {
        const s64 t0{now()};

        container.clear();

        stats[size_t(Stat::clear)] += double(now() - t0) * invElementCount;
    }

    // Reserved insertion
    {
        container.reserve(presentKeys.size());

        const s64 t0{now()};

        for (const K & key : presentKeys) {
            if constexpr (isSet) {
                container.emplace(key);
            }
            else {
                container.emplace(key, typename Container::mapped_type{});
            }
        }

        stats[size_t(Stat::insertReserved)] += double(now() - t0) * invElementCount;
    }

    // Destruct
    {
        const s64 t0{now()};

        container.~Container();

        stats[size_t(Stat::destruction)] += double(now() - t0) * invElementCount;
    }

    for (size_t i{}; i < stats.size(); ++i) {
        results.get(containerI, presentKeys.size(), Stat(i)) += stats[i];
    }
}

template <typename Container, typename K>
static void timeTypical(const size_t containerI, const std::span<const K> keys, Stats & results)
{
    static_assert(std::is_same_v<K, typename Container::key_type>);

    static constexpr bool isSet{!IsMap<Container>};

    static volatile size_t v{};

    Container container{};
    container.reserve(keys.size());

    const s64 t0{now()};

    // Insert
    for (const K & key : keys) {
        if constexpr (isSet) {
            container.emplace(key);
        }
        else {
            container.emplace(key, typename Container::mapped_type{});
        }
    }

    const s64 t1{now()};

    // Access
    for (const K & key : keys) {
        v = v + container.count(key);
    }

    const s64 t2{now()};

    // Iterate
    for (const auto & element : container) {
        // Important to actually use the value as to load the memory
        if constexpr (isSet) {
            v = v + reinterpret_cast<const size_t &>(element);
        }
        else {
            v = v + reinterpret_cast<const size_t &>(element.first);
        }
    }

    const s64 t3{now()};

    // Erase
    for (const K & key : keys) {
        container.erase(key);
    }

    const s64 t4{now()};

    const double invElementCount{1.0 / double(keys.size())};

    results.get(containerI, keys.size(), Stat::insertReserved) += double(t1 - t0) * invElementCount;
    results.get(containerI, keys.size(), Stat::accessPresent) += double(t2 - t1) * invElementCount;
    results.get(containerI, keys.size(), Stat::iterateFull) += double(t3 - t2) * invElementCount;
    results.get(containerI, keys.size(), Stat::erase) += double(t4 - t3) * invElementCount;
}

template <typename CommonKey, typename ContainerInfo, typename... ContainerInfos>
static void timeContainers(const size_t containerI, const std::vector<CommonKey> & presentKeys, const std::vector<CommonKey> & absentKeys, Stats & results)
{
    using Container = typename ContainerInfo::Container;

    if constexpr (!std::is_same_v<Container, void>) {
        using K = typename Container::key_type;
        static_assert(sizeof(CommonKey) == sizeof(K) && alignof(CommonKey) == alignof(K));

        const std::span<const K> presentKeys_{reinterpret_cast<const K *>(presentKeys.data()), presentKeys.size()};
        const std::span<const K> absentKeys_{reinterpret_cast<const K *>(absentKeys.data()), absentKeys.size()};
        time<Container>(containerI, presentKeys_, absentKeys_, results);
    }

    if constexpr (sizeof...(ContainerInfos) != 0u) {
        timeContainers<CommonKey, ContainerInfos...>(containerI + 1u, presentKeys, absentKeys, results);
    }
}

template <typename CommonKey, typename ContainerInfo, typename... ContainerInfos>
static void timeContainersTypical(const size_t containerI, const std::vector<CommonKey> & keys, Stats & results)
{
    using Container = typename ContainerInfo::Container;

    if constexpr (!std::is_same_v<Container, void>) {
        using K = typename Container::key_type;
        static_assert(sizeof(CommonKey) == sizeof(K) && alignof(CommonKey) == alignof(K));

        const std::span<const K> keys_{reinterpret_cast<const K *>(keys.data()), keys.size()};
        timeTypical<Container>(containerI, keys_, results);
    }

    if constexpr (sizeof...(ContainerInfos) != 0u) {
        timeContainersTypical<CommonKey, ContainerInfos...>(containerI + 1u, keys, results);
    }
}

template <typename CommonKey, typename ContainerInfo, typename... ContainerInfos>
static void compareMemory(const size_t containerI, const std::vector<CommonKey> & keys, Stats & results)
{
    using Container = typename ContainerInfo::Container;
    using AllocatorContainer = typename ContainerInfo::AllocatorContainer;
    if constexpr (!std::is_same_v<AllocatorContainer, void>) static_assert(std::is_same_v<typename Container::key_type, typename AllocatorContainer::key_type>);
    using K = typename Container::key_type;
    static_assert(sizeof(CommonKey) == sizeof(K) && alignof(CommonKey) == alignof(K));

    const std::span<const K> keys_{reinterpret_cast<const K *>(keys.data()), keys.size()};

    if constexpr (!std::is_same_v<Container, void>) {
        results.get(containerI, keys.size(), Stat::objectSize) = sizeof(Container);
        results.get(containerI, keys.size(), Stat::iteratorSize) = sizeof(typename Container::iterator);
    }

    if constexpr (!std::is_same_v<AllocatorContainer, void>) {
        AllocatorContainer container{};
        container.reserve(keys_.size());
        for (const K & key : keys_) {
            if constexpr (IsMap<Container>) {
                container.emplace(key, typename Container::mapped_type{});
            }
            else {
                container.emplace(key);
            }
        }
        results.get(containerI, keys.size(), Stat::memoryOverhead) = double(container.get_allocator().stats().current - keys.size() * sizeof(K)) / double(keys.size());
    }

    if constexpr (sizeof...(ContainerInfos) != 0u) {
        compareMemory<CommonKey, ContainerInfos...>(containerI + 1u, keys, results);
    }
}

template <typename CommonKey, typename... ContainerInfos>
static void compareDetailedSized(const size_t elementCount, const size_t roundCount, qc::Random & random, Stats & results)
{
    const double invRoundCount{1.0 / double(roundCount)};

    std::vector<CommonKey> presentKeys(elementCount);
    std::vector<CommonKey> absentKeys(elementCount);
    for (CommonKey & key : presentKeys) key = random.next<CommonKey>();

    for (size_t round{0u}; round < roundCount; ++round) {
        std::swap(presentKeys, absentKeys);
        for (CommonKey & key : presentKeys) key = random.next<CommonKey>();

        timeContainers<CommonKey, ContainerInfos...>(0u, presentKeys, absentKeys, results);
    }

    for (const size_t containerI : results.presentContainerIndices()) {
        for (const Stat stat : results.presentStats()) {
            results.at(containerI, elementCount, stat) *= invRoundCount;
        }
    }

    compareMemory<CommonKey, ContainerInfos...>(0u, presentKeys, results);
}

template <typename CommonKey, typename... ContainerInfos>
static void compareDetailed(Stats & results)
{
    qc::Random random{size_t(std::chrono::steady_clock::now().time_since_epoch().count())};

    for (const auto [elementCount, roundCount] : detailedElementRoundCounts) {
        if (elementCount > std::numeric_limits<qc::utype<CommonKey>>::max()) {
            break;
        }

        std::cout << "Comparing " << roundCount << " rounds of " << elementCount << " elements...";

        compareDetailedSized<CommonKey, ContainerInfos...>(elementCount, roundCount, random, results);

        std::cout << " done" << std::endl;
    }

    results.setContainerNames<ContainerInfos...>();
}

template <typename CommonKey, typename... ContainerInfos>
static void compareTypicalSized(const size_t elementCount, const size_t roundCount, qc::Random & random, Stats & results)
{
    std::vector<CommonKey> keys(elementCount);

    for (size_t round{0u}; round < roundCount; ++round) {
        for (CommonKey & key : keys) key = random.next<CommonKey>();
        timeContainersTypical<CommonKey, ContainerInfos...>(0u, keys, results);
    }

    const double invRoundCount{1.0 / double(roundCount)};
    for (const size_t containerI : results.presentContainerIndices()) {
        for (const Stat stat : results.presentStats()) {
            results.at(containerI, elementCount, stat) *= invRoundCount;
        }
    }
}

template <typename CommonKey, typename... ContainerInfos>
static void compareTypical(Stats & results)
{
    qc::Random random{size_t(std::chrono::steady_clock::now().time_since_epoch().count())};

    for (const auto [elementCount, roundCount] : typicalElementRoundCounts) {
        if (elementCount > std::numeric_limits<qc::utype<CommonKey>>::max()) {
            break;
        }

        std::cout << "Comparing " << roundCount << " rounds of " << elementCount << " elements...";

        compareTypicalSized<CommonKey, ContainerInfos...>(elementCount, roundCount, random, results);

        std::cout << " done" << std::endl;
    }

    results.setContainerNames<ContainerInfos...>();
}

enum class CompareMode { oneVsOne, detailed, typical };

template <CompareMode mode, typename CommonKey, typename... ContainerInfos>
static void compare()
{
    static const std::filesystem::path outFilePath{"out.txt"};

    // 1-vs-1
    if constexpr (mode == CompareMode::oneVsOne) {
        static_assert(sizeof...(ContainerInfos) == 2);
        Stats results{};
        compareDetailed<CommonKey, ContainerInfos...>(results);
        std::cout << std::endl;
        for (const auto[elementCount, roundCount] : detailedElementRoundCounts) {
            reportComparison(results, 1, 0, elementCount);
            std::cout << std::endl;
        }
    }
    // Detailed
    else if constexpr (mode == CompareMode::detailed) {
        Stats results{};
        compareDetailed<CommonKey, ContainerInfos...>(results);
        std::ofstream ofs{outFilePath};
        printOpsChartable(results, ofs);
        std::cout << "Wrote results to " << outFilePath << std::endl;
    }
    // Typical
    else if constexpr (mode == CompareMode::typical) {
        Stats results{};
        compareTypical<CommonKey, ContainerInfos...>(results);
        std::ofstream ofs{outFilePath};
        printTypicalChartable(results, ofs);
        std::cout << "Wrote results to " << outFilePath << std::endl;
    }
}

template <typename K, bool sizeMode = false, bool doTrivialComplex = false>
struct QcHashSetInfo
{
    using Container = qc::hash::RawSet<K>;
    using AllocatorContainer = qc::hash::RawSet<K, typename qc::hash::RawSet<K>::hasher, qc::memory::RecordAllocator<K>>;

    static constexpr bool isTrivial{std::is_same_v<K, Trivial<sizeof(K)>>};
    static inline const std::string name{sizeMode ? std::format("{}{}", (doTrivialComplex ? isTrivial ? "Trivial " : "Complex " : ""), sizeof(K)) : "qc::hash::RawSet"};
};

template <typename K, typename V, bool sizeMode = false, bool doTrivialComplex = false>
struct QcHashMapInfo
{
    using Container = qc::hash::RawMap<K, V>;
    using AllocatorContainer = qc::hash::RawMap<K, V, typename qc::hash::RawMap<K, V>::hasher, qc::memory::RecordAllocator<std::pair<K, V>>>;

    static constexpr bool isKeyTrivial{std::is_same_v<K, Trivial<sizeof(K)>>};
    static constexpr bool isValTrivial{std::is_same_v<V, Trivial<sizeof(V)>>};
    static inline const std::string name{sizeMode ? std::format("{}{} : {}{}", (doTrivialComplex ? isKeyTrivial ? "Trivial " : "Complex " : ""), sizeof(K), (doTrivialComplex ? isValTrivial ? "Trivial " : "Complex " : ""), sizeof(V)) : "qc::hash::RawMap"};
};

template <typename K>
struct StdSetInfo
{
    using Container = std::unordered_set<K>;
    using AllocatorContainer = std::unordered_set<K, typename std::unordered_set<K>::hasher, typename std::unordered_set<K>::key_equal, qc::memory::RecordAllocator<K>>;

    static inline const std::string name{"std::unordered_map"};
};

template <typename K, typename V>
struct StdMapInfo
{
    using Container = std::unordered_map<K, V>;
    using AllocatorContainer = std::unordered_map<K, V, typename std::unordered_map<K, V>::hasher, typename std::unordered_map<K, V>::key_equal, qc::memory::RecordAllocator<std::pair<K, V>>>;

    static inline const std::string name{"std::unordered_map"};
};

template <typename K>
struct AbslSetInfo
{
    using Container = std::conditional_t<sizeof(size_t) == 8, absl::flat_hash_set<K>, void>;
    using AllocatorContainer = std::conditional_t<sizeof(size_t) == 8, absl::flat_hash_set<K, typename absl::flat_hash_set<K>::hasher, typename absl::flat_hash_set<K>::key_equal, qc::memory::RecordAllocator<K>>, void>;

    static inline const std::string name{"absl::flat_hash_set"};
};

template <typename K, typename V>
struct AbslMapInfo
{
    using Container = absl::flat_hash_map<K, V>;
    using AllocatorContainer = std::conditional_t<sizeof(size_t) == 8, std::unordered_map<K, V, typename absl::flat_hash_map<K, V>::hasher, typename absl::flat_hash_map<K, V>::key_equal, qc::memory::RecordAllocator<std::pair<K, V>>>, void>;

    static inline const std::string name{"absl::flat_hash_map"};
};

template <typename K>
struct RobinHoodSetInfo
{
    using Container = robin_hood::unordered_set<K>;
    using AllocatorContainer = void;

    static inline const std::string name{"robin_hood::unordered_set"};
};

template <typename K, typename V>
struct RobinHoodMapInfo
{
    using Container = robin_hood::unordered_map<K, V>;
    using AllocatorContainer = void;

    static inline const std::string name{"robin_hood::unordered_map"};
};

template <typename K>
struct SkaSetInfo
{
    using Container = ska::flat_hash_set<K>;
    using AllocatorContainer = ska::flat_hash_set<K, typename ska::flat_hash_set<K>::hasher, typename ska::flat_hash_set<K>::key_equal, qc::memory::RecordAllocator<K>>;

    static inline const std::string name{"ska::flat_hash_set"};
};

template <typename K, typename V>
struct SkaMapInfo
{
    using Container = ska::flat_hash_map<K, V>;
    using AllocatorContainer = ska::flat_hash_map<K, V, typename ska::flat_hash_map<K, V>::hasher, typename ska::flat_hash_map<K, V>::key_equal, qc::memory::RecordAllocator<std::pair<K, V>>>;

    static inline const std::string name{"ska::flat_hash_map"};
};

template <typename K>
struct TslRobinSetInfo
{
    using Container = tsl::robin_set<K>;
    using AllocatorContainer = tsl::robin_set<K, typename tsl::robin_set<K>::hasher, typename tsl::robin_set<K>::key_equal, qc::memory::RecordAllocator<K>>;

    static inline const std::string name{"tsl::robin_set"};
};

template <typename K, typename V>
struct TslRobinMapInfo
{
    using Container = tsl::robin_map<K, V>;
    using AllocatorContainer = tsl::robin_map<K, V, typename tsl::robin_map<K, V>::hasher, typename tsl::robin_map<K, V>::key_equal, qc::memory::RecordAllocator<std::pair<K, V>>>;

    static inline const std::string name{"tsl::robin_map"};
};

template <typename K>
struct TslSparseSetInfo
{
    using Container = tsl::sparse_set<K>;
    using AllocatorContainer = tsl::sparse_set<K, typename tsl::sparse_set<K>::hasher, typename tsl::sparse_set<K>::key_equal, qc::memory::RecordAllocator<K>>;

    static inline const std::string name{"tsl::sparse_hash_set"};
};

template <typename K, typename V>
struct TslSparseMapInfo
{
    using Container = tsl::sparse_map<K, V>;
    using AllocatorContainer = tsl::sparse_map<K, V, typename tsl::sparse_map<K, V>::hasher, typename tsl::sparse_map<K, V>::key_equal, qc::memory::RecordAllocator<std::pair<K, V>>>;

    static inline const std::string name{"tsl::sparse_hash_map"};
};

int main()
{
    // Set comparison
    if constexpr (false) {
        using K = size_t;
        compare<CompareMode::typical, K,
            QcHashSetInfo<K>,
            StdSetInfo<K>,
            AbslSetInfo<K>,
            RobinHoodSetInfo<K>,
            SkaSetInfo<K>,
            TslRobinSetInfo<K>,
            TslSparseSetInfo<K>
        >();
    }
    // Map comparison
    else if constexpr (false) {
        using K = size_t;
        using V = std::string;
        compare<CompareMode::typical, K,
            QcHashMapInfo<K, V>,
            StdMapInfo<K, V>,
            AbslMapInfo<K, V>,
            RobinHoodMapInfo<K, V>,
            SkaMapInfo<K, V>,
            TslRobinMapInfo<K, V>,
            TslSparseMapInfo<K, V>
        >();
    }
    // Architecture comparison
    else if constexpr (true) {
        using K = u32;
        compare<CompareMode::typical, K, QcHashSetInfo<K>>();
    }
    // Set vs map
    else if constexpr (false) {
        compare<CompareMode::detailed, size_t,
            QcHashSetInfo<size_t, true>,
            QcHashMapInfo<size_t, Trivial<8>, true>,
            QcHashMapInfo<size_t, Trivial<16>, true>,
            QcHashMapInfo<size_t, Trivial<32>, true>,
            QcHashMapInfo<size_t, Trivial<64>, true>,
            QcHashMapInfo<size_t, Trivial<128>, true>,
            QcHashMapInfo<size_t, Trivial<256>, true>
        >();
    }
    // Trivial vs complex
    else if constexpr (false) {
        using K = size_t;
        compare<CompareMode::detailed, size_t,
            QcHashSetInfo<Trivial<sizeof(K)>, true, true>,
            QcHashSetInfo<Complex<sizeof(K)>, true, true>,
            QcHashMapInfo<Trivial<sizeof(K)>, Trivial<sizeof(K)>, true, true>,
            QcHashMapInfo<Complex<sizeof(K)>, Trivial<sizeof(K)>, true, true>,
            QcHashMapInfo<Trivial<sizeof(K)>, Complex<sizeof(K)>, true, true>,
            QcHashMapInfo<Complex<sizeof(K)>, Complex<sizeof(K)>, true, true>
        >();
    }

    return 0;
}
