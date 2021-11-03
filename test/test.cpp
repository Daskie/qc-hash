// First include in order to test its own includes
#include <qc-hash.hpp>

#include <array>
#include <chrono>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

#include <qc-core/core.hpp>
#include <qc-core/memory.hpp>
#include <qc-core/random.hpp>

using namespace std::string_literals;
using namespace qc::types;

using qc::hash::RawMap;
using qc::hash::RawSet;
using qc::hash::_RawFriend;

template <typename K>
struct NullHash
{
    size_t operator()(const K &) const noexcept
    {
        return 0u;
    }
};

struct qc::hash::_RawFriend
{
    template <typename K> using RawKey = typename RawSet<K>::_RawKey;

    template <typename K> static constexpr auto vacantKey{RawSet<K>::_vacantKey};
    template <typename K> static constexpr auto graveKey{RawSet<K>::_graveKey};

    template <typename K, typename H, typename A>
    static const K & getElement(const RawSet<K, H, A> & set, const size_t slotI)
    {
        return set._elements[slotI];
    }

    template <typename K, typename H, typename A>
    static K & getElement(RawSet<K, H, A> & set, const size_t slotI)
    {
        return set._elements[slotI];
    }

    template <typename K, typename H, typename A>
    static bool isPresent(const RawSet<K, H, A> & set, const size_t slotI)
    {
        return set._isPresent(set._raw(set._elements[slotI]));
    }

    template <typename K, typename H, typename A>
    static bool isVacant(const RawSet<K, H, A> & set, const size_t slotI)
    {
        return getElement(set, slotI) == vacantKey<K>;
    }

    template <typename K, typename H, typename A>
    static bool isGrave(const RawSet<K, H, A> & set, const size_t slotI)
    {
        return getElement(set, slotI) == graveKey<K>;
    }

    template <typename K, typename H, typename A, typename It>
    static size_t slotI(const RawSet<K, H, A> & set, const It it)
    {
        return it._element - set._elements;
    }

    template <typename K, typename H, typename A, typename It>
    static size_t dist(const RawSet<K, H, A> & set, const It it) {
        const size_t slotI{_RawFriend::slotI(set, it)};
        const size_t idealSlotI{set.slot(*it)};
        return slotI >= idealSlotI ? slotI - idealSlotI : set.slot_count() - idealSlotI + slotI;
    }
};

struct TrackedStats2
{
    int defConstructs{0u};
    int copyConstructs{0u};
    int moveConstructs{0u};
    int copyAssigns{0u};
    int moveAssigns{0u};
    int destructs{0u};

    int constructs() const { return defConstructs + copyConstructs + moveConstructs; }

    int assigns() const { return copyAssigns + moveAssigns; }

    int copies() const { return copyConstructs + copyAssigns; }

    int moves() const { return moveConstructs + moveAssigns; }

    int all() const { return constructs() + assigns() + destructs; }

    bool operator==(const TrackedStats2 &) const = default;
};

// For some reason memory allocation goes bananas when this is named `Tracked`?????
// And by bananas, I mean placement new for some reason writes into later bytes
struct Tracked2
{
    inline static TrackedStats2 totalStats{};

    inline static std::unordered_map<const Tracked2 *, TrackedStats2> registry{};

    static void resetTotals()
    {
        totalStats = {};
    }

    int val{};

    explicit Tracked2(const int val) :
        val{val}
    {
        registry[this] = {};
    }

    Tracked2()
    {
        registry[this] = {};
        ++registry[this].defConstructs;
        ++totalStats.defConstructs;
    }

    Tracked2(const Tracked2 & other) :
        val{other.val}
    {
        registry[this] = registry[&other];

        ++registry[this].copyConstructs;
        ++totalStats.copyConstructs;
    }

    Tracked2(Tracked2 && other) noexcept :
        val{std::exchange(other.val, {})}
    {
        registry[this] = registry[&other];

        ++registry[this].moveConstructs;
        ++totalStats.moveConstructs;
    }

    Tracked2 & operator=(const Tracked2 & other)
    {
        val = other.val;
        registry[this] = registry[&other];

        ++registry[this].copyAssigns;
        ++totalStats.copyAssigns;

        return *this;
    }

    Tracked2 & operator=(Tracked2 && other) noexcept
    {
        val = std::exchange(other.val, 0);
        registry[this] = registry[&other];

        ++registry[this].moveAssigns;
        ++totalStats.moveAssigns;

        return *this;
    }

    ~Tracked2()
    {
        ++registry[this].destructs;
        ++totalStats.destructs;
    }

    const TrackedStats2 & stats() const
    {
        return registry[this];
    }

};

static bool operator==(const Tracked2 & t1, const Tracked2 & t2)
{
    return t1.val == t2.val;
}

struct Tracked2Hash
{
    size_t operator()(const Tracked2 & tracked) const noexcept
    {
        return size_t(tracked.val);
    }
};

template <> struct qc::hash::IsUniquelyRepresentable<Tracked2> : std::true_type {};

using TrackedSet = RawSet<Tracked2, Tracked2Hash>;
using TrackedMap = RawMap<Tracked2, Tracked2, Tracked2Hash>;

template <typename K> using MemRecordSet = RawSet<K, typename RawSet<K>::hasher, qc::memory::RecordAllocator<K>>;
template <typename K, typename V> using MemRecordMap = RawMap<K, V, typename RawMap<K, V>::hasher, qc::memory::RecordAllocator<std::pair<K, V>>>;

using Tracked2MemRecordSet = RawSet<Tracked2, Tracked2Hash, qc::memory::RecordAllocator<Tracked2>>;
using Tracked2MemRecordMap = RawMap<Tracked2, Tracked2, Tracked2Hash, qc::memory::RecordAllocator<Tracked2>>;

TEST(identityHash, general)
{
    { // Standard
        struct Custom { size_t a; };

        const qc::hash::IdentityHash<Custom> h{};

        EXPECT_EQ(0x76543210u, h(Custom{0x76543210u}));
    }
    { // Oversized
        struct Custom { size_t a, b; };

        const qc::hash::IdentityHash<Custom> h{};

        EXPECT_EQ(0x76543210u, h(Custom{0x76543210u, 0xFEDCBA98}));
    }
    { // Unaligned small
        struct Custom { u8 a, b, c; };

        const qc::hash::IdentityHash<Custom> h{};

        EXPECT_EQ(0x543210u, h(Custom{u8(0x10u), u8(0x32u), u8(0x54u)}));
    }
    { // Unaligned large
        struct Custom { u16 a, b, c, d, e; };

        const qc::hash::IdentityHash<Custom> h{};

        EXPECT_EQ(size_t(sizeof(size_t) >= 8 ? 0x7766554433221100u : 0x33221100u), h(Custom{u16(0x1100u), u16(0x3322u), u16(0x5544u), u16(0x7766u), u16(0x9988u)}));
    }
}

TEST(identityHash, toSizeTSameAsMemcpy)
{
    std::array<u8, 8u> arr{0x10u, 0x32u, 0x54u, 0x76u, 0x98u, 0xBAu, 0xDCu, 0xFEu};
    size_t val;
    memcpy(&val, &arr, sizeof(size_t));
    EXPECT_EQ(val, qc::hash::_toSizeT(arr));
}

template <typename T>
static void testIntegerHash()
{
    const qc::hash::IdentityHash<T> h{};

    EXPECT_EQ(0u, h(T(0)));
    EXPECT_EQ(123u, h(T(123)));
    EXPECT_EQ(qc::utype<T>(std::numeric_limits<T>::min()), h(std::numeric_limits<T>::min()));
    EXPECT_EQ(qc::utype<T>(std::numeric_limits<T>::max()), h(std::numeric_limits<T>::max()));
}

TEST(identityHash, integers)
{
    testIntegerHash<u8>();
    testIntegerHash<s8>();
    testIntegerHash<u16>();
    testIntegerHash<s16>();
    testIntegerHash<u32>();
    testIntegerHash<s32>();
    if constexpr (sizeof(size_t) >= 8) {
        testIntegerHash<u64>();
        testIntegerHash<s64>();
    }
}

template <typename T>
static void testEnumHash()
{
    const qc::hash::IdentityHash<T> h{};

    EXPECT_EQ(0u, h(T(0)));
    EXPECT_EQ(123u, h(T(123)));
    EXPECT_EQ(qc::utype<T>(std::numeric_limits<std::underlying_type_t<T>>::min()), h(T(std::numeric_limits<std::underlying_type_t<T>>::min())));
    EXPECT_EQ(qc::utype<T>(std::numeric_limits<std::underlying_type_t<T>>::max()), h(T(std::numeric_limits<std::underlying_type_t<T>>::max())));
}

TEST(identityHash, enums)
{
    enum class EnumU8 : u8 {};
    enum class EnumS8 : s8 {};
    enum class EnumU16 : u16 {};
    enum class EnumS16 : s16 {};
    enum class EnumU32 : u32 {};
    enum class EnumS32 : s32 {};
    enum class EnumU64 : u64 {};
    enum class EnumS64 : s64 {};

    testEnumHash<EnumU8>();
    testEnumHash<EnumS8>();
    testEnumHash<EnumU16>();
    testEnumHash<EnumS16>();
    testEnumHash<EnumU32>();
    testEnumHash<EnumS32>();
    #ifdef _WIN64
    testEnumHash<EnumU64>();
    testEnumHash<EnumS64>();
    #endif
}

template <typename T>
static void testPointerHash()
{
    const qc::hash::IdentityHash<T *> h{};

    T * const p0{};
    T * const p1{p0 + 1};
    T * const p2{p0 - 1};
    T * const p3{p0 + 123};

    EXPECT_EQ(size_t(0u), h(p0));
    EXPECT_EQ(size_t(1u), h(p1));
    EXPECT_EQ(size_t(intptr_t(-1)) >> (std::bit_width(alignof(T)) - 1), h(p2));
    EXPECT_EQ(size_t(123u), h(p3));
}

TEST(identityHash, pointers)
{
    struct alignas(1) S1 { u8 data[1]; };
    struct alignas(2) S2 { u8 data[2]; };
    struct alignas(4) S4 { u8 data[4]; };
    struct alignas(8) S8 { u8 data[8]; };
    struct alignas(16) S16 { u8 data[16]; };
    struct alignas(32) S32 { u8 data[32]; };
    struct alignas(64) S64 { u8 data[64]; };

    testPointerHash<S1>();
    testPointerHash<S2>();
    testPointerHash<S4>();
    testPointerHash<S8>();
    testPointerHash<S16>();
    testPointerHash<S32>();
    testPointerHash<S64>();
}

TEST(fastHash, general)
{
    qc::hash::FastHash<size_t> fastHash{};
    if constexpr (sizeof(size_t) == 8) {
        EXPECT_EQ(7016536041891711906u, fastHash(0x12345678u));
    }
    else if constexpr (sizeof(size_t) == 4) {
        EXPECT_EQ(1291257483u, fastHash(0x12345678u));
    }

    RawSet<int, qc::hash::FastHash<int>> s{};
    for (int i{0}; i < 100; ++i) {
        EXPECT_TRUE(s.insert(i).second);
    }
    EXPECT_EQ(100u, s.size());
    for (int i{0}; i < 100; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
    for (int i{0}; i < 100; ++i) {
        EXPECT_TRUE(s.erase(i));
    }
    EXPECT_TRUE(s.empty());
}

TEST(fastHash, sharedPtr)
{
    std::shared_ptr<int> val{std::make_shared<int>()};
    EXPECT_EQ(qc::hash::FastHash<int *>{}(val.get()), qc::hash::FastHash<std::shared_ptr<int>>{}(val));
}

TEST(fastHash, string)
{
    char cStr[]{"abcdefghijklmnopqrstuvwxyz"};
    const char * constCStr{cStr};
    std::string stdStr{cStr};
    std::string_view strView{cStr};

    #ifdef _WIN64
    const size_t hash{8454416734917819949u};
    #else
    const size_t hash{3459995157u};
    #endif

    EXPECT_EQ(hash, qc::hash::FastHash<std::string>{}(cStr));
    EXPECT_EQ(hash, qc::hash::FastHash<std::string>{}(constCStr));
    EXPECT_EQ(hash, qc::hash::FastHash<std::string>{}(stdStr));
    EXPECT_EQ(hash, qc::hash::FastHash<std::string>{}(strView));

    EXPECT_EQ(hash, qc::hash::FastHash<std::string_view>{}(cStr));
    EXPECT_EQ(hash, qc::hash::FastHash<std::string_view>{}(constCStr));
    EXPECT_EQ(hash, qc::hash::FastHash<std::string_view>{}(stdStr));
    EXPECT_EQ(hash, qc::hash::FastHash<std::string_view>{}(strView));
}

template <typename T, typename T_> concept FastHashHeterogeneous = requires (qc::hash::FastHash<T> h, T_ k) { { h(k) } -> std::same_as<size_t>; };

TEST(fastHash, stringHeterogeneity)
{
    static_assert(FastHashHeterogeneous<std::string, std::string>);
    static_assert(FastHashHeterogeneous<std::string, std::string_view>);
    static_assert(FastHashHeterogeneous<std::string, const char *>);
    static_assert(FastHashHeterogeneous<std::string, char *>);

    static_assert(FastHashHeterogeneous<std::string_view, std::string>);
    static_assert(FastHashHeterogeneous<std::string_view, std::string_view>);
    static_assert(FastHashHeterogeneous<std::string_view, const char *>);
    static_assert(FastHashHeterogeneous<std::string_view, char *>);

    static_assert(!FastHashHeterogeneous<const char *, std::string>);
    static_assert(!FastHashHeterogeneous<const char *, std::string_view>);
    static_assert(FastHashHeterogeneous<const char *, const char *>);
    static_assert(FastHashHeterogeneous<const char *, char *>);

    static_assert(!FastHashHeterogeneous<char *, std::string>);
    static_assert(!FastHashHeterogeneous<char *, std::string_view>);
    static_assert(FastHashHeterogeneous<char *, const char *>);
    static_assert(FastHashHeterogeneous<char *, char *>);
}

TEST(faseHash, zeroSequences)
{
    #ifdef _WIN64
    constexpr size_t hash01{14313749767032793493u};
    constexpr size_t hash02{10180755460356035370u};
    constexpr size_t hash03{6047761153679277247u};
    constexpr size_t hash04{1914766847002519124u};
    constexpr size_t hash05{16228516614035312617u};
    constexpr size_t hash06{12095522307358554494u};
    constexpr size_t hash07{7962528000681796371u};
    constexpr size_t hash08{3829533694005038248u};
    constexpr size_t hash09{16354269775938918017u};
    constexpr size_t hash10{1774305018856974138u};
    constexpr size_t hash11{5641084335484581875u};
    constexpr size_t hash12{9507863652112189612u};
    constexpr size_t hash13{13374642968739797349u};
    constexpr size_t hash14{17241422285367405086u};
    constexpr size_t hash15{2661457528285461207u};
    constexpr size_t hash16{6528236844913068944u};
    #else
    constexpr size_t hash01{1540483477u};
    constexpr size_t hash02{3080966954u};
    constexpr size_t hash03{326483135u};
    constexpr size_t hash04{1866966612u};
    constexpr size_t hash05{3390362525u};
    constexpr size_t hash06{4068435030u};
    constexpr size_t hash07{451540239u};
    constexpr size_t hash08{1129612744u};
    constexpr size_t hash09{3691282965u};
    constexpr size_t hash10{1238113986u};
    constexpr size_t hash11{3079912303u};
    constexpr size_t hash12{626743324u};
    constexpr size_t hash13{3208407549u};
    constexpr size_t hash14{3124826030u};
    constexpr size_t hash15{3041244511u};
    constexpr size_t hash16{2957662992u};
    #endif

    EXPECT_EQ(hash01, qc::hash::FastHash<u8>{}(0u));
    EXPECT_EQ(hash02, qc::hash::FastHash<u16>{}(0u));
    EXPECT_EQ(hash04, qc::hash::FastHash<u32>{}(0u));
    EXPECT_EQ(hash08, qc::hash::FastHash<u64>{}(0u));

    EXPECT_EQ(hash01, (qc::hash::FastHash<std::array<u8,  1>>{}(std::array<u8,  1>{})));
    EXPECT_EQ(hash02, (qc::hash::FastHash<std::array<u8,  2>>{}(std::array<u8,  2>{})));
    EXPECT_EQ(hash03, (qc::hash::FastHash<std::array<u8,  3>>{}(std::array<u8,  3>{})));
    EXPECT_EQ(hash04, (qc::hash::FastHash<std::array<u8,  4>>{}(std::array<u8,  4>{})));
    EXPECT_EQ(hash05, (qc::hash::FastHash<std::array<u8,  5>>{}(std::array<u8,  5>{})));
    EXPECT_EQ(hash06, (qc::hash::FastHash<std::array<u8,  6>>{}(std::array<u8,  6>{})));
    EXPECT_EQ(hash07, (qc::hash::FastHash<std::array<u8,  7>>{}(std::array<u8,  7>{})));
    EXPECT_EQ(hash08, (qc::hash::FastHash<std::array<u8,  8>>{}(std::array<u8,  8>{})));
    EXPECT_EQ(hash09, (qc::hash::FastHash<std::array<u8,  9>>{}(std::array<u8,  9>{})));
    EXPECT_EQ(hash10, (qc::hash::FastHash<std::array<u8, 10>>{}(std::array<u8, 10>{})));
    EXPECT_EQ(hash11, (qc::hash::FastHash<std::array<u8, 11>>{}(std::array<u8, 11>{})));
    EXPECT_EQ(hash12, (qc::hash::FastHash<std::array<u8, 12>>{}(std::array<u8, 12>{})));
    EXPECT_EQ(hash13, (qc::hash::FastHash<std::array<u8, 13>>{}(std::array<u8, 13>{})));
    EXPECT_EQ(hash14, (qc::hash::FastHash<std::array<u8, 14>>{}(std::array<u8, 14>{})));
    EXPECT_EQ(hash15, (qc::hash::FastHash<std::array<u8, 15>>{}(std::array<u8, 15>{})));
    EXPECT_EQ(hash16, (qc::hash::FastHash<std::array<u8, 16>>{}(std::array<u8, 16>{})));
}

TEST(set, constructor_default)
{
    MemRecordSet<int> s{};
    EXPECT_EQ(qc::hash::config::minCapacity, s.capacity());
    EXPECT_EQ(size_t(0u), s.size());
    EXPECT_EQ(0u, s.get_allocator().stats().allocations);
}

TEST(set, constructor_capacity)
{
    qc::memory::RecordAllocator<int> allocator{};
    EXPECT_EQ(size_t(  16u), (MemRecordSet<int>{   0u, allocator}.capacity()));
    EXPECT_EQ(size_t(  16u), (MemRecordSet<int>{   1u, allocator}.capacity()));
    EXPECT_EQ(size_t(  16u), (MemRecordSet<int>{  16u, allocator}.capacity()));
    EXPECT_EQ(size_t(  32u), (MemRecordSet<int>{  17u, allocator}.capacity()));
    EXPECT_EQ(size_t(  32u), (MemRecordSet<int>{  32u, allocator}.capacity()));
    EXPECT_EQ(size_t(  64u), (MemRecordSet<int>{  33u, allocator}.capacity()));
    EXPECT_EQ(size_t(  64u), (MemRecordSet<int>{  64u, allocator}.capacity()));
    EXPECT_EQ(size_t( 128u), (MemRecordSet<int>{  65u, allocator}.capacity()));
    EXPECT_EQ(size_t(1024u), (MemRecordSet<int>{1000u, allocator}.capacity()));
    EXPECT_EQ(0u, allocator.stats().allocations);
}

TEST(set, constructor_range)
{
    std::vector<int> values{
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
        20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
        30, 31, 32, 33, 34, 35, 36, 37, 38, 39
    };
    MemRecordSet<int> s{values.cbegin(), values.cend()};
    EXPECT_EQ(size_t(40u), s.size());
    EXPECT_EQ(size_t(64u), s.capacity());
    for (const int v : values) {
        EXPECT_TRUE(s.contains(v));
    }
    EXPECT_EQ(1u, s.get_allocator().stats().allocations);
}

TEST(set, constructor_initializerList)
{
    MemRecordSet<int> s{{
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
        20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
        30, 31, 32, 33, 34, 35, 36, 37, 38, 39
    }};
    EXPECT_EQ(size_t(40u), s.size());
    EXPECT_EQ(size_t(64u), s.capacity());
    for (int i{0}; i < 40; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
    EXPECT_EQ(1u, s.get_allocator().stats().allocations);
}

TEST(set, constructor_copy)
{
    // Trivial type
    {
        MemRecordSet<int> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.insert(i);
        }
        const size_t prevAllocCount{s1.get_allocator().stats().allocations};
        MemRecordSet<int> s2{s1};
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s1, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().stats().allocations);
    }

    // Non-trivial type
    {
        Tracked2MemRecordSet s1{};
        for (int i{0}; i < 100; ++i) {
            s1.emplace(i);
        }
        const size_t prevAllocCount{s1.get_allocator().stats().allocations};
        Tracked2MemRecordSet s2{s1};
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s1, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().stats().allocations);
    }
}

TEST(set, constructor_move)
{
    // Trivial type
    {
        MemRecordSet<int> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.insert(i);
        }
        MemRecordSet<int> ref{s1};
        const size_t prevAllocCount{s1.get_allocator().stats().allocations};
        MemRecordSet<int> s2{std::move(s1)};
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(ref, s2);
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc::hash::config::minCapacity, s1.capacity());
        EXPECT_EQ(prevAllocCount, s2.get_allocator().stats().allocations);
    }
    // Non-trivial type
    {
        Tracked2MemRecordSet s1{};
        for (int i{0}; i < 100; ++i) {
            s1.emplace(i);
        }
        Tracked2MemRecordSet ref{s1};
        const size_t prevAllocCount{s1.get_allocator().stats().allocations};
        Tracked2MemRecordSet s2{std::move(s1)};
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(ref, s2);
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc::hash::config::minCapacity, s1.capacity());
        EXPECT_EQ(prevAllocCount, s2.get_allocator().stats().allocations);
    }
}

TEST(set, assignOperator_initializerList)
{
    MemRecordSet<int> s{};
    for (int i{0}; i < 100; ++i) {
        s.insert(i);
    }
    EXPECT_EQ(128u, s.capacity());
    const size_t prevAllocCount{s.get_allocator().stats().allocations};

    s = {0, 1, 2, 3, 4, 5};
    EXPECT_EQ(size_t(6u), s.size());
    EXPECT_EQ(qc::hash::config::minCapacity, s.capacity());
    for (int i{0}; i < 6; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
}

TEST(set, assignOperator_copy)
{
    // Trivial type
    {
        MemRecordSet<int> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.insert(i);
        }
        const size_t prevAllocCount{s1.get_allocator().stats().allocations};

        MemRecordSet<int> s2{};
        s2 = s1;
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s1, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().stats().allocations);

        s2 = s2;
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s2, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().stats().allocations);
    }

    // Non-trivial type
    {
        Tracked2MemRecordSet s1{};
        for (int i{0}; i < 100; ++i) {
            s1.emplace(i);
        }
        const size_t prevAllocCount{s1.get_allocator().stats().allocations};

        Tracked2MemRecordSet s2{};
        s2 = s1;
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s1, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().stats().allocations);

        s2 = s2;
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s2, s2);
        EXPECT_EQ(prevAllocCount + 1u, s2.get_allocator().stats().allocations);
    }
}

TEST(set, assignOperator_move)
{
    // Trivial type
    {
        MemRecordSet<int> s1{};
        for (int i{0}; i < 100; ++i) {
            s1.insert(i);
        }
        MemRecordSet<int> ref{s1};
        const size_t prevAllocCount{s1.get_allocator().stats().allocations};

        MemRecordSet<int> s2{};
        s2 = std::move(s1);
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(ref, s2);
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc::hash::config::minCapacity, s1.capacity());
        EXPECT_EQ(prevAllocCount, s2.get_allocator().stats().allocations);

        s2 = std::move(s2);
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s2, s2);
        EXPECT_EQ(prevAllocCount, s2.get_allocator().stats().allocations);

        s2 = std::move(s1);
        EXPECT_TRUE(s2.empty());
        EXPECT_EQ(qc::hash::config::minCapacity, s2.capacity());
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc::hash::config::minCapacity, s1.capacity());
        EXPECT_EQ(s1, s2);
    }

    // Non-trivial type
    {
        Tracked2MemRecordSet s1{};
        for (int i{0}; i < 100; ++i) {
            s1.emplace(i);
        }
        Tracked2MemRecordSet ref{s1};
        const size_t prevAllocCount{s1.get_allocator().stats().allocations};

        Tracked2MemRecordSet s2{};
        s2 = std::move(s1);
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(ref, s2);
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc::hash::config::minCapacity, s1.capacity());
        EXPECT_EQ(prevAllocCount, s2.get_allocator().stats().allocations);

        s2 = std::move(s2);
        EXPECT_EQ(100u, s2.size());
        EXPECT_EQ(128u, s2.capacity());
        EXPECT_EQ(s2, s2);
        EXPECT_EQ(prevAllocCount, s2.get_allocator().stats().allocations);

        s2 = std::move(s1);
        EXPECT_TRUE(s2.empty());
        EXPECT_EQ(qc::hash::config::minCapacity, s2.capacity());
        EXPECT_TRUE(s1.empty());
        EXPECT_EQ(qc::hash::config::minCapacity, s1.capacity());
        EXPECT_EQ(s1, s2);
    }
}

// Keep parallel with `emplace_lVal` and `tryEmplace_lVal` tests
TEST(set, insert_lVal)
{
    Tracked2::resetTotals();
    Tracked2MemRecordSet s{};

    for (int i{0}; i < 100; ++i) {
        const Tracked2 val{i};

        auto [it1, inserted1]{s.insert(val)};
        EXPECT_TRUE(inserted1);
        EXPECT_EQ(val, *it1);

        auto [it2, inserted2]{s.insert(val)};
        EXPECT_FALSE(inserted2);
        EXPECT_EQ(it1, it2);
    }

    EXPECT_EQ(100u, s.size());
    EXPECT_EQ(128u, s.capacity());
    EXPECT_EQ(4u, s.get_allocator().stats().allocations);
    EXPECT_EQ(100, Tracked2::totalStats.copyConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.destructs);
    EXPECT_EQ(0, Tracked2::totalStats.defConstructs);
    EXPECT_EQ(0, Tracked2::totalStats.assigns());
}

// Keep parallel with `emplace_rVal` and `tryEmplace_rVal` tests
TEST(set, insert_rVal)
{
    TrackedSet s{};
    Tracked2 val1{7};
    Tracked2 val2{7};

    const auto [it1, inserted1]{s.insert(std::move(val1))};
    EXPECT_TRUE(inserted1);
    const Tracked2 & tracked{*it1};
    EXPECT_EQ(7, int(tracked.val));
    EXPECT_EQ(1, tracked.stats().moveConstructs);
    EXPECT_EQ(1, tracked.stats().all());
    EXPECT_EQ(0, int(val1.val));

    const auto [it2, inserted2]{s.insert(std::move(val2))};
    EXPECT_FALSE(inserted2);
    EXPECT_EQ(it1, it2);
    EXPECT_EQ(7, int(val2.val));
    EXPECT_EQ(0, val2.stats().all());
}

TEST(set, insert_range)
{
    Tracked2MemRecordSet s{};
    std::vector<Tracked2> values{};
    for (int i{0}; i < 100; ++i) values.emplace_back(i);

    Tracked2::resetTotals();
    s.insert(values.cbegin(), values.cend());
    EXPECT_EQ(100u, s.size());
    EXPECT_EQ(128u, s.capacity());
    for (int i{0}; i < 100; ++i) {
        EXPECT_TRUE(s.contains(Tracked2{i}));
    }
    EXPECT_EQ(4u, s.get_allocator().stats().allocations);
    EXPECT_EQ(100, Tracked2::totalStats.copyConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.destructs);
    EXPECT_EQ(0, Tracked2::totalStats.defConstructs);
    EXPECT_EQ(0, Tracked2::totalStats.assigns());
}

TEST(set, insert_initializerList)
{
    MemRecordSet<int> s{};
    s.insert({0, 1, 2, 3, 4, 5});
    EXPECT_EQ(6u, s.size());
    EXPECT_EQ(16u, s.capacity());
    for (int i{0}; i < 6; ++i) {
        EXPECT_TRUE(s.contains(i));
    }
    EXPECT_EQ(1u, s.get_allocator().stats().allocations);
}

// Keep parallel with `insert_lVal` and `tryEmplace_lVal` tests
TEST(set, emplace_lVal)
{
    Tracked2::resetTotals();
    Tracked2MemRecordSet s{};

    for (int i{0}; i < 100; ++i) {
        const Tracked2 val{i};

        auto [it1, inserted1]{s.emplace(val)};
        EXPECT_TRUE(inserted1);
        EXPECT_EQ(val, *it1);

        auto [it2, inserted2]{s.emplace(val)};
        EXPECT_FALSE(inserted2);
        EXPECT_EQ(it1, it2);
    }

    EXPECT_EQ(100u, s.size());
    EXPECT_EQ(128u, s.capacity());
    EXPECT_EQ(4u, s.get_allocator().stats().allocations);
    EXPECT_EQ(100, Tracked2::totalStats.copyConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.destructs);
    EXPECT_EQ(0, Tracked2::totalStats.defConstructs);
    EXPECT_EQ(0, Tracked2::totalStats.assigns());
}

// Keep parallel with `insert_rVal` and `tryEmplace_rVal` tests
TEST(set, emplace_rVal)
{
    TrackedSet s{};
    Tracked2 val1{7};
    Tracked2 val2{7};

    const auto [it1, inserted1]{s.emplace(std::move(val1))};
    EXPECT_TRUE(inserted1);
    const Tracked2 & tracked{*it1};
    EXPECT_EQ(7, int(tracked.val));
    EXPECT_EQ(1, tracked.stats().moveConstructs);
    EXPECT_EQ(1, tracked.stats().all());
    EXPECT_EQ(0, int(val1.val));

    const auto [it2, inserted2]{s.emplace(std::move(val2))};
    EXPECT_FALSE(inserted2);
    EXPECT_EQ(it1, it2);
    EXPECT_EQ(7, int(val2.val));
    EXPECT_EQ(0, val2.stats().all());
}

TEST(set, emplace_keyArgs)
{
    TrackedSet s{};
    const auto [it, inserted]{s.emplace(7)};
    EXPECT_TRUE(inserted);
    EXPECT_EQ(7, int(it->val));
    EXPECT_EQ(1, it->stats().moveConstructs);
    EXPECT_EQ(1, it->stats().all());
}

// Keep parallel with `insert_lVal` and `emplace_lVal` tests
TEST(set, tryEmplace_lVal)
{
    Tracked2::resetTotals();
    Tracked2MemRecordSet s{};

    for (int i{0}; i < 100; ++i) {
        const Tracked2 val{i};

        auto [it1, inserted1]{s.try_emplace(val)};
        EXPECT_TRUE(inserted1);
        EXPECT_EQ(val, *it1);

        auto [it2, inserted2]{s.try_emplace(val)};
        EXPECT_FALSE(inserted2);
        EXPECT_EQ(it1, it2);
    }

    EXPECT_EQ(100u, s.size());
    EXPECT_EQ(128u, s.capacity());
    EXPECT_EQ(4u, s.get_allocator().stats().allocations);
    EXPECT_EQ(100, Tracked2::totalStats.copyConstructs);
    EXPECT_EQ(16 + 32 + 64, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.destructs);
    EXPECT_EQ(0, Tracked2::totalStats.defConstructs);
    EXPECT_EQ(0, Tracked2::totalStats.assigns());
}

// Keep parallel with `insert_rVal` and `emplace_rVal` tests
TEST(set, tryEmplace_rVal)
{
    TrackedSet s{};
    Tracked2 val1{7};
    Tracked2 val2{7};

    const auto [it1, inserted1]{s.try_emplace(std::move(val1))};
    EXPECT_TRUE(inserted1);
    const Tracked2 & tracked{*it1};
    EXPECT_EQ(7, int(tracked.val));
    EXPECT_EQ(1, tracked.stats().moveConstructs);
    EXPECT_EQ(1, tracked.stats().all());
    EXPECT_EQ(0, int(val1.val));

    const auto [it2, inserted2]{s.try_emplace(std::move(val2))};
    EXPECT_FALSE(inserted2);
    EXPECT_EQ(it1, it2);
    EXPECT_EQ(7, int(val2.val));
    EXPECT_EQ(0, val2.stats().all());
}

TEST(set, eraseKey)
{
    TrackedSet s{};

    Tracked2::resetTotals();
    EXPECT_FALSE(s.erase(Tracked2{0}));
    EXPECT_EQ(1, Tracked2::totalStats.destructs);
    EXPECT_EQ(1, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{0}; i < 100; ++i) {
        s.emplace(i);
    }
    EXPECT_EQ(size_t(100u), s.size());
    EXPECT_EQ(size_t(128u), s.capacity());
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.destructs);
    EXPECT_EQ(Tracked2::totalStats.moveConstructs + Tracked2::totalStats.destructs, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    EXPECT_FALSE(s.erase(Tracked2{100}));
    EXPECT_EQ(1, Tracked2::totalStats.destructs);
    EXPECT_EQ(Tracked2::totalStats.destructs, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{0}; i < 100; ++i) {
        EXPECT_TRUE(s.erase(Tracked2{i}));
        EXPECT_EQ(size_t(100u - i - 1u), s.size());
    }
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(size_t(128u), s.capacity());
    EXPECT_EQ(200, Tracked2::totalStats.destructs);
    EXPECT_EQ(Tracked2::totalStats.destructs, Tracked2::totalStats.all());
    Tracked2::resetTotals();
}

TEST(set, eraseIterator)
{
    RawSet<int> s{};

    for (int i{0}; i < 100; ++i) {
        s.insert(i);
    }
    EXPECT_EQ(size_t(100u), s.size());
    EXPECT_EQ(size_t(128u), s.capacity());

    for (int i{0}; i < 100; ++i) {
        s.erase(s.find(i));
        EXPECT_EQ(size_t(100u - i - 1u), s.size());
    }
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(size_t(128u), s.capacity());
}

TEST(set, clear)
{
    // Trivially destructible type
    {
        RawSet<int> s{};
        for (int i{0}; i < 100; ++i) s.insert(i);
        EXPECT_EQ(size_t(100u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());

        s.clear();
        EXPECT_EQ(size_t(0u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());
    }

    // Non-trivially destructible type
    {

        TrackedSet s{};
        for (int i{0}; i < 100; ++i) s.emplace(i);
        EXPECT_EQ(size_t(100u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());

        Tracked2::resetTotals();
        s.clear();
        EXPECT_EQ(size_t(0u), s.size());
        EXPECT_EQ(size_t(128u), s.capacity());
        EXPECT_EQ(100, Tracked2::totalStats.destructs);
        EXPECT_EQ(100, Tracked2::totalStats.all());
    }
}

// Keep parallel with `count` test
TEST(set, contains)
{
    RawSet<int> s{};
    for (int i{0}; i < 100; ++i) {
        s.insert(i);
        for (int j{0}; j <= i; ++j) {
            EXPECT_TRUE(s.contains(j));
        }
    }

    s.clear();
    for (int i{0}; i < 100; ++i) {
        EXPECT_FALSE(s.contains(i));
    }
}

// Keep parallel with `contains` test
TEST(set, count)
{
    RawSet<int> s{};
    for (int i{0}; i < 100; ++i) {
        s.insert(i);
        for (int j{0}; j <= i; ++j) {
            EXPECT_EQ(1u, s.count(j));
        }
    }

    s.clear();
    for (int i{0}; i < 100; ++i) {
        EXPECT_EQ(0u, s.count(i));
    }
}

TEST(set, begin)
{
    RawSet<int> s{};
    EXPECT_EQ(s.end(), s.begin());

    s.insert(7);
    EXPECT_NE(s.end(), s.begin());
    EXPECT_EQ(s.begin(), s.begin());
    EXPECT_EQ(7, *s.begin());

    auto it{s.begin()};
    ++it;
    EXPECT_EQ(s.end(), it);

    EXPECT_EQ(s.begin(), s.cbegin());
}

TEST(set, end)
{
    RawSet<int> s{};
    EXPECT_EQ(s.begin(), s.end());
    EXPECT_EQ(s.end(), s.end());

    s.insert(7);
    EXPECT_NE(s.begin(), s.end());

    EXPECT_EQ(s.end(), s.cend());
}

// Keep parallel with `equal_range` test
TEST(set, find)
{
    RawSet<int> s{};
    EXPECT_EQ(s.end(), s.find(0));

    for (int i{0}; i < 100; ++i) {
        s.insert(i);
    }
    for (int i{0}; i < 100; ++i) {
        const auto it{s.find(i)};
        EXPECT_NE(s.end(), it);
        EXPECT_EQ(i, *it);
    }

    EXPECT_EQ(s.end(), s.find(100));
}

TEST(set, slot)
{
    RawSet<int> s(128);
    s.insert(7);
    EXPECT_EQ(_RawFriend::slotI(s, s.find(7)), s.slot(7));
}

// `reserve` method is synonymous with `rehash` method
TEST(set, reserve) {}

TEST(set, rehash)
{
    RawSet<int> s{};

    EXPECT_EQ(qc::hash::config::minCapacity, s.capacity());

    s.rehash(0u);
    EXPECT_EQ(qc::hash::config::minCapacity, s.capacity());

    s.rehash(1u);
    EXPECT_EQ(qc::hash::config::minCapacity, s.capacity());

    for (int i{0}; i < 16; ++i) {
        s.insert(i);
    }
    EXPECT_EQ(size_t(16u), s.size());
    EXPECT_EQ(size_t(16u), s.capacity());

    s.emplace(16);
    EXPECT_EQ(size_t(17u), s.size());
    EXPECT_EQ(size_t(32u), s.capacity());

    for (int i{17}; i < 128; ++i) {
        s.emplace(i);
    }
    EXPECT_EQ(size_t(128u), s.size());
    EXPECT_EQ(size_t(128u), s.capacity());

    s.rehash(500u);
    EXPECT_EQ(size_t(128u), s.size());
    EXPECT_EQ(size_t(256u), s.capacity());
    for (int i = 0; i < 128; ++i) {
        EXPECT_TRUE(s.contains(i));
    }

    s.rehash(10u);
    EXPECT_EQ(size_t(128u), s.size());
    EXPECT_EQ(size_t(128u), s.capacity());
    for (int i = 0; i < 128; ++i) {
        EXPECT_TRUE(s.contains(i));
    }

    s.clear();
    EXPECT_EQ(size_t(0u), s.size());
    EXPECT_EQ(size_t(128u), s.capacity());

    s.rehash(0u);
    EXPECT_EQ(qc::hash::config::minCapacity, s.capacity());
}

TEST(set, swap)
{
    RawSet<int> s1{1, 2, 3};
    RawSet<int> s2{4, 5, 6};
    RawSet<int> s3{s1};
    RawSet<int> s4{s2};
    EXPECT_EQ(s1, s3);
    EXPECT_EQ(s2, s4);
    s3.swap(s4);
    EXPECT_EQ(s2, s3);
    EXPECT_EQ(s1, s4);
    std::swap(s3, s4);
    EXPECT_EQ(s1, s3);
    EXPECT_EQ(s2, s4);

    auto it1{s1.cbegin()};
    auto it2{s2.cbegin()};
    auto it3{it1};
    auto it4{it2};
    EXPECT_EQ(it1, it3);
    EXPECT_EQ(it2, it4);
    std::swap(it1, it2);
    EXPECT_EQ(it1, it4);
    EXPECT_EQ(it2, it3);
}

TEST(set, size_empty_capacity_slotCount)
{
    RawSet<int> s{};
    EXPECT_EQ(0u, s.size());
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(qc::hash::config::minCapacity, s.capacity());

    for (int i{0}; i < 100; ++i) {
        s.insert(i);
    }
    EXPECT_EQ(100u, s.size());
    EXPECT_FALSE(s.empty());
    EXPECT_EQ(128u, s.capacity());
    EXPECT_EQ(256u, s.slot_count());
}

TEST(set, maxSize)
{
    RawSet<int> s{};
    if constexpr (sizeof(size_t) == 4) {
        EXPECT_EQ(0b01000000'00000000'00000000'00000010u, s.max_size());
    }
    else if constexpr (sizeof(size_t) == 8) {
        EXPECT_EQ(0b01000000'00000000'00000000'00000000'00000000'00000000'00000000'00000010u, s.max_size());
    }
}

TEST(set, maxSlotCount)
{
    RawSet<int> s{};
    if constexpr (sizeof(size_t) == 4) {
        EXPECT_EQ(0b10000000'00000000'00000000'00000000u, s.max_slot_count());
    }
    else if constexpr (sizeof(size_t) == 8) {
        EXPECT_EQ(0b10000000'00000000'00000000'00000000'00000000'00000000'00000000'00000000u, s.max_slot_count());
    }
}

TEST(set, loadFactor)
{
    RawSet<int> s{};
    EXPECT_EQ(0.0f, s.load_factor());

    s.insert(7);
    EXPECT_EQ(1.0f / 32.0f, s.load_factor());
}

TEST(set, maxLoadFactor)
{
    RawSet<int> s{};
    EXPECT_EQ(0.5f, s.max_load_factor());

    s.insert(7);
    EXPECT_EQ(0.5f, s.max_load_factor());
}

TEST(set, getters)
{
    RawSet<int> s{};
    s.hash_function();
    s.get_allocator();
}

TEST(set, equality)
{
    RawSet<int> s1{}, s2{};
    for (int i{0}; i < 100; ++i) {
        s1.emplace(i);
        s2.emplace(i + 100);
    }
    EXPECT_TRUE(s1 == s1);
    EXPECT_TRUE(s1 != s2);

    s2 = s1;
    EXPECT_TRUE(s1 == s2);
}

TEST(set, iteratorTrivial)
{
    static_assert(std::is_trivial_v<RawSet<int>::iterator>);
    static_assert(std::is_trivial_v<RawSet<int>::const_iterator>);
    static_assert(std::is_standard_layout_v<RawSet<int>::iterator>);
    static_assert(std::is_standard_layout_v<RawSet<int>::const_iterator>);
}

TEST(set, iterator)
{
    RawSet<int> s{};
    for (int i{0}; i < 100; ++i) {
        s.insert(i);
    }

    int i{0};
    for (auto it{s.begin()}; it != s.end(); ++it) {
        EXPECT_EQ(i + 1, ++*it);
        EXPECT_EQ(i, --*it);
        ++i;
    }
}

TEST(set, forEachLoop)
{
    RawSet<int> s{};
    for (int i{0}; i < 100; ++i) {
        s.insert(i);
    }

    int i{0};
    for (int & val : s) {
        EXPECT_EQ(i + 1, ++val);
        EXPECT_EQ(i, --val);
        ++i;
    }
}

TEST(set, iteratorConversion)
{
    // Just checking for compilation

    RawSet<int> s{1, 2, 3};

    RawSet<int>::iterator it1(s.begin());
    RawSet<int>::const_iterator cit1 = s.cbegin();

    //it1 = cit1; // Should not compile
    static_assert(!std::is_convertible_v<RawSet<int>::const_iterator, RawSet<int>::iterator>);
    cit1 = it1;

    ++*it1;
    //++*cit1; // Should not compile
    static_assert(std::is_const_v<std::remove_reference_t<decltype(*cit1)>>);

    RawSet<int>::iterator it2{it1};
    it2 = it1;
    RawSet<int>::const_iterator cit2{cit1};
    cit2 = cit1;

    RawSet<int>::iterator it3{std::move(it1)};
    it3 = std::move(it1);
    RawSet<int>::const_iterator cit3{std::move(cit1)};
    cit3 = std::move(cit1);

    it1 == cit1;
    cit1 == it1;
}

TEST(set, singleElementInitializerList)
{
    RawSet<int> s{100};
    EXPECT_EQ(1u, s.size());
    EXPECT_EQ(qc::hash::config::minCapacity, s.capacity());
    EXPECT_EQ(100, *s.cbegin());
}

TEST(set, noPreemtiveRehash)
{
    RawSet<int> s{};
    for (int i{0}; i < int(qc::hash::config::minCapacity) - 1; ++i) s.insert(i);
    EXPECT_EQ(qc::hash::config::minCapacity, s.capacity());
    s.emplace(int(qc::hash::config::minCapacity - 1));
    EXPECT_EQ(qc::hash::config::minCapacity, s.capacity());
    s.emplace(int(qc::hash::config::minCapacity - 1));
    EXPECT_EQ(qc::hash::config::minCapacity, s.capacity());
}

struct SetDistStats
{
    size_t min, max, median;
    double mean, stdDev;
};

template <typename V>
SetDistStats calcStats(const RawSet<V> & set)
{
    SetDistStats distStats{};
    std::map<size_t, size_t> histo{};

    distStats.min = ~size_t(0u);
    for (auto it{set.cbegin()}; it != set.cend(); ++it) {
        const size_t dist{_RawFriend::dist(set, it)};
        //++distStats.histo[dist];
        if (dist < distStats.min) distStats.min = dist;
        else if (dist > distStats.max) distStats.max = dist;
        distStats.mean += dist;
    }
    distStats.mean /= double(set.size());

    for (auto it{set.cbegin()}; it != set.cend(); ++it) {
        const size_t dist{_RawFriend::dist(set, it)};
        double diff{double(dist) - distStats.mean};
        distStats.stdDev += diff * diff;
    }
    distStats.stdDev = std::sqrt(distStats.stdDev / double(set.size()));

    size_t medianCount{0u};
    for (const auto distCount : histo) {
        if (distCount.second > medianCount) {
            distStats.median = distCount.first;
            medianCount = distCount.second;
        }
    }

    return distStats;
}

TEST(set, stats) {
    constexpr size_t size{8192};

    RawSet<int> s(size);
    for (int i{0}; i < size; ++i) {
        s.insert(rand());
    }

    const SetDistStats stats{calcStats(s)};
    EXPECT_EQ(0u, stats.median);
    EXPECT_NEAR(0.2, stats.mean, 0.1);
    EXPECT_NEAR(0.65, stats.stdDev, 0.1);
}

template <typename K, typename V> void testStaticMemory()
{
    static constexpr size_t capacity{128u};
    static constexpr size_t slotCount{capacity * 2u};

    MemRecordSet<K> s(capacity);
    s.emplace(K{});
    EXPECT_EQ(sizeof(size_t) * 4u, sizeof(RawSet<K>));
    EXPECT_EQ((slotCount + 4u) * sizeof(K), s.get_allocator().stats().current);

    MemRecordMap<K, V> m(capacity);
    m.emplace(K{}, V{});
    EXPECT_EQ(sizeof(size_t) * 4u, sizeof(RawMap<K, V>));
    EXPECT_EQ((slotCount + 4u) * sizeof(std::pair<K, V>), m.get_allocator().stats().current);
}

TEST(set, staticMemory)
{
    testStaticMemory<s8, s8>();
    testStaticMemory<s8, s16>();
    testStaticMemory<s8, s32>();
    testStaticMemory<s8, s64>();
    testStaticMemory<s16, s8>();
    testStaticMemory<s16, s16>();
    testStaticMemory<s16, s32>();
    testStaticMemory<s16, s64>();
    testStaticMemory<s32, s8>();
    testStaticMemory<s32, s16>();
    testStaticMemory<s32, s32>();
    testStaticMemory<s32, s64>();
    #ifdef _WIN64
    testStaticMemory<s64, s8>();
    testStaticMemory<s64, s16>();
    testStaticMemory<s64, s32>();
    testStaticMemory<s64, s64>();
    #endif

    testStaticMemory<s8, std::tuple<s8, s8, s8>>();
    testStaticMemory<s8, std::tuple<s8, s8, s8, s8, s8>>();
}

TEST(set, dynamicMemory)
{
    MemRecordSet<int> s(1024u);
    const size_t slotSize{sizeof(int)};

    size_t current{0u}, total{0u}, allocations{0u}, deallocations{0u};
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.rehash(64u);
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    for (int i{0}; i < 32; ++i) s.emplace(i);
    current = (64u + 4u) * slotSize;
    total += current;
    ++allocations;
    EXPECT_EQ(64u, s.slot_count());
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.emplace(64);
    current = (128u + 4u) * slotSize;
    total += current;
    ++allocations;
    ++deallocations;
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.clear();
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.rehash(1024u);
    current = (1024u + 4u) * slotSize;
    total += current;
    ++allocations;
    ++deallocations;
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    for (int i{0}; i < 128; ++i) s.emplace(i);
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.emplace(128);
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.erase(128);
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);

    while (!s.empty()) s.erase(s.begin());
    EXPECT_EQ(current, s.get_allocator().stats().current);
    EXPECT_EQ(total, s.get_allocator().stats().total);
    EXPECT_EQ(allocations, s.get_allocator().stats().allocations);
    EXPECT_EQ(deallocations, s.get_allocator().stats().deallocations);
}

TEST(set, circuity)
{
    RawSet<int> s(16u);

    // With zero key absent

    s.insert(31);
    EXPECT_EQ(31, _RawFriend::getElement(s, 31));
    EXPECT_TRUE(_RawFriend::isVacant(s, 0));
    EXPECT_TRUE(_RawFriend::isVacant(s, 1));

    s.insert(63);
    EXPECT_EQ(31, _RawFriend::getElement(s, 31));
    EXPECT_EQ(63, _RawFriend::getElement(s, 0));
    EXPECT_TRUE(_RawFriend::isVacant(s, 1));

    s.insert(95);
    EXPECT_EQ(31, _RawFriend::getElement(s, 31));
    EXPECT_EQ(63, _RawFriend::getElement(s, 0));
    EXPECT_EQ(95, _RawFriend::getElement(s, 1));

    s.erase(31);
    EXPECT_TRUE(_RawFriend::isGrave(s, 31));
    EXPECT_EQ(63, _RawFriend::getElement(s, 0));
    EXPECT_EQ(95, _RawFriend::getElement(s, 1));

    s.erase(95);
    EXPECT_TRUE(_RawFriend::isGrave(s, 31));
    EXPECT_EQ(63, _RawFriend::getElement(s, 0));
    EXPECT_TRUE(_RawFriend::isGrave(s, 1));

    s.erase(63);
    EXPECT_TRUE(_RawFriend::isGrave(s, 31));
    EXPECT_TRUE(_RawFriend::isGrave(s, 0));
    EXPECT_TRUE(_RawFriend::isGrave(s, 1));
}

TEST(set, terminal)
{
    RawSet<uint> s(16u);
    s.insert(0u);
    s.insert(1u);
    EXPECT_EQ(_RawFriend::vacantKey<int>, _RawFriend::getElement(s, 32u));
    EXPECT_EQ(_RawFriend::graveKey<int>, _RawFriend::getElement(s, 33u));
    EXPECT_EQ(0u, _RawFriend::getElement(s, 34u));
    EXPECT_EQ(0u, _RawFriend::getElement(s, 35u));

    const auto it1{++s.begin()};
    EXPECT_EQ(1u, *it1);

    auto it{it1};
    ++it;
    EXPECT_EQ(s.end(), it);

    s.insert(_RawFriend::graveKey<int>);
    it = it1;
    ++it;
    EXPECT_EQ(_RawFriend::graveKey<int>, *it);
    ++it;
    EXPECT_EQ(s.end(), it);

    s.insert(_RawFriend::vacantKey<int>);
    it = it1;
    ++it;
    EXPECT_EQ(_RawFriend::graveKey<int>, *it);
    ++it;
    EXPECT_EQ(_RawFriend::vacantKey<int>, *it);
    ++it;
    EXPECT_EQ(s.end(), it);

    s.erase(_RawFriend::graveKey<int>);
    it = it1;
    ++it;
    EXPECT_EQ(_RawFriend::vacantKey<int>, *it);
    ++it;
    EXPECT_EQ(s.end(), it);

    s.erase(0u);
    s.erase(1u);
    it = s.begin();
    EXPECT_EQ(_RawFriend::vacantKey<int>, *it);
    ++it;
    EXPECT_EQ(s.end(), it);

    s.insert(_RawFriend::graveKey<int>);
    it = s.begin();
    EXPECT_EQ(_RawFriend::graveKey<int>, *it);
    ++it;
    EXPECT_EQ(_RawFriend::vacantKey<int>, *it);
    ++it;
    EXPECT_EQ(s.end(), it);

    s.erase(_RawFriend::vacantKey<int>);
    it = s.begin();
    EXPECT_EQ(_RawFriend::graveKey<int>, *it);
    ++it;
    EXPECT_EQ(s.end(), it);

    s.erase(_RawFriend::graveKey<int>);
    it = s.begin();
    EXPECT_EQ(s.end(), it);
}

TEST(set, middleZero)
{
    RawSet<uint> s(16u);
    s.insert(10u);
    _RawFriend::getElement(s, 8u) = _RawFriend::vacantKey<uint>;
    _RawFriend::getElement(s, 9u) = _RawFriend::graveKey<uint>;
    _RawFriend::getElement(s, 10u) = 0u;

    auto it{s.begin()};
    EXPECT_EQ(0u, *it);

    ++it;
    EXPECT_EQ(s.end(), it);
}

TEST(set, allBytes)
{
    std::vector<std::byte> keys{};
    keys.reserve(256u);
    for (size_t i{0}; i < 256; ++i) keys.push_back(std::byte(i));

    qc::Random random{};
    for (int iteration{0}; iteration < 256; ++iteration) {
        std::shuffle(keys.begin(), keys.end(), random.engine());

        RawSet<std::byte> s{};

        for (const std::byte key : keys) {
            EXPECT_TRUE(s.insert(key).second);
        }
        EXPECT_EQ(256u, s.size());

        for (uint k{0u}; k < 256u; ++k) {
            EXPECT_TRUE(s.contains(std::byte(k)));
        }

        uint expectedK{0u};
        for (const auto k: s) {
            EXPECT_EQ(std::byte(expectedK), k);
            ++expectedK;
        }

        // Iterator erasure
        expectedK = 0u;
        for (auto it{s.begin()}; it != s.end(); ++it, ++expectedK) {
            EXPECT_EQ(std::byte(expectedK), *it);
            s.erase(it);
        }
        EXPECT_TRUE(s.empty());
    }
}

TEST(set, smartPtrs)
{
    // unique_ptr
    {
        RawSet<std::unique_ptr<int>> s{};
        const auto [it, result]{s.emplace(new int{7})};
        EXPECT_TRUE(result);
        EXPECT_EQ(7, **it);
        EXPECT_TRUE(s.contains(*it));
        EXPECT_FALSE(s.contains(std::make_unique<int>(8)));
        EXPECT_TRUE(s.erase(*it));
    }
    // shared_ptr
    {
        RawSet<std::shared_ptr<int>> s{};
        const auto [it, result]{s.emplace(new int{7})};
        EXPECT_TRUE(result);
        EXPECT_EQ(7, **it);
        EXPECT_TRUE(s.contains(*it));
        EXPECT_FALSE(s.contains(std::make_shared<int>(8)));
        EXPECT_TRUE(s.erase(*it));
    }
}

TEST(map, general)
{
    TrackedMap m{100};

    Tracked2::resetTotals();
    for (int i{0}; i < 25; ++i) {
        const auto [it, inserted]{m.emplace(Tracked2{i}, i + 100)};
        EXPECT_TRUE(inserted);
        EXPECT_EQ(i, int(it->first.val));
        EXPECT_EQ(i + 100, int(it->second.val));
    }
    EXPECT_EQ(25, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(25, Tracked2::totalStats.destructs);
    EXPECT_EQ(50, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{25}; i < 50; ++i) {
        const auto [it, inserted]{m.emplace(std::piecewise_construct, std::forward_as_tuple(i), std::forward_as_tuple(i + 100))};
        EXPECT_TRUE(inserted);
        EXPECT_EQ(i, int(it->first.val));
        EXPECT_EQ(i + 100, int(it->second.val));
    }
    EXPECT_EQ(25, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(25, Tracked2::totalStats.destructs);
    EXPECT_EQ(50, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{50}; i < 75; ++i) {
        const auto [it, inserted]{m.try_emplace(Tracked2{i}, i + 100)};
        EXPECT_TRUE(inserted);
        EXPECT_EQ(i, int(it->first.val));
        EXPECT_EQ(i + 100, int(it->second.val));
    }
    EXPECT_EQ(25, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(25, Tracked2::totalStats.destructs);
    EXPECT_EQ(50, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{50}; i < 75; ++i) {
        const auto [it, inserted]{m.try_emplace(Tracked2{i}, i + 100)};
        EXPECT_FALSE(inserted);
        EXPECT_EQ(i, int(it->first.val));
    }
    EXPECT_EQ(0, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(25, Tracked2::totalStats.destructs);
    EXPECT_EQ(25, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{75}; i < 100; ++i) {
        const auto [it, inserted]{m.insert(std::pair<Tracked2, Tracked2>{Tracked2{i}, Tracked2{i + 100}})};
        EXPECT_TRUE(inserted);
        EXPECT_EQ(i, int(it->first.val));
        EXPECT_EQ(i + 100, int(it->second.val));
    }
    EXPECT_EQ(100, Tracked2::totalStats.moveConstructs);
    EXPECT_EQ(100, Tracked2::totalStats.destructs);
    EXPECT_EQ(200, Tracked2::totalStats.all());

    for (int i{0}; i < 100; ++i) {
        EXPECT_EQ(i + 100, int(m.at(Tracked2{i}).val));
        EXPECT_EQ(i + 100, int(m[Tracked2{i}].val));
    }

    EXPECT_THROW(m.at(Tracked2{100}), std::out_of_range);
    EXPECT_EQ(Tracked2{}, m[Tracked2{100}]);
    m[Tracked2{100}] = Tracked2{200};
    EXPECT_EQ(Tracked2{200}, m[Tracked2{100}]);

    TrackedMap m2{m};
    EXPECT_EQ(m, m2);

    m2[Tracked2{100}].val = 400;
    EXPECT_NE(m, m2);
}

TEST(map, unalignedKey)
{
    struct Double
    {
        u8 a, b;
        bool operator==(const Double &) const = default;
    };

    struct Triple
    {
        u8 a, b, c;
        bool operator==(const Triple &) const = default;
    };

    RawMap<Triple, Double> map{};

    for (int key{0}; key < 100; ++key) {
        EXPECT_TRUE(map.emplace(Triple{u8(key), u8(50 + key), u8(100 + key)}, Double{u8(25 + key), u8(75 + key)}).second);
    }

    EXPECT_EQ(100u, map.size());

    for (int key{0}; key < 100; ++key) {
        EXPECT_EQ((Double{u8(25 + key), u8(75 + key)}), map.at(Triple{u8(key), u8(50 + key), u8(100 + key)}));
    }
}

TEST(map, largeKey)
{
    struct Big
    {
        size_t a, b, c;
        bool operator==(const Big &) const = default;
    };
    RawMap<Big, Big> map{};

    for (size_t key{0u}; key < 100u; ++key) {
        EXPECT_TRUE(map.emplace(Big{key, 50u + key, 100u + key}, Big{25u + key, 75u + key, 125u + key}).second);
    }

    EXPECT_EQ(100u, map.size());

    for (size_t key{0u}; key < 100u; ++key) {
        EXPECT_EQ((Big{25u + key, 75u + key, 125u + key}), map.at(Big{key, 50u + key, 100u + key}));
    }
}

template <typename K, typename K_>
concept HeterogeneityCompiles = requires (RawSet<K> set, RawMap<K, int> map, const K_ & k, const qc::hash::IdentityHash<K> identityHash, const qc::hash::FastHash<K> fastHash) {
    set.erase(k);
    set.contains(k);
    set.count(k);

    map.erase(k);
    map.contains(k);
    map.count(k);
    map.at(k);

    identityHash(k);
    fastHash(k);
};

TEST(heterogeneity, general)
{
    static_assert(HeterogeneityCompiles<u8, u8>);
    static_assert(!HeterogeneityCompiles<u8, u16>);
    static_assert(!HeterogeneityCompiles<u8, u32>);
    static_assert(!HeterogeneityCompiles<u8, u64>);
    static_assert(!HeterogeneityCompiles<u8, s8>);
    static_assert(!HeterogeneityCompiles<u8, s16>);
    static_assert(!HeterogeneityCompiles<u8, s32>);
    static_assert(!HeterogeneityCompiles<u8, s64>);
    static_assert(!HeterogeneityCompiles<u8, bool>);

    static_assert(HeterogeneityCompiles<u16, u8>);
    static_assert(HeterogeneityCompiles<u16, u16>);
    static_assert(!HeterogeneityCompiles<u16, u32>);
    static_assert(!HeterogeneityCompiles<u16, u64>);
    static_assert(!HeterogeneityCompiles<u16, s8>);
    static_assert(!HeterogeneityCompiles<u16, s16>);
    static_assert(!HeterogeneityCompiles<u16, s32>);
    static_assert(!HeterogeneityCompiles<u16, s64>);
    static_assert(!HeterogeneityCompiles<u16, bool>);

    static_assert(HeterogeneityCompiles<u32, u8>);
    static_assert(HeterogeneityCompiles<u32, u16>);
    static_assert(HeterogeneityCompiles<u32, u32>);
    static_assert(!HeterogeneityCompiles<u32, u64>);
    static_assert(!HeterogeneityCompiles<u32, s8>);
    static_assert(!HeterogeneityCompiles<u32, s16>);
    static_assert(!HeterogeneityCompiles<u32, s32>);
    static_assert(!HeterogeneityCompiles<u32, s64>);
    static_assert(!HeterogeneityCompiles<u32, bool>);

    #ifdef _WIN64
    static_assert(HeterogeneityCompiles<u64, u8>);
    static_assert(HeterogeneityCompiles<u64, u16>);
    static_assert(HeterogeneityCompiles<u64, u32>);
    static_assert(HeterogeneityCompiles<u64, u64>);
    static_assert(!HeterogeneityCompiles<u64, s8>);
    static_assert(!HeterogeneityCompiles<u64, s16>);
    static_assert(!HeterogeneityCompiles<u64, s32>);
    static_assert(!HeterogeneityCompiles<u64, s64>);
    static_assert(!HeterogeneityCompiles<u64, bool>);
    #endif

    static_assert(HeterogeneityCompiles<s8, s8>);
    static_assert(!HeterogeneityCompiles<s8, s16>);
    static_assert(!HeterogeneityCompiles<s8, s32>);
    static_assert(!HeterogeneityCompiles<s8, s64>);
    static_assert(!HeterogeneityCompiles<s8, u8>);
    static_assert(!HeterogeneityCompiles<s8, u16>);
    static_assert(!HeterogeneityCompiles<s8, u32>);
    static_assert(!HeterogeneityCompiles<s8, u64>);
    static_assert(!HeterogeneityCompiles<s8, bool>);

    static_assert(HeterogeneityCompiles<s16, s8>);
    static_assert(HeterogeneityCompiles<s16, s16>);
    static_assert(!HeterogeneityCompiles<s16, s32>);
    static_assert(!HeterogeneityCompiles<s16, s64>);
    static_assert(HeterogeneityCompiles<s16, u8>);
    static_assert(!HeterogeneityCompiles<s16, u16>);
    static_assert(!HeterogeneityCompiles<s16, u32>);
    static_assert(!HeterogeneityCompiles<s16, u64>);
    static_assert(!HeterogeneityCompiles<s16, bool>);

    static_assert(HeterogeneityCompiles<s32, s8>);
    static_assert(HeterogeneityCompiles<s32, s16>);
    static_assert(HeterogeneityCompiles<s32, s32>);
    static_assert(!HeterogeneityCompiles<s32, s64>);
    static_assert(HeterogeneityCompiles<s32, u8>);
    static_assert(HeterogeneityCompiles<s32, u16>);
    static_assert(!HeterogeneityCompiles<s32, u32>);
    static_assert(!HeterogeneityCompiles<s32, u64>);
    static_assert(!HeterogeneityCompiles<s32, bool>);

    #ifdef _WIN64
    static_assert(HeterogeneityCompiles<s64, s8>);
    static_assert(HeterogeneityCompiles<s64, s16>);
    static_assert(HeterogeneityCompiles<s64, s32>);
    static_assert(HeterogeneityCompiles<s64, s64>);
    static_assert(HeterogeneityCompiles<s64, u8>);
    static_assert(HeterogeneityCompiles<s64, u16>);
    static_assert(HeterogeneityCompiles<s64, u32>);
    static_assert(!HeterogeneityCompiles<s64, u64>);
    static_assert(!HeterogeneityCompiles<s64, bool>);
    #endif

    static_assert(HeterogeneityCompiles<int *, int *>);
    static_assert(HeterogeneityCompiles<int *, const int *>);
    static_assert(HeterogeneityCompiles<const int *, int *>);
    static_assert(HeterogeneityCompiles<const int *, const int *>);

    static_assert(HeterogeneityCompiles<std::unique_ptr<int>, int *>);
    static_assert(HeterogeneityCompiles<std::unique_ptr<int>, const int *>);
    static_assert(HeterogeneityCompiles<std::unique_ptr<const int>, int *>);
    static_assert(HeterogeneityCompiles<std::unique_ptr<const int>, const int *>);

    static_assert(!HeterogeneityCompiles<int *, std::unique_ptr<int>>);
    static_assert(!HeterogeneityCompiles<int *, std::unique_ptr<const int>>);
    static_assert(!HeterogeneityCompiles<const int *, std::unique_ptr<int>>);
    static_assert(!HeterogeneityCompiles<const int *, std::unique_ptr<const int>>);

    static_assert(HeterogeneityCompiles<std::shared_ptr<int>, int *>);
    static_assert(HeterogeneityCompiles<std::shared_ptr<int>, const int *>);
    static_assert(HeterogeneityCompiles<std::shared_ptr<const int>, int *>);
    static_assert(HeterogeneityCompiles<std::shared_ptr<const int>, const int *>);

    static_assert(!HeterogeneityCompiles<int *, std::shared_ptr<int>>);
    static_assert(!HeterogeneityCompiles<int *, std::shared_ptr<const int>>);
    static_assert(!HeterogeneityCompiles<const int *, std::shared_ptr<int>>);
    static_assert(!HeterogeneityCompiles<const int *, std::shared_ptr<const int>>);

    static_assert(!HeterogeneityCompiles<std::unique_ptr<int>, std::shared_ptr<int>>);
    static_assert(!HeterogeneityCompiles<std::shared_ptr<int>, std::unique_ptr<int>>);

    struct Base {};
    struct Derived : Base {};

    static_assert(HeterogeneityCompiles<Base *, Derived *>);
    static_assert(HeterogeneityCompiles<Base *, const Derived *>);
    static_assert(HeterogeneityCompiles<const Base *, Derived *>);
    static_assert(HeterogeneityCompiles<const Base *, const Derived *>);

    static_assert(HeterogeneityCompiles<std::unique_ptr<Base>, Derived *>);
    static_assert(HeterogeneityCompiles<std::unique_ptr<Base>, const Derived *>);
    static_assert(HeterogeneityCompiles<std::unique_ptr<const Base>, Derived *>);
    static_assert(HeterogeneityCompiles<std::unique_ptr<const Base>, const Derived *>);

    static_assert(HeterogeneityCompiles<std::shared_ptr<Base>, Derived *>);
    static_assert(HeterogeneityCompiles<std::shared_ptr<Base>, const Derived *>);
    static_assert(HeterogeneityCompiles<std::shared_ptr<const Base>, Derived *>);
    static_assert(HeterogeneityCompiles<std::shared_ptr<const Base>, const Derived *>);

    static_assert(!HeterogeneityCompiles<Derived *, Base *>);
    static_assert(!HeterogeneityCompiles<Derived *, const Base *>);
    static_assert(!HeterogeneityCompiles<const Derived *, Base *>);
    static_assert(!HeterogeneityCompiles<const Derived *, const Base *>);

    static_assert(!HeterogeneityCompiles<std::unique_ptr<Derived>, Base *>);
    static_assert(!HeterogeneityCompiles<std::unique_ptr<Derived>, const Base *>);
    static_assert(!HeterogeneityCompiles<std::unique_ptr<const Derived>, Base *>);
    static_assert(!HeterogeneityCompiles<std::unique_ptr<const Derived>, const Base *>);

    static_assert(!HeterogeneityCompiles<std::shared_ptr<Derived>, Base *>);
    static_assert(!HeterogeneityCompiles<std::shared_ptr<Derived>, const Base *>);
    static_assert(!HeterogeneityCompiles<std::shared_ptr<const Derived>, Base *>);
    static_assert(!HeterogeneityCompiles<std::shared_ptr<const Derived>, const Base *>);
}

struct alignas(size_t) CustomType
{
    qc::sized<sizeof(size_t) / 2>::utype x, y;
};

struct OtherCustomType
{
    size_t x;
};

template <> struct qc::hash::IsCompatible<CustomType, OtherCustomType> : std::true_type {};

template <>
struct qc::hash::IdentityHash<CustomType> {
    size_t operator()(const CustomType & v) const {
        return reinterpret_cast<const size_t &>(v);
    }
    size_t operator()(const OtherCustomType & v) const {
        return reinterpret_cast<const size_t &>(v);
    }
};

template <>
struct qc::hash::FastHash<CustomType> {
    size_t operator()(const CustomType & v) const {
        return qc::hash::fastHash(v);
    }
    size_t operator()(const OtherCustomType & v) const {
        return qc::hash::fastHash(v);
    }
};

TEST(heterogeneity, custom)
{
    static_assert(HeterogeneityCompiles<CustomType, OtherCustomType>);
    static_assert(HeterogeneityCompiles<CustomType, const OtherCustomType>);
    static_assert(!HeterogeneityCompiles<CustomType, size_t>);
}

TEST(rawable, general)
{
    static_assert(qc::hash::Rawable<bool>);

    static_assert(qc::hash::Rawable<char>);
    static_assert(qc::hash::Rawable<u8>);
    static_assert(qc::hash::Rawable<s8>);
    static_assert(qc::hash::Rawable<u16>);
    static_assert(qc::hash::Rawable<s16>);
    static_assert(qc::hash::Rawable<u32>);
    static_assert(qc::hash::Rawable<s32>);
    static_assert(qc::hash::Rawable<u64>);
    static_assert(qc::hash::Rawable<s64>);

    static_assert(!qc::hash::Rawable<float>);
    static_assert(!qc::hash::Rawable<double>);
    static_assert(!qc::hash::Rawable<long double>);

    static_assert(qc::hash::Rawable<std::shared_ptr<float>>);
    static_assert(qc::hash::Rawable<std::unique_ptr<float>>);
    static_assert(!qc::hash::Rawable<std::weak_ptr<float>>);

    struct Custom8_2 { u8 v1, v2; };
    struct Custom8_3 { u8 v1, v2, v3; };
    struct Custom8_4 { u8 v1, v2, v3, v4; };
    struct Custom8_5 { u8 v1, v2, v3, v4, v5; };
    struct Custom8_6 { u8 v1, v2, v3, v4, v5, v6; };
    struct Custom8_7 { u8 v1, v2, v3, v4, v5, v6, v7; };
    struct Custom8_8 { u8 v1, v2, v3, v4, v5, v6, v7, v8; };
    struct Custom8_9 { u8 v1, v2, v3, v4, v5, v6, v7, v8, v9; };
    static_assert(qc::hash::Rawable<Custom8_2>);
    static_assert(qc::hash::Rawable<Custom8_3>);
    static_assert(qc::hash::Rawable<Custom8_4>);
    static_assert(qc::hash::Rawable<Custom8_5>);
    static_assert(qc::hash::Rawable<Custom8_6>);
    static_assert(qc::hash::Rawable<Custom8_7>);
    static_assert(qc::hash::Rawable<Custom8_8>);
    static_assert(qc::hash::Rawable<Custom8_9>);

    struct Custom16_2 { u16 v1, v2; };
    struct Custom16_3 { u16 v1, v2, v3; };
    struct Custom16_4 { u16 v1, v2, v3, v4; };
    struct Custom16_5 { u16 v1, v2, v3, v4, v5; };
    static_assert(qc::hash::Rawable<Custom16_2>);
    static_assert(qc::hash::Rawable<Custom16_3>);
    static_assert(qc::hash::Rawable<Custom16_4>);
    static_assert(qc::hash::Rawable<Custom16_5>);

    struct Custom32_2 { u32 v1, v2; };
    struct Custom32_3 { u32 v1, v2, v3; };
    static_assert(qc::hash::Rawable<Custom32_2>);
    static_assert(qc::hash::Rawable<Custom32_3>);

    struct Custom64_2 { u64 v1, v2; };
    static_assert(qc::hash::Rawable<Custom64_2>);

    static_assert(qc::hash::Rawable<std::pair<int, int>>);
    static_assert(qc::hash::Rawable<std::pair<Custom32_3, double *>>);
    static_assert(!qc::hash::Rawable<std::pair<int, float>>);
    static_assert(!qc::hash::Rawable<std::pair<float, int>>);

    static_assert(!qc::hash::Rawable<std::string>);
    static_assert(!qc::hash::Rawable<std::string_view>);
}

TEST(rawType, general)
{
    struct alignas(1) Aligned1 { u8 vals[1]; };
    struct alignas(2) Aligned2 { u8 vals[2]; };
    struct alignas(4) Aligned4 { u8 vals[4]; };
    struct alignas(8) Aligned8 { u8 vals[8]; };
    struct alignas(16) Aligned16 { u8 vals[16]; };

    struct alignas(1) Unaligned2 { u8 vals[2]; };
    struct alignas(2) Unaligned4 { u8 vals[4]; };
    struct alignas(4) Unaligned8 { u8 vals[8]; };
    struct alignas(8) Unaligned16 { u8 vals[16]; };

    static_assert(std::is_same_v<qc::hash::RawType<Aligned1>, u8>);
    static_assert(std::is_same_v<qc::hash::RawType<Aligned2>, u16>);
    static_assert(std::is_same_v<qc::hash::RawType<Aligned4>, u32>);
    static_assert(std::is_same_v<qc::hash::RawType<Aligned8>, u64>);
    static_assert(std::is_same_v<qc::hash::RawType<Aligned16>, qc::hash::UnsignedMulti<8, 2>>);

    static_assert(std::is_same_v<qc::hash::RawType<Unaligned2>, qc::hash::UnsignedMulti<1, 2>>);
    static_assert(std::is_same_v<qc::hash::RawType<Unaligned4>, qc::hash::UnsignedMulti<2, 2>>);
    static_assert(std::is_same_v<qc::hash::RawType<Unaligned8>, qc::hash::UnsignedMulti<4, 2>>);
    static_assert(std::is_same_v<qc::hash::RawType<Unaligned16>, qc::hash::UnsignedMulti<8, 2>>);
}

static void randomGeneralTest(const size_t size, const size_t iterations, qc::Random & random)
{
    static volatile size_t volatileKey{};

    std::vector<size_t> keys{};
    keys.reserve(size);

    for (size_t it{0}; it < iterations; ++it) {
        keys.clear();
        for (size_t i{}; i < size; ++i) {
            keys.push_back(random.next<size_t>());
        }

        if (random.next<bool>()) keys[random.next<size_t>(size)] = _RawFriend::vacantKey<size_t>;
        if (random.next<bool>()) keys[random.next<size_t>(size)] = _RawFriend::graveKey<size_t>;

        RawSet<size_t> s{};

        for (const size_t & key : keys) {
            EXPECT_TRUE(s.insert(key).second);
        }

        EXPECT_EQ(keys.size(), s.size());

        for (const size_t & key : keys) {
            EXPECT_TRUE(s.contains(key));
        }

        for (const size_t & key: s) {
            volatileKey = key;
        }

        std::shuffle(keys.begin(), keys.end(), random.engine());

        for (size_t i{0}; i < keys.size() / 2; ++i) {
            EXPECT_TRUE(s.erase(keys[i]));
        }

        EXPECT_EQ(keys.size() / 2, s.size());

        for (size_t i{0}; i < keys.size() / 2; ++i) {
            EXPECT_FALSE(s.erase(keys[i]));
        }

        for (size_t i{keys.size() / 2}; i < keys.size(); ++i) {
            EXPECT_TRUE(s.erase(keys[i]));
        }

        EXPECT_EQ(0u, s.size());

        for (const size_t & key : keys) {
            EXPECT_TRUE(s.insert(key).second);
        }

        EXPECT_EQ(keys.size(), s.size());

        s.clear();

        EXPECT_EQ(0u, s.size());
    }
}

TEST(set, randomGeneralTests)
{
    qc::Random random{size_t(std::chrono::steady_clock::now().time_since_epoch().count())};
    for (size_t size{10}, iterations{10000}; size <= 10000; size *= 10, iterations /= 10) {
        randomGeneralTest(size, iterations, random);
    }
}
