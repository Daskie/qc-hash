///
/// TEMPORARILY OUT OF COMMISSION
/// WILL SWITCH TO GOOGLE BENCHMARK SOON
///

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

#include <qc-core/record-allocator.hpp>
#include <qc-core/random.hpp>
#include <qc-core/vector.hpp>

#include <qc-hash.hpp>

#pragma warning(push)
#pragma warning(disable: 4127 4244 4293 4305 4309 4324 4365 4458 5219)
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include "external/robin_hood.h"
#include "external/flat_hash_map.hpp"
#include "external/tsl/robin_map.h"
#include "external/tsl/robin_set.h"
#include "external/tsl/sparse_map.h"
#include "external/tsl/sparse_set.h"
#pragma warning(pop)

using namespace qc::types;

template <typename C> concept IsMap = !std::is_same_v<typename C::mapped_type, void>;

static const std::vector<std::pair<u64, u64>> detailedElementRoundNsRelease{
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
    {10'000'000u,       3u}};

static const std::vector<std::pair<u64, u64>> detailedElementRoundNsDebug{
    {       10u, 100'000u},
    {      100u,  10'000u},
    {    1'000u,   1'000u},
    {   10'000u,     100u},
    {  100'000u,      10u},
    {1'000'000u,       3u}};

static const std::vector<std::pair<u64, u64>> & detailedElementRoundNs{qc::debug ? detailedElementRoundNsDebug : detailedElementRoundNsRelease};

static const u64 detailedChartRows{qc::max(detailedElementRoundNsRelease.size(), detailedElementRoundNsDebug.size())};

static const std::vector<std::pair<u64, u64>> typicalElementRoundNs{
    {        10u, 1'000'000u},
    {       100u,   100'000u},
    {     1'000u,    10'000u},
    {    10'000u,     1'000u},
    {   100'000u,       100u},
    { 1'000'000u,        10u},
    {10'000'000u,         3u}};

enum class Stat : u64
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

static const std::array<std::string, u64(Stat::_n)> statNames{
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
    "Destruction"};

template <u64 size> struct Trivial;

template <u64 size> requires (size <= 8)
struct Trivial<size>
{
    typename qc::sized<size>::utype val;
};

template <u64 size> requires (size > 8)
struct Trivial<size>
{
    std::array<u64, size / 8> val;
};

static_assert(std::is_trivial_v<Trivial<1>>);
static_assert(std::is_trivial_v<Trivial<8>>);
static_assert(std::is_trivial_v<Trivial<64>>);

template <u64 size>
class Complex : public Trivial<size>
{
    public:

    constexpr Complex() : Trivial<size>{} {}

    constexpr Complex(const Complex & other) = default;

    constexpr Complex(Complex && other) :
        Trivial<size>{std::exchange(other.val, {})}
    {}

    Complex & operator=(const Complex &) = delete;

    Complex && operator=(Complex && other)
    {
        Trivial::val = std::exchange(other.val, {});
    }

    constexpr ~Complex() {}
};

static_assert(!std::is_trivial_v<Complex<1>>);
static_assert(!std::is_trivial_v<Complex<8>>);
static_assert(!std::is_trivial_v<Complex<64>>);

template <u64 size> requires (size <= sizeof(u64))
struct qc::hash::IdentityHash<Trivial<size>>
{
    constexpr u64 operator()(const Trivial<size> k) const
    {
        return k.val;
    }
};

template <u64 size> requires (size <= sizeof(u64))
struct qc::hash::IdentityHash<Complex<size>>
{
    constexpr u64 operator()(const Complex<size> & k) const
    {
        return k.val;
    }
};

class Stats
{
    public:

    f64 & get(const u64 containerI, const u64 elementN, const Stat stat)
    {
        _presentContainerIndices.insert(containerI);
        _presentElementNs.insert(elementN);
        _presentStats.insert(stat);
        return _table[containerI][elementN][stat];
    }

    f64 & at(const u64 containerI, const u64 elementN, const Stat stat)
    {
        return const_cast<f64 &>(const_cast<const Stats *>(this)->at(containerI, elementN, stat));
    }

    const f64 & at(const u64 containerI, const u64 elementN, const Stat stat) const
    {
        return _table.at(containerI).at(elementN).at(stat);
    }

    const std::set<u64> presentContainerIndices() const { return _presentContainerIndices; }

    const std::set<u64> presentElementNs() const { return _presentElementNs; }

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

    const std::string & containerName(const u64 containerI) const
    {
        return _containerNames.at(containerI);
    }

    private:

    std::map<u64, std::map<u64, std::map<Stat, f64>>> _table{};
    std::set<u64> _presentContainerIndices{};
    std::set<u64> _presentElementNs{};
    std::set<Stat> _presentStats{};
    std::vector<std::string> _containerNames{};
};

static s64 now()
{
    return std::chrono::nanoseconds(std::chrono::steady_clock::now().time_since_epoch()).count();
}

static void printTime(const s64 nanoseconds, const u64 width)
{
    if (nanoseconds < 10000)
    {
        std::cout << std::setw(nat(width) - 3) << nanoseconds << " ns";
        return;
    }

    const s64 microseconds{(nanoseconds + 500) / 1000};
    if (microseconds < 10000)
    {
        std::cout << std::setw(nat(width) - 3) << microseconds << " us";
        return;
    }

    const s64 milliseconds{(microseconds + 500) / 1000};
    if (milliseconds < 10000)
    {
        std::cout << std::setw(nat(width) - 3) << milliseconds << " ms";
        return;
    }

    const s64 seconds{(milliseconds + 500) / 1000};
    std::cout << std::setw(nat(width) - 3) << seconds << " s ";
}

static void printFactor(const s64 t1, const s64 t2, const u64 width)
{
    const f64 absFactor{t1 >= t2 ? f64(t1) / f64(t2) : f64(t2) / f64(t1)};
    s32 percent{qc::round<s32>(absFactor * 100.0) - 100};
    if (t1 < t2)
    {
        percent = -percent;
    }
    std::cout << std::setw(nat(width) - 2) << percent << " %";
}

#pragma warning(suppress: 4505)
static void reportComparison(const Stats & results, const u64 container1I, const u64 container2I, const u64 elementN)
{
    const std::string c1Header{std::format("{:d} Elements", elementN)};
    const std::string c4Header{"% Faster"};

    const std::string name1{results.containerName(container1I)};
    const std::string name2{results.containerName(container2I)};

    u64 c1Width{c1Header.size()};
    for (const Stat stat : results.presentStats())
    {
        qc::maxify(c1Width, statNames[u64(stat)].size());
    }
    const u64 c2Width{qc::max(name1.size(), u64{7u})};
    const u64 c3Width{qc::max(name2.size(), u64{7u})};
    const u64 c4Width{qc::max(c4Header.size(), u64{8u})};

    std::cout << std::format(" {:^{}} | {:^{}} | {:^{}} | {:^{}} ", c1Header, c1Width, name1, c2Width, name2, c3Width, c4Header, c4Width) << std::endl;
    std::cout << std::setfill('-') << std::setw(nat(c1Width) + 3) << "+" << std::setw(nat(c2Width) + 3) << "+" << std::setw(nat(c3Width) + 3) << "+" << std::setw(nat(c4Width) + 2) << "" << std::setfill(' ') << std::endl;

    for (const Stat stat : results.presentStats())
    {
        const s64 t1{s64(std::round(results.at(container1I, elementN, stat)))};
        const s64 t2{s64(std::round(results.at(container2I, elementN, stat)))};

        std::cout << std::format(" {:^{}} | ", statNames[u64(stat)], c1Width);
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
    for (const Stat stat : results.presentStats())
    {
        ofs << statNames[u64(stat)] << ','; for (const u64 containerI : results.presentContainerIndices()) ofs << results.containerName(containerI) << ','; ofs << std::endl;

        u64 lineN{0u};
        for (const u64 elementN : results.presentElementNs())
        {
            ofs << elementN << ',';
            for (const u64 containerI : results.presentContainerIndices())
            {
                ofs << results.at(containerI, elementN, stat) << ',';
            }
            ofs << std::endl;
            ++lineN;
        }
        for (; lineN < detailedChartRows; ++lineN)
        {
            ofs << std::endl;
        }
    }
}

#pragma warning(suppress: 4505)
static void printTypicalChartable(const Stats & results, std::ostream & ofs)
{
    for (const u64 elementN : results.presentElementNs())
    {
        ofs << elementN << ",Insert,Access,Iterate,Erase" << std::endl;

        for (const u64 containerI: results.presentContainerIndices())
        {
            ofs << results.containerName(containerI);
            ofs << ',' << results.at(containerI, elementN, Stat::insertReserved);
            ofs << ',' << results.at(containerI, elementN, Stat::accessPresent);
            ofs << ',' << results.at(containerI, elementN, Stat::iterateFull);
            ofs << ',' << results.at(containerI, elementN, Stat::erase);
            ofs << std::endl;
        }
    }
}

template <typename Container, typename K>
static void time(const u64 containerI, const std::span<const K> presentKeys, const std::span<const K> absentKeys, Stats & results)
{
    static_assert(std::is_same_v<K, typename Container::key_type>);

    static constexpr bool isSet{!IsMap<Container>};

    static volatile u64 v{};

    const std::span<const K> firstHalfPresentKeys{&presentKeys[0], presentKeys.size() / 2};
    const std::span<const K> secondHalfPresentKeys{&presentKeys[presentKeys.size() / 2], presentKeys.size() / 2};
    const f64 invElementN{1.0 / f64(presentKeys.size())};
    const f64 invHalfElementN{invElementN * 2.0};

    alignas(Container) std::byte backingMemory[sizeof(Container)];
    Container * containerPtr;

    std::array<f64, u64(Stat::_n)> stats{};

    // Construct
    {
        const s64 t0{now()};

        containerPtr = new (backingMemory) Container{};

        stats[u64(Stat::construct)] += f64(now() - t0);
    }

    Container & container{*containerPtr};

    // Insert to full capacity
    {
        const s64 t0{now()};

        for (const K & key : presentKeys)
        {
            if constexpr (isSet)
            {
                container.emplace(key);
            }
            else
            {
                container.emplace(key, typename Container::mapped_type{});
            }
        }

        stats[u64(Stat::insert)] += f64(now() - t0) * invElementN;
    }

    // Full capacity insert present elements
    {
        const s64 t0{now()};

        for (const K & key : presentKeys)
        {
            if constexpr (isSet)
            {
                container.emplace(key);
            }
            else
            {
                container.emplace(key, typename Container::mapped_type{});
            }
        }

        stats[u64(Stat::insertPresent)] += f64(now() - t0) * invElementN;
    }

    // Full capacity access present elements
    {
        const s64 t0{now()};

        for (const K & key : presentKeys)
        {
            v = v + container.count(reinterpret_cast<const typename Container::key_type &>(key));
        }

        stats[u64(Stat::accessPresent)] += f64(now() - t0) * invElementN;
    }

    // Full capacity access absent elements
    {
        const s64 t0{now()};

        for (const K & key : absentKeys)
        {
            v = v + container.count(key);
        }

        stats[u64(Stat::accessAbsent)] += f64(now() - t0) * invElementN;
    }

    // Full capacity iteration
    {
        const s64 t0{now()};

        for (const auto & element : container)
        {
            // Important to actually use the value as to load the memory
            if constexpr (isSet)
            {
                v = v + reinterpret_cast<const u64 &>(element);
            }
            else
            {
                v = v + reinterpret_cast<const u64 &>(element.first);
            }
        }

        stats[u64(Stat::iterateFull)] += f64(now() - t0) * invElementN;
    }

    // Full capacity erase absent elements
    {
        const s64 t0{now()};

        for (const K & key : absentKeys)
        {
            container.erase(key);
        }

        stats[u64(Stat::eraseAbsent)] += f64(now() - t0) * invElementN;
    }

    // Half erasure
    {
        const s64 t0{now()};

        for (const K & key : secondHalfPresentKeys)
        {
            container.erase(key);
        }

        stats[u64(Stat::erase)] += f64(now() - t0) * invHalfElementN;
    }

    // Half capacity iteration
    {
        const s64 t0{now()};

        for (const auto & element : container)
        {
            // Important to actually use the value as to load the memory
            if constexpr (isSet)
            {
                v = v + reinterpret_cast<const u64 &>(element);
            }
            else
            {
                v = v + reinterpret_cast<const u64 &>(element.first);
            }
        }

        stats[u64(Stat::iterateHalf)] += f64(now() - t0) * invHalfElementN;
    }

    // Erase remaining elements
    {
        const s64 t0{now()};

        for (const K & key : firstHalfPresentKeys)
        {
            container.erase(key);
        }

        stats[u64(Stat::erase)] += f64(now() - t0) * invHalfElementN;
    }

    // Empty access
    {
        const s64 t0{now()};

        for (const K & key : presentKeys)
        {
            v = v + container.count(key);
        }

        stats[u64(Stat::accessEmpty)] += f64(now() - t0) * invElementN;
    }

    // Empty iteration
    {
        const s64 t0{now()};

        for (const auto & element : container)
        {
            // Important to actually use the value as to load the memory
            if constexpr (isSet)
            {
                v = v + reinterpret_cast<const u64 &>(element);
            }
            else
            {
                v = v + reinterpret_cast<const u64 &>(element.first);
            }
        }

        stats[u64(Stat::iterateEmpty)] += f64(now() - t0);
    }

    // Insert single element
    if constexpr (isSet)
    {
        container.emplace(presentKeys.front());
    }
    else
    {
        container.emplace(presentKeys.front(), typename Container::mapped_type{});
    }

    // Single element begin
    {
        const s64 t0{now()};

        volatile const auto it{container.cbegin()};

        stats[u64(Stat::loneBegin)] += f64(now() - t0);
    }

    // Single element end
    {
        const s64 t0{now()};

        volatile const auto it{container.cend()};

        stats[u64(Stat::loneEnd)] += f64(now() - t0);
    }

    // Erase single element
    container.erase(presentKeys.front());

    // Reinsertion
    {
        const s64 t0{now()};

        for (const K & key : presentKeys)
        {
            if constexpr (isSet)
            {
                container.emplace(key);
            }
            else
            {
                container.emplace(key, typename Container::mapped_type{});
            }
        }

        stats[u64(Stat::refill)] += f64(now() - t0) * invElementN;
    }

    // Clear
    {
        const s64 t0{now()};

        container.clear();

        stats[u64(Stat::clear)] += f64(now() - t0) * invElementN;
    }

    // Reserved insertion
    {
        container.reserve(presentKeys.size());

        const s64 t0{now()};

        for (const K & key : presentKeys)
        {
            if constexpr (isSet)
            {
                container.emplace(key);
            }
            else
            {
                container.emplace(key, typename Container::mapped_type{});
            }
        }

        stats[u64(Stat::insertReserved)] += f64(now() - t0) * invElementN;
    }

    // Destruct
    {
        const s64 t0{now()};

        container.~Container();

        stats[u64(Stat::destruction)] += f64(now() - t0) * invElementN;
    }

    for (u64 i{}; i < stats.size(); ++i)
    {
        results.get(containerI, presentKeys.size(), Stat(i)) += stats[i];
    }
}

template <typename Container, typename K>
static void timeTypical(const u64 containerI, const std::span<const K> keys, Stats & results)
{
    static_assert(std::is_same_v<K, typename Container::key_type>);

    static constexpr bool isSet{!IsMap<Container>};

    static volatile u64 v{};

    Container container{};
    container.reserve(keys.size());

    const s64 t0{now()};

    // Insert
    for (const K & key : keys)
    {
        if constexpr (isSet)
        {
            container.emplace(key);
        }
        else
        {
            container.emplace(key, typename Container::mapped_type{});
        }
    }

    const s64 t1{now()};

    // Access
    for (const K & key : keys)
    {
        v = v + container.count(key);
    }

    const s64 t2{now()};

    // Iterate
    for (const auto & element : container)
    {
        // Important to actually use the value as to load the memory
        if constexpr (isSet)
        {
            v = v + reinterpret_cast<const u64 &>(element);
        }
        else
        {
            v = v + reinterpret_cast<const u64 &>(element.first);
        }
    }

    const s64 t3{now()};

    // Erase
    for (const K & key : keys)
    {
        container.erase(key);
    }

    const s64 t4{now()};

    const f64 invElementN{1.0 / f64(keys.size())};

    results.get(containerI, keys.size(), Stat::insertReserved) += f64(t1 - t0) * invElementN;
    results.get(containerI, keys.size(), Stat::accessPresent) += f64(t2 - t1) * invElementN;
    results.get(containerI, keys.size(), Stat::iterateFull) += f64(t3 - t2) * invElementN;
    results.get(containerI, keys.size(), Stat::erase) += f64(t4 - t3) * invElementN;
}

template <typename CommonKey, typename ContainerInfo, typename... ContainerInfos>
static void timeContainers(const u64 containerI, const std::vector<CommonKey> & presentKeys, const std::vector<CommonKey> & absentKeys, Stats & results)
{
    using Container = typename ContainerInfo::Container;

    if constexpr (!std::is_same_v<Container, void>)
    {
        using K = typename Container::key_type;
        static_assert(sizeof(CommonKey) == sizeof(K) && alignof(CommonKey) == alignof(K));

        const std::span<const K> presentKeys_{reinterpret_cast<const K *>(presentKeys.data()), presentKeys.size()};
        const std::span<const K> absentKeys_{reinterpret_cast<const K *>(absentKeys.data()), absentKeys.size()};
        time<Container>(containerI, presentKeys_, absentKeys_, results);
    }

    if constexpr (sizeof...(ContainerInfos) != 0u)
    {
        timeContainers<CommonKey, ContainerInfos...>(containerI + 1u, presentKeys, absentKeys, results);
    }
}

template <typename CommonKey, typename ContainerInfo, typename... ContainerInfos>
static void timeContainersTypical(const u64 containerI, const std::vector<CommonKey> & keys, Stats & results)
{
    using Container = typename ContainerInfo::Container;

    if constexpr (!std::is_same_v<Container, void>)
    {
        using K = typename Container::key_type;
        static_assert(sizeof(CommonKey) == sizeof(K) && alignof(CommonKey) == alignof(K));

        const std::span<const K> keys_{reinterpret_cast<const K *>(keys.data()), keys.size()};
        timeTypical<Container>(containerI, keys_, results);
    }

    if constexpr (sizeof...(ContainerInfos) != 0u)
    {
        timeContainersTypical<CommonKey, ContainerInfos...>(containerI + 1u, keys, results);
    }
}

template <typename CommonKey, typename ContainerInfo, typename... ContainerInfos>
static void compareMemory(const u64 containerI, const std::vector<CommonKey> & keys, Stats & results)
{
    using Container = typename ContainerInfo::Container;
    using AllocatorContainer = typename ContainerInfo::AllocatorContainer;
    if constexpr (!std::is_same_v<AllocatorContainer, void>) static_assert(std::is_same_v<typename Container::key_type, typename AllocatorContainer::key_type>);
    using K = typename Container::key_type;
    static_assert(sizeof(CommonKey) == sizeof(K) && alignof(CommonKey) == alignof(K));

    const std::span<const K> keys_{reinterpret_cast<const K *>(keys.data()), keys.size()};

    if constexpr (!std::is_same_v<Container, void>)
    {
        results.get(containerI, keys.size(), Stat::objectSize) = sizeof(Container);
        results.get(containerI, keys.size(), Stat::iteratorSize) = sizeof(typename Container::iterator);
    }

    if constexpr (!std::is_same_v<AllocatorContainer, void>)
    {
        AllocatorContainer container{};
        container.reserve(keys_.size());
        for (const K & key : keys_)
        {
            if constexpr (IsMap<Container>)
            {
                container.emplace(key, typename Container::mapped_type{});
            }
            else
            {
                container.emplace(key);
            }
        }
        results.get(containerI, keys.size(), Stat::memoryOverhead) = f64(container.get_allocator().stats().current - keys.size() * sizeof(K)) / f64(keys.size());
    }

    if constexpr (sizeof...(ContainerInfos) != 0u)
    {
        compareMemory<CommonKey, ContainerInfos...>(containerI + 1u, keys, results);
    }
}

template <typename CommonKey, typename... ContainerInfos>
static void compareDetailedSized(const u64 elementN, const u64 roundN, qc::Random<u64> & random, Stats & results)
{
    const f64 invRoundN{1.0 / f64(roundN)};

    std::vector<CommonKey> presentKeys(elementN);
    std::vector<CommonKey> absentKeys(elementN);
    for (CommonKey & key : presentKeys) key = random.next<CommonKey>();

    for (u64 round{0u}; round < roundN; ++round)
    {
        std::swap(presentKeys, absentKeys);
        for (CommonKey & key : presentKeys) key = random.next<CommonKey>();

        timeContainers<CommonKey, ContainerInfos...>(0u, presentKeys, absentKeys, results);
    }

    for (const u64 containerI : results.presentContainerIndices())
    {
        for (const Stat stat : results.presentStats())
        {
            results.at(containerI, elementN, stat) *= invRoundN;
        }
    }

    compareMemory<CommonKey, ContainerInfos...>(0u, presentKeys, results);
}

template <typename CommonKey, typename... ContainerInfos>
static void compareDetailed(Stats & results)
{
    qc::Random random{u64(std::chrono::steady_clock::now().time_since_epoch().count())};

    for (const auto [elementN, roundN] : detailedElementRoundNs)
    {
        if (elementN > std::numeric_limits<qc::utype<CommonKey>>::max())
        {
            break;
        }

        std::cout << "Comparing " << roundN << " rounds of " << elementN << " elements...";

        compareDetailedSized<CommonKey, ContainerInfos...>(elementN, roundN, random, results);

        std::cout << " done" << std::endl;
    }

    results.setContainerNames<ContainerInfos...>();
}

template <typename CommonKey, typename... ContainerInfos>
static void compareTypicalSized(const u64 elementN, const u64 roundN, qc::Random<u64> & random, Stats & results)
{
    std::vector<CommonKey> keys(elementN);

    for (u64 round{0u}; round < roundN; ++round)
    {
        for (CommonKey & key : keys) key = random.next<CommonKey>();
        timeContainersTypical<CommonKey, ContainerInfos...>(0u, keys, results);
    }

    const f64 invRoundN{1.0 / f64(roundN)};
    for (const u64 containerI : results.presentContainerIndices())
    {
        for (const Stat stat : results.presentStats())
        {
            results.at(containerI, elementN, stat) *= invRoundN;
        }
    }
}

template <typename CommonKey, typename... ContainerInfos>
static void compareTypical(Stats & results)
{
    qc::Random random{u64(std::chrono::steady_clock::now().time_since_epoch().count())};

    for (const auto [elementN, roundN] : typicalElementRoundNs)
    {
        if (elementN > std::numeric_limits<qc::utype<CommonKey>>::max())
        {
            break;
        }

        std::cout << "Comparing " << roundN << " rounds of " << elementN << " elements...";

        compareTypicalSized<CommonKey, ContainerInfos...>(elementN, roundN, random, results);

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
    if constexpr (mode == CompareMode::oneVsOne)
    {
        static_assert(sizeof...(ContainerInfos) == 2);
        Stats results{};
        compareTypical<CommonKey, ContainerInfos...>(results);
        std::cout << std::endl;
        for (const auto [elementN, roundN] : typicalElementRoundNs)
        {
            reportComparison(results, 1, 0, elementN);
            std::cout << std::endl;
        }
    }
        // Detailed
    else if constexpr (mode == CompareMode::detailed)
    {
        Stats results{};
        compareDetailed<CommonKey, ContainerInfos...>(results);
        std::ofstream ofs{outFilePath};
        printOpsChartable(results, ofs);
        std::cout << "Wrote results to " << outFilePath << std::endl;
    }
        // Typical
    else if constexpr (mode == CompareMode::typical)
    {
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
    using Container = std::conditional_t<sizeof(u64) == 8, absl::flat_hash_set<K>, void>;
    using AllocatorContainer = std::conditional_t<sizeof(u64) == 8, absl::flat_hash_set<K, typename absl::flat_hash_set<K>::hasher, typename absl::flat_hash_set<K>::key_equal, qc::memory::RecordAllocator<K>>, void>;

    static inline const std::string name{"absl::flat_hash_set"};
};

template <typename K, typename V>
struct AbslMapInfo
{
    using Container = absl::flat_hash_map<K, V>;
    using AllocatorContainer = std::conditional_t<sizeof(u64) == 8, std::unordered_map<K, V, typename absl::flat_hash_map<K, V>::hasher, typename absl::flat_hash_map<K, V>::key_equal, qc::memory::RecordAllocator<std::pair<K, V>>>, void>;

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

s32 main()
{
    // 1v1
    if constexpr (false)
    {
        using K = u64;
        compare<CompareMode::oneVsOne, K, QcHashSetInfo<K>, AbslSetInfo<K>>();
    }
    // Set comparison
    else if constexpr (true)
    {
        using K = u64;
        compare<CompareMode::typical, K,
            QcHashSetInfo<K>,
            StdSetInfo<K>,
            AbslSetInfo<K>,
            RobinHoodSetInfo<K>,
            SkaSetInfo<K>,
            TslRobinSetInfo<K>,
            TslSparseSetInfo<K>>();
    }
    // Map comparison
    else if constexpr (false)
    {
        using K = u64;
        using V = std::string;
        compare<CompareMode::typical, K,
            QcHashMapInfo<K, V>,
            StdMapInfo<K, V>,
            AbslMapInfo<K, V>,
            RobinHoodMapInfo<K, V>,
            SkaMapInfo<K, V>,
            TslRobinMapInfo<K, V>,
            TslSparseMapInfo<K, V>>();
    }
    // Architecture comparison
    else if constexpr (false)
    {
        using K = u32;
        compare<CompareMode::typical, K, QcHashSetInfo<K>>();
    }
    // Set vs map
    else if constexpr (false)
    {
        compare<CompareMode::detailed, u64,
            QcHashSetInfo<u64, true>,
            QcHashMapInfo<u64, Trivial<8>, true>,
            QcHashMapInfo<u64, Trivial<16>, true>,
            QcHashMapInfo<u64, Trivial<32>, true>,
            QcHashMapInfo<u64, Trivial<64>, true>,
            QcHashMapInfo<u64, Trivial<128>, true>,
            QcHashMapInfo<u64, Trivial<256>, true>>();
    }
    // Trivial vs complex
    else if constexpr (false)
    {
        using K = u64;
        compare<CompareMode::detailed, u64,
            QcHashSetInfo<Trivial<sizeof(K)>, true, true>,
            QcHashSetInfo<Complex<sizeof(K)>, true, true>,
            QcHashMapInfo<Trivial<sizeof(K)>, Trivial<sizeof(K)>, true, true>,
            QcHashMapInfo<Complex<sizeof(K)>, Trivial<sizeof(K)>, true, true>,
            QcHashMapInfo<Trivial<sizeof(K)>, Complex<sizeof(K)>, true, true>,
            QcHashMapInfo<Complex<sizeof(K)>, Complex<sizeof(K)>, true, true>>();
    }

    return 0;
}
