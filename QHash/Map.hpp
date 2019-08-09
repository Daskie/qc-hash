#pragma once

//==============================================================================
// QHash Map (version 2.0.1)
//------------------------------------------------------------------------------
// Austin Quick, 2016 - 2019
// https://github.com/Daskie/QHash
// ...
//------------------------------------------------------------------------------

#include "Hash.hpp"

#define QC_MAP Map<K, V, H, E, A>
#define QC_MAP_TEMPLATE template <typename K, typename V, typename H, typename E, typename A>

namespace qc {

namespace config::map {

    constexpr unat minCapacity(16u);
    constexpr unat minBucketCount(minCapacity * 2u);
    constexpr bool useIdentityHash(true);

}

namespace config::set { using namespace config::map; }

namespace detail::map { // Ignore me :)

    template <typename K> using DefaultHash = std::conditional_t<config::map::useIdentityHash && sizeof(K) <= sizeof(nat), IdentityHash<K>, Hash<K>>;

    template <typename T> struct BucketBase {
        T & entry() { return reinterpret_cast<T &>(*this); }
        const T & entry() const { return reinterpret_cast<const T &>(*this); }
    };
    template <typename K, typename V> struct Types {
        using T = std::pair<K, V>;
        static constexpr int k_keyEnd = sizeof(K);
        static constexpr int k_memStart = (k_keyEnd + alignof(V) - 1u) / alignof(V) * alignof(V);
        static constexpr int k_memEnd = k_memStart + sizeof(V);
        static constexpr int k_interSize = k_memStart - k_keyEnd;
        static constexpr int k_postSize = sizeof(T) - k_memEnd;
        static constexpr int k_maxSize = k_postSize >= k_interSize ? k_postSize : k_interSize;
        static constexpr int k_distSize = k_maxSize >= 8u ? 8u : k_maxSize >= 4u ? 4u : k_maxSize >= 2u ? 2u : k_maxSize >= 1u ? 1u : alignof(T);
        using Dist = hash::utype<k_distSize>;
        struct BucketInter : public BucketBase<T> { K key; Dist dist; V val; };
        struct BucketPost  : public BucketBase<T> { K key; V val; Dist dist; };
        using Bucket = std::conditional_t<k_postSize >= k_interSize, BucketPost, BucketInter>;
    };
    template <typename K> struct Types<K, void> {
        using T = K;
        using Dist = hash::utype<alignof(K)>;
        struct Bucket : public BucketBase<T> { K key; Dist dist; ~Bucket() = delete; };
    };

}

// Map
//==============================================================================
// ...

template <
    typename K,
    typename V,
    typename H = detail::map::DefaultHash<K>,
    typename E = std::equal_to<K>,
    typename A = std::allocator<typename detail::map::Types<K, V>::T>
>
class Map;

// Set
//==============================================================================
// ...
// Defined as a `Map` whose value type is `void`.

template <
    typename K,
    typename H = detail::map::DefaultHash<K>,
    typename E = std::equal_to<K>,
    typename A = std::allocator<K>
>
using Set = Map<K, void, H, E, A>;

// Map
//==============================================================================

QC_MAP_TEMPLATE
class Map {

    template <bool t_const> class Iterator;
    friend Iterator;

    using T = typename detail::map::Types<K, V>::T;
    using Dist = typename detail::map::Types<K, V>::Dist;
    using Bucket = typename detail::map::Types<K, V>::Bucket;
    using Allocator = typename std::allocator_traits<A>::template rebind_alloc<Bucket>;
    using AllocatorTraits = std::allocator_traits<Allocator>;

    static_assert(std::is_move_constructible_v<T>, "Value type must be move constructable");
    static_assert(std::is_move_assignable_v<T>, "Value type must be move assignable");
    static_assert(std::is_swappable_v<T>, "Value type must be swappable");

    static constexpr bool k_isSet = std::is_same_v<V, void>;

    ////////////////////////////////////////////////////////////////////////////

    public:

    //=== Public Types =========================================================

    using key_type = K;
    using mapped_type = V;
    using value_type = T;
    using hasher = H;
    using key_equal = E;
    using allocator_type = A;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = typename std::allocator_traits<A>::pointer;
    using const_pointer = typename std::allocator_traits<A>::const_pointer;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;
    
    //=== Public Functions =====================================================

    // operator==
    //--------------------------------------------------------------------------
    // Returns whether `s1` and `s2` have the same entries.

    QC_MAP_TEMPLATE friend bool operator==(const QC_MAP & s1, const QC_MAP & s2);

    // operator!=
    //--------------------------------------------------------------------------
    // Returns whether `s1` and `s2` have different entries.

    QC_MAP_TEMPLATE friend bool operator!=(const QC_MAP & s1, const QC_MAP & s2);

    // swap
    //--------------------------------------------------------------------------
    // Swaps the contents of `s1` and `s2`.

    friend void swap(Map & s1, Map & s2) noexcept;

    //=== Public Methods =======================================================

    // Map
    //--------------------------------------------------------------------------
    // Memory is not allocated until the first entry is inserted.

    explicit Map(unat minCapacity = config::map::minCapacity, const H & hash = H(), const E & equal = E(), const A & alloc = A());
    Map(unat minCapacity, const A & alloc);
    Map(unat minCapacity, const H & hash, const A & alloc);
    explicit Map(const A & alloc);
    template <typename It> Map(It first, It last, unat minCapacity = 0u, const H & hash = H(), const E & equal = E(), const A & alloc = A());
    template <typename It> Map(It first, It last, unat minCapacity, const A & alloc);
    template <typename It> Map(It first, It last, unat minCapacity, const H & hash, const A & alloc);
    Map(std::initializer_list<T> entries, unat minCapacity = 0u, const H & hash = H(), const E & equal = E(), const A & alloc = A());
    Map(std::initializer_list<T> entries, unat minCapacity, const A & alloc);
    Map(std::initializer_list<T> entries, unat minCapacity, const H & hash, const A & alloc);
    Map(const Map & other);
    Map(const Map & other, const A & alloc);
    Map(Map && other);
    Map(Map && other, const A & alloc);
    Map(Map && other, A && alloc);

    // ~Map
    //--------------------------------------------------------------------------
    // Destructs all entries and frees all memory allocated.

    ~Map();

    // operator=
    //--------------------------------------------------------------------------
    // ...

    Map & operator=(std::initializer_list<T> entries);
    Map & operator=(const Map & other);
    Map & operator=(Map && other) noexcept;

    // insert
    //--------------------------------------------------------------------------
    // Prefer try_emplace over emplace over this.
    // Invalidates iterators.

    std::pair<iterator, bool> insert(const T & entry);
    std::pair<iterator, bool> insert(T && entry);
    template <typename It> void insert(It first, It last);
    void insert(std::initializer_list<T> entries);

    // emplace
    //--------------------------------------------------------------------------
    // Prefer try_emplace over this, but prefer this over insert.
    // Invalidates iterators.

    std::pair<iterator, bool> emplace(const T & entry);
    std::pair<iterator, bool> emplace(T && entry);
    template <typename K_, typename V_> std::pair<iterator, bool> emplace(K_ && key, V_ && val);
    template <typename... KArgs, typename... VArgs> std::pair<iterator, bool> emplace(std::piecewise_construct_t, std::tuple<KArgs...> && kArgs, std::tuple<VArgs...> && vArgs);

    // try_emplace
    //--------------------------------------------------------------------------
    // If there is no existing entry for `key`, creates a new entry in place.
    // Choose this as the default insertion method of choice.
    // Invalidates iterators.

    template <typename... VArgs> std::pair<iterator, bool> try_emplace(const K & key, VArgs &&... valArgs);
    template <typename... VArgs> std::pair<iterator, bool> try_emplace(K && key, VArgs &&... valArgs);

    // erase
    //--------------------------------------------------------------------------
    // The variations that return iterators always return the end iterator, as
    // this method can trigger a rehash.
    // Invalidates iterators.

    unat erase(const K & key);
    iterator erase(const_iterator position);
    iterator erase(const_iterator first, const_iterator last);

    // clear
    //--------------------------------------------------------------------------
    // All elements are removed and destructed.
    // Does not change capacity or free memory.
    // Invalidates iterators.

    void clear();

    // contains
    //--------------------------------------------------------------------------
    // Returns whether or not the map contains an entry for `key`.

    bool contains(const K & key) const;
    bool contains(const K & key, unat hash) const;

    // count
    //--------------------------------------------------------------------------
    // Returns `1` if the map contains an entry for `key` and `0` if it does not.

    unat count(const K & key) const;
    unat count(const K & key, unat hash) const;

    // at
    //--------------------------------------------------------------------------
    // ...

    std::add_lvalue_reference_t<V> at(const K & key);
    std::add_lvalue_reference_t<const V> at(const K & key) const;

    // operator[]
    //--------------------------------------------------------------------------
    // ...

    std::add_lvalue_reference_t<V> operator[](const K & key);
    std::add_lvalue_reference_t<V> operator[](K && key);

    // begin
    //--------------------------------------------------------------------------
    // Returns an iterator to the first entry in the map.

    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    const_iterator cbegin() const noexcept;

    // end
    //--------------------------------------------------------------------------
    // Returns an iterator to one-past the end of the map.

    iterator end() noexcept;
    const_iterator end() const noexcept;
    const_iterator cend() const noexcept;

    // find
    //--------------------------------------------------------------------------
    // Returns an iterator to the entry for `key`, or the end iterator if no
    // such entry exists.

    iterator find(const K & key);
    const_iterator find(const K & key) const;
    iterator find(const K & key, unat hash);
    const_iterator find(const K & key, unat hash) const;

    // equal_range
    //--------------------------------------------------------------------------
    // As a key may correspont to as most one entry, this method is equivalent
    // to `find`, except returning a pair of duplicate iterators.

    std::pair<iterator, iterator> equal_range(const K & key);
    std::pair<const_iterator, const_iterator> equal_range(const K & key) const;
    std::pair<iterator, iterator> equal_range(const K & key, unat hash);
    std::pair<const_iterator, const_iterator> equal_range(const K & key, unat hash) const;

    // reserve
    //--------------------------------------------------------------------------
    // Ensures the map is large enough to hold `capacity` entries without
    // rehashing.
    // Equivalent to `rehash(2 * capacity)`.
    // Invalidates iterators.

    void reserve(unat capacity);

    // rehash
    //--------------------------------------------------------------------------
    // Ensures the number of buckets is equal to the smallest power of two
    // greater than or equal to both `bucketCount` and the current size.
    // Equivalent to `reserve(bucketCount / 2)`.
    // Invalidates iterators.

    void rehash(unat bucketCount);

    // swap
    //--------------------------------------------------------------------------
    // Swaps the contents of this map and `other`'s
    // Invalidates iterators

    void swap(Map & other) noexcept;

    // empty
    //--------------------------------------------------------------------------
    // Returns whether or not the map is empty.

    bool empty() const noexcept;

    // size
    //--------------------------------------------------------------------------
    // Returns the number of entries in the map.

    unat size() const noexcept;

    // max_size
    //--------------------------------------------------------------------------
    // Equivalent to `max_bucket_count() * 2`

    unat max_size() const;

    // capacity
    //--------------------------------------------------------------------------
    // Equivalent to `bucket_count() / 2`.

    unat capacity() const;

    // bucket_count
    //--------------------------------------------------------------------------
    // Will always be at least twice the number of entries.
    // Equivalent to `capacity() * 2`.

    unat bucket_count() const;

    // max_bucket_count
    //--------------------------------------------------------------------------
    // Equivalent to `max_size() / 2`.

    unat max_bucket_count() const;

    // bucket
    //--------------------------------------------------------------------------
    // Returns the index of the bucket into which `key` would fall.

    unat bucket(const K & key) const;

    // bucket_size
    //--------------------------------------------------------------------------
    // How many entries are "in" the bucket at index `i`.

    unat bucket_size(unat i) const;

    // load_factor
    //--------------------------------------------------------------------------
    // Returns the ratio of entries to buckets.
    // Equivalent to `float(m_size) / float(m_bucketCount)`.

    float load_factor() const;

    // max_load_factor
    //--------------------------------------------------------------------------
    // Is always `0.5f`.

    float max_load_factor() const;

    // hash_function
    //--------------------------------------------------------------------------
    // Returns an instance of `H` or `hasher`.

    hasher hash_function() const;

    // key_eq
    //--------------------------------------------------------------------------
    // Returns an instance of `E` or `key_equal`.

    key_equal key_eq() const;

    // get_allocator
    //--------------------------------------------------------------------------
    // Returns an instance of `A` or `allocator_type`.

    allocator_type get_allocator() const;

    ////////////////////////////////////////////////////////////////////////////

    private:

    //=== Private Variables ====================================================

    unat m_size;
    unat m_bucketCount;
    Bucket * m_buckets;
    H m_hash;
    E m_equal;
    Allocator m_alloc;

    //=== Private Methods ======================================================

    template <typename KTuple, typename VTuple, unat... t_kIndices, unat... t_vIndices>
    std::pair<iterator, bool> m_emplace(KTuple && kTuple, VTuple && vTuple, std::index_sequence<t_kIndices...>, std::index_sequence<t_vIndices...>);

    template <typename K_, typename... Args> std::pair<iterator, bool> m_try_emplace(unat hash, K_ && key, Args &&... args);

    void m_propagate(T & entry, unat i, Dist dist);

    void m_erase(const_iterator position);

    template <bool t_zeroDists> void m_clear();

    template <bool t_const> std::pair<Iterator<t_const>, Iterator<t_const>> m_equal_range(const K & key, unat hash) const;

    template <bool t_const> Iterator<t_const> m_begin() const noexcept;

    template <bool t_const> Iterator<t_const> m_end() const noexcept;

    template <bool t_const> Iterator<t_const> m_find(const K & key, unat hash) const;

    void m_rehash(unat bucketCount);

    unat m_indexOf(unat hash) const;

    void m_allocate();

    void m_deallocate();

    void m_zeroDists();

    void m_copyBuckets(const Bucket * bucket);

    void m_moveBuckets(Bucket * bucket);

};

// Iterator
//==============================================================================
// Forward iterator

QC_MAP_TEMPLATE
template <bool t_const>
class QC_MAP::Iterator {

    friend Map;

    public:

    //=== Public Types =========================================================

    using iterator_category = std::forward_iterator_tag;
    using value_type = std::conditional_t<t_const, const Map::T, Map::T>;
    using difference_type = ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

    //=== Public Methods =======================================================

    // Iterator
    //--------------------------------------------------------------------------
    // ...

    constexpr Iterator(const Iterator & other) noexcept = default;
    template <bool t_const_ = t_const, typename = std::enable_if_t<t_const_>>
    constexpr Iterator(const Iterator<!t_const> & other) noexcept;

    // ~Iterator
    //--------------------------------------------------------------------------
    // ...

    ~Iterator() = default;

    // operator*
    //--------------------------------------------------------------------------
    // ...

    value_type & operator*() const;

    // operator->
    //--------------------------------------------------------------------------
    // ...

    value_type * operator->() const;

    // operator++
    //--------------------------------------------------------------------------
    // Incrementing past the end iterator is undefined and unsupported behavior.

    Iterator & operator++();

    // operator++ int
    //--------------------------------------------------------------------------
    // Incrementing past the end iterator is undefined and unsupported behavior.

    Iterator operator++(int);

    // operator==
    //--------------------------------------------------------------------------
    // ...

    template <bool t_const_> bool operator==(const Iterator<t_const_> & it) const;

    // operator!=
    //--------------------------------------------------------------------------
    // ...

    template <bool t_const_> bool operator!=(const Iterator<t_const_> & it) const;

    ////////////////////////////////////////////////////////////////////////////

    private:

    //=== Private Types ========================================================

    using Bucket = std::conditional_t<t_const, const Map::Bucket, Map::Bucket>;

    //=== Private Variables ====================================================

    Bucket * m_bucket;

    //=== Private Methods ======================================================

    template <typename Bucket_> constexpr explicit Iterator(Bucket_ * bucket) noexcept;

};

}

#include "Map.tpp"
