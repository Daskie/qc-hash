#pragma once

//
// QC Hash 2.1.2
//
// Austin Quick, 2016 - 2020
// https://github.com/Daskie/qc-hash
// ...
//

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "qc-hash.hpp"

namespace qc::hash {

    namespace config {

        constexpr size_t minCapacity(16u); // Must be at least `sizeof(size_t) / 2`
        constexpr size_t minBucketCount(minCapacity << 1);
        constexpr bool useIdentityHash(true);

    }

    template <typename K> using _DefaultHash = std::conditional_t<config::useIdentityHash && sizeof(K) <= sizeof(size_t), IdentityHash<K>, Hash<K>>;

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

        template <bool constant> class _Iterator;
        friend _Iterator;

        using _Dist = typename _Types<K, T>::Dist;
        using _Bucket = typename _Types<K, T>::Bucket;
        using _Allocator = typename std::allocator_traits<A>::template rebind_alloc<_Bucket>;
        using _AllocatorTraits = std::allocator_traits<_Allocator>;

        public: //--------------------------------------------------------------

        using key_type = K;
        using mapped_type = T;
        using value_type = typename _Types<key_type, mapped_type>::V;
        using hasher = H;
        using key_equal = E;
        using allocator_type = A;
        using reference = value_type &;
        using const_reference = const value_type &;
        using pointer = typename std::allocator_traits<allocator_type>::pointer;
        using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        using iterator = _Iterator<false>;
        using const_iterator = _Iterator<true>;

        static_assert(std::is_nothrow_move_constructible_v<value_type>);
        static_assert(std::is_nothrow_move_assignable_v<value_type>);
        static_assert(std::is_nothrow_swappable_v<value_type>);
        static_assert(std::is_nothrow_destructible_v<value_type>);
        static_assert(std::is_nothrow_default_constructible_v<hasher>);
        static_assert(std::is_nothrow_move_constructible_v<hasher>);
        static_assert(std::is_nothrow_move_assignable_v<hasher>);
        static_assert(std::is_nothrow_swappable_v<hasher>);
        static_assert(std::is_nothrow_destructible_v<hasher>);
        static_assert(std::is_nothrow_default_constructible_v<key_equal>);
        static_assert(std::is_nothrow_move_constructible_v<key_equal>);
        static_assert(std::is_nothrow_move_assignable_v<key_equal>);
        static_assert(std::is_nothrow_swappable_v<key_equal>);
        static_assert(std::is_nothrow_destructible_v<key_equal>);
        static_assert(std::is_nothrow_default_constructible_v<allocator_type>);
        static_assert(std::is_nothrow_move_constructible_v<allocator_type>);
        static_assert(std::is_nothrow_move_assignable_v<allocator_type> || !_AllocatorTraits::propagate_on_container_move_assignment::value);
        static_assert(std::is_nothrow_swappable_v<allocator_type> || !_AllocatorTraits::propagate_on_container_swap::value);
        static_assert(std::is_nothrow_destructible_v<allocator_type>);

        //
        // Memory is not allocated until the first entry is inserted.
        //
        explicit Map(size_t minCapacity = config::minCapacity, const hasher & hash = hasher(), const key_equal & equal = key_equal(), const allocator_type & alloc = allocator_type()) noexcept;
        Map(size_t minCapacity, const allocator_type & alloc) noexcept;
        Map(size_t minCapacity, const hasher & hash, const allocator_type & alloc) noexcept;
        explicit Map(const allocator_type & alloc) noexcept;
        template <typename It> Map(It first, It last, size_t minCapacity = 0u, const hasher & hash = hasher(), const key_equal & equal = key_equal(), const allocator_type & alloc = allocator_type());
        template <typename It> Map(It first, It last, size_t minCapacity, const allocator_type & alloc);
        template <typename It> Map(It first, It last, size_t minCapacity, const hasher & hash, const allocator_type & alloc);
        Map(std::initializer_list<value_type> entries, size_t minCapacity = 0u, const hasher & hash = hasher(), const key_equal & equal = key_equal(), const allocator_type & alloc = allocator_type());
        Map(std::initializer_list<value_type> entries, size_t minCapacity, const allocator_type & alloc);
        Map(std::initializer_list<value_type> entries, size_t minCapacity, const hasher & hash, const allocator_type & alloc);
        Map(const Map & other);
        Map(const Map & other, const allocator_type & alloc);
        Map(Map && other) noexcept;
        Map(Map && other, const allocator_type & alloc) noexcept;
        Map(Map && other, allocator_type && alloc) noexcept;

        //
        // ...
        //
        Map & operator=(std::initializer_list<value_type> entries);
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
        std::pair<iterator, bool> insert(const value_type & entry);
        std::pair<iterator, bool> insert(value_type && entry);
        template <typename It> void insert(It first, It last);
        void insert(std::initializer_list<value_type> entries);

        //
        // Prefer try_emplace over this, but prefer this over insert.
        // Invalidates iterators.
        //
        std::pair<iterator, bool> emplace(const value_type & entry);
        std::pair<iterator, bool> emplace(value_type && entry);
        template <typename K_, typename V_> std::pair<iterator, bool> emplace(K_ && key, V_ && val);
        template <typename... KArgs, typename... TArgs> std::pair<iterator, bool> emplace(std::piecewise_construct_t, std::tuple<KArgs...> && kArgs, std::tuple<TArgs...> && tArgs);

        //
        // If there is no existing entry for `key`, creates a new entry in
        // place.
        // Choose this as the default insertion method of choice.
        // Invalidates iterators.
        //
        template <typename... TArgs> std::pair<iterator, bool> try_emplace(const key_type & key, TArgs &&... valArgs);
        template <typename... TArgs> std::pair<iterator, bool> try_emplace(key_type && key, TArgs &&... valArgs);

        //
        // The variations that return iterators always return the end iterator,
        // as this method can trigger a rehash.
        // Invalidates iterators.
        //
        size_t erase(const key_type & key);
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
        bool contains(const key_type & key) const;
        bool contains(const key_type & key, size_t hash) const;

        //
        // Returns `1` if the map contains an entry for `key` and `0` if it does
        // not.
        //
        size_t count(const key_type & key) const;
        size_t count(const key_type & key, size_t hash) const;

        //
        // ...
        //
        std::add_lvalue_reference_t<mapped_type> at(const key_type & key);
        std::add_lvalue_reference_t<const mapped_type> at(const key_type & key) const;

        //
        // ...
        //
        std::add_lvalue_reference_t<mapped_type> operator[](const key_type & key);
        std::add_lvalue_reference_t<mapped_type> operator[](key_type && key);

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
        iterator find(const key_type & key);
        const_iterator find(const key_type & key) const;
        iterator find(const key_type & key, size_t hash);
        const_iterator find(const key_type & key, size_t hash) const;

        //
        // As a key may correspont to as most one entry, this method is
        // equivalent to `find`, except returning a pair of duplicate iterators.
        //
        std::pair<iterator, iterator> equal_range(const key_type & key);
        std::pair<const_iterator, const_iterator> equal_range(const key_type & key) const;
        std::pair<iterator, iterator> equal_range(const key_type & key, size_t hash);
        std::pair<const_iterator, const_iterator> equal_range(const key_type & key, size_t hash) const;

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
        size_t bucket(const key_type & key) const;

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
        hasher hash_function() const noexcept;

        //
        // Returns an instance of `key_equal`.
        //
        key_equal key_eq() const noexcept;

        //
        // Returns an instance of `allocator_type`.
        //
        allocator_type get_allocator() const noexcept;

        //
        // Returns whether `other` has the same entries.
        //
        bool operator==(const Map & other);

        private: //-------------------------------------------------------------

        static constexpr bool _isSet = std::is_same_v<mapped_type, void>;

        size_t _size;
        size_t _bucketCount;
        _Bucket * _buckets;
        hasher _hash;
        key_equal _equal;
        _Allocator _alloc;

        template <typename KTuple, typename VTuple, size_t... kIndices, size_t... vIndices> std::pair<iterator, bool> _emplace(KTuple && kTuple, VTuple && vTuple, std::index_sequence<kIndices...>, std::index_sequence<vIndices...>);

        template <typename K_, typename... Args> std::pair<iterator, bool> _try_emplace(size_t hash, K_ && key, Args &&... args);

        void _propagate(value_type & entry, size_t i, _Dist dist);

        void _erase(const_iterator position);

        template <bool zeroDists> void _clear() noexcept;

        template <bool constant> std::pair<_Iterator<constant>, _Iterator<constant>> _equal_range(const key_type & key, size_t hash) const;

        template <bool constant> _Iterator<constant> _begin() const noexcept;

        template <bool constant> _Iterator<constant> _end() const noexcept;

        template <bool constant> _Iterator<constant> _find(const key_type & key, size_t hash) const;

        void _rehash(size_t bucketCount);

        size_t _indexOf(size_t hash) const noexcept;

        void _allocate();

        void _deallocate();

        void _zeroDists() noexcept;

        void _copyBuckets(const _Bucket * bucket);

        void _moveBuckets(_Bucket * bucket);

    };

    //
    // Forward iterator
    //
    template <typename K, typename T, typename H, typename E, typename A>
    template <bool constant>
    class Map<K, T, H, E, A>::_Iterator {

        friend Map;

        public: //--------------------------------------------------------------

        using iterator_category = std::forward_iterator_tag;
        using value_type = std::conditional_t<constant, const Map::value_type, Map::value_type>;
        using difference_type = ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

        //
        // ...
        //
        constexpr _Iterator(const _Iterator & other) noexcept = default;
        template <bool constant_ = constant, typename = std::enable_if_t<constant_>> constexpr _Iterator(const _Iterator<!constant> & other) noexcept;

        //
        // ...
        //
        value_type & operator*() const noexcept;

        //
        // ...
        //
        value_type * operator->() const noexcept;

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

namespace std {

    //
    // Specializes std::swap for qc::Map.
    //
    template <typename K, typename T, typename H, typename E, typename A> void swap(qc::hash::Map<K, T, H, E, A> & s1, qc::hash::Map<K, T, H, E, A> & s2) noexcept;

}

// INLINE IMPLEMENTATION ///////////////////////////////////////////////////////////////////////////////////////////////

#define QC_HASH_MAP Map<K, T, H, E, A>
#define QC_HASH_MAP_TEMPLATE template <typename K, typename T, typename H, typename E, typename A>

namespace qc::hash {

    // Map =====================================================================

    QC_HASH_MAP_TEMPLATE
    inline QC_HASH_MAP::Map(const size_t minCapacity, const hasher & hash, const key_equal & equal, const allocator_type & alloc) noexcept:
        _size(),
        _bucketCount(minCapacity <= config::minCapacity ? config::minBucketCount : _ceil2(minCapacity << 1)),
        _buckets(nullptr),
        _hash(hash),
        _equal(equal),
        _alloc(alloc)
    {}

    QC_HASH_MAP_TEMPLATE
    inline QC_HASH_MAP::Map(const size_t minCapacity, const allocator_type & alloc) noexcept :
        Map(minCapacity, hasher(), key_equal(), alloc)
    {}

    QC_HASH_MAP_TEMPLATE
    inline QC_HASH_MAP::Map(const size_t minCapacity, const hasher & hash, const allocator_type & alloc) noexcept :
        Map(minCapacity, hash, key_equal(), alloc)
    {}

    QC_HASH_MAP_TEMPLATE
    inline QC_HASH_MAP::Map(const allocator_type & alloc) noexcept :
        Map(config::minCapacity, hasher(), key_equal(), alloc)
    {}

    QC_HASH_MAP_TEMPLATE
    template <typename It>
    inline QC_HASH_MAP::Map(const It first, const It last, const size_t minCapacity, const hasher & hash, const key_equal & equal, const allocator_type & alloc) :
        Map(minCapacity ? minCapacity : std::distance(first, last), hash, equal, alloc)
    {
        insert(first, last);
    }

    QC_HASH_MAP_TEMPLATE
    template <typename It>
    inline QC_HASH_MAP::Map(const It first, const It last, const size_t minCapacity, const allocator_type & alloc) :
        Map(first, last, minCapacity, hasher(), key_equal(), alloc)
    {}

    QC_HASH_MAP_TEMPLATE
    template <typename It>
    inline QC_HASH_MAP::Map(const It first, const It last, const size_t minCapacity, const hasher & hash, const allocator_type & alloc) :
        Map(first, last, minCapacity, hash, key_equal(), alloc)
    {}

    QC_HASH_MAP_TEMPLATE
    inline QC_HASH_MAP::Map(std::initializer_list<value_type> entries, size_t minCapacity, const hasher & hash, const key_equal & equal, const allocator_type & alloc) :
        Map(minCapacity ? minCapacity : entries.size(), hash, equal, alloc)
    {
        insert(entries);
    }

    QC_HASH_MAP_TEMPLATE
    inline QC_HASH_MAP::Map(const std::initializer_list<value_type> entries, const size_t minCapacity, const allocator_type & alloc) :
        Map(entries, minCapacity, hasher(), key_equal(), alloc)
    {}

    QC_HASH_MAP_TEMPLATE
    inline QC_HASH_MAP::Map(const std::initializer_list<value_type> entries, const size_t minCapacity, const hasher & hash, const allocator_type & alloc) :
        Map(entries, minCapacity, hash, key_equal(), alloc)
    {}

    QC_HASH_MAP_TEMPLATE
    inline QC_HASH_MAP::Map(const Map & other) :
        Map(other, std::allocator_traits<allocator_type>::select_on_container_copy_construction(other._alloc))
    {}

    QC_HASH_MAP_TEMPLATE
    inline QC_HASH_MAP::Map(const Map & other, const allocator_type & alloc) :
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

    QC_HASH_MAP_TEMPLATE
    inline QC_HASH_MAP::Map(Map && other) noexcept :
        Map(std::move(other), std::move(other._alloc))
    {}

    QC_HASH_MAP_TEMPLATE
    inline QC_HASH_MAP::Map(Map && other, const allocator_type & alloc) noexcept :
        Map(std::move(other), allocator_type(alloc))
    {}

    QC_HASH_MAP_TEMPLATE
    inline QC_HASH_MAP::Map(Map && other, allocator_type && alloc) noexcept :
        _size(std::exchange(other._size, 0u)),
        _bucketCount(std::exchange(other._bucketCount, 0u)),
        _buckets(std::exchange(other._buckets, nullptr)),
        _hash(std::move(other._hash)),
        _equal(std::move(other._equal)),
        _alloc(std::move(alloc))
    {}

    QC_HASH_MAP_TEMPLATE
    inline QC_HASH_MAP & QC_HASH_MAP::operator=(const std::initializer_list<value_type> entries) {
        clear();
        insert(entries);

        return *this;
    }

    QC_HASH_MAP_TEMPLATE
    inline QC_HASH_MAP & QC_HASH_MAP::operator=(const Map & other) {
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
            _alloc = std::allocator_traits<allocator_type>::select_on_container_copy_construction(other._alloc);
        }

        if (other._buckets) {
            if (!_buckets) {
                _allocate();
            }
            _copyBuckets(other._buckets);
        }

        return *this;
    }

    QC_HASH_MAP_TEMPLATE
    inline QC_HASH_MAP & QC_HASH_MAP::operator=(Map && other) noexcept {
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

    QC_HASH_MAP_TEMPLATE
        inline QC_HASH_MAP::~Map() noexcept {
        if (_buckets) {
            _clear<false>();
            _deallocate();
        }
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::insert(const value_type & entry) -> std::pair<iterator, bool> {
        return emplace(entry);
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::insert(value_type && entry) -> std::pair<iterator, bool> {
        return emplace(std::move(entry));
    }

    QC_HASH_MAP_TEMPLATE
    template <typename It>
    inline void QC_HASH_MAP::insert(It first, const It last) {
        while (first != last) {
            emplace(*first);
            ++first;
        }
    }

    QC_HASH_MAP_TEMPLATE
    inline void QC_HASH_MAP::insert(const std::initializer_list<value_type> entries) {
        for (const value_type & entry : entries) {
            emplace(entry);
        }
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::emplace(const value_type & entry) -> std::pair<iterator, bool> {
        if constexpr (_isSet) {
            return try_emplace(entry);
        }
        else {
            return try_emplace(entry.first, entry.second);
        }
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::emplace(value_type && entry) -> std::pair<iterator, bool> {
        if constexpr (_isSet) {
            return try_emplace(std::move(entry));
        }
        else {
            return try_emplace(std::move(entry.first), std::move(entry.second));
        }
    }

    QC_HASH_MAP_TEMPLATE
    template <typename K_, typename V_>
    inline auto QC_HASH_MAP::emplace(K_ && key, V_ && val) -> std::pair<iterator, bool> {
        static_assert(!_isSet, "This is not a set operation");
        return try_emplace(std::forward<K_>(key), std::forward<V_>(val));
    }

    QC_HASH_MAP_TEMPLATE
    template <typename... KArgs, typename... TArgs>
    inline auto QC_HASH_MAP::emplace(const std::piecewise_construct_t, std::tuple<KArgs...> && kArgs, std::tuple<TArgs...> && tArgs) -> std::pair<iterator, bool> {
        static_assert(!_isSet, "This is not a set operation");
        return _emplace(std::move(kArgs), std::move(tArgs), std::index_sequence_for<KArgs...>(), std::index_sequence_for<TArgs...>());
    }

    QC_HASH_MAP_TEMPLATE
    template <typename KTuple, typename VTuple, size_t... kIndices, size_t... vIndices>
    inline auto QC_HASH_MAP::_emplace(KTuple && kTuple, VTuple && vTuple, const std::index_sequence<kIndices...>, const std::index_sequence<vIndices...>) -> std::pair<iterator, bool> {
        return try_emplace(key_type(std::move(std::get<kIndices>(kTuple))...), std::move(std::get<vIndices>(vTuple))...);
    }

    QC_HASH_MAP_TEMPLATE
    template <typename... TArgs>
    inline auto QC_HASH_MAP::try_emplace(const key_type & key, TArgs &&... vargs) -> std::pair<iterator, bool> {
        return _try_emplace(_hash(key), key, std::forward<TArgs>(vargs)...);
    }

    QC_HASH_MAP_TEMPLATE
    template <typename... TArgs>
    inline auto QC_HASH_MAP::try_emplace(key_type && key, TArgs &&... vargs) -> std::pair<iterator, bool> {
        return _try_emplace(_hash(key), std::move(key), std::forward<TArgs>(vargs)...);
    }

    QC_HASH_MAP_TEMPLATE
    template <typename K_, typename... TArgs>
    inline auto QC_HASH_MAP::_try_emplace(const size_t hash, K_ && key, TArgs &&... vargs) -> std::pair<iterator, bool> {
        static_assert(sizeof...(TArgs) == 0u || std::is_default_constructible_v<mapped_type>, "Mapped type must be default constructible");

        if (!_buckets) _allocate();
        size_t i(_indexOf(hash));
        _Dist dist(1u);

        while (true) {
            _Bucket & bucket(_buckets[i]);

            // Can be inserted
            if (bucket.dist < dist) {
                if (_size >= (_bucketCount >> 1)) {
                    _rehash(_bucketCount << 1);
                    return _try_emplace(hash, std::forward<K_>(key), std::forward<TArgs>(vargs)...);
                }

                // Talue here has smaller dist, robin hood
                if (bucket.dist) {
                    _propagate(bucket.entry(), i + 1u, bucket.dist + 1u);
                    bucket.entry().~value_type();
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

            // Talue already exists
            if (_equal(bucket.key, key)) {
                return { iterator(&bucket), false };
            }

            ++i;
            ++dist;

            if (i >= _bucketCount) i = 0u;
        }

        // Will never reach reach this return
        return { end(), false };
    }

    QC_HASH_MAP_TEMPLATE
    inline void QC_HASH_MAP::_propagate(value_type & entry, size_t i, _Dist dist) {
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

    QC_HASH_MAP_TEMPLATE
    inline size_t QC_HASH_MAP::erase(const key_type & key) {
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

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::erase(const const_iterator position) -> iterator {
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

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::erase(const_iterator first, const const_iterator last) -> iterator {
        if (first != last) {
            do {
                _erase(first);
                ++first;
            } while (first != last);
            reserve(_size);
        }
        return end();
    }

    QC_HASH_MAP_TEMPLATE
    inline void QC_HASH_MAP::_erase(const const_iterator position) {
        size_t i(position._bucket - _buckets), j(i + 1u);

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

        _buckets[i].entry().~value_type();
        _buckets[i].dist = 0u;
        --_size;
    }

    QC_HASH_MAP_TEMPLATE
    inline void QC_HASH_MAP::clear() noexcept {
        _clear<true>();
    }

    QC_HASH_MAP_TEMPLATE
    template <bool zeroDists>
    inline void QC_HASH_MAP::_clear() noexcept {
        if constexpr (std::is_trivially_destructible_v<value_type>) {
            if constexpr (zeroDists) {
                if (_size) _zeroDists();
            }
        }
        else {
            for (size_t i(0u), n(0u); n < _size; ++i) {
                if (_buckets[i].dist) {
                    _buckets[i].entry().~value_type();
                    if constexpr (zeroDists) {
                        _buckets[i].dist = 0u;
                    }
                    ++n;
                }
            }
        }

        _size = 0u;
    }

    QC_HASH_MAP_TEMPLATE
    inline bool QC_HASH_MAP::contains(const key_type & key) const {
        return contains(key, _hash(key));
    }

    QC_HASH_MAP_TEMPLATE
    inline bool QC_HASH_MAP::contains(const key_type & key, const size_t hash) const {
        return find(key, hash) != cend();
    }

    QC_HASH_MAP_TEMPLATE
    inline size_t QC_HASH_MAP::count(const key_type & key) const {
        return contains(key);
    }

    QC_HASH_MAP_TEMPLATE
    inline size_t QC_HASH_MAP::count(const key_type & key, const size_t hash) const {
        return contains(key, hash);
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::at(const key_type & key) -> std::add_lvalue_reference_t<mapped_type> {
        static_assert(!_isSet, "This is not a set operation");
        return find(key)->second;
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::at(const key_type & key) const -> std::add_lvalue_reference_t<const mapped_type> {
        static_assert(!_isSet, "This is not a set operation");
        return find(key)->second;
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::operator[](const key_type & key) -> std::add_lvalue_reference_t<mapped_type> {
        static_assert(!_isSet, "This is not a set operation");
        return try_emplace(key).first->second;
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::operator[](key_type && key) -> std::add_lvalue_reference_t<mapped_type> {
        static_assert(!_isSet, "This is not a set operation");
        return try_emplace(std::move(key)).first->second;
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::begin() noexcept -> iterator {
        return _begin<false>();
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::begin() const noexcept -> const_iterator {
        return _begin<true>();
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::cbegin() const noexcept -> const_iterator {
        return _begin<true>();
    }

    QC_HASH_MAP_TEMPLATE
    template <bool constant>
    inline auto QC_HASH_MAP::_begin() const noexcept -> _Iterator<constant> {
        if (!_size) {
            return _end<constant>();
        }

        for (size_t i(0u); ; ++i) {
            if (_buckets[i].dist) {
                return _Iterator<constant>(_buckets + i);
            }
        }
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::end() noexcept -> iterator {
        return _end<false>();
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::end() const noexcept -> const_iterator {
        return _end<true>();
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::cend() const noexcept -> const_iterator {
        return _end<true>();
    }

    QC_HASH_MAP_TEMPLATE
    template <bool constant>
    inline auto QC_HASH_MAP::_end() const noexcept -> _Iterator<constant> {
        return _Iterator<constant>(_buckets + _bucketCount);
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::find(const key_type & key) -> iterator {
        return find(key, _hash(key));
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::find(const key_type & key) const -> const_iterator {
        return find(key, _hash(key));
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::find(const key_type & key, const size_t hash) -> iterator {
        return _find<false>(key, hash);
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::find(const key_type & key, const size_t hash) const -> const_iterator {
        return _find<true>(key, hash);
    }

    QC_HASH_MAP_TEMPLATE
    template <bool constant>
    inline auto QC_HASH_MAP::_find(const key_type & key, const size_t hash) const -> _Iterator<constant> {
        if (!_buckets) {
            return _end<constant>();
        }

        size_t i(_indexOf(hash));
        _Dist dist(1u);

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
        };

        // Will never reach reach this return
        return _end<constant>();
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::equal_range(const key_type & key) -> std::pair<iterator, iterator> {
        return equal_range(key, _hash(key));
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::equal_range(const key_type & key) const -> std::pair<const_iterator, const_iterator> {
        return equal_range(key, _hash(key));
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::equal_range(const key_type & key, const size_t hash) -> std::pair<iterator, iterator> {
        return _equal_range<false>(key, hash);
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::equal_range(const key_type & key, const size_t hash) const -> std::pair<const_iterator, const_iterator> {
        return _equal_range<true>(key, hash);
    }

    QC_HASH_MAP_TEMPLATE
    template <bool constant>
    inline auto QC_HASH_MAP::_equal_range(const key_type & key, const size_t hash) const -> std::pair<_Iterator<constant>, _Iterator<constant>> {
        _Iterator<constant> it(_find<constant>(key, hash));
        return { it, it };
    }

    QC_HASH_MAP_TEMPLATE
    inline void QC_HASH_MAP::reserve(const size_t capacity) {
        rehash(capacity << 1);
    }

    QC_HASH_MAP_TEMPLATE
    inline void QC_HASH_MAP::rehash(size_t bucketCount) {
        bucketCount = _ceil2(bucketCount);
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

    QC_HASH_MAP_TEMPLATE
    inline void QC_HASH_MAP::_rehash(const size_t bucketCount) {
        const size_t oldSize(_size);
        const size_t oldBucketCount(_bucketCount);
        _Bucket * const oldBuckets(_buckets);

        _size = 0u;
        _bucketCount = bucketCount;
        _allocate();

        for (size_t i(0u), n(0u); n < oldSize; ++i) {
            _Bucket & bucket(oldBuckets[i]);
            if (bucket.dist) {
                emplace(std::move(bucket.entry()));
                bucket.entry().~value_type();
                ++n;
            }
        }

        _AllocatorTraits::deallocate(_alloc, oldBuckets, oldBucketCount + 1u);
    }

    QC_HASH_MAP_TEMPLATE
    inline void QC_HASH_MAP::swap(Map & other) noexcept {
        std::swap(_size, other._size);
        std::swap(_bucketCount, other._bucketCount);
        std::swap(_buckets, other._buckets);
        std::swap(_hash, other._hash);
        std::swap(_equal, other._equal);
        if constexpr (_AllocatorTraits::propagate_on_container_swap::value) {
            std::swap(_alloc, other._alloc);
        }
    }

    QC_HASH_MAP_TEMPLATE
    inline bool QC_HASH_MAP::empty() const noexcept {
        return _size == 0u;
    }

    QC_HASH_MAP_TEMPLATE
    inline size_t QC_HASH_MAP::size() const noexcept {
        return _size;
    }

    QC_HASH_MAP_TEMPLATE
    inline size_t QC_HASH_MAP::max_size() const noexcept {
        return max_bucket_count() >> 1u;
    }

    QC_HASH_MAP_TEMPLATE
    inline size_t QC_HASH_MAP::capacity() const noexcept {
        return _bucketCount >> 1u;
    }

    QC_HASH_MAP_TEMPLATE
    inline size_t QC_HASH_MAP::bucket_count() const noexcept {
        return _bucketCount;
    }

    QC_HASH_MAP_TEMPLATE
    inline size_t QC_HASH_MAP::max_bucket_count() const noexcept {
        return std::numeric_limits<size_t>::max() - 1u;
    }

    QC_HASH_MAP_TEMPLATE
    inline size_t QC_HASH_MAP::bucket(const key_type & key) const {
        return _indexOf(_hash(key));
    }

    QC_HASH_MAP_TEMPLATE
    inline size_t QC_HASH_MAP::bucket_size(size_t i) const noexcept {
        if (i >= _bucketCount || !_buckets) {
            return 0u;
        }

        _Dist dist(1u);
        while (_buckets[i].dist > dist) {
            ++i;
            ++dist;

            if (i >= _bucketCount) i = 0u;
        }

        size_t n(0u);
        while (_buckets[i].dist == dist) {
            ++i;
            ++dist;
            ++n;

            if (i >= _bucketCount) i = 0u;
        }

        return n;
    }

    QC_HASH_MAP_TEMPLATE
    inline float QC_HASH_MAP::load_factor() const noexcept {
        return float(_size) / float(_bucketCount);
    }

    QC_HASH_MAP_TEMPLATE
    inline float QC_HASH_MAP::max_load_factor() const noexcept {
        return 0.5f;
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::hash_function() const noexcept -> hasher {
        return _hash;
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::key_eq() const noexcept -> key_equal {
        return _equal;
    }

    QC_HASH_MAP_TEMPLATE
    inline auto QC_HASH_MAP::get_allocator() const noexcept -> allocator_type {
        return _alloc;
    }

    QC_HASH_MAP_TEMPLATE
    inline bool QC_HASH_MAP::operator==(const QC_HASH_MAP & other) {
        if (_size != other._size) {
            return false;
        }

        if (this == &other) {
            return true;
        }

        for (const auto & v : other) {
            if (!contains(v)) {
                return false;
            }
        }

        return true;
    }

    QC_HASH_MAP_TEMPLATE
    inline size_t QC_HASH_MAP::_indexOf(const size_t hash) const noexcept {
        return hash & (_bucketCount - 1u);
    }

    QC_HASH_MAP_TEMPLATE
    inline void QC_HASH_MAP::_allocate() {
        _buckets = _AllocatorTraits::allocate(_alloc, _bucketCount + 1u);
        _zeroDists();
        _buckets[_bucketCount].dist = std::numeric_limits<_Dist>::max();
    }

    QC_HASH_MAP_TEMPLATE
    inline void QC_HASH_MAP::_deallocate() {
        _AllocatorTraits::deallocate(_alloc, _buckets, _bucketCount + 1u);
        _buckets = nullptr;
    }

    QC_HASH_MAP_TEMPLATE
    inline void QC_HASH_MAP::_zeroDists() noexcept {
        // If dist is smaller than a word, and the bucket is small, just zero everything
        if constexpr (sizeof(_Dist) < sizeof(size_t) && sizeof(_Bucket) <= sizeof(size_t) * 2) {
            std::fill_n(reinterpret_cast<size_t *>(_buckets), (_bucketCount * sizeof(_Bucket)) / sizeof(size_t), 0u);
        }
        else {
            for (size_t i(0u); i < _bucketCount; ++i) _buckets[i].dist = 0u;
        }
    }

    QC_HASH_MAP_TEMPLATE
    inline void QC_HASH_MAP::_copyBuckets(const _Bucket * const buckets) {
        if constexpr (std::is_trivially_copyable_v<value_type>) {
            if (_size) {
                std::copy_n(reinterpret_cast<const size_t *>(buckets), (_bucketCount * sizeof(_Bucket)) / sizeof(size_t), reinterpret_cast<size_t *>(_buckets));
            }
        }
        else {
            for (size_t i(0u), n(0u); n < _size; ++i) {
                if (_buckets[i].dist = buckets[i].dist) {
                    _AllocatorTraits::construct(_alloc, &_buckets[i].entry(), buckets[i].entry());
                    ++n;
                }
            }
        }
    }

    QC_HASH_MAP_TEMPLATE
    inline void QC_HASH_MAP::_moveBuckets(_Bucket * const buckets) {
        if constexpr (std::is_trivially_copyable_v<value_type>) {
            if (_size) {
                std::copy_n(reinterpret_cast<const size_t *>(buckets), (_bucketCount * sizeof(_Bucket)) / sizeof(size_t), reinterpret_cast<size_t *>(_buckets));
            }
        }
        else {
            for (size_t i(0u), n(0u); n < _size; ++i) {
                if (_buckets[i].dist = buckets[i].dist) {
                    _AllocatorTraits::construct(_alloc, &_buckets[i].entry(), std::move(buckets[i].entry()));
                    ++n;
                }
            }
        }
    }

    // Iterator ================================================================

    QC_HASH_MAP_TEMPLATE
    template <bool constant>
    template <bool constant_, typename>
    constexpr QC_HASH_MAP::_Iterator<constant>::_Iterator(const _Iterator<!constant> & other) noexcept :
        _bucket(other._bucket)
    {}

    QC_HASH_MAP_TEMPLATE
    template <bool constant>
    template <typename _Bucket_>
    constexpr QC_HASH_MAP::_Iterator<constant>::_Iterator(_Bucket_ * const bucket) noexcept :
        _bucket(const_cast<_Bucket *>(bucket))
    {}

    QC_HASH_MAP_TEMPLATE
    template <bool constant>
    inline auto QC_HASH_MAP::_Iterator<constant>::operator*() const noexcept -> value_type & {
        return _bucket->entry();
    }

    QC_HASH_MAP_TEMPLATE
    template <bool constant>
    inline auto QC_HASH_MAP::_Iterator<constant>::operator->() const noexcept -> value_type * {
        return &_bucket->entry();
    }

    QC_HASH_MAP_TEMPLATE
    template <bool constant>
    inline auto QC_HASH_MAP::_Iterator<constant>::operator++() noexcept -> _Iterator & {
        do {
            ++_bucket;
        } while (!_bucket->dist);

        return *this;
    }

    QC_HASH_MAP_TEMPLATE
    template <bool constant>
    inline auto QC_HASH_MAP::_Iterator<constant>::operator++(int) noexcept -> _Iterator {
        _Iterator temp(*this);
        operator++();
        return temp;
    }

    QC_HASH_MAP_TEMPLATE
    template <bool constant>
    template <bool constant_>
    inline bool QC_HASH_MAP::_Iterator<constant>::operator==(const _Iterator<constant_> & o) const noexcept {
        return _bucket == o._bucket;
    }

}

namespace std {

    QC_HASH_MAP_TEMPLATE
    inline void swap(qc::hash::QC_HASH_MAP & s1, qc::hash::QC_HASH_MAP & s2) noexcept {
        s1.swap(s2);
    }

}
