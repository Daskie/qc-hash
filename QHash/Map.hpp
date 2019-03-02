//==============================================================================
// Map.hpp /////////////////////////////////////////////////////////////////////
//==============================================================================
// Austin Quick, 2016 - 2019
//------------------------------------------------------------------------------
// https://github.com/Daskie/QHash
// ...
//------------------------------------------------------------------------------



#pragma once



#include <memory>

#include "Hash.hpp"



#define QC_MAP Map<K, T, H, E, A>
#define QC_MAP_TEMPLATE template <typename K, typename T, typename H, typename E, typename A>



namespace qc {



namespace config {

    namespace map {

        constexpr unat minCapacity(16);
        constexpr unat minBucketCount(minCapacity * 2);
        constexpr bool useIdentityHash(true);

    }

    namespace set {

        using namespace map;

    }

}

namespace detail::map {

    template <typename K> using DefaultHash = std::conditional_t<config::map::useIdentityHash && sizeof(K) <= sizeof(nat), IdentityHash<K>, Hash<K>>;

    template <typename V> struct BucketBase {
        V & val() { return reinterpret_cast<V &>(*this); }
        const V & val() const { return reinterpret_cast<const V &>(*this); }
    };
    template <typename K, typename T> struct Types {
        using V = std::pair<K, T>;
        static constexpr int k_keyEnd = sizeof(K);
        static constexpr int k_memStart = (k_keyEnd + alignof(T) - 1) / alignof(T) * alignof(T);
        static constexpr int k_memEnd = k_memStart + sizeof(T);
        static constexpr int k_interSize = k_memStart - k_keyEnd;
        static constexpr int k_postSize = sizeof(V) - k_memEnd;
        static constexpr int k_maxSize = k_postSize >= k_interSize ? k_postSize : k_interSize;
        static constexpr int k_distSize = k_maxSize >= 8 ? 8 : k_maxSize >= 4 ? 4 : k_maxSize >= 2 ? 2 : k_maxSize >= 1 ? 1 : alignof(V);
        using Dist = hash::utype<k_distSize>;
        struct BucketInter : public BucketBase<V> { K key; Dist dist; T mem; };
        struct BucketPost  : public BucketBase<V> { K key; T mem; Dist dist; };
        using Bucket = std::conditional_t<k_postSize >= k_interSize, BucketPost, BucketInter>;
    };
    template <typename K> struct Types<K, void> {
        using V = K;
        using Dist = hash::utype<alignof(K)>;
        struct Bucket : public BucketBase<V> { K key; Dist dist; ~Bucket() = delete; };
    };

}

template <typename K, typename T, typename H = detail::map::DefaultHash<K>, typename E = std::equal_to<K>, typename A = std::allocator<typename detail::map::Types<K, T>::V>> class Map;
template <typename K, typename H = detail::map::DefaultHash<K>, typename E = std::equal_to<K>, typename A = std::allocator<K>> using Set = Map<K, void, H, E, A>;

// Map
//==============================================================================
// ...

QC_MAP_TEMPLATE
class Map {

    using V = typename detail::map::Types<K, T>::V;
    using Dist = typename detail::map::Types<K, T>::Dist;
    using Bucket = typename detail::map::Types<K, T>::Bucket;
    using Allocator = typename std::allocator_traits<A>::template rebind_alloc<Bucket>;
    using AllocatorTraits = std::allocator_traits<Allocator>;

    static_assert(std::is_move_constructible_v<V>, "Value type must be move constructable");
    static_assert(std::is_move_assignable_v<V>, "Value type must be move assignable");
    static_assert(std::is_swappable_v<V>, "Value type must be swappable");

    static constexpr bool k_isSet = std::is_same_v<T, void>;

    template <bool t_const> class Iterator;
    friend Iterator;

    ////////////////////////////////////////////////////////////////////////////

    public:

    // Types ===================================================================

    using key_type = K;
    using mapped_type = T;
    using value_type = V;
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

    // Methods =================================================================

    // Map
    //--------------------------------------------------------------------------
    //

    explicit Map(unat minCapacity = config::map::minCapacity, const H & hash = H(), const E & equal = E(), const A & alloc = A());
    Map(unat minCapacity, const A & alloc);
    Map(unat minCapacity, const H & hash, const A & alloc);
    explicit Map(const A & alloc);
    Map(const Map & other);
    Map(const Map & other, const A & alloc);
    Map(Map && other);
    Map(Map && other, const A & alloc);
    Map(Map && other, A && alloc);
    template <typename It> Map(It first, It last, unat minCapacity = 0, const H & hash = H(), const E & equal = E(), const A & alloc = A());
    template <typename It> Map(It first, It last, unat minCapacity, const A & alloc);
    template <typename It> Map(It first, It last, unat minCapacity, const H & hash, const A & alloc);
    Map(std::initializer_list<V> values, unat minCapacity = 0, const H & hash = H(), const E & equal = E(), const A & alloc = A());
    Map(std::initializer_list<V> values, unat minCapacity, const A & alloc);
    Map(std::initializer_list<V> values, unat minCapacity, const H & hash, const A & alloc);

    // ~Map
    //--------------------------------------------------------------------------
    // 

    ~Map();

    // operator=
    //--------------------------------------------------------------------------
    //

    Map & operator=(const Map & other);
    Map & operator=(Map && other) noexcept;
    Map & operator=(std::initializer_list<V> values);

    // begin
    //--------------------------------------------------------------------------
    // 

    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    const_iterator cbegin() const noexcept;
    template <bool t_const> Iterator<t_const> begin_private() const noexcept;

    // end
    //--------------------------------------------------------------------------
    // 

    iterator end() noexcept;
    const_iterator end() const noexcept;
    const_iterator cend() const noexcept;
    template <bool t_const> Iterator<t_const> end_private() const noexcept;

    // capacity
    //--------------------------------------------------------------------------
    // Equivalent to bucket_count() / 2

    unat capacity() const;
    
    // bucket_count
    //--------------------------------------------------------------------------
    // 

    unat bucket_count() const;
    
    // max_bucket_count
    //--------------------------------------------------------------------------
    // 

    unat max_bucket_count() const;
    
    // bucket_size
    //--------------------------------------------------------------------------
    // 

    unat bucket_size(unat i) const;
    
    // bucket_size
    //--------------------------------------------------------------------------
    // 

    unat bucket(const K & key) const;
    
    // empty
    //--------------------------------------------------------------------------
    // 

    bool empty() const noexcept;
    
    // size
    //--------------------------------------------------------------------------
    // 

    unat size() const noexcept;
    
    // max_size
    //--------------------------------------------------------------------------
    // 

    unat max_size() const;

    // clear
    //--------------------------------------------------------------------------
    // All elements are removed and destructed
    // Does not change capacity
    // Invalidates iterators

    void clear();

    // insert
    //--------------------------------------------------------------------------
    // ...
    // Invalidates iterators

    std::pair<iterator, bool> insert(const V & value);
    std::pair<iterator, bool> insert(V && value);
    template <typename V_, std::enable_if_t<std::is_constructible_v<V, V_ &&>, int> = 0> std::pair<iterator, bool> insert(V_ && value);
    template <typename It> void insert(It first, It last);
    void insert(std::initializer_list<V> values);

    // emplace
    //--------------------------------------------------------------------------
    // ...
    // Invalidates iterators

    std::pair<iterator, bool> emplace(const V & value);
    std::pair<iterator, bool> emplace(V && value);
    template <typename... Args> std::pair<iterator, bool> emplace(Args &&... args);

    // try_emplace
    //--------------------------------------------------------------------------
    // ...
    // Invalidates iterators

    template <typename... Args> std::pair<iterator, bool> try_emplace(const K & key, Args &&... args);
    template <typename... Args> std::pair<iterator, bool> try_emplace(K && key, Args &&... args);

    // erase
    //--------------------------------------------------------------------------
    // ...
    // Invalidates iterators

    unat erase(const K & key);
    // Always returns end iterator as erase can trigger a rehash
    iterator erase(const_iterator position);
    // Always returns end iterator as erase can trigger a rehash
    iterator erase(const_iterator first, const_iterator last);

    // swap
    //--------------------------------------------------------------------------
    // ...
    // Invalidates iterators

    void swap(Map & other) noexcept;

    // at
    //--------------------------------------------------------------------------
    // ...

    std::add_lvalue_reference_t<T> at(const K & key);
    std::add_lvalue_reference_t<const T> at(const K & key) const;

    // at
    //--------------------------------------------------------------------------
    // ...

    std::add_lvalue_reference_t<T> operator[](const K & key);
    std::add_lvalue_reference_t<T> operator[](K && key);

    // count
    //--------------------------------------------------------------------------
    // 

    unat count(const K & key) const;
    unat count(const K & key, unat hash) const;

    // find
    //--------------------------------------------------------------------------
    // 

    iterator find(const K & key);
    const_iterator find(const K & key) const;
    iterator find(const K & key, unat hash);
    const_iterator find(const K & key, unat hash) const;
    template <bool t_const> Iterator<t_const> find_private(const K & key, unat hash) const;

    // equal_range
    //--------------------------------------------------------------------------
    // 

    std::pair<iterator, iterator> equal_range(const K & key);
    std::pair<const_iterator, const_iterator> equal_range(const K & key) const;
    std::pair<iterator, iterator> equal_range(const K & key, unat hash);
    std::pair<const_iterator, const_iterator> equal_range(const K & key, unat hash) const;
    template <bool t_const> std::pair<Iterator<t_const>, Iterator<t_const>> equal_range_private(const K & key, unat hash) const;

    // contains
    //--------------------------------------------------------------------------
    // 

    bool contains(const K & key) const;
    bool contains(const K & key, unat hash) const;

    // load_factor
    //--------------------------------------------------------------------------
    // 

    float load_factor() const;

    // max_load_factor
    //--------------------------------------------------------------------------
    // 

    float max_load_factor() const;

    // rehash
    //--------------------------------------------------------------------------
    // Does a rehash such that the number of buckets is equal to the smallest
    // power of two greater than or equal to both `bucketCount` and the current
    // size.
    // Invalidates iterators

    void rehash(unat bucketCount);

    // reserve
    //--------------------------------------------------------------------------
    // Equivalent to rehash(2 * `capacity`)
    // Invalidates iterators

    void reserve(unat capacity);

    // hash_function
    //--------------------------------------------------------------------------
    // 

    hasher hash_function() const;

    // key_eq
    //--------------------------------------------------------------------------
    // 

    key_equal key_eq() const;

    // get_allocator
    //--------------------------------------------------------------------------
    // 

    allocator_type get_allocator() const;
    
    // Functions ===============================================================

    // operator==
    //--------------------------------------------------------------------------
    // Returns whether `s1` and `s2` have the same values

    QC_MAP_TEMPLATE friend bool operator==(const QC_MAP & s1, const QC_MAP & s2);

    // operator==
    //--------------------------------------------------------------------------
    // Returns whether `s1` and `s2` have different values

    QC_MAP_TEMPLATE friend bool operator!=(const QC_MAP & s1, const QC_MAP & s2);

    // swap
    //--------------------------------------------------------------------------
    // 

    friend void swap(Map & s1, Map & s2) noexcept;

    ////////////////////////////////////////////////////////////////////////////

    private:

    // Variables ===============================================================

    unat m_size;
    unat m_bucketCount;
    Bucket * m_buckets;
    H m_hash;
    E m_equal;
    Allocator m_alloc;

    // Methods =================================================================

    template <typename K_, typename... Args> std::pair<iterator, bool> try_emplace_private(unat hash, K_ && key, Args &&... args);

    void propagate(V & value, unat i, Dist dist);

    void erase_private(const_iterator position);

    unat detIndex(unat hash) const {
        return hash & (m_bucketCount - 1);
    }

    void rehash_private(unat bucketCount);

    void allocate();

    void deallocate();

    void zeroDists();

    template <bool t_zeroDists> void clear_private();

    void copyBuckets(const Bucket * bucket);

    void moveBuckets(Bucket * bucket);

    static constexpr const K & keyOf(const V & value) {
        if constexpr (k_isSet) {
            return value;
        }
        else {
            return value.first;
        }
    }

    template <bool t_const, typename Bucket_> static constexpr Iterator<t_const> makeIterator(Bucket_ * bucket) {
        return Iterator<t_const>(const_cast<std::conditional_t<t_const, const Bucket *, Bucket *>>(bucket));
    }

};



// Iterator
//==============================================================================
// Forward iterator

QC_MAP_TEMPLATE
template <bool t_const>
class QC_MAP::Iterator {

    friend Map;

    using V = std::conditional_t<t_const, const Map::V, Map::V>;
    using Dist = std::conditional_t<t_const, const Map::Dist, Map::Dist>;
    using Bucket = std::conditional_t<t_const, const Map::Bucket, Map::Bucket>;

    ////////////////////////////////////////////////////////////////////////////

    public:

    // Types ===================================================================

    using iterator_category = std::forward_iterator_tag;
    using value_type = V;
    using difference_type = ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

    // Iterator
    //--------------------------------------------------------------------------
    // 

    Iterator(const Iterator & other) noexcept = default;
    template <bool t_const_ = t_const, typename = std::enable_if_t<t_const_>>
    Iterator(const Iterator<!t_const> & other) noexcept;

    // ~Iterator
    //--------------------------------------------------------------------------
    // 

    ~Iterator() = default;

    // operator=
    //--------------------------------------------------------------------------
    // 

    //Iterator & operator=(const Iterator & other) noexcept = default;
    //template <bool t_const_ = t_const, typename = std::enable_if_t<t_const_>>
    //Iterator & operator=(const Iterator<!t_const> & other) noexcept;

    // operator++
    //--------------------------------------------------------------------------
    // 

    Iterator & operator++();

    // operator++ int
    //--------------------------------------------------------------------------
    // 

    Iterator operator++(int);

    // operator==
    //--------------------------------------------------------------------------
    // 

    template <bool t_const_> bool operator==(const Iterator<t_const_> & it) const;

    // operator!=
    //--------------------------------------------------------------------------
    // 

    template <bool t_const_> bool operator!=(const Iterator<t_const_> & it) const;

    // operator*
    //--------------------------------------------------------------------------
    // 

    value_type & operator*() const;

    // operator->
    //--------------------------------------------------------------------------
    // 

    value_type * operator->() const;

    ////////////////////////////////////////////////////////////////////////////

    private:

    // Variables ===============================================================

    Bucket * m_bucket;

    // Methods =================================================================

    Iterator(Bucket * bucket) noexcept;

};



}



#include "Map.tpp"