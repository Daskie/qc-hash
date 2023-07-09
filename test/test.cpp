// First include in order to test its own includes
#include <qc-hash.hpp>

#include <cmath>

#include <array>
#include <chrono>
#include <map>
#include <unordered_map>
#include <vector>

#include <qc-core/core.hpp>
#include <qc-core/record-allocator.hpp>
#include <qc-core/random.hpp>

#include <gtest/gtest.h>

using namespace std::string_literals;
using namespace qc::primitives;

using qc::hash::RawMap;
using qc::hash::RawSet;
using qc::hash::RawFriend;

template <typename K>
struct NullHash
{
    u64 operator()(const K &) const
    {
        return 0u;
    }
};

struct qc::hash::RawFriend
{
    template <typename K> using RawKey = typename RawSet<K>::_RawKey;

    template <typename K> static constexpr auto vacantKey{RawSet<K>::_vacantKey};
    template <typename K> static constexpr auto graveKey{RawSet<K>::_graveKey};

    template <typename K, typename H, typename A>
    static const K & getElement(const RawSet<K, H, A> & set, const u64 slotI)
    {
        return set._elements[slotI];
    }

    template <typename K, typename H, typename A>
    static K & getElement(RawSet<K, H, A> & set, const u64 slotI)
    {
        return set._elements[slotI];
    }

    template <typename K, typename H, typename A>
    static bool isPresent(const RawSet<K, H, A> & set, const u64 slotI)
    {
        return set._isPresent(set._raw(set._elements[slotI]));
    }

    template <typename K, typename H, typename A>
    static bool isVacant(const RawSet<K, H, A> & set, const u64 slotI)
    {
        return RawType<K>(getElement(set, slotI)) == vacantKey<K>;
    }

    template <typename K, typename H, typename A>
    static bool isGrave(const RawSet<K, H, A> & set, const u64 slotI)
    {
        return RawType<K>(getElement(set, slotI)) == graveKey<K>;
    }

    template <typename K, typename H, typename A, typename It>
    static u64 slotI(const RawSet<K, H, A> & set, const It it)
    {
        return u64(it._element - set._elements);
    }

    template <typename K, typename H, typename A, typename It>
    static u64 dist(const RawSet<K, H, A> & set, const It it)
    {
        const u64 slotI{RawFriend::slotI(set, it)};
        const u64 idealSlotI{set.slot(*it)};
        return slotI >= idealSlotI ? slotI - idealSlotI : set.slot_n() - idealSlotI + slotI;
    }
};

struct TrackedStats2
{
    int defConstructs{0};
    int copyConstructs{0};
    int moveConstructs{0};
    int copyAssigns{0};
    int moveAssigns{0};
    int destructs{0};

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

    explicit Tracked2(const int val_) :
        val{val_}
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

    Tracked2(Tracked2 && other) :
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

    Tracked2 & operator=(Tracked2 && other)
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
    u64 operator()(const Tracked2 & tracked) const
    {
        return u64(tracked.val);
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
        struct Custom { u64 a; };

        const qc::hash::IdentityHash<Custom> h{};

        ASSERT_EQ(0x76543210u, h(Custom{0x76543210u}));
    }
    { // Oversized
        struct Custom { u64 a, b; };

        const qc::hash::IdentityHash<Custom> h{};

        ASSERT_EQ(0x76543210u, h(Custom{0x76543210u, 0xFEDCBA98}));
    }
    { // Unaligned small
        struct Custom { u8 a, b, c; };

        const qc::hash::IdentityHash<Custom> h{};

        ASSERT_EQ(0x543210u, h(Custom{u8(0x10u), u8(0x32u), u8(0x54u)}));
    }
    { // Unaligned large
        struct Custom { u16 a, b, c, d, e; };

        const qc::hash::IdentityHash<Custom> h{};

        ASSERT_EQ(0x7766554433221100u, h(Custom{u16(0x1100u), u16(0x3322u), u16(0x5544u), u16(0x7766u), u16(0x9988u)}));
    }
}

TEST(identityHash, toSizeTSameAsMemcpy)
{
    std::array<u8, 8u> arr{0x10u, 0x32u, 0x54u, 0x76u, 0x98u, 0xBAu, 0xDCu, 0xFEu};
    u64 val;
    memcpy(&val, &arr, sizeof(u64));
    ASSERT_EQ(val, qc::hash::_private::qc_hash::getLowBytes<u64>(arr));
}

template <typename T>
static void testIntegerHash()
{
    const qc::hash::IdentityHash<T> h{};

    ASSERT_EQ(0u, h(T(0)));
    ASSERT_EQ(123u, h(T(123)));
    ASSERT_EQ(static_cast<qc::Sized<sizeof(T)>::U>(std::numeric_limits<T>::min()), h(std::numeric_limits<T>::min()));
    ASSERT_EQ(static_cast<qc::Sized<sizeof(T)>::U>(std::numeric_limits<T>::max()), h(std::numeric_limits<T>::max()));
}

TEST(identityHash, integers)
{
    testIntegerHash<u8>();
    testIntegerHash<s8>();
    testIntegerHash<u16>();
    testIntegerHash<s16>();
    testIntegerHash<u32>();
    testIntegerHash<s32>();
    testIntegerHash<u64>();
    testIntegerHash<s64>();
}

template <typename T>
static void testEnumHash()
{
    const qc::hash::IdentityHash<T> h{};

    ASSERT_EQ(0u, h(T(0)));
    ASSERT_EQ(123u, h(T(123)));
    ASSERT_EQ(static_cast<qc::Sized<sizeof(T)>::U>(std::numeric_limits<std::underlying_type_t<T>>::min()), h(T(std::numeric_limits<std::underlying_type_t<T>>::min())));
    ASSERT_EQ(static_cast<qc::Sized<sizeof(T)>::U>(std::numeric_limits<std::underlying_type_t<T>>::max()), h(T(std::numeric_limits<std::underlying_type_t<T>>::max())));
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
    testEnumHash<EnumU64>();
    testEnumHash<EnumS64>();
}

template <typename T>
static void testPointerHash()
{
    const qc::hash::IdentityHash<T *> h{};

    T * const p0{};
    T * const p1{p0 + 1};
    T * const p2{p0 - 1};
    T * const p3{p0 + 123};

    ASSERT_EQ(0u, h(p0));
    ASSERT_EQ(1u, h(p1));
    ASSERT_EQ(u64(intptr_t{-1}) >> (std::bit_width(alignof(T)) - 1u), h(p2));
    ASSERT_EQ(123u, h(p3));
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
    qc::hash::FastHash<u64> fastHash{};
    ASSERT_EQ(7016536041891711906u, fastHash(0x12345678u));
    ASSERT_EQ(1291257483u, qc::hash::fastHash::hash<u32>(0x12345678u));

    RawSet<int, qc::hash::FastHash<int>> s{};
    for (int i{0}; i < 100; ++i)
    {
        ASSERT_TRUE(s.insert(i).second);
    }
    ASSERT_EQ(100u, s.size());
    for (int i{0}; i < 100; ++i)
    {
        ASSERT_TRUE(s.contains(i));
    }
    for (int i{0}; i < 100; ++i)
    {
        ASSERT_TRUE(s.erase(i));
    }
    ASSERT_TRUE(s.empty());
}

TEST(fastHash, sharedPtr)
{
    std::shared_ptr<int> val{std::make_shared<int>()};
    ASSERT_EQ(qc::hash::FastHash<int *>{}(val.get()), qc::hash::FastHash<std::shared_ptr<int>>{}(val));
}

TEST(fastHash, string)
{
    char cStr[]{"abcdefghijklmnopqrstuvwxyz"};
    const char * constCStr{cStr};
    std::string stdStr{cStr};
    std::string_view strView{cStr};

    const u64 hash64{8454416734917819949u};
    const u32 hash32{3459995157u};

    ASSERT_EQ(hash64, qc::hash::FastHash<std::string>{}(cStr));
    ASSERT_EQ(hash64, qc::hash::FastHash<std::string>{}(constCStr));
    ASSERT_EQ(hash64, qc::hash::FastHash<std::string>{}(stdStr));
    ASSERT_EQ(hash64, qc::hash::FastHash<std::string>{}(strView));

    ASSERT_EQ(hash64, qc::hash::FastHash<std::string_view>{}(cStr));
    ASSERT_EQ(hash64, qc::hash::FastHash<std::string_view>{}(constCStr));
    ASSERT_EQ(hash64, qc::hash::FastHash<std::string_view>{}(stdStr));
    ASSERT_EQ(hash64, qc::hash::FastHash<std::string_view>{}(strView));

    ASSERT_EQ(hash32, qc::hash::fastHash::hash<u32>(strView.data(), strView.length()));
}

template <typename T, typename T_> concept FastHashHeterogeneous = requires (qc::hash::FastHash<T> h, T_ k) { { h(k) } -> std::same_as<u64>; };

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

TEST(fastHash, zeroSequences)
{
    constexpr u64 hash64_01{14313749767032793493u};
    constexpr u64 hash64_02{10180755460356035370u};
    constexpr u64 hash64_03{6047761153679277247u};
    constexpr u64 hash64_04{1914766847002519124u};
    constexpr u64 hash64_05{16228516614035312617u};
    constexpr u64 hash64_06{12095522307358554494u};
    constexpr u64 hash64_07{7962528000681796371u};
    constexpr u64 hash64_08{3829533694005038248u};
    constexpr u64 hash64_09{16354269775938918017u};
    constexpr u64 hash64_10{1774305018856974138u};
    constexpr u64 hash64_11{5641084335484581875u};
    constexpr u64 hash64_12{9507863652112189612u};
    constexpr u64 hash64_13{13374642968739797349u};
    constexpr u64 hash64_14{17241422285367405086u};
    constexpr u64 hash64_15{2661457528285461207u};
    constexpr u64 hash64_16{6528236844913068944u};

    constexpr u32 hash32_01{1540483477u};
    constexpr u32 hash32_02{3080966954u};
    constexpr u32 hash32_03{326483135u};
    constexpr u32 hash32_04{1866966612u};
    constexpr u32 hash32_05{3390362525u};
    constexpr u32 hash32_06{4068435030u};
    constexpr u32 hash32_07{451540239u};
    constexpr u32 hash32_08{1129612744u};
    constexpr u32 hash32_09{3691282965u};
    constexpr u32 hash32_10{1238113986u};
    constexpr u32 hash32_11{3079912303u};
    constexpr u32 hash32_12{626743324u};
    constexpr u32 hash32_13{3208407549u};
    constexpr u32 hash32_14{3124826030u};
    constexpr u32 hash32_15{3041244511u};
    constexpr u32 hash32_16{2957662992u};

    ASSERT_EQ(hash64_01, qc::hash::fastHash::hash<u64>(u8{0u}));
    ASSERT_EQ(hash64_02, qc::hash::fastHash::hash<u64>(u16{0u}));
    ASSERT_EQ(hash64_04, qc::hash::fastHash::hash<u64>(u32{0u}));
    ASSERT_EQ(hash64_08, qc::hash::fastHash::hash<u64>(u64{0u}));

    ASSERT_EQ(hash32_01, qc::hash::fastHash::hash<u32>(u8{0u}));
    ASSERT_EQ(hash32_02, qc::hash::fastHash::hash<u32>(u16{0u}));
    ASSERT_EQ(hash32_04, qc::hash::fastHash::hash<u32>(u32{0u}));
    ASSERT_EQ(hash32_08, qc::hash::fastHash::hash<u32>(u64{0u}));

    const std::array<u8, 16u> zeroArr{};

    ASSERT_EQ(hash64_01, qc::hash::fastHash::hash<u64>(zeroArr.data(),  1u));
    ASSERT_EQ(hash64_02, qc::hash::fastHash::hash<u64>(zeroArr.data(),  2u));
    ASSERT_EQ(hash64_03, qc::hash::fastHash::hash<u64>(zeroArr.data(),  3u));
    ASSERT_EQ(hash64_04, qc::hash::fastHash::hash<u64>(zeroArr.data(),  4u));
    ASSERT_EQ(hash64_05, qc::hash::fastHash::hash<u64>(zeroArr.data(),  5u));
    ASSERT_EQ(hash64_06, qc::hash::fastHash::hash<u64>(zeroArr.data(),  6u));
    ASSERT_EQ(hash64_07, qc::hash::fastHash::hash<u64>(zeroArr.data(),  7u));
    ASSERT_EQ(hash64_08, qc::hash::fastHash::hash<u64>(zeroArr.data(),  8u));
    ASSERT_EQ(hash64_09, qc::hash::fastHash::hash<u64>(zeroArr.data(),  9u));
    ASSERT_EQ(hash64_10, qc::hash::fastHash::hash<u64>(zeroArr.data(), 10u));
    ASSERT_EQ(hash64_11, qc::hash::fastHash::hash<u64>(zeroArr.data(), 11u));
    ASSERT_EQ(hash64_12, qc::hash::fastHash::hash<u64>(zeroArr.data(), 12u));
    ASSERT_EQ(hash64_13, qc::hash::fastHash::hash<u64>(zeroArr.data(), 13u));
    ASSERT_EQ(hash64_14, qc::hash::fastHash::hash<u64>(zeroArr.data(), 14u));
    ASSERT_EQ(hash64_15, qc::hash::fastHash::hash<u64>(zeroArr.data(), 15u));
    ASSERT_EQ(hash64_16, qc::hash::fastHash::hash<u64>(zeroArr.data(), 16u));

    ASSERT_EQ(hash32_01, qc::hash::fastHash::hash<u32>(zeroArr.data(),  1u));
    ASSERT_EQ(hash32_02, qc::hash::fastHash::hash<u32>(zeroArr.data(),  2u));
    ASSERT_EQ(hash32_03, qc::hash::fastHash::hash<u32>(zeroArr.data(),  3u));
    ASSERT_EQ(hash32_04, qc::hash::fastHash::hash<u32>(zeroArr.data(),  4u));
    ASSERT_EQ(hash32_05, qc::hash::fastHash::hash<u32>(zeroArr.data(),  5u));
    ASSERT_EQ(hash32_06, qc::hash::fastHash::hash<u32>(zeroArr.data(),  6u));
    ASSERT_EQ(hash32_07, qc::hash::fastHash::hash<u32>(zeroArr.data(),  7u));
    ASSERT_EQ(hash32_08, qc::hash::fastHash::hash<u32>(zeroArr.data(),  8u));
    ASSERT_EQ(hash32_09, qc::hash::fastHash::hash<u32>(zeroArr.data(),  9u));
    ASSERT_EQ(hash32_10, qc::hash::fastHash::hash<u32>(zeroArr.data(), 10u));
    ASSERT_EQ(hash32_11, qc::hash::fastHash::hash<u32>(zeroArr.data(), 11u));
    ASSERT_EQ(hash32_12, qc::hash::fastHash::hash<u32>(zeroArr.data(), 12u));
    ASSERT_EQ(hash32_13, qc::hash::fastHash::hash<u32>(zeroArr.data(), 13u));
    ASSERT_EQ(hash32_14, qc::hash::fastHash::hash<u32>(zeroArr.data(), 14u));
    ASSERT_EQ(hash32_15, qc::hash::fastHash::hash<u32>(zeroArr.data(), 15u));
    ASSERT_EQ(hash32_16, qc::hash::fastHash::hash<u32>(zeroArr.data(), 16u));
}

TEST(set, constructor_default)
{
    MemRecordSet<int> s{};
    ASSERT_EQ(qc::hash::config::minMapCapacity, s.capacity());
    ASSERT_EQ(0u, s.size());
    ASSERT_EQ(0u, s.get_allocator().stats().allocations);
}

TEST(set, constructor_capacity)
{
    qc::memory::RecordAllocator<int> allocator{};
    ASSERT_EQ(  16u, (MemRecordSet<int>{   0u, allocator}.capacity()));
    ASSERT_EQ(  16u, (MemRecordSet<int>{   1u, allocator}.capacity()));
    ASSERT_EQ(  16u, (MemRecordSet<int>{  16u, allocator}.capacity()));
    ASSERT_EQ(  32u, (MemRecordSet<int>{  17u, allocator}.capacity()));
    ASSERT_EQ(  32u, (MemRecordSet<int>{  32u, allocator}.capacity()));
    ASSERT_EQ(  64u, (MemRecordSet<int>{  33u, allocator}.capacity()));
    ASSERT_EQ(  64u, (MemRecordSet<int>{  64u, allocator}.capacity()));
    ASSERT_EQ( 128u, (MemRecordSet<int>{  65u, allocator}.capacity()));
    ASSERT_EQ(1024u, (MemRecordSet<int>{1000u, allocator}.capacity()));
    ASSERT_EQ(0u, allocator.stats().allocations);
}

TEST(set, constructor_range)
{
    std::vector<int> values{
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
        20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
        30, 31, 32, 33, 34, 35, 36, 37, 38, 39};
    MemRecordSet<int> s{values.cbegin(), values.cend()};
    ASSERT_EQ(40u, s.size());
    ASSERT_EQ(64u, s.capacity());
    for (const int v : values)
    {
        ASSERT_TRUE(s.contains(v));
    }
    ASSERT_EQ(1u, s.get_allocator().stats().allocations);
}

TEST(set, constructor_initializerList)
{
    MemRecordSet<int> s{{
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
        10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
        20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
        30, 31, 32, 33, 34, 35, 36, 37, 38, 39}};
    ASSERT_EQ(40u, s.size());
    ASSERT_EQ(64u, s.capacity());
    for (int i{0}; i < 40; ++i)
    {
        ASSERT_TRUE(s.contains(i));
    }
    ASSERT_EQ(1u, s.get_allocator().stats().allocations);
}

TEST(set, constructor_copy)
{
    // Trivial type
    {
        MemRecordSet<int> s1{};
        for (int i{0}; i < 100; ++i)
        {
            s1.insert(i);
        }
        const u64 prevAllocN{s1.get_allocator().stats().allocations};
        MemRecordSet<int> s2{s1};
        ASSERT_EQ(100u, s2.size());
        ASSERT_EQ(128u, s2.capacity());
        ASSERT_EQ(s1, s2);
        ASSERT_EQ(prevAllocN + 1u, s2.get_allocator().stats().allocations);
    }

    // Non-trivial type
    {
        Tracked2MemRecordSet s1{};
        for (int i{0}; i < 100; ++i)
        {
            s1.emplace(i);
        }
        const u64 prevAllocN{s1.get_allocator().stats().allocations};
        Tracked2MemRecordSet s2{s1};
        ASSERT_EQ(100u, s2.size());
        ASSERT_EQ(128u, s2.capacity());
        ASSERT_EQ(s1, s2);
        ASSERT_EQ(prevAllocN + 1u, s2.get_allocator().stats().allocations);
    }
}

TEST(set, constructor_move)
{
    // Trivial type
    {
        MemRecordSet<int> s1{};
        for (int i{0}; i < 100; ++i)
        {
            s1.insert(i);
        }
        MemRecordSet<int> ref{s1};
        const u64 prevAllocN{s1.get_allocator().stats().allocations};
        MemRecordSet<int> s2{std::move(s1)};
        ASSERT_EQ(100u, s2.size());
        ASSERT_EQ(128u, s2.capacity());
        ASSERT_EQ(ref, s2);
        ASSERT_TRUE(s1.empty());
        ASSERT_EQ(qc::hash::config::minMapCapacity, s1.capacity());
        ASSERT_EQ(prevAllocN, s2.get_allocator().stats().allocations);
    }
    // Non-trivial type
    {
        Tracked2MemRecordSet s1{};
        for (int i{0}; i < 100; ++i)
        {
            s1.emplace(i);
        }
        Tracked2MemRecordSet ref{s1};
        const u64 prevAllocN{s1.get_allocator().stats().allocations};
        Tracked2MemRecordSet s2{std::move(s1)};
        ASSERT_EQ(100u, s2.size());
        ASSERT_EQ(128u, s2.capacity());
        ASSERT_EQ(ref, s2);
        ASSERT_TRUE(s1.empty());
        ASSERT_EQ(qc::hash::config::minMapCapacity, s1.capacity());
        ASSERT_EQ(prevAllocN, s2.get_allocator().stats().allocations);
    }
}

TEST(set, assignOperator_initializerList)
{
    MemRecordSet<int> s{};
    for (int i{0}; i < 100; ++i)
    {
        s.insert(i);
    }
    ASSERT_EQ(128u, s.capacity());

    s = {0, 1, 2, 3, 4, 5};
    ASSERT_EQ(6u, s.size());
    ASSERT_EQ(qc::hash::config::minMapCapacity, s.capacity());
    ASSERT_EQ(1u, s.get_allocator().stats().allocations);
    for (int i{0}; i < 6; ++i)
    {
        ASSERT_TRUE(s.contains(i));
    }
}

TEST(set, assignOperator_copy)
{
    // Trivial type
    {
        MemRecordSet<int> s1{};
        for (int i{0}; i < 100; ++i)
        {
            s1.insert(i);
        }
        const u64 prevAllocN{s1.get_allocator().stats().allocations};

        MemRecordSet<int> s2{};
        s2 = s1;
        ASSERT_EQ(100u, s2.size());
        ASSERT_EQ(128u, s2.capacity());
        ASSERT_EQ(s1, s2);
        ASSERT_EQ(prevAllocN + 1u, s2.get_allocator().stats().allocations);

        GCC_DIAGNOSTIC_PUSH
        GCC_DIAGNOSTIC_IGNORED("-Wself-assign")
        s2 = s2;
        GCC_DIAGNOSTIC_POP
        ASSERT_EQ(100u, s2.size());
        ASSERT_EQ(128u, s2.capacity());
        ASSERT_EQ(s2, s2);
        ASSERT_EQ(prevAllocN + 1u, s2.get_allocator().stats().allocations);
    }

    // Non-trivial type
    {
        Tracked2MemRecordSet s1{};
        for (int i{0}; i < 100; ++i)
        {
            s1.emplace(i);
        }
        const u64 prevAllocN{s1.get_allocator().stats().allocations};

        Tracked2MemRecordSet s2{};
        s2 = s1;
        ASSERT_EQ(100u, s2.size());
        ASSERT_EQ(128u, s2.capacity());
        ASSERT_EQ(s1, s2);
        ASSERT_EQ(prevAllocN + 1u, s2.get_allocator().stats().allocations);

        GCC_DIAGNOSTIC_PUSH
        GCC_DIAGNOSTIC_IGNORED("-Wself-assign")
        s2 = s2;
        GCC_DIAGNOSTIC_POP
        ASSERT_EQ(100u, s2.size());
        ASSERT_EQ(128u, s2.capacity());
        ASSERT_EQ(s2, s2);
        ASSERT_EQ(prevAllocN + 1u, s2.get_allocator().stats().allocations);
    }
}

TEST(set, assignOperator_move)
{
    // Trivial type
    {
        MemRecordSet<int> s1{};
        for (int i{0}; i < 100; ++i)
        {
            s1.insert(i);
        }
        MemRecordSet<int> ref{s1};
        const u64 prevAllocN{s1.get_allocator().stats().allocations};

        MemRecordSet<int> s2{};
        s2 = std::move(s1);
        ASSERT_EQ(100u, s2.size());
        ASSERT_EQ(128u, s2.capacity());
        ASSERT_EQ(ref, s2);
        ASSERT_TRUE(s1.empty());
        ASSERT_EQ(qc::hash::config::minMapCapacity, s1.capacity());
        ASSERT_EQ(prevAllocN, s2.get_allocator().stats().allocations);

        GCC_DIAGNOSTIC_PUSH
        GCC_DIAGNOSTIC_IGNORED("-Wself-move")
        s2 = std::move(s2);
        GCC_DIAGNOSTIC_POP
        ASSERT_EQ(100u, s2.size());
        ASSERT_EQ(128u, s2.capacity());
        ASSERT_EQ(s2, s2);
        ASSERT_EQ(prevAllocN, s2.get_allocator().stats().allocations);

        s2 = std::move(s1);
        ASSERT_TRUE(s2.empty());
        ASSERT_EQ(qc::hash::config::minMapCapacity, s2.capacity());
        ASSERT_TRUE(s1.empty());
        ASSERT_EQ(qc::hash::config::minMapCapacity, s1.capacity());
        ASSERT_EQ(s1, s2);
    }

    // Non-trivial type
    {
        Tracked2MemRecordSet s1{};
        for (int i{0}; i < 100; ++i)
        {
            s1.emplace(i);
        }
        Tracked2MemRecordSet ref{s1};
        const u64 prevAllocN{s1.get_allocator().stats().allocations};

        Tracked2MemRecordSet s2{};
        s2 = std::move(s1);
        ASSERT_EQ(100u, s2.size());
        ASSERT_EQ(128u, s2.capacity());
        ASSERT_EQ(ref, s2);
        ASSERT_TRUE(s1.empty());
        ASSERT_EQ(qc::hash::config::minMapCapacity, s1.capacity());
        ASSERT_EQ(prevAllocN, s2.get_allocator().stats().allocations);

        GCC_DIAGNOSTIC_PUSH
        GCC_DIAGNOSTIC_IGNORED("-Wself-move")
        s2 = std::move(s2);
        GCC_DIAGNOSTIC_POP
        ASSERT_EQ(100u, s2.size());
        ASSERT_EQ(128u, s2.capacity());
        ASSERT_EQ(s2, s2);
        ASSERT_EQ(prevAllocN, s2.get_allocator().stats().allocations);

        s2 = std::move(s1);
        ASSERT_TRUE(s2.empty());
        ASSERT_EQ(qc::hash::config::minMapCapacity, s2.capacity());
        ASSERT_TRUE(s1.empty());
        ASSERT_EQ(qc::hash::config::minMapCapacity, s1.capacity());
        ASSERT_EQ(s1, s2);
    }
}

// Keep parallel with `emplace_lVal` and `tryEmplace_lVal` tests
TEST(set, insert_lVal)
{
    Tracked2::resetTotals();
    Tracked2MemRecordSet s{};

    for (int i{0}; i < 100; ++i)
    {
        const Tracked2 val{i};

        auto [it1, inserted1]{s.insert(val)};
        ASSERT_TRUE(inserted1);
        ASSERT_EQ(val, *it1);

        auto [it2, inserted2]{s.insert(val)};
        ASSERT_FALSE(inserted2);
        ASSERT_EQ(it1, it2);
    }

    ASSERT_EQ(100u, s.size());
    ASSERT_EQ(128u, s.capacity());
    ASSERT_EQ(4u, s.get_allocator().stats().allocations);
    ASSERT_EQ(100, Tracked2::totalStats.copyConstructs);
    ASSERT_EQ(16 + 32 + 64, Tracked2::totalStats.moveConstructs);
    ASSERT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.destructs);
    ASSERT_EQ(0, Tracked2::totalStats.defConstructs);
    ASSERT_EQ(0, Tracked2::totalStats.assigns());
}

// Keep parallel with `emplace_rVal` and `tryEmplace_rVal` tests
TEST(set, insert_rVal)
{
    TrackedSet s{};
    Tracked2 val1{7};
    Tracked2 val2{7};

    const auto [it1, inserted1]{s.insert(std::move(val1))};
    ASSERT_TRUE(inserted1);
    const Tracked2 & tracked{*it1};
    ASSERT_EQ(7, int(tracked.val));
    ASSERT_EQ(1, tracked.stats().moveConstructs);
    ASSERT_EQ(1, tracked.stats().all());
    ASSERT_EQ(0, val1.val);

    const auto [it2, inserted2]{s.insert(std::move(val2))};
    ASSERT_FALSE(inserted2);
    ASSERT_EQ(it1, it2);
    ASSERT_EQ(7, val2.val);
    ASSERT_EQ(0, val2.stats().all());
}

TEST(set, insert_range)
{
    Tracked2MemRecordSet s{};
    std::vector<Tracked2> values{};
    for (int i{0}; i < 100; ++i) values.emplace_back(i);

    Tracked2::resetTotals();
    s.insert(values.cbegin(), values.cend());
    ASSERT_EQ(100u, s.size());
    ASSERT_EQ(128u, s.capacity());
    for (int i{0}; i < 100; ++i)
    {
        ASSERT_TRUE(s.contains(Tracked2{i}));
    }
    ASSERT_EQ(4u, s.get_allocator().stats().allocations);
    ASSERT_EQ(100, Tracked2::totalStats.copyConstructs);
    ASSERT_EQ(16 + 32 + 64, Tracked2::totalStats.moveConstructs);
    ASSERT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.destructs);
    ASSERT_EQ(0, Tracked2::totalStats.defConstructs);
    ASSERT_EQ(0, Tracked2::totalStats.assigns());
}

TEST(set, insert_initializerList)
{
    MemRecordSet<int> s{};
    s.insert({0, 1, 2, 3, 4, 5});
    ASSERT_EQ(6u, s.size());
    ASSERT_EQ(16u, s.capacity());
    for (int i{0}; i < 6; ++i)
    {
        ASSERT_TRUE(s.contains(i));
    }
    ASSERT_EQ(1u, s.get_allocator().stats().allocations);
}

// Keep parallel with `insert_lVal` and `tryEmplace_lVal` tests
TEST(set, emplace_lVal)
{
    Tracked2::resetTotals();
    Tracked2MemRecordSet s{};

    for (int i{0}; i < 100; ++i)
    {
        const Tracked2 val{i};

        auto[it1, inserted1]{s.emplace(val)};
        ASSERT_TRUE(inserted1);
        ASSERT_EQ(val, *it1);

        auto[it2, inserted2]{s.emplace(val)};
        ASSERT_FALSE(inserted2);
        ASSERT_EQ(it1, it2);
    }

    ASSERT_EQ(100u, s.size());
    ASSERT_EQ(128u, s.capacity());
    ASSERT_EQ(4u, s.get_allocator().stats().allocations);
    ASSERT_EQ(100, Tracked2::totalStats.copyConstructs);
    ASSERT_EQ(16 + 32 + 64, Tracked2::totalStats.moveConstructs);
    ASSERT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.destructs);
    ASSERT_EQ(0, Tracked2::totalStats.defConstructs);
    ASSERT_EQ(0, Tracked2::totalStats.assigns());
}

// Keep parallel with `insert_rVal` and `tryEmplace_rVal` tests
TEST(set, emplace_rVal)
{
    TrackedSet s{};
    Tracked2 val1{7};
    Tracked2 val2{7};

    const auto [it1, inserted1]{s.emplace(std::move(val1))};
    ASSERT_TRUE(inserted1);
    const Tracked2 & tracked{*it1};
    ASSERT_EQ(7, int(tracked.val));
    ASSERT_EQ(1, tracked.stats().moveConstructs);
    ASSERT_EQ(1, tracked.stats().all());
    ASSERT_EQ(0, val1.val);

    const auto [it2, inserted2]{s.emplace(std::move(val2))};
    ASSERT_FALSE(inserted2);
    ASSERT_EQ(it1, it2);
    ASSERT_EQ(7, val2.val);
    ASSERT_EQ(0, val2.stats().all());
}

TEST(set, emplace_keyArgs)
{
    TrackedSet s{};
    const auto [it, inserted]{s.emplace(7)};
    ASSERT_TRUE(inserted);
    ASSERT_EQ(7, it->val);
    ASSERT_EQ(1, it->stats().moveConstructs);
    ASSERT_EQ(1, it->stats().all());
}

// Keep parallel with `insert_lVal` and `emplace_lVal` tests
TEST(set, tryEmplace_lVal)
{
    Tracked2::resetTotals();
    Tracked2MemRecordSet s{};

    for (int i{0}; i < 100; ++i)
    {
        const Tracked2 val{i};

        auto [it1, inserted1]{s.try_emplace(val)};
        ASSERT_TRUE(inserted1);
        ASSERT_EQ(val, *it1);

        auto [it2, inserted2]{s.try_emplace(val)};
        ASSERT_FALSE(inserted2);
        ASSERT_EQ(it1, it2);
    }

    ASSERT_EQ(100u, s.size());
    ASSERT_EQ(128u, s.capacity());
    ASSERT_EQ(4u, s.get_allocator().stats().allocations);
    ASSERT_EQ(100, Tracked2::totalStats.copyConstructs);
    ASSERT_EQ(16 + 32 + 64, Tracked2::totalStats.moveConstructs);
    ASSERT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.destructs);
    ASSERT_EQ(0, Tracked2::totalStats.defConstructs);
    ASSERT_EQ(0, Tracked2::totalStats.assigns());
}

// Keep parallel with `insert_rVal` and `emplace_rVal` tests
TEST(set, tryEmplace_rVal)
{
    TrackedSet s{};
    Tracked2 val1{7};
    Tracked2 val2{7};

    const auto [it1, inserted1]{s.try_emplace(std::move(val1))};
    ASSERT_TRUE(inserted1);
    const Tracked2 & tracked{*it1};
    ASSERT_EQ(7, int(tracked.val));
    ASSERT_EQ(1, tracked.stats().moveConstructs);
    ASSERT_EQ(1, tracked.stats().all());
    ASSERT_EQ(0, val1.val);

    const auto [it2, inserted2]{s.try_emplace(std::move(val2))};
    ASSERT_FALSE(inserted2);
    ASSERT_EQ(it1, it2);
    ASSERT_EQ(7, val2.val);
    ASSERT_EQ(0, val2.stats().all());
}

TEST(set, eraseKey)
{
    TrackedSet s{};

    Tracked2::resetTotals();
    ASSERT_FALSE(s.erase(Tracked2{0}));
    ASSERT_EQ(1, Tracked2::totalStats.destructs);
    ASSERT_EQ(1, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{0}; i < 100; ++i)
    {
        s.emplace(i);
    }
    ASSERT_EQ(100u, s.size());
    ASSERT_EQ(128u, s.capacity());
    ASSERT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.moveConstructs);
    ASSERT_EQ(100 + 16 + 32 + 64, Tracked2::totalStats.destructs);
    ASSERT_EQ(Tracked2::totalStats.moveConstructs + Tracked2::totalStats.destructs, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    ASSERT_FALSE(s.erase(Tracked2{100}));
    ASSERT_EQ(1, Tracked2::totalStats.destructs);
    ASSERT_EQ(Tracked2::totalStats.destructs, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{0}; i < 100; ++i)
    {
        ASSERT_TRUE(s.erase(Tracked2{i}));
        ASSERT_EQ(u64(100 - i - 1), s.size());
    }
    ASSERT_TRUE(s.empty());
    ASSERT_EQ(128u, s.capacity());
    ASSERT_EQ(200, Tracked2::totalStats.destructs);
    ASSERT_EQ(Tracked2::totalStats.destructs, Tracked2::totalStats.all());
    Tracked2::resetTotals();
}

TEST(set, eraseIterator)
{
    RawSet<int> s{};

    for (int i{0}; i < 100; ++i)
    {
        s.insert(i);
    }
    ASSERT_EQ(100u, s.size());
    ASSERT_EQ(128u, s.capacity());

    for (int i{0}; i < 100; ++i)
    {
        s.erase(s.find(i));
        ASSERT_EQ(u64(100 - i - 1), s.size());
    }
    ASSERT_TRUE(s.empty());
    ASSERT_EQ(128u, s.capacity());
}

TEST(set, clear)
{
    // Trivially destructible type
    {
        RawSet<int> s{};
        for (int i{0}; i < 100; ++i) s.insert(i);
        ASSERT_EQ(100u, s.size());
        ASSERT_EQ(128u, s.capacity());

        s.clear();
        ASSERT_EQ(0u, s.size());
        ASSERT_EQ(128u, s.capacity());
    }

    // Non-trivially destructible type
    {

        TrackedSet s{};
        for (int i{0}; i < 100; ++i) s.emplace(i);
        ASSERT_EQ(100u, s.size());
        ASSERT_EQ(128u, s.capacity());

        Tracked2::resetTotals();
        s.clear();
        ASSERT_EQ(0u, s.size());
        ASSERT_EQ(128u, s.capacity());
        ASSERT_EQ(100, Tracked2::totalStats.destructs);
        ASSERT_EQ(100, Tracked2::totalStats.all());
    }
}

// Keep parallel with `count` test
TEST(set, contains)
{
    RawSet<int> s{};
    for (int i{0}; i < 100; ++i)
    {
        s.insert(i);
        for (int j{0}; j <= i; ++j)
        {
            ASSERT_TRUE(s.contains(j));
        }
    }

    s.clear();
    for (int i{0}; i < 100; ++i)
    {
        ASSERT_FALSE(s.contains(i));
    }
}

// Keep parallel with `contains` test
TEST(set, count)
{
    RawSet<int> s{};
    for (int i{0}; i < 100; ++i)
    {
        s.insert(i);
        for (int j{0}; j <= i; ++j)
        {
            ASSERT_EQ(1u, s.count(j));
        }
    }

    s.clear();
    for (int i{0}; i < 100; ++i)
    {
        ASSERT_EQ(0u, s.count(i));
    }
}

TEST(set, begin)
{
    RawSet<int> s{};
    ASSERT_EQ(s.end(), s.begin());

    s.insert(7);
    ASSERT_NE(s.end(), s.begin());
    ASSERT_EQ(s.begin(), s.begin());
    ASSERT_EQ(7, *s.begin());

    auto it{s.begin()};
    ++it;
    ASSERT_EQ(s.end(), it);

    ASSERT_EQ(s.begin(), s.cbegin());
}

TEST(set, end)
{
    RawSet<int> s{};
    ASSERT_EQ(s.begin(), s.end());
    ASSERT_EQ(s.end(), s.end());

    s.insert(7);
    ASSERT_NE(s.begin(), s.end());

    ASSERT_EQ(s.end(), s.cend());
}

// Keep parallel with `equal_range` test
TEST(set, find)
{
    RawSet<int> s{};
    ASSERT_EQ(s.end(), s.find(0));

    for (int i{0}; i < 100; ++i)
    {
        s.insert(i);
    }
    for (int i{0}; i < 100; ++i)
    {
        const auto it{s.find(i)};
        ASSERT_NE(s.end(), it);
        ASSERT_EQ(i, *it);
    }

    ASSERT_EQ(s.end(), s.find(100));
}

TEST(set, slot)
{
    RawSet<int> s(128);
    s.insert(7);
    ASSERT_EQ(RawFriend::slotI(s, s.find(7)), s.slot(7));
}

// `reserve` method is synonymous with `rehash` method
TEST(set, reserve) {}

TEST(set, rehash)
{
    RawSet<int> s{};

    ASSERT_EQ(qc::hash::config::minMapCapacity, s.capacity());

    s.rehash(0u);
    ASSERT_EQ(qc::hash::config::minMapCapacity, s.capacity());

    s.rehash(1u);
    ASSERT_EQ(qc::hash::config::minMapCapacity, s.capacity());

    for (int i{0}; i < 16; ++i)
    {
        s.insert(i);
    }
    ASSERT_EQ(16u, s.size());
    ASSERT_EQ(16u, s.capacity());

    s.emplace(16);
    ASSERT_EQ(17u, s.size());
    ASSERT_EQ(32u, s.capacity());

    for (int i{17}; i < 128; ++i)
    {
        s.emplace(i);
    }
    ASSERT_EQ(128u, s.size());
    ASSERT_EQ(128u, s.capacity());

    s.rehash(500u);
    ASSERT_EQ(128u, s.size());
    ASSERT_EQ(256u, s.capacity());
    for (int i = 0; i < 128; ++i)
    {
        ASSERT_TRUE(s.contains(i));
    }

    s.rehash(10u);
    ASSERT_EQ(128u, s.size());
    ASSERT_EQ(128u, s.capacity());
    for (int i = 0; i < 128; ++i)
    {
        ASSERT_TRUE(s.contains(i));
    }

    s.clear();
    ASSERT_EQ(0u, s.size());
    ASSERT_EQ(128u, s.capacity());

    s.rehash(0u);
    ASSERT_EQ(qc::hash::config::minMapCapacity, s.capacity());
}

TEST(set, swap)
{
    RawSet<int> s1{1, 2, 3};
    RawSet<int> s2{4, 5, 6};
    RawSet<int> s3{s1};
    RawSet<int> s4{s2};
    ASSERT_EQ(s1, s3);
    ASSERT_EQ(s2, s4);
    s3.swap(s4);
    ASSERT_EQ(s2, s3);
    ASSERT_EQ(s1, s4);
    std::swap(s3, s4);
    ASSERT_EQ(s1, s3);
    ASSERT_EQ(s2, s4);

    auto it1{s1.cbegin()};
    auto it2{s2.cbegin()};
    auto it3{it1};
    auto it4{it2};
    ASSERT_EQ(it1, it3);
    ASSERT_EQ(it2, it4);
    std::swap(it1, it2);
    ASSERT_EQ(it1, it4);
    ASSERT_EQ(it2, it3);
}

TEST(set, size_empty_capacity_slotN)
{
    RawSet<int> s{};
    ASSERT_EQ(0u, s.size());
    ASSERT_TRUE(s.empty());
    ASSERT_EQ(qc::hash::config::minMapCapacity, s.capacity());

    for (int i{0}; i < 100; ++i)
    {
        s.insert(i);
    }
    ASSERT_EQ(100u, s.size());
    ASSERT_FALSE(s.empty());
    ASSERT_EQ(128u, s.capacity());
    ASSERT_EQ(256u, s.slot_n());
}

TEST(set, maxSize)
{
    RawSet<int> s{};
    ASSERT_EQ(0b01000000'00000000'00000000'00000000'00000000'00000000'00000000'00000010u, s.max_size());
}

TEST(set, maxSlotN)
{
    RawSet<int> s{};
    ASSERT_EQ(0b10000000'00000000'00000000'00000000'00000000'00000000'00000000'00000000u, s.max_slot_n());
}

TEST(set, loadFactor)
{
    RawSet<int> s{};
    ASSERT_EQ(0.0f, s.load_factor());

    s.insert(7);
    ASSERT_EQ(1.0f / 32.0f, s.load_factor());
}

TEST(set, maxLoadFactor)
{
    RawSet<int> s{};
    ASSERT_EQ(0.5f, s.max_load_factor());

    s.insert(7);
    ASSERT_EQ(0.5f, s.max_load_factor());
}

TEST(set, getters)
{
    RawSet<int> s{};
    static_cast<void>(s.hash_function());
    static_cast<void>(s.get_allocator());
}

TEST(set, equality)
{
    RawSet<int> s1{}, s2{};
    for (int i{0}; i < 100; ++i)
    {
        s1.emplace(i);
        s2.emplace(i + 100);
    }
    ASSERT_TRUE(s1 == s1);
    ASSERT_TRUE(s1 != s2);

    s2 = s1;
    ASSERT_TRUE(s1 == s2);
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
    for (int i{0}; i < 100; ++i)
    {
        s.insert(i);
    }

    int i{0};
    for (auto it{s.begin()}; it != s.end(); ++it)
    {
        ASSERT_EQ(i + 1, ++*it);
        ASSERT_EQ(i, --*it);
        ++i;
    }
}

TEST(set, forEachLoop)
{
    RawSet<int> s{};
    for (int i{0}; i < 100; ++i)
    {
        s.insert(i);
    }

    int i{0};
    for (int & val : s)
    {
        ASSERT_EQ(i + 1, ++val);
        ASSERT_EQ(i, --val);
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

    static_cast<void>(it1 == cit1);
    static_cast<void>(cit1 == it1);
}

TEST(set, iteratorAssignability)
{
    static_assert(std::is_assignable_v<RawSet<int>::iterator, RawSet<int>::iterator>);
    static_assert(std::is_assignable_v<RawSet<int>::const_iterator, RawSet<int>::iterator>);
    static_assert(!std::is_assignable_v<RawSet<int>::iterator, RawSet<int>::const_iterator>);
    static_assert(std::is_assignable_v<RawSet<int>::const_iterator, RawSet<int>::const_iterator>);
}

TEST(set, singleElementInitializerList)
{
    RawSet<int> s{100};
    ASSERT_EQ(1u, s.size());
    ASSERT_EQ(qc::hash::config::minMapCapacity, s.capacity());
    ASSERT_EQ(100, *s.cbegin());
}

TEST(set, noPreemtiveRehash)
{
    RawSet<int> s{};
    for (int i{0}; i < int(qc::hash::config::minMapCapacity) - 1; ++i) s.insert(i);
    ASSERT_EQ(qc::hash::config::minMapCapacity, s.capacity());
    s.emplace(int(qc::hash::config::minMapCapacity - 1));
    ASSERT_EQ(qc::hash::config::minMapCapacity, s.capacity());
    s.emplace(int(qc::hash::config::minMapCapacity - 1));
    ASSERT_EQ(qc::hash::config::minMapCapacity, s.capacity());
}

struct SetDistStats
{
    u64 min, max, median;
    double mean, stdDev;
};

template <typename V>
SetDistStats calcStats(const RawSet<V> & set)
{
    SetDistStats distStats{};
    std::map<u64, u64> histo{};

    distStats.min = ~u64{0u};
    for (auto it{set.cbegin()}; it != set.cend(); ++it)
    {
        const u64 dist{RawFriend::dist(set, it)};
        //++distStats.histo[dist];
        if (dist < distStats.min) distStats.min = dist;
        else if (dist > distStats.max) distStats.max = dist;
        distStats.mean += double(dist);
    }
    distStats.mean /= double(set.size());

    for (auto it{set.cbegin()}; it != set.cend(); ++it)
    {
        const u64 dist{RawFriend::dist(set, it)};
        double diff{double(dist) - distStats.mean};
        distStats.stdDev += diff * diff;
    }
    distStats.stdDev = std::sqrt(distStats.stdDev / double(set.size()));

    u64 medianN{0u};
    for (const auto & distN : histo)
    {
        if (distN.second > medianN)
        {
            distStats.median = distN.first;
            medianN = distN.second;
        }
    }

    return distStats;
}

TEST(set, stats)
{
    constexpr u64 size{8192u};

    qc::Random random{};

    RawSet<int> s(size);
    for (u64 i{0u}; i < size; ++i)
    {
        s.insert(random.next<int>());
    }

    const SetDistStats stats{calcStats(s)};
    ASSERT_EQ(0u, stats.median);
    ASSERT_NEAR(0.5, stats.mean, 0.1);
    ASSERT_NEAR(1.25, stats.stdDev, 0.25);
}

template <typename K, typename V> void testStaticMemory()
{
    static constexpr u64 capacity{128u};
    static constexpr u64 slotN{capacity * 2u};

    MemRecordSet<K> s(capacity);
    s.emplace(K{});
    ASSERT_EQ(sizeof(u64) * 4u, sizeof(RawSet<K>));
    ASSERT_EQ((slotN + 4u) * sizeof(K), s.get_allocator().stats().current);

    MemRecordMap<K, V> m(capacity);
    m.emplace(K{}, V{});
    ASSERT_EQ(sizeof(u64) * 4u, sizeof(RawMap<K, V>));
    ASSERT_EQ((slotN + 4u) * sizeof(std::pair<K, V>), m.get_allocator().stats().current);
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
    const u64 slotSize{sizeof(int)};

    u64 current{0u}, total{0u}, allocations{0u}, deallocations{0u};
    ASSERT_EQ(current, s.get_allocator().stats().current);
    ASSERT_EQ(total, s.get_allocator().stats().total);
    ASSERT_EQ(allocations, s.get_allocator().stats().allocations);
    ASSERT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.rehash(64u);
    ASSERT_EQ(current, s.get_allocator().stats().current);
    ASSERT_EQ(total, s.get_allocator().stats().total);
    ASSERT_EQ(allocations, s.get_allocator().stats().allocations);
    ASSERT_EQ(deallocations, s.get_allocator().stats().deallocations);

    for (int i{0}; i < 32; ++i) s.emplace(i);
    current = (64u + 4u) * slotSize;
    total += current;
    ++allocations;
    ASSERT_EQ(64u, s.slot_n());
    ASSERT_EQ(current, s.get_allocator().stats().current);
    ASSERT_EQ(total, s.get_allocator().stats().total);
    ASSERT_EQ(allocations, s.get_allocator().stats().allocations);
    ASSERT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.emplace(64);
    current = (128u + 4u) * slotSize;
    total += current;
    ++allocations;
    ++deallocations;
    ASSERT_EQ(current, s.get_allocator().stats().current);
    ASSERT_EQ(total, s.get_allocator().stats().total);
    ASSERT_EQ(allocations, s.get_allocator().stats().allocations);
    ASSERT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.clear();
    ASSERT_EQ(current, s.get_allocator().stats().current);
    ASSERT_EQ(total, s.get_allocator().stats().total);
    ASSERT_EQ(allocations, s.get_allocator().stats().allocations);
    ASSERT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.rehash(1024u);
    current = (1024u + 4u) * slotSize;
    total += current;
    ++allocations;
    ++deallocations;
    ASSERT_EQ(current, s.get_allocator().stats().current);
    ASSERT_EQ(total, s.get_allocator().stats().total);
    ASSERT_EQ(allocations, s.get_allocator().stats().allocations);
    ASSERT_EQ(deallocations, s.get_allocator().stats().deallocations);

    for (int i{0}; i < 128; ++i) s.emplace(i);
    ASSERT_EQ(current, s.get_allocator().stats().current);
    ASSERT_EQ(total, s.get_allocator().stats().total);
    ASSERT_EQ(allocations, s.get_allocator().stats().allocations);
    ASSERT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.emplace(128);
    ASSERT_EQ(current, s.get_allocator().stats().current);
    ASSERT_EQ(total, s.get_allocator().stats().total);
    ASSERT_EQ(allocations, s.get_allocator().stats().allocations);
    ASSERT_EQ(deallocations, s.get_allocator().stats().deallocations);

    s.erase(128);
    ASSERT_EQ(current, s.get_allocator().stats().current);
    ASSERT_EQ(total, s.get_allocator().stats().total);
    ASSERT_EQ(allocations, s.get_allocator().stats().allocations);
    ASSERT_EQ(deallocations, s.get_allocator().stats().deallocations);

    while (!s.empty()) s.erase(s.begin());
    ASSERT_EQ(current, s.get_allocator().stats().current);
    ASSERT_EQ(total, s.get_allocator().stats().total);
    ASSERT_EQ(allocations, s.get_allocator().stats().allocations);
    ASSERT_EQ(deallocations, s.get_allocator().stats().deallocations);
}

TEST(set, circuity)
{
    RawSet<int> s(16u);

    // With zero key absent

    s.insert(31);
    ASSERT_EQ(31, RawFriend::getElement(s, 31));
    ASSERT_TRUE(RawFriend::isVacant(s, 0));
    ASSERT_TRUE(RawFriend::isVacant(s, 1));

    s.insert(63);
    ASSERT_EQ(31, RawFriend::getElement(s, 31));
    ASSERT_EQ(63, RawFriend::getElement(s, 0));
    ASSERT_TRUE(RawFriend::isVacant(s, 1));

    s.insert(95);
    ASSERT_EQ(31, RawFriend::getElement(s, 31));
    ASSERT_EQ(63, RawFriend::getElement(s, 0));
    ASSERT_EQ(95, RawFriend::getElement(s, 1));

    s.erase(31);
    ASSERT_TRUE(RawFriend::isGrave(s, 31));
    ASSERT_EQ(63, RawFriend::getElement(s, 0));
    ASSERT_EQ(95, RawFriend::getElement(s, 1));

    s.erase(95);
    ASSERT_TRUE(RawFriend::isGrave(s, 31));
    ASSERT_EQ(63, RawFriend::getElement(s, 0));
    ASSERT_TRUE(RawFriend::isGrave(s, 1));

    s.erase(63);
    ASSERT_TRUE(RawFriend::isGrave(s, 31));
    ASSERT_TRUE(RawFriend::isGrave(s, 0));
    ASSERT_TRUE(RawFriend::isGrave(s, 1));
}

TEST(set, terminal)
{
    RawSet<u32> s(16u);
    s.insert(0u);
    s.insert(1u);
    ASSERT_EQ(RawFriend::vacantKey<int>, RawFriend::getElement(s, 32u));
    ASSERT_EQ(RawFriend::graveKey<int>, RawFriend::getElement(s, 33u));
    ASSERT_EQ(0u, RawFriend::getElement(s, 34u));
    ASSERT_EQ(0u, RawFriend::getElement(s, 35u));

    const auto it1{++s.begin()};
    ASSERT_EQ(1u, *it1);

    auto it{it1};
    ++it;
    ASSERT_EQ(s.end(), it);

    s.insert(RawFriend::graveKey<int>);
    it = it1;
    ++it;
    ASSERT_EQ(RawFriend::graveKey<int>, *it);
    ++it;
    ASSERT_EQ(s.end(), it);

    s.insert(RawFriend::vacantKey<int>);
    it = it1;
    ++it;
    ASSERT_EQ(RawFriend::graveKey<int>, *it);
    ++it;
    ASSERT_EQ(RawFriend::vacantKey<int>, *it);
    ++it;
    ASSERT_EQ(s.end(), it);

    s.erase(RawFriend::graveKey<int>);
    it = it1;
    ++it;
    ASSERT_EQ(RawFriend::vacantKey<int>, *it);
    ++it;
    ASSERT_EQ(s.end(), it);

    s.erase(0u);
    s.erase(1u);
    it = s.begin();
    ASSERT_EQ(RawFriend::vacantKey<int>, *it);
    ++it;
    ASSERT_EQ(s.end(), it);

    s.insert(RawFriend::graveKey<int>);
    it = s.begin();
    ASSERT_EQ(RawFriend::graveKey<int>, *it);
    ++it;
    ASSERT_EQ(RawFriend::vacantKey<int>, *it);
    ++it;
    ASSERT_EQ(s.end(), it);

    s.erase(RawFriend::vacantKey<int>);
    it = s.begin();
    ASSERT_EQ(RawFriend::graveKey<int>, *it);
    ++it;
    ASSERT_EQ(s.end(), it);

    s.erase(RawFriend::graveKey<int>);
    it = s.begin();
    ASSERT_EQ(s.end(), it);
}

TEST(set, middleZero)
{
    RawSet<u32> s(16u);
    s.insert(10u);
    RawFriend::getElement(s, 8u) = RawFriend::vacantKey<u32>;
    RawFriend::getElement(s, 9u) = RawFriend::graveKey<u32>;
    RawFriend::getElement(s, 10u) = 0u;

    auto it{s.begin()};
    ASSERT_EQ(0u, *it);

    ++it;
    ASSERT_EQ(s.end(), it);
}

TEST(set, allBytes)
{
    std::vector<std::byte> keys{};
    keys.reserve(256u);
    for (u64 i{0u}; i < 256u; ++i) keys.push_back(std::byte(i));

    qc::Random random{};
    for (int iteration{0}; iteration < 256; ++iteration)
    {
        std::shuffle(keys.begin(), keys.end(), random);

        RawSet<std::byte> s{};

        for (const std::byte key : keys)
        {
            ASSERT_TRUE(s.insert(key).second);
        }
        ASSERT_EQ(256u, s.size());

        for (u32 k{0u}; k < 256u; ++k)
        {
            ASSERT_TRUE(s.contains(std::byte(k)));
        }

        u32 expectedK{0u};
        for (const auto k : s)
        {
            ASSERT_EQ(std::byte(expectedK), k);
            ++expectedK;
        }

        // Iterator erasure
        expectedK = 0u;
        for (auto it{s.begin()}; it != s.end(); ++it, ++expectedK)
        {
            ASSERT_EQ(std::byte(expectedK), *it);
            s.erase(it);
        }
        ASSERT_TRUE(s.empty());
    }
}

TEST(set, smartPtrs)
{
    // unique_ptr
    {
        RawSet<std::unique_ptr<int>> s{};
        const auto [it, result]{s.emplace(new int{7})};
        ASSERT_TRUE(result);
        ASSERT_EQ(7, **it);
        ASSERT_TRUE(s.contains(*it));
        ASSERT_FALSE(s.contains(std::make_unique<int>(8)));
        ASSERT_TRUE(s.erase(*it));
    }
    // shared_ptr
    {
        RawSet<std::shared_ptr<int>> s{};
        const auto [it, result]{s.emplace(new int{7})};
        ASSERT_TRUE(result);
        ASSERT_EQ(7, **it);
        ASSERT_TRUE(s.contains(*it));
        ASSERT_FALSE(s.contains(std::make_shared<int>(8)));
        ASSERT_TRUE(s.erase(*it));
    }
}

TEST(map, general)
{
    TrackedMap m{100};

    Tracked2::resetTotals();
    for (int i{0}; i < 25; ++i)
    {
        const auto [it, inserted]{m.emplace(Tracked2{i}, i + 100)};
        ASSERT_TRUE(inserted);
        ASSERT_EQ(i, it->first.val);
        ASSERT_EQ(i + 100, it->second.val);
    }
    ASSERT_EQ(25, Tracked2::totalStats.moveConstructs);
    ASSERT_EQ(25, Tracked2::totalStats.destructs);
    ASSERT_EQ(50, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{25}; i < 50; ++i)
    {
        const auto [it, inserted]{m.emplace(std::piecewise_construct, std::forward_as_tuple(i), std::forward_as_tuple(i + 100))};
        ASSERT_TRUE(inserted);
        ASSERT_EQ(i, it->first.val);
        ASSERT_EQ(i + 100, it->second.val);
    }
    ASSERT_EQ(25, Tracked2::totalStats.moveConstructs);
    ASSERT_EQ(25, Tracked2::totalStats.destructs);
    ASSERT_EQ(50, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{50}; i < 75; ++i)
    {
        const auto [it, inserted]{m.try_emplace(Tracked2{i}, i + 100)};
        ASSERT_TRUE(inserted);
        ASSERT_EQ(i, it->first.val);
        ASSERT_EQ(i + 100, it->second.val);
    }
    ASSERT_EQ(25, Tracked2::totalStats.moveConstructs);
    ASSERT_EQ(25, Tracked2::totalStats.destructs);
    ASSERT_EQ(50, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{50}; i < 75; ++i)
    {
        const auto [it, inserted]{m.try_emplace(Tracked2{i}, i + 100)};
        ASSERT_FALSE(inserted);
        ASSERT_EQ(i, it->first.val);
    }
    ASSERT_EQ(0, Tracked2::totalStats.moveConstructs);
    ASSERT_EQ(25, Tracked2::totalStats.destructs);
    ASSERT_EQ(25, Tracked2::totalStats.all());

    Tracked2::resetTotals();
    for (int i{75}; i < 100; ++i)
    {
        const auto [it, inserted]{m.insert(std::pair<Tracked2, Tracked2>{Tracked2{i}, Tracked2{i + 100}})};
        ASSERT_TRUE(inserted);
        ASSERT_EQ(i, it->first.val);
        ASSERT_EQ(i + 100, it->second.val);
    }
    ASSERT_EQ(100, Tracked2::totalStats.moveConstructs);
    ASSERT_EQ(100, Tracked2::totalStats.destructs);
    ASSERT_EQ(200, Tracked2::totalStats.all());

    for (int i{0}; i < 100; ++i)
    {
        #ifdef QC_HASH_EXCEPTIONS_ENABLED
        ASSERT_EQ(i + 100, m.at(Tracked2{i}).val);
        #endif
        ASSERT_EQ(i + 100, m[Tracked2{i}].val);
    }

    #ifdef QC_HASH_EXCEPTIONS_ENABLED
    ASSERT_THROW(static_cast<void>(m.at(Tracked2{100})), std::out_of_range);
    #endif
    ASSERT_EQ(Tracked2{}, m[Tracked2{100}]);
    m[Tracked2{100}] = Tracked2{200};
    ASSERT_EQ(Tracked2{200}, m[Tracked2{100}]);

    TrackedMap m2{m};
    ASSERT_EQ(m, m2);

    m2[Tracked2{100}].val = 400;
    ASSERT_NE(m, m2);
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

    for (int key{0}; key < 100; ++key)
    {
        ASSERT_TRUE(map.emplace(Triple{u8(key), u8(50 + key), u8(100 + key)}, Double{u8(25 + key), u8(75 + key)}).second);
    }

    ASSERT_EQ(100u, map.size());

    for (int key{0}; key < 100; ++key)
    {
        ASSERT_EQ((Double{u8(25 + key), u8(75 + key)}), (map[Triple{u8(key), u8(50 + key), u8(100 + key)}]));
    }
}

TEST(map, largeKey)
{
    struct Big
    {
        u64 a, b, c;
        bool operator==(const Big &) const = default;
    };
    RawMap<Big, Big> map{};

    for (u64 key{0u}; key < 100u; ++key)
    {
        ASSERT_TRUE(map.emplace(Big{key, 50u + key, 100u + key}, Big{25u + key, 75u + key, 125u + key}).second);
    }

    ASSERT_EQ(100u, map.size());

    for (u64 key{0u}; key < 100u; ++key)
    {
        ASSERT_EQ((Big{25u + key, 75u + key, 125u + key}), (map[Big{key, 50u + key, 100u + key}]));
    }
}

TEST(map, iteratorAssignability)
{
    static_assert(std::is_assignable_v<RawMap<int, int>::iterator, RawMap<int, int>::iterator>);
    static_assert(std::is_assignable_v<RawMap<int, int>::const_iterator, RawMap<int, int>::iterator>);
    static_assert(!std::is_assignable_v<RawMap<int, int>::iterator, RawMap<int, int>::const_iterator>);
    static_assert(std::is_assignable_v<RawMap<int, int>::const_iterator, RawMap<int, int>::const_iterator>);
}

template <typename K, typename K_>
concept HeterogeneityCompiles = requires (RawSet<K> set, RawMap<K, int> map, const K_ & k, const qc::hash::IdentityHash<K> identityHash, const qc::hash::FastHash<K> fastHash)
{
    set.erase(k);
    set.contains(k);
    set.count(k);

    map.erase(k);
    map.contains(k);
    map.count(k);
    map[k];
    #ifdef QC_HASH_EXCEPTIONS_ENABLED
    map.at(k);
    #endif

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

    static_assert(HeterogeneityCompiles<u64, u8>);
    static_assert(HeterogeneityCompiles<u64, u16>);
    static_assert(HeterogeneityCompiles<u64, u32>);
    static_assert(HeterogeneityCompiles<u64, u64>);
    static_assert(!HeterogeneityCompiles<u64, s8>);
    static_assert(!HeterogeneityCompiles<u64, s16>);
    static_assert(!HeterogeneityCompiles<u64, s32>);
    static_assert(!HeterogeneityCompiles<u64, s64>);
    static_assert(!HeterogeneityCompiles<u64, bool>);

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

    static_assert(HeterogeneityCompiles<s64, s8>);
    static_assert(HeterogeneityCompiles<s64, s16>);
    static_assert(HeterogeneityCompiles<s64, s32>);
    static_assert(HeterogeneityCompiles<s64, s64>);
    static_assert(HeterogeneityCompiles<s64, u8>);
    static_assert(HeterogeneityCompiles<s64, u16>);
    static_assert(HeterogeneityCompiles<s64, u32>);
    static_assert(!HeterogeneityCompiles<s64, u64>);
    static_assert(!HeterogeneityCompiles<s64, bool>);

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

struct alignas(8u) CustomType
{
    u32 x, y;
};

struct OtherCustomType
{
    u64 x;
};

template <> struct qc::hash::IsCompatible<CustomType, OtherCustomType> : std::true_type {};

template <>
struct qc::hash::IdentityHash<CustomType>
{
    u64 operator()(const CustomType & v) const
    {
        return reinterpret_cast<const u64 &>(v);
    }

    u64 operator()(const OtherCustomType & v) const
    {
        return reinterpret_cast<const u64 &>(v);
    }
};

template <>
struct qc::hash::FastHash<CustomType>
{
    u64 operator()(const CustomType & v) const
    {
        return qc::hash::fastHash::hash<u64>(v);
    }

    u64 operator()(const OtherCustomType & v) const
    {
        return qc::hash::fastHash::hash<u64>(v);
    }
};

TEST(heterogeneity, custom)
{
    static_assert(HeterogeneityCompiles<CustomType, OtherCustomType>);
    static_assert(HeterogeneityCompiles<CustomType, const OtherCustomType>);
    static_assert(!HeterogeneityCompiles<CustomType, u64>);
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
    struct alignas(1) Aligned1 { u8 vals[1u]; };
    struct alignas(2) Aligned2 { u8 vals[2u]; };
    struct alignas(4) Aligned4 { u8 vals[4u]; };
    struct alignas(8) Aligned8 { u8 vals[8u]; };
    struct alignas(16) Aligned16 { u8 vals[16u]; };

    struct alignas(1) Unaligned2 { u8 vals[2u]; };
    struct alignas(2) Unaligned4 { u8 vals[4u]; };
    struct alignas(4) Unaligned8 { u8 vals[8u]; };
    struct alignas(8) Unaligned16 { u8 vals[16u]; };

    static_assert(std::is_same_v<qc::hash::RawType<Aligned1>, u8>);
    static_assert(std::is_same_v<qc::hash::RawType<Aligned2>, u16>);
    static_assert(std::is_same_v<qc::hash::RawType<Aligned4>, u32>);
    static_assert(std::is_same_v<qc::hash::RawType<Aligned8>, u64>);
    static_assert(std::is_same_v<qc::hash::RawType<Aligned16>, qc::hash::UnsignedMulti<8u, 2u>>);

    static_assert(std::is_same_v<qc::hash::RawType<Unaligned2>, qc::hash::UnsignedMulti<1u, 2u>>);
    static_assert(std::is_same_v<qc::hash::RawType<Unaligned4>, qc::hash::UnsignedMulti<2u, 2u>>);
    static_assert(std::is_same_v<qc::hash::RawType<Unaligned8>, qc::hash::UnsignedMulti<4u, 2u>>);
    static_assert(std::is_same_v<qc::hash::RawType<Unaligned16>, qc::hash::UnsignedMulti<8u, 2u>>);
}

static void randomGeneralTest(const u64 size, const u64 iterations, qc::Random<u64> & random)
{
    [[maybe_unused]] static volatile u64 volatileKey{};

    std::vector<u64> keys{};
    keys.reserve(size);

    for (u64 it{0u}; it < iterations; ++it)
    {
        keys.clear();
        for (u64 i{}; i < size; ++i)
        {
            keys.push_back(random.next<u64>());
        }

        if (random.next<bool>()) keys[random.next<u64>(size)] = RawFriend::vacantKey<u64>;
        if (random.next<bool>()) keys[random.next<u64>(size)] = RawFriend::graveKey<u64>;

        RawSet<u64> s{};

        for (const u64 & key : keys)
        {
            ASSERT_TRUE(s.insert(key).second);
        }

        ASSERT_EQ(keys.size(), s.size());

        for (const u64 & key : keys)
        {
            ASSERT_TRUE(s.contains(key));
        }

        for (const u64 & key : s)
        {
            volatileKey = key;
        }

        std::shuffle(keys.begin(), keys.end(), random);

        for (u64 i{0u}; i < keys.size() / 2u; ++i)
        {
            ASSERT_TRUE(s.erase(keys[i]));
        }

        ASSERT_EQ(keys.size() / 2u, s.size());

        for (u64 i{0u}; i < keys.size() / 2u; ++i)
        {
            ASSERT_FALSE(s.erase(keys[i]));
        }

        for (u64 i{keys.size() / 2u}; i < keys.size(); ++i)
        {
            ASSERT_TRUE(s.erase(keys[i]));
        }

        ASSERT_EQ(0u, s.size());

        for (const u64 & key : keys)
        {
            ASSERT_TRUE(s.insert(key).second);
        }

        ASSERT_EQ(keys.size(), s.size());

        s.clear();

        ASSERT_EQ(0u, s.size());
    }
}

TEST(set, randomGeneralTests)
{
    qc::Random random{u64(std::chrono::steady_clock::now().time_since_epoch().count())};
    for (u64 size{10u}, iterations{10000u}; size <= 10000u; size *= 10u, iterations /= 10u)
    {
        randomGeneralTest(size, iterations, random);
    }
}
