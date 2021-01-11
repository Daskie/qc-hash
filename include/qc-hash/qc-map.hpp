#pragma once

//
// QC Hash 2.2.1
//
// Austin Quick, 2016 - 2020
// https://github.com/Daskie/qc-hash
// ...
//

#include <algorithm>
#include <bit>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "qc-hash.hpp"

namespace qc_hash {

    namespace config {

        constexpr size_t minCapacity{16u}; // Must be at least `sizeof(size_t) / 2`
        constexpr size_t minBucketCount{minCapacity * 2u};
        constexpr bool useIdentityHash{true};

    }

    template <typename K> struct _DefaultHashHelper { using type = Hash<K>; };
    template <typename K> requires (config::useIdentityHash && sizeof(K) <= sizeof(size_t)) struct _DefaultHashHelper<K> { using type = IdentityHash<K>; };
    template <typename K> using _DefaultHash = typename _DefaultHashHelper<K>::type;

    template <typename V> struct _BucketBase {
        V & entry() noexcept { return reinterpret_cast<V &>(*this); }
        const V & entry() const noexcept { return reinterpret_cast<const V &>(*this); }
    };

    template <typename K, typename T> struct _Types {
        using V = std::pair<K, T>;
        static constexpr int keyEnd = sizeof(K);
        static constexpr int memStart = (keyEnd + alignof(T) - 1u) / alignof(T) * alignof(T);
        static constexpr int memEnd = memStart + sizeof(T);
        static constexpr int interSize = memStart - keyEnd;
        static constexpr int postSize = sizeof(V) - memEnd;
        static constexpr int maxSize = postSize >= interSize ? postSize : interSize;
        static constexpr int distSize = maxSize >= 8u ? 8u : maxSize >= 4u ? 4u : maxSize >= 2u ? 2u : maxSize >= 1u ? 1u : alignof(V);
        using Dist = _utype<distSize>;
        struct BucketInter : public _BucketBase<V> { K key; Dist dist; T val; };
        struct BucketPost  : public _BucketBase<V> { K key; T val; Dist dist; };
        using Bucket = std::conditional_t<postSize >= interSize, BucketPost, BucketInter>;
    };

    template <typename K> struct _Types<K, void> {
        using V = K;
        using Dist = _utype<alignof(K)>;
        struct Bucket : public _BucketBase<V> { K key; Dist dist; ~Bucket() = delete; };
    };

    //
    // ...
    //
    template <typename K, typename T, typename H = _DefaultHash<K>, typename E = std::equal_to<K>, typename A = std::allocator<typename _Types<K, T>::V>> class Map;

    //
    // ...
    // Defined as a `Map` whose mapped type is `void`.
    //
    template <typename K, typename H = _DefaultHash<K>, typename E = std::equal_to<K>, typename A = std::allocator<K>> using Set = Map<K, void, H, E, A>;

    template <typename K, typename T, typename H, typename E, typename A> class Map {

        using V = typename _Types<K, T>::V;

        template <bool constant> class _Iterator;
        template <bool constant> friend class _Iterator;

        using _Dist = typename _Types<K, T>::Dist;
        using _Bucket = typename _Types<K, T>::Bucket;
        using _Allocator = typename std::allocator_traits<A>::template rebind_alloc<_Bucket>;
        using _AllocatorTraits = std::allocator_traits<_Allocator>;

        public: //--------------------------------------------------------------

        using key_type = K;
        using mapped_type = T;
        using value_type = V;
        using hasher = H;
        using key_equal = E;
        using allocator_type = A;
        using reference = V &;
        using const_reference = const V &;
        using pointer = typename std::allocator_traits<A>::pointer;
        using const_pointer = typename std::allocator_traits<A>::const_pointer;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        using iterator = _Iterator<false>;
        using const_iterator = _Iterator<true>;

        static_assert(std::is_nothrow_move_constructible_v<V>);
        static_assert(std::is_nothrow_move_assignable_v<V>);
        static_assert(std::is_nothrow_swappable_v<V>);
        static_assert(std::is_nothrow_destructible_v<V>);
        static_assert(std::is_nothrow_default_constructible_v<H>);
        static_assert(std::is_nothrow_move_constructible_v<H>);
        static_assert(std::is_nothrow_move_assignable_v<H>);
        static_assert(std::is_nothrow_swappable_v<H>);
        static_assert(std::is_nothrow_destructible_v<H>);
        static_assert(std::is_nothrow_default_constructible_v<E>);
        static_assert(std::is_nothrow_move_constructible_v<E>);
        static_assert(std::is_nothrow_move_assignable_v<E>);
        static_assert(std::is_nothrow_swappable_v<E>);
        static_assert(std::is_nothrow_destructible_v<E>);
        static_assert(std::is_nothrow_default_constructible_v<A>);
        static_assert(std::is_nothrow_move_constructible_v<A>);
        static_assert(std::is_nothrow_move_assignable_v<A> || !_AllocatorTraits::propagate_on_container_move_assignment::value);
        static_assert(std::is_nothrow_swappable_v<A> || !_AllocatorTraits::propagate_on_container_swap::value);
        static_assert(std::is_nothrow_destructible_v<A>);

        //
        // Memory is not allocated until the first entry is inserted.
        //
        explicit Map(size_t minCapacity = config::minCapacity, const H & hash = {}, const E & equal = {}, const A & alloc = {}) noexcept;
        Map(size_t minCapacity, const A & alloc) noexcept;
        Map(size_t minCapacity, const H & hash, const A & alloc) noexcept;
        explicit Map(const A & alloc) noexcept;
        template <typename It> Map(It first, It last, size_t minCapacity = 0u, const H & hash = {}, const E & equal = {}, const A & alloc = {});
        template <typename It> Map(It first, It last, size_t minCapacity, const A & alloc);
        template <typename It> Map(It first, It last, size_t minCapacity, const H & hash, const A & alloc);
        Map(std::initializer_list<V> entries, size_t minCapacity = 0u, const H & hash = {}, const E & equal = {}, const A & alloc = {});
        Map(std::initializer_list<V> entries, size_t minCapacity, const A & alloc);
        Map(std::initializer_list<V> entries, size_t minCapacity, const H & hash, const A & alloc);
        Map(const Map & other);
        Map(const Map & other, const A & alloc);
        Map(Map && other) noexcept;
        Map(Map && other, const A & alloc) noexcept;
        Map(Map && other, A && alloc) noexcept;

        //
        // ...
        //
        Map & operator=(std::initializer_list<V> entries);
        Map & operator=(const Map & other);
        Map & operator=(Map && other) noexcept;

        //
        // Destructs all entries and frees all memory allocated.
        //
        ~Map() noexcept;

        //
        // Prefer try_emplace over emplace over this.
        // Invalidates iterators.
        //
        std::pair<iterator, bool> insert(const V & entry);
        std::pair<iterator, bool> insert(V && entry);
        template <typename It> void insert(It first, It last);
        void insert(std::initializer_list<V> entries);

        //
        // Prefer try_emplace over this, but prefer this over insert.
        // Invalidates iterators.
        //
        std::pair<iterator, bool> emplace(const V & entry);
        std::pair<iterator, bool> emplace(V && entry);
        template <typename K_, typename T_> std::pair<iterator, bool> emplace(K_ && key, T_ && val) requires (!std::is_same_v<T, void>);
        template <typename... KArgs, typename... TArgs> std::pair<iterator, bool> emplace(std::piecewise_construct_t, std::tuple<KArgs...> && kArgs, std::tuple<TArgs...> && tArgs) requires (!std::is_same_v<T, void>);

        //
        // If there is no existing entry for `key`, creates a new entry in
        // place.
        // Choose this as the default insertion method of choice.
        // Invalidates iterators.
        //
        template <typename... TArgs> std::pair<iterator, bool> try_emplace(const K & key, TArgs &&... valArgs);
        template <typename... TArgs> std::pair<iterator, bool> try_emplace(K && key, TArgs &&... valArgs);

        //
        // The variations that return iterators always return the end iterator,
        // as this method can trigger a rehash.
        // Invalidates iterators.
        //
        size_t erase(const K & key);
        iterator erase(const_iterator position);
        iterator erase(const_iterator first, const_iterator last);

        //
        // All elements are removed and destructed.
        // Does not change capacity or free memory.
        // Invalidates iterators.
        //
        void clear() noexcept;

        //
        // Returns whether or not the map contains an entry for `key`.
        //
        bool contains(const K & key) const;
        bool contains(const K & key, size_t hash) const;

        //
        // Returns `1` if the map contains an entry for `key` and `0` if it does
        // not.
        //
        size_t count(const K & key) const;
        size_t count(const K & key, size_t hash) const;

        //
        // ...
        //
        std::add_lvalue_reference_t<T> at(const K & key) requires (!std::is_same_v<T, void>);
        std::add_lvalue_reference_t<const T> at(const K & key) const requires (!std::is_same_v<T, void>);

        //
        // ...
        // TODO: Use requires once MSVC gets its shit together
        //
        std::add_lvalue_reference_t<T> operator[](const K & key);
        std::add_lvalue_reference_t<T> operator[](K && key);

        //
        // Returns an iterator to the first entry in the map.
        //
        iterator begin() noexcept;
        const_iterator begin() const noexcept;
        const_iterator cbegin() const noexcept;

        //
        // Returns an iterator to one-past the end of the map.
        //
        iterator end() noexcept;
        const_iterator end() const noexcept;
        const_iterator cend() const noexcept;

        //
        // Returns an iterator to the entry for `key`, or the end iterator if no
        // such entry exists.
        //
        iterator find(const K & key);
        const_iterator find(const K & key) const;
        iterator find(const K & key, size_t hash);
        const_iterator find(const K & key, size_t hash) const;

        //
        // As a key may correspond to as most one entry, this method is
        // equivalent to `find`, except returning a pair of duplicate iterators.
        //
        std::pair<iterator, iterator> equal_range(const K & key);
        std::pair<const_iterator, const_iterator> equal_range(const K & key) const;
        std::pair<iterator, iterator> equal_range(const K & key, size_t hash);
        std::pair<const_iterator, const_iterator> equal_range(const K & key, size_t hash) const;

        //
        // Ensures the map is large enough to hold `capacity` entries without
        // rehashing.
        // Equivalent to `rehash(2 * capacity)`.
        // Invalidates iterators.
        //
        void reserve(size_t capacity);

        //
        // Ensures the number of buckets is equal to the smallest power of two
        // greater than or equal to both `bucketCount` and the current size.
        // Equivalent to `reserve(bucketCount / 2)`.
        // Invalidates iterators.
        //
        void rehash(size_t bucketCount);

        //
        // Swaps the contents of this map and `other`'s
        // Invalidates iterators
        //
        void swap(Map & other) noexcept;

        //
        // Returns whether or not the map is empty.
        //
        bool empty() const noexcept;

        //
        // Returns the number of entries in the map.
        //
        size_t size() const noexcept;

        //
        // Equivalent to `max_bucket_count() * 2`
        //
        size_t max_size() const noexcept;

        //
        // Equivalent to `bucket_count() / 2`.
        //
        size_t capacity() const noexcept;

        //
        // Will always be at least twice the number of entries.
        // Equivalent to `capacity() * 2`.
        //
        size_t bucket_count() const noexcept;

        //
        // Equivalent to `max_size() / 2`.
        //
        size_t max_bucket_count() const noexcept;

        //
        // Returns the index of the bucket into which `key` would fall.
        //
        size_t bucket(const K & key) const;

        //
        // How many entries are "in" the bucket at index `i`.
        //
        size_t bucket_size(size_t i) const noexcept;

        //
        // Returns the ratio of entries to buckets.
        // Equivalent to `float(_size) / float(_bucketCount)`.
        //
        float load_factor() const noexcept;

        //
        // Is always `0.5f`.
        //
        float max_load_factor() const noexcept;

        //
        // Returns an instance of `hasher`.
        //
        H hash_function() const noexcept;

        //
        // Returns an instance of `key_equal`.
        //
        E key_eq() const noexcept;

        //
        // Returns an instance of `allocator_type`.
        //
        A get_allocator() const noexcept;

        private: //-------------------------------------------------------------

        static constexpr bool _isSet{std::is_same_v<T, void>};

        size_t _size;
        size_t _bucketCount;
        _Bucket * _buckets;
        H _hash;
        E _equal;
        _Allocator _alloc;

        template <typename KTuple, typename VTuple, size_t... kIndices, size_t... vIndices> std::pair<iterator, bool> _emplace(KTuple && kTuple, VTuple && vTuple, std::index_sequence<kIndices...>, std::index_sequence<vIndices...>);

        template <typename K_, typename... Args> std::pair<iterator, bool> _try_emplace(size_t hash, K_ && key, Args &&... args);

        void _propagate(V & entry, size_t i, _Dist dist);

        void _erase(const_iterator position);

        template <bool zeroDists> void _clear() noexcept;

        template <bool constant> std::pair<_Iterator<constant>, _Iterator<constant>> _equal_range(const K & key, size_t hash) const;

        template <bool constant> _Iterator<constant> _begin() const noexcept;

        template <bool constant> _Iterator<constant> _end() const noexcept;

        template <bool constant> _Iterator<constant> _find(const K & key, size_t hash) const;

        void _rehash(size_t bucketCount);

        size_t _indexOf(size_t hash) const noexcept;

        void _allocate();

        void _deallocate();

        void _zeroDists() noexcept;

        void _copyBuckets(const _Bucket * bucket);

        void _moveBuckets(_Bucket * bucket);

    };

    template <typename K, typename T, typename H, typename E, typename A> bool operator==(const Map<K, T, H, E, A> & m1, const Map<K, T, H, E, A> & m2);

    //
    // Forward iterator
    //
    template <typename K, typename T, typename H, typename E, typename A>
    template <bool constant>
    class Map<K, T, H, E, A>::_Iterator {

        friend Map;

        using V = std::conditional_t<constant, const Map::V, Map::V>;

        public: //--------------------------------------------------------------

        using iterator_category = std::forward_iterator_tag;
        using value_type = V;
        using difference_type = ptrdiff_t;
        using pointer = V *;
        using reference = V &;

        //
        // ...
        //
        constexpr _Iterator(const _Iterator & other) noexcept = default;
        template <bool constant_> requires (constant && !constant_) constexpr _Iterator(const _Iterator<constant_> & other) noexcept;

        //
        // ...
        //
        V & operator*() const noexcept;

        //
        // ...
        //
        V * operator->() const noexcept;

        //
        // Incrementing past the end iterator is undefined and unsupported behavior.
        //
        _Iterator & operator++() noexcept;

        //
        // Incrementing past the end iterator is undefined and unsupported behavior.
        //
        _Iterator operator++(int) noexcept;

        //
        // ...
        //
        template <bool constant_> bool operator==(const _Iterator<constant_> & it) const noexcept;

        private: //-------------------------------------------------------------

        using _Bucket = std::conditional_t<constant, const Map::_Bucket, Map::_Bucket>;

        _Bucket * _bucket;

        template <typename _Bucket_> constexpr explicit _Iterator(_Bucket_ * bucket) noexcept;

    };

}

// INLINE IMPLEMENTATION ///////////////////////////////////////////////////////////////////////////////////////////////

// TODO: Remove once MSVC has this
#ifdef _MSC_VER
namespace std {

    template <typename T> requires (::std::is_unsigned_v<T>)
    inline constexpr T bit_ceil(T v) {
        --v;
                                       v |= v >>  1;
                                       v |= v >>  2;
                                       v |= v >>  4;
        if constexpr (sizeof(T) >= 2u) v |= v >>  8;
        if constexpr (sizeof(T) >= 4u) v |= v >> 16;
        if constexpr (sizeof(T) >= 8u) v |= v >> 32;
        return ++v;
    }

}
#endif

namespace qc_hash {

    // Map =====================================================================

    template <typename K, typename T, typename H, typename E, typename A>
    inline Map<K, T, H, E, A>::Map(const size_t minCapacity, const H & hash, const E & equal, const A & alloc) noexcept:
        _size(),
        _bucketCount(minCapacity <= config::minCapacity ? config::minBucketCount : std::bit_ceil(minCapacity << 1)),
        _buckets(nullptr),
        _hash(hash),
        _equal(equal),
        _alloc(alloc)
    {}

    template <typename K, typename T, typename H, typename E, typename A>
    inline Map<K, T, H, E, A>::Map(const size_t minCapacity, const A & alloc) noexcept :
        Map(minCapacity, H{}, E{}, alloc)
    {}

    template <typename K, typename T, typename H, typename E, typename A>
    inline Map<K, T, H, E, A>::Map(const size_t minCapacity, const H & hash, const A & alloc) noexcept :
        Map(minCapacity, hash, E{}, alloc)
    {}

    template <typename K, typename T, typename H, typename E, typename A>
    inline Map<K, T, H, E, A>::Map(const A & alloc) noexcept :
        Map(config::minCapacity, H{}, E{}, alloc)
    {}

    template <typename K, typename T, typename H, typename E, typename A>
    template <typename It>
    inline Map<K, T, H, E, A>::Map(const It first, const It last, const size_t minCapacity, const H & hash, const E & equal, const A & alloc) :
        Map(minCapacity ? minCapacity : std::distance(first, last), hash, equal, alloc)
    {
        insert(first, last);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    template <typename It>
    inline Map<K, T, H, E, A>::Map(const It first, const It last, const size_t minCapacity, const A & alloc) :
        Map(first, last, minCapacity, H{}, E{}, alloc)
    {}

    template <typename K, typename T, typename H, typename E, typename A>
    template <typename It>
    inline Map<K, T, H, E, A>::Map(const It first, const It last, const size_t minCapacity, const H & hash, const A & alloc) :
        Map(first, last, minCapacity, hash, E{}, alloc)
    {}

    template <typename K, typename T, typename H, typename E, typename A>
    inline Map<K, T, H, E, A>::Map(std::initializer_list<V> entries, size_t minCapacity, const H & hash, const E & equal, const A & alloc) :
        Map(minCapacity ? minCapacity : entries.size(), hash, equal, alloc)
    {
        insert(entries);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline Map<K, T, H, E, A>::Map(const std::initializer_list<V> entries, const size_t minCapacity, const A & alloc) :
        Map(entries, minCapacity, H{}, E{}, alloc)
    {}

    template <typename K, typename T, typename H, typename E, typename A>
    inline Map<K, T, H, E, A>::Map(const std::initializer_list<V> entries, const size_t minCapacity, const H & hash, const A & alloc) :
        Map(entries, minCapacity, hash, E{}, alloc)
    {}

    template <typename K, typename T, typename H, typename E, typename A>
    inline Map<K, T, H, E, A>::Map(const Map & other) :
        Map(other, std::allocator_traits<A>::select_on_container_copy_construction(other._alloc))
    {}

    template <typename K, typename T, typename H, typename E, typename A>
    inline Map<K, T, H, E, A>::Map(const Map & other, const A & alloc) :
        _size(other._size),
        _bucketCount(other._bucketCount),
        _buckets(nullptr),
        _hash(other._hash),
        _equal(other._equal),
        _alloc(alloc)
    {
        _allocate();
        _copyBuckets(other._buckets);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline Map<K, T, H, E, A>::Map(Map && other) noexcept :
        Map(std::move(other), std::move(other._alloc))
    {}

    template <typename K, typename T, typename H, typename E, typename A>
    inline Map<K, T, H, E, A>::Map(Map && other, const A & alloc) noexcept :
        Map(std::move(other), A(alloc))
    {}

    template <typename K, typename T, typename H, typename E, typename A>
    inline Map<K, T, H, E, A>::Map(Map && other, A && alloc) noexcept :
        _size(std::exchange(other._size, 0u)),
        _bucketCount(std::exchange(other._bucketCount, 0u)),
        _buckets(std::exchange(other._buckets, nullptr)),
        _hash(std::move(other._hash)),
        _equal(std::move(other._equal)),
        _alloc(std::move(alloc))
    {}

    template <typename K, typename T, typename H, typename E, typename A>
    inline Map<K, T, H, E, A> & Map<K, T, H, E, A>::operator=(const std::initializer_list<V> entries) {
        clear();
        insert(entries);

        return *this;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline Map<K, T, H, E, A> & Map<K, T, H, E, A>::operator=(const Map & other) {
        if (&other == this) {
            return *this;
        }

        if (_buckets) {
            _clear<false>();
            if (_bucketCount != other._bucketCount || _alloc != other._alloc) {
                _deallocate();
            }
        }

        _size = other._size;
        _bucketCount = other._bucketCount;
        _hash = other._hash;
        _equal = other._equal;
        if constexpr (_AllocatorTraits::propagate_on_container_copy_assignment::value) {
            _alloc = std::allocator_traits<A>::select_on_container_copy_construction(other._alloc);
        }

        if (other._buckets) {
            if (!_buckets) {
                _allocate();
            }
            _copyBuckets(other._buckets);
        }

        return *this;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline Map<K, T, H, E, A> & Map<K, T, H, E, A>::operator=(Map && other) noexcept {
        if (&other == this) {
            return *this;
        }

        if (_buckets) {
            _clear<false>();
            _deallocate();
        }

        _size = other._size;
        _bucketCount = other._bucketCount;
        _hash = std::move(other._hash);
        _equal = std::move(other._equal);
        if constexpr (_AllocatorTraits::propagate_on_container_move_assignment::value) {
            _alloc = std::move(other._alloc);
        }

        if (_AllocatorTraits::propagate_on_container_move_assignment::value || _alloc == other._alloc) {
            _buckets = other._buckets;
            other._buckets = nullptr;
        }
        else {
            _allocate();
            _moveBuckets(other._buckets);
            other._clear<false>();
            other._deallocate();
        }

        other._size = 0u;
        other._bucketCount = 0u;

        return *this;
    }

    template <typename K, typename T, typename H, typename E, typename A>
        inline Map<K, T, H, E, A>::~Map() noexcept {
        if (_buckets) {
            _clear<false>();
            _deallocate();
        }
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::insert(const V & entry) -> std::pair<iterator, bool> {
        return emplace(entry);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::insert(V && entry) -> std::pair<iterator, bool> {
        return emplace(std::move(entry));
    }

    template <typename K, typename T, typename H, typename E, typename A>
    template <typename It>
    inline void Map<K, T, H, E, A>::insert(It first, const It last) {
        while (first != last) {
            emplace(*first);
            ++first;
        }
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline void Map<K, T, H, E, A>::insert(const std::initializer_list<V> entries) {
        for (const V & entry : entries) {
            emplace(entry);
        }
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::emplace(const V & entry) -> std::pair<iterator, bool> {
        if constexpr (_isSet) {
            return try_emplace(entry);
        }
        else {
            return try_emplace(entry.first, entry.second);
        }
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::emplace(V && entry) -> std::pair<iterator, bool> {
        if constexpr (_isSet) {
            return try_emplace(std::move(entry));
        }
        else {
            return try_emplace(std::move(entry.first), std::move(entry.second));
        }
    }

    template <typename K, typename T, typename H, typename E, typename A>
    template <typename K_, typename T_>
    inline auto Map<K, T, H, E, A>::emplace(K_ && key, T_ && val) -> std::pair<iterator, bool> requires (!std::is_same_v<T, void>) {
        return try_emplace(std::forward<K_>(key), std::forward<T_>(val));
    }

    template <typename K, typename T, typename H, typename E, typename A>
    template <typename... KArgs, typename... TArgs>
    inline auto Map<K, T, H, E, A>::emplace(const std::piecewise_construct_t, std::tuple<KArgs...> && kArgs, std::tuple<TArgs...> && tArgs) -> std::pair<iterator, bool> requires (!std::is_same_v<T, void>) {
        return _emplace(std::move(kArgs), std::move(tArgs), std::index_sequence_for<KArgs...>(), std::index_sequence_for<TArgs...>());
    }

    template <typename K, typename T, typename H, typename E, typename A>
    template <typename KTuple, typename VTuple, size_t... kIndices, size_t... vIndices>
    inline auto Map<K, T, H, E, A>::_emplace(KTuple && kTuple, VTuple && vTuple, const std::index_sequence<kIndices...>, const std::index_sequence<vIndices...>) -> std::pair<iterator, bool> {
        return try_emplace(K(std::move(std::get<kIndices>(kTuple))...), std::move(std::get<vIndices>(vTuple))...);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    template <typename... TArgs>
    inline auto Map<K, T, H, E, A>::try_emplace(const K & key, TArgs &&... valArgs) -> std::pair<iterator, bool> {
        return _try_emplace(_hash(key), key, std::forward<TArgs>(valArgs)...);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    template <typename... TArgs>
    inline auto Map<K, T, H, E, A>::try_emplace(K && key, TArgs &&... valArgs) -> std::pair<iterator, bool> {
        return _try_emplace(_hash(key), std::move(key), std::forward<TArgs>(valArgs)...);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    template <typename K_, typename... TArgs>
    inline auto Map<K, T, H, E, A>::_try_emplace(const size_t hash, K_ && key, TArgs &&... vargs) -> std::pair<iterator, bool> {
        static_assert(sizeof...(TArgs) == 0u || std::is_default_constructible_v<T>, "Mapped type must be default constructible");

        if (!_buckets) _allocate();
        size_t i{_indexOf(hash)};
        _Dist dist{1u};

        while (true) {
            _Bucket & bucket(_buckets[i]);

            // Can be inserted
            if (bucket.dist < dist) {
                if (_size >= (_bucketCount >> 1)) {
                    _rehash(_bucketCount << 1);
                    return _try_emplace(hash, std::forward<K_>(key), std::forward<TArgs>(vargs)...);
                }

                // Value here has smaller dist, robin hood
                if (bucket.dist) {
                    _propagate(bucket.entry(), i + 1u, bucket.dist + 1u);
                    bucket.entry().~V();
                }

                // Open slot
                _AllocatorTraits::construct(_alloc, &bucket.key, std::forward<K_>(key));
                if constexpr (!_isSet) {
                    _AllocatorTraits::construct(_alloc, &bucket.val, std::forward<TArgs>(vargs)...);
                }

                bucket.dist = dist;
                ++_size;
                return { iterator(&bucket), true };
            }

            // Value already exists
            if (_equal(bucket.key, key)) {
                return { iterator(&bucket), false };
            }

            ++i;
            ++dist;

            if (i >= _bucketCount) i = 0u;
        }
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline void Map<K, T, H, E, A>::_propagate(V & entry, size_t i, _Dist dist) {
        while (true) {
            if (i >= _bucketCount) i = 0u;
            _Bucket & bucket(_buckets[i]);

            if (!bucket.dist) {
                _AllocatorTraits::construct(_alloc, &bucket.entry(), std::move(entry));
                bucket.dist = dist;
                return;
            }

            if (bucket.dist < dist) {
                std::swap(entry, bucket.entry());
                std::swap(dist, bucket.dist);
            }

            ++i;
            ++dist;
        }
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline size_t Map<K, T, H, E, A>::erase(const K & key) {
        const iterator it(find(key));
        if (it == end()) {
            return 0u;
        }
        _erase(it);
        if (_size <= (_bucketCount >> 3) && _bucketCount > config::minBucketCount) {
            _rehash(_bucketCount >> 1);
        }
        return 1u;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::erase(const const_iterator position) -> iterator {
        iterator endIt(end());
        if (position != endIt) {
            _erase(position);
            if (_size <= (_bucketCount >> 3) && _bucketCount > config::minBucketCount) {
                _rehash(_bucketCount >> 1);
                endIt = end();
            }
        }
        return endIt;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::erase(const_iterator first, const const_iterator last) -> iterator {
        if (first != last) {
            do {
                _erase(first);
                ++first;
            } while (first != last);
            reserve(_size);
        }
        return end();
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline void Map<K, T, H, E, A>::_erase(const const_iterator position) {
        size_t i{size_t(position._bucket - _buckets)};
        size_t j{i + 1u};

        while (true) {
            if (j >= _bucketCount) j = 0u;

            if (_buckets[j].dist <= 1u) {
                break;
            }

            _buckets[i].entry() = std::move(_buckets[j].entry());
            _buckets[i].dist = _buckets[j].dist - 1u;

            ++i; ++j;
            if (i >= _bucketCount) i = 0u;
        }

        _buckets[i].entry().~V();
        _buckets[i].dist = 0u;
        --_size;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline void Map<K, T, H, E, A>::clear() noexcept {
        _clear<true>();
    }

    template <typename K, typename T, typename H, typename E, typename A>
    template <bool zeroDists>
    inline void Map<K, T, H, E, A>::_clear() noexcept {
        if constexpr (std::is_trivially_destructible_v<V>) {
            if constexpr (zeroDists) {
                if (_size) _zeroDists();
            }
        }
        else {
            for (size_t i{0u}, n{0u}; n < _size; ++i) {
                if (_buckets[i].dist) {
                    _buckets[i].entry().~V();
                    if constexpr (zeroDists) {
                        _buckets[i].dist = 0u;
                    }
                    ++n;
                }
            }
        }

        _size = 0u;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline bool Map<K, T, H, E, A>::contains(const K & key) const {
        return contains(key, _hash(key));
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline bool Map<K, T, H, E, A>::contains(const K & key, const size_t hash) const {
        return find(key, hash) != cend();
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline size_t Map<K, T, H, E, A>::count(const K & key) const {
        return contains(key);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline size_t Map<K, T, H, E, A>::count(const K & key, const size_t hash) const {
        return contains(key, hash);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline std::add_lvalue_reference_t<T> Map<K, T, H, E, A>::at(const K & key) requires (!std::is_same_v<T, void>) {
        return find(key)->second;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline std::add_lvalue_reference_t<const T> Map<K, T, H, E, A>::at(const K & key) const requires (!std::is_same_v<T, void>) {
        return find(key)->second;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline std::add_lvalue_reference_t<T> Map<K, T, H, E, A>::operator[](const K & key) {
        static_assert(!std::is_same_v<T, void>);
        return try_emplace(key).first->second;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline std::add_lvalue_reference_t<T> Map<K, T, H, E, A>::operator[](K && key) {
        return try_emplace(std::move(key)).first->second;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::begin() noexcept -> iterator {
        return _begin<false>();
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::begin() const noexcept -> const_iterator {
        return _begin<true>();
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::cbegin() const noexcept -> const_iterator {
        return _begin<true>();
    }

    template <typename K, typename T, typename H, typename E, typename A>
    template <bool constant>
    inline auto Map<K, T, H, E, A>::_begin() const noexcept -> _Iterator<constant> {
        if (!_size) {
            return _end<constant>();
        }

        for (size_t i{0u}; ; ++i) {
            if (_buckets[i].dist) {
                return _Iterator<constant>(_buckets + i);
            }
        }
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::end() noexcept -> iterator {
        return _end<false>();
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::end() const noexcept -> const_iterator {
        return _end<true>();
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::cend() const noexcept -> const_iterator {
        return _end<true>();
    }

    template <typename K, typename T, typename H, typename E, typename A>
    template <bool constant>
    inline auto Map<K, T, H, E, A>::_end() const noexcept -> _Iterator<constant> {
        return _Iterator<constant>(_buckets + _bucketCount);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::find(const K & key) -> iterator {
        return find(key, _hash(key));
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::find(const K & key) const -> const_iterator {
        return find(key, _hash(key));
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::find(const K & key, const size_t hash) -> iterator {
        return _find<false>(key, hash);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::find(const K & key, const size_t hash) const -> const_iterator {
        return _find<true>(key, hash);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    template <bool constant>
    inline auto Map<K, T, H, E, A>::_find(const K & key, const size_t hash) const -> _Iterator<constant> {
        if (!_buckets) {
            return _end<constant>();
        }

        size_t i{_indexOf(hash)};
        _Dist dist{1u};

        while (true) {
            const _Bucket & bucket(_buckets[i]);

            if (bucket.dist < dist) {
                return _end<constant>();
            }

            if (_equal(bucket.key, key)) {
                return _Iterator<constant>(&bucket);
            }

            ++i;
            ++dist;

            if (i >= _bucketCount) i = 0u;
        }
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::equal_range(const K & key) -> std::pair<iterator, iterator> {
        return equal_range(key, _hash(key));
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::equal_range(const K & key) const -> std::pair<const_iterator, const_iterator> {
        return equal_range(key, _hash(key));
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::equal_range(const K & key, const size_t hash) -> std::pair<iterator, iterator> {
        return _equal_range<false>(key, hash);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline auto Map<K, T, H, E, A>::equal_range(const K & key, const size_t hash) const -> std::pair<const_iterator, const_iterator> {
        return _equal_range<true>(key, hash);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    template <bool constant>
    inline auto Map<K, T, H, E, A>::_equal_range(const K & key, const size_t hash) const -> std::pair<_Iterator<constant>, _Iterator<constant>> {
        _Iterator<constant> it(_find<constant>(key, hash));
        return { it, it };
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline void Map<K, T, H, E, A>::reserve(const size_t capacity) {
        rehash(capacity << 1);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline void Map<K, T, H, E, A>::rehash(size_t bucketCount) {
        bucketCount = std::bit_ceil(bucketCount);
        if (bucketCount < config::minBucketCount) bucketCount = config::minBucketCount;
        else if (bucketCount < (_size << 1)) bucketCount = _size << 1u;

        if (bucketCount != _bucketCount) {
            if (_buckets) {
                _rehash(bucketCount);
            }
            else {
                _bucketCount = bucketCount;
            }
        }
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline void Map<K, T, H, E, A>::_rehash(const size_t bucketCount) {
        const size_t oldSize{_size};
        const size_t oldBucketCount{_bucketCount};
        _Bucket * const oldBuckets(_buckets);

        _size = 0u;
        _bucketCount = bucketCount;
        _allocate();

        for (size_t i{0u}, n{0u}; n < oldSize; ++i) {
            _Bucket & bucket(oldBuckets[i]);
            if (bucket.dist) {
                emplace(std::move(bucket.entry()));
                bucket.entry().~V();
                ++n;
            }
        }

        _AllocatorTraits::deallocate(_alloc, oldBuckets, oldBucketCount + 1u);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline void Map<K, T, H, E, A>::swap(Map & other) noexcept {
        std::swap(_size, other._size);
        std::swap(_bucketCount, other._bucketCount);
        std::swap(_buckets, other._buckets);
        std::swap(_hash, other._hash);
        std::swap(_equal, other._equal);
        if constexpr (_AllocatorTraits::propagate_on_container_swap::value) {
            std::swap(_alloc, other._alloc);
        }
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline bool Map<K, T, H, E, A>::empty() const noexcept {
        return _size == 0u;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline size_t Map<K, T, H, E, A>::size() const noexcept {
        return _size;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline size_t Map<K, T, H, E, A>::max_size() const noexcept {
        return max_bucket_count() >> 1u;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline size_t Map<K, T, H, E, A>::capacity() const noexcept {
        return _bucketCount >> 1u;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline size_t Map<K, T, H, E, A>::bucket_count() const noexcept {
        return _bucketCount;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline size_t Map<K, T, H, E, A>::max_bucket_count() const noexcept {
        return std::numeric_limits<size_t>::max() - 1u;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline size_t Map<K, T, H, E, A>::bucket(const K & key) const {
        return _indexOf(_hash(key));
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline size_t Map<K, T, H, E, A>::bucket_size(size_t i) const noexcept {
        if (i >= _bucketCount || !_buckets) {
            return 0u;
        }

        _Dist dist{1u};
        while (_buckets[i].dist > dist) {
            ++i;
            ++dist;

            if (i >= _bucketCount) i = 0u;
        }

        size_t n{0u};
        while (_buckets[i].dist == dist) {
            ++i;
            ++dist;
            ++n;

            if (i >= _bucketCount) i = 0u;
        }

        return n;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline float Map<K, T, H, E, A>::load_factor() const noexcept {
        return float(_size) / float(_bucketCount);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline float Map<K, T, H, E, A>::max_load_factor() const noexcept {
        return 0.5f;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline H Map<K, T, H, E, A>::hash_function() const noexcept {
        return _hash;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline E Map<K, T, H, E, A>::key_eq() const noexcept {
        return _equal;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline A Map<K, T, H, E, A>::get_allocator() const noexcept {
        return A(_alloc);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline size_t Map<K, T, H, E, A>::_indexOf(const size_t hash) const noexcept {
        return hash & (_bucketCount - 1u);
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline void Map<K, T, H, E, A>::_allocate() {
        _buckets = _AllocatorTraits::allocate(_alloc, _bucketCount + 1u);
        _zeroDists();
        _buckets[_bucketCount].dist = std::numeric_limits<_Dist>::max();
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline void Map<K, T, H, E, A>::_deallocate() {
        _AllocatorTraits::deallocate(_alloc, _buckets, _bucketCount + 1u);
        _buckets = nullptr;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline void Map<K, T, H, E, A>::_zeroDists() noexcept {
        // If dist is smaller than a word, and the bucket is small, just zero everything
        if constexpr (sizeof(_Dist) < sizeof(size_t) && sizeof(_Bucket) <= sizeof(size_t) * 2) {
            std::fill_n(reinterpret_cast<size_t *>(_buckets), (_bucketCount * sizeof(_Bucket)) / sizeof(size_t), 0u);
        }
        else {
            for (size_t i{0u}; i < _bucketCount; ++i) _buckets[i].dist = 0u;
        }
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline void Map<K, T, H, E, A>::_copyBuckets(const _Bucket * const buckets) {
        if constexpr (std::is_trivially_copyable_v<V>) {
            if (_size) {
                std::copy_n(reinterpret_cast<const size_t *>(buckets), (_bucketCount * sizeof(_Bucket)) / sizeof(size_t), reinterpret_cast<size_t *>(_buckets));
            }
        }
        else {
            for (size_t i{0u}, n{0u}; n < _size; ++i) {
                if ((_buckets[i].dist = buckets[i].dist)) {
                    _AllocatorTraits::construct(_alloc, &_buckets[i].entry(), buckets[i].entry());
                    ++n;
                }
            }
        }
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline void Map<K, T, H, E, A>::_moveBuckets(_Bucket * const buckets) {
        if constexpr (std::is_trivially_copyable_v<V>) {
            if (_size) {
                std::copy_n(reinterpret_cast<const size_t *>(buckets), (_bucketCount * sizeof(_Bucket)) / sizeof(size_t), reinterpret_cast<size_t *>(_buckets));
            }
        }
        else {
            for (size_t i{0u}, n{0u}; n < _size; ++i) {
                if ((_buckets[i].dist = buckets[i].dist)) {
                    _AllocatorTraits::construct(_alloc, &_buckets[i].entry(), std::move(buckets[i].entry()));
                    ++n;
                }
            }
        }
    }

    template <typename K, typename T, typename H, typename E, typename A>
    inline bool operator==(const Map<K, T, H, E, A> & m1, const Map<K, T, H, E, A> & m2) {
        if (m1.size() != m2.size()) {
            return false;
        }

        if (&m1 == &m2) {
            return true;
        }

        for (const auto & v : m1) {
            if (!m2.contains(v)) {
                return false;
            }
        }

        return true;
    }

    // Iterator ================================================================

    template <typename K, typename T, typename H, typename E, typename A>
    template <bool constant>
    template <bool constant_> requires (constant && !constant_)
    constexpr Map<K, T, H, E, A>::_Iterator<constant>::_Iterator(const _Iterator<constant_> & other) noexcept:
        _bucket{other._bucket}
    {}

    template <typename K, typename T, typename H, typename E, typename A>
    template <bool constant>
    template <typename _Bucket_>
    constexpr Map<K, T, H, E, A>::_Iterator<constant>::_Iterator(_Bucket_ * const bucket) noexcept :
        _bucket(const_cast<_Bucket *>(bucket))
    {}

    template <typename K, typename T, typename H, typename E, typename A>
    template <bool constant>
    inline auto Map<K, T, H, E, A>::_Iterator<constant>::operator*() const noexcept -> V & {
        return _bucket->entry();
    }

    template <typename K, typename T, typename H, typename E, typename A>
    template <bool constant>
    inline auto Map<K, T, H, E, A>::_Iterator<constant>::operator->() const noexcept -> V * {
        return &_bucket->entry();
    }

    template <typename K, typename T, typename H, typename E, typename A>
    template <bool constant>
    inline auto Map<K, T, H, E, A>::_Iterator<constant>::operator++() noexcept -> _Iterator & {
        do {
            ++_bucket;
        } while (!_bucket->dist);

        return *this;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    template <bool constant>
    inline auto Map<K, T, H, E, A>::_Iterator<constant>::operator++(int) noexcept -> _Iterator {
        _Iterator temp(*this);
        operator++();
        return temp;
    }

    template <typename K, typename T, typename H, typename E, typename A>
    template <bool constant>
    template <bool constant_>
    inline bool Map<K, T, H, E, A>::_Iterator<constant>::operator==(const _Iterator<constant_> & it) const noexcept {
        return _bucket == it._bucket;
    }

}
