#pragma once

//
// QHash Map 2.0.2
//
// Austin Quick, 2016 - 2020
// https://github.com/Daskie/QHash
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

namespace qc {

    namespace config::map {

        constexpr size_t minCapacity(16u); // Must be at least `sizeof(size_t) / 2`
        constexpr size_t minBucketCount(minCapacity << 1);
        constexpr bool useIdentityHash(true);

    }

    namespace config::set { using namespace config::map; }

    namespace detail::map { // Ignore me :)

        template <typename K> using DefaultHash = std::conditional_t<config::map::useIdentityHash && sizeof(K) <= sizeof(size_t), IdentityHash<K>, Hash<K>>;

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

    //
    // ...
    //
    template <
        typename K,
        typename V,
        typename H = detail::map::DefaultHash<K>,
        typename E = std::equal_to<K>,
        typename A = std::allocator<typename detail::map::Types<K, V>::T>
    >
    class Map;

    //
    // ...
    // Defined as a `Map` whose value type is `void`.
    //
    template <
        typename K,
        typename H = detail::map::DefaultHash<K>,
        typename E = std::equal_to<K>,
        typename A = std::allocator<K>
    >
    using Set = Map<K, void, H, E, A>;

    template <typename K, typename V, typename H, typename E, typename A> class Map {

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

        public: //--------------------------------------------------------------

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

        //
        // Memory is not allocated until the first entry is inserted.
        //
        explicit Map(size_t minCapacity = config::map::minCapacity, const H & hash = H(), const E & equal = E(), const A & alloc = A());
        Map(size_t minCapacity, const A & alloc);
        Map(size_t minCapacity, const H & hash, const A & alloc);
        explicit Map(const A & alloc);
        template <typename It> Map(It first, It last, size_t minCapacity = 0u, const H & hash = H(), const E & equal = E(), const A & alloc = A());
        template <typename It> Map(It first, It last, size_t minCapacity, const A & alloc);
        template <typename It> Map(It first, It last, size_t minCapacity, const H & hash, const A & alloc);
        Map(std::initializer_list<T> entries, size_t minCapacity = 0u, const H & hash = H(), const E & equal = E(), const A & alloc = A());
        Map(std::initializer_list<T> entries, size_t minCapacity, const A & alloc);
        Map(std::initializer_list<T> entries, size_t minCapacity, const H & hash, const A & alloc);
        Map(const Map & other);
        Map(const Map & other, const A & alloc);
        Map(Map && other);
        Map(Map && other, const A & alloc);
        Map(Map && other, A && alloc);

        //
        // ...
        //
        Map & operator=(std::initializer_list<T> entries);
        Map & operator=(const Map & other);
        Map & operator=(Map && other) noexcept;

        //
        // Destructs all entries and frees all memory allocated.
        //
        ~Map();

        //
        // Prefer try_emplace over emplace over this.
        // Invalidates iterators.
        //
        std::pair<iterator, bool> insert(const T & entry);
        std::pair<iterator, bool> insert(T && entry);
        template <typename It> void insert(It first, It last);
        void insert(std::initializer_list<T> entries);

        //
        // Prefer try_emplace over this, but prefer this over insert.
        // Invalidates iterators.
        //
        std::pair<iterator, bool> emplace(const T & entry);
        std::pair<iterator, bool> emplace(T && entry);
        template <typename K_, typename V_> std::pair<iterator, bool> emplace(K_ && key, V_ && val);
        template <typename... KArgs, typename... VArgs> std::pair<iterator, bool> emplace(std::piecewise_construct_t, std::tuple<KArgs...> && kArgs, std::tuple<VArgs...> && vArgs);

        //
        // If there is no existing entry for `key`, creates a new entry in
        // place.
        // Choose this as the default insertion method of choice.
        // Invalidates iterators.
        //
        template <typename... VArgs> std::pair<iterator, bool> try_emplace(const K & key, VArgs &&... valArgs);
        template <typename... VArgs> std::pair<iterator, bool> try_emplace(K && key, VArgs &&... valArgs);

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
        void clear();

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
        std::add_lvalue_reference_t<V> at(const K & key);
        std::add_lvalue_reference_t<const V> at(const K & key) const;

        //
        // ...
        //
        std::add_lvalue_reference_t<V> operator[](const K & key);
        std::add_lvalue_reference_t<V> operator[](K && key);

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
        // As a key may correspont to as most one entry, this method is
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
        void swap(Map & other);

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
        size_t max_size() const;

        //
        // Equivalent to `bucket_count() / 2`.
        //
        size_t capacity() const;

        //
        // Will always be at least twice the number of entries.
        // Equivalent to `capacity() * 2`.
        //
        size_t bucket_count() const;

        //
        // Equivalent to `max_size() / 2`.
        //
        size_t max_bucket_count() const;

        //
        // Returns the index of the bucket into which `key` would fall.
        //
        size_t bucket(const K & key) const;

        //
        // How many entries are "in" the bucket at index `i`.
        //
        size_t bucket_size(size_t i) const;

        //
        // Returns the ratio of entries to buckets.
        // Equivalent to `float(m_size) / float(m_bucketCount)`.
        //
        float load_factor() const;

        //
        // Is always `0.5f`.
        //
        float max_load_factor() const;

        //
        // Returns an instance of `H` or `hasher`.
        //
        hasher hash_function() const;

        //
        // Returns an instance of `E` or `key_equal`.
        //
        key_equal key_eq() const;

        //
        // Returns an instance of `A` or `allocator_type`.
        //
        allocator_type get_allocator() const;

        //
        // Returns whether `other` has the same entries.
        //
        bool operator==(const Map & other);

        private: //-------------------------------------------------------------

        static constexpr bool k_isSet = std::is_same_v<V, void>;

        size_t m_size;
        size_t m_bucketCount;
        Bucket * m_buckets;
        H m_hash;
        E m_equal;
        Allocator m_alloc;

        template <typename KTuple, typename VTuple, size_t... t_kIndices, size_t... t_vIndices> std::pair<iterator, bool> m_emplace(KTuple && kTuple, VTuple && vTuple, std::index_sequence<t_kIndices...>, std::index_sequence<t_vIndices...>);

        template <typename K_, typename... Args> std::pair<iterator, bool> m_try_emplace(size_t hash, K_ && key, Args &&... args);

        void m_propagate(T & entry, size_t i, Dist dist);

        void m_erase(const_iterator position);

        template <bool t_zeroDists> void m_clear();

        template <bool t_const> std::pair<Iterator<t_const>, Iterator<t_const>> m_equal_range(const K & key, size_t hash) const;

        template <bool t_const> Iterator<t_const> m_begin() const noexcept;

        template <bool t_const> Iterator<t_const> m_end() const noexcept;

        template <bool t_const> Iterator<t_const> m_find(const K & key, size_t hash) const;

        void m_rehash(size_t bucketCount);

        size_t m_indexOf(size_t hash) const;

        void m_allocate();

        void m_deallocate();

        void m_zeroDists();

        void m_copyBuckets(const Bucket * bucket);

        void m_moveBuckets(Bucket * bucket);

    };

    //
    // Forward iterator
    //
    template <typename K, typename V, typename H, typename E, typename A>
    template <bool t_const>
    class Map<K, V, H, E, A>::Iterator {

        friend Map;

        public: //--------------------------------------------------------------

        using iterator_category = std::forward_iterator_tag;
        using value_type = std::conditional_t<t_const, const Map::T, Map::T>;
        using difference_type = ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

        //
        // ...
        //
        constexpr Iterator(const Iterator & other) noexcept = default;
        template <bool t_const_ = t_const, typename = std::enable_if_t<t_const_>> constexpr Iterator(const Iterator<!t_const> & other) noexcept;

        //
        // ...
        //
        value_type & operator*() const;

        //
        // ...
        //
        value_type * operator->() const;

        //
        // Incrementing past the end iterator is undefined and unsupported behavior.
        //
        Iterator & operator++();

        //
        // Incrementing past the end iterator is undefined and unsupported behavior.
        //
        Iterator operator++(int);

        //
        // ...
        //
        template <bool t_const_> bool operator==(const Iterator<t_const_> & it) const;

        private: //-------------------------------------------------------------

        using Bucket = std::conditional_t<t_const, const Map::Bucket, Map::Bucket>;

        Bucket * m_bucket;

        template <typename Bucket_> constexpr explicit Iterator(Bucket_ * bucket) noexcept;

    };

}

namespace std {

    //
    // Specializes std::swap for qc::Map.
    //
    template <typename K, typename V, typename H, typename E, typename A> void swap(qc::Map<K, V, H, E, A> & s1, qc::Map<K, V, H, E, A> & s2);

}

// IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////////

#define QC_MAP Map<K, V, H, E, A>
#define QC_MAP_TEMPLATE template <typename K, typename V, typename H, typename E, typename A>

namespace qc {

    // Map =====================================================================

    QC_MAP_TEMPLATE
    inline QC_MAP::Map(size_t minCapacity, const H & hash, const E & equal, const A & alloc) :
        m_size(),
        m_bucketCount(minCapacity <= config::map::minCapacity ? config::map::minBucketCount : detail::hash::ceil2(minCapacity << 1)),
        m_buckets(nullptr),
        m_hash(hash),
        m_equal(equal),
        m_alloc(alloc)
    {}

    QC_MAP_TEMPLATE
    inline QC_MAP::Map(size_t minCapacity, const A & alloc) :
        Map(minCapacity, H(), E(), alloc)
    {}

    QC_MAP_TEMPLATE
    inline QC_MAP::Map(size_t minCapacity, const H & hash, const A & alloc) :
        Map(minCapacity, hash, E(), alloc)
    {}

    QC_MAP_TEMPLATE
    inline QC_MAP::Map(const A & alloc) :
        Map(config::map::minCapacity, H(), E(), alloc)
    {}

    QC_MAP_TEMPLATE
    template <typename It>
    inline QC_MAP::Map(It first, It last, size_t minCapacity, const H & hash, const E & equal, const A & alloc) :
        Map(minCapacity ? minCapacity : std::distance(first, last), hash, equal, alloc)
    {
        insert(first, last);
    }

    QC_MAP_TEMPLATE
    template <typename It>
    inline QC_MAP::Map(It first, It last, size_t minCapacity, const A & alloc) :
        Map(first, last, minCapacity, H(), E(), alloc)
    {}

    QC_MAP_TEMPLATE
    template <typename It>
    inline QC_MAP::Map(It first, It last, size_t minCapacity, const H & hash, const A & alloc) :
        Map(first, last, minCapacity, hash, E(), alloc)
    {}

    QC_MAP_TEMPLATE
    inline QC_MAP::Map(std::initializer_list<T> entries, size_t minCapacity, const H & hash, const E & equal, const A & alloc) :
        Map(minCapacity ? minCapacity : entries.size(), hash, equal, alloc)
    {
        insert(entries);
    }

    QC_MAP_TEMPLATE
    inline QC_MAP::Map(std::initializer_list<T> entries, size_t minCapacity, const A & alloc) :
        Map(entries, minCapacity, H(), E(), alloc)
    {}

    QC_MAP_TEMPLATE
    inline QC_MAP::Map(std::initializer_list<T> entries, size_t minCapacity, const H & hash, const A & alloc) :
        Map(entries, minCapacity, hash, E(), alloc)
    {}

    QC_MAP_TEMPLATE
    inline QC_MAP::Map(const Map & other) :
        Map(other, std::allocator_traits<A>::select_on_container_copy_construction(other.m_alloc))
    {}

    QC_MAP_TEMPLATE
    inline QC_MAP::Map(const Map & other, const A & alloc) :
        m_size(other.m_size),
        m_bucketCount(other.m_bucketCount),
        m_buckets(nullptr),
        m_hash(other.m_hash),
        m_equal(other.m_equal),
        m_alloc(alloc)
    {
        m_allocate();
        m_copyBuckets(other.m_buckets);
    }

    QC_MAP_TEMPLATE
    inline QC_MAP::Map(Map && other) :
        Map(std::move(other), std::move(other.m_alloc))
    {}

    QC_MAP_TEMPLATE
    inline QC_MAP::Map(Map && other, const A & alloc) :
        Map(std::move(other), A(alloc))
    {}

    QC_MAP_TEMPLATE
    inline QC_MAP::Map(Map && other, A && alloc) :
        m_size(other.m_size),
        m_bucketCount(other.m_bucketCount),
        m_buckets(other.m_buckets),
        m_hash(std::move(other.m_hash)),
        m_equal(std::move(other.m_equal)),
        m_alloc(std::move(alloc))
    {
        other.m_size = 0u;
        other.m_bucketCount = 0u;
        other.m_buckets = nullptr;
    }

    QC_MAP_TEMPLATE
    inline QC_MAP::~Map() {
        if (m_buckets) {
            m_clear<false>();
            m_deallocate();
        }
    }

    QC_MAP_TEMPLATE
    inline QC_MAP & QC_MAP::operator=(std::initializer_list<T> entries) {
        clear();
        insert(entries);

        return *this;
    }

    QC_MAP_TEMPLATE
    inline QC_MAP & QC_MAP::operator=(const Map & other) {
        if (&other == this) {
            return *this;
        }

        if (m_buckets) {
            m_clear<false>();
            if (m_bucketCount != other.m_bucketCount || m_alloc != other.m_alloc) {
                m_deallocate();
            }
        }

        m_size = other.m_size;
        m_bucketCount = other.m_bucketCount;
        m_hash = other.m_hash;
        m_equal = other.m_equal;
        if constexpr (AllocatorTraits::propagate_on_container_copy_assignment::value) {
            m_alloc = std::allocator_traits<A>::select_on_container_copy_construction(other.m_alloc);
        }

        if (other.m_buckets) {
            if (!m_buckets) {
                m_allocate();
            }
            m_copyBuckets(other.m_buckets);
        }

        return *this;
    }

    QC_MAP_TEMPLATE
    inline QC_MAP & QC_MAP::operator=(Map && other) noexcept {
        if (&other == this) {
            return *this;
        }

        if (m_buckets) {
            m_clear<false>();
            m_deallocate();
        }

        m_size = other.m_size;
        m_bucketCount = other.m_bucketCount;
        m_hash = std::move(other.m_hash);
        m_equal = std::move(other.m_equal);
        if constexpr (AllocatorTraits::propagate_on_container_move_assignment::value) {
            m_alloc = std::move(other.m_alloc);
        }

        if (AllocatorTraits::propagate_on_container_move_assignment::value || m_alloc == other.m_alloc) {
            m_buckets = other.m_buckets;
            other.m_buckets = nullptr;
        }
        else {
            m_allocate();
            m_moveBuckets(other.m_buckets);
            other.m_clear<false>();
            other.m_deallocate();
        }

        other.m_size = 0u;
        other.m_bucketCount = 0u;

        return *this;
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::insert(const T & entry) -> std::pair<iterator, bool> {
        return emplace(entry);
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::insert(T && entry) -> std::pair<iterator, bool> {
        return emplace(std::move(entry));
    }

    QC_MAP_TEMPLATE
    template <typename It>
    inline void QC_MAP::insert(It first, It last) {
        while (first != last) {
            emplace(*first);
            ++first;
        }
    }

    QC_MAP_TEMPLATE
    inline void QC_MAP::insert(std::initializer_list<T> entries) {
        for (const T & entry : entries) {
            emplace(entry);
        }
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::emplace(const T & entry) -> std::pair<iterator, bool> {
        if constexpr (k_isSet) {
            return try_emplace(entry);
        }
        else {
            return try_emplace(entry.first, entry.second);
        }
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::emplace(T && entry) -> std::pair<iterator, bool> {
        if constexpr (k_isSet) {
            return try_emplace(std::move(entry));
        }
        else {
            return try_emplace(std::move(entry.first), std::move(entry.second));
        }
    }

    QC_MAP_TEMPLATE
    template <typename K_, typename V_>
    inline auto QC_MAP::emplace(K_ && key, V_ && val) -> std::pair<iterator, bool> {
        static_assert(!k_isSet, "This is not a set operation");
        return try_emplace(std::forward<K_>(key), std::forward<V_>(val));
    }

    QC_MAP_TEMPLATE
    template <typename... KArgs, typename... VArgs>
    inline auto QC_MAP::emplace(std::piecewise_construct_t, std::tuple<KArgs...> && kArgs, std::tuple<VArgs...> && vArgs) -> std::pair<iterator, bool> {
        static_assert(!k_isSet, "This is not a set operation");
        return m_emplace(std::move(kArgs), std::move(vArgs), std::index_sequence_for<KArgs...>(), std::index_sequence_for<VArgs...>());
    }

    QC_MAP_TEMPLATE
    template <typename KTuple, typename VTuple, size_t... t_kIndices, size_t... t_vIndices>
    inline auto QC_MAP::m_emplace(KTuple && kTuple, VTuple && vTuple, std::index_sequence<t_kIndices...>, std::index_sequence<t_vIndices...>) -> std::pair<iterator, bool> {
        return try_emplace(K(std::move(std::get<t_kIndices>(kTuple))...), std::move(std::get<t_vIndices>(vTuple))...);
    }

    QC_MAP_TEMPLATE
    template <typename... VArgs>
    inline auto QC_MAP::try_emplace(const K & key, VArgs &&... vargs) -> std::pair<iterator, bool> {
        return m_try_emplace(m_hash(key), key, std::forward<VArgs>(vargs)...);
    }

    QC_MAP_TEMPLATE
    template <typename... VArgs>
    inline auto QC_MAP::try_emplace(K && key, VArgs &&... vargs) -> std::pair<iterator, bool> {
        return m_try_emplace(m_hash(key), std::move(key), std::forward<VArgs>(vargs)...);
    }

    QC_MAP_TEMPLATE
    template <typename K_, typename... VArgs>
    inline auto QC_MAP::m_try_emplace(size_t hash, K_ && key, VArgs &&... vargs) -> std::pair<iterator, bool> {
        static_assert(sizeof...(VArgs) == 0u || std::is_default_constructible_v<V>, "Value type must be default constructible");

        if (!m_buckets) m_allocate();
        size_t i(m_indexOf(hash));
        Dist dist(1u);

        while (true) {
            Bucket & bucket(m_buckets[i]);

            // Can be inserted
            if (bucket.dist < dist) {
                if (m_size >= (m_bucketCount >> 1)) {
                    m_rehash(m_bucketCount << 1);
                    return m_try_emplace(hash, std::forward<K_>(key), std::forward<VArgs>(vargs)...);
                }

                // Talue here has smaller dist, robin hood
                if (bucket.dist) {
                    m_propagate(bucket.entry(), i + 1u, bucket.dist + 1u);
                    bucket.entry().~T();
                }

                // Open slot
                AllocatorTraits::construct(m_alloc, &bucket.key, std::forward<K_>(key));
                if constexpr (!k_isSet) {
                    AllocatorTraits::construct(m_alloc, &bucket.val, std::forward<VArgs>(vargs)...);
                }

                bucket.dist = dist;
                ++m_size;
                return { iterator(&bucket), true };
            }

            // Talue already exists
            if (m_equal(bucket.key, key)) {
                return { iterator(&bucket), false };
            }

            ++i;
            ++dist;

            if (i >= m_bucketCount) i = 0u;
        }

        // Will never reach reach this return
        return { end(), false };
    }

    QC_MAP_TEMPLATE
    inline void QC_MAP::m_propagate(T & entry, size_t i, Dist dist) {
        while (true) {
            if (i >= m_bucketCount) i = 0u;
            Bucket & bucket(m_buckets[i]);

            if (!bucket.dist) {
                AllocatorTraits::construct(m_alloc, &bucket.entry(), std::move(entry));
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

    QC_MAP_TEMPLATE
    inline size_t QC_MAP::erase(const K & key) {
        iterator it(find(key));
        if (it == end()) {
            return 0u;
        }
        m_erase(it);
        if (m_size <= (m_bucketCount >> 3) && m_bucketCount > config::set::minBucketCount) {
            m_rehash(m_bucketCount >> 1);
        }
        return 1u;
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::erase(const_iterator position) -> iterator {
        iterator endIt(end());
        if (position != endIt) {
            m_erase(position);
            if (m_size <= (m_bucketCount >> 3) && m_bucketCount > config::map::minBucketCount) {
                m_rehash(m_bucketCount >> 1);
                endIt = end();
            }
        }
        return endIt;
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::erase(const_iterator first, const_iterator last) -> iterator {
        if (first != last) {
            do {
                m_erase(first);
                ++first;
            } while (first != last);
            reserve(m_size);
        }
        return end();
    }

    QC_MAP_TEMPLATE
    inline void QC_MAP::m_erase(const_iterator position) {
        size_t i(position.m_bucket - m_buckets), j(i + 1u);

        while (true) {
            if (j >= m_bucketCount) j = 0u;

            if (m_buckets[j].dist <= 1u) {
                break;
            }

            m_buckets[i].entry() = std::move(m_buckets[j].entry());
            m_buckets[i].dist = m_buckets[j].dist - 1u;

            ++i; ++j;
            if (i >= m_bucketCount) i = 0u;
        }

        m_buckets[i].entry().~T();
        m_buckets[i].dist = 0u;
        --m_size;
    }

    QC_MAP_TEMPLATE
    inline void QC_MAP::clear() {
        m_clear<true>();
    }

    QC_MAP_TEMPLATE
    template <bool t_zeroDists>
    inline void QC_MAP::m_clear() {
        if constexpr (std::is_trivially_destructible_v<T>) {
            if constexpr (t_zeroDists) {
                if (m_size) m_zeroDists();
            }
        }
        else {
            for (size_t i(0u), n(0u); n < m_size; ++i) {
                if (m_buckets[i].dist) {
                    m_buckets[i].entry().~T();
                    if constexpr (t_zeroDists) {
                        m_buckets[i].dist = 0u;
                    }
                    ++n;
                }
            }
        }

        m_size = 0u;
    }

    QC_MAP_TEMPLATE
    inline bool QC_MAP::contains(const K & key) const {
        return contains(key, m_hash(key));
    }

    QC_MAP_TEMPLATE
    inline bool QC_MAP::contains(const K & key, size_t hash) const {
        return find(key, hash) != cend();
    }

    QC_MAP_TEMPLATE
    inline size_t QC_MAP::count(const K & key) const {
        return contains(key);
    }

    QC_MAP_TEMPLATE
    inline size_t QC_MAP::count(const K & key, size_t hash) const {
        return contains(key, hash);
    }

    QC_MAP_TEMPLATE
    inline std::add_lvalue_reference_t<V> QC_MAP::at(const K & key) {
        static_assert(!k_isSet, "This is not a set operation");
        return find(key)->second;
    }

    QC_MAP_TEMPLATE
    inline std::add_lvalue_reference_t<const V> QC_MAP::at(const K & key) const {
        static_assert(!k_isSet, "This is not a set operation");
        return find(key)->second;
    }

    QC_MAP_TEMPLATE
    inline std::add_lvalue_reference_t<V> QC_MAP::operator[](const K & key) {
        static_assert(!k_isSet, "This is not a set operation");
        return try_emplace(key).first->second;
    }

    QC_MAP_TEMPLATE
    inline std::add_lvalue_reference_t<V> QC_MAP::operator[](K && key) {
        static_assert(!k_isSet, "This is not a set operation");
        return try_emplace(std::move(key)).first->second;
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::begin() noexcept -> iterator {
        return m_begin<false>();
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::begin() const noexcept -> const_iterator {
        return m_begin<true>();
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::cbegin() const noexcept -> const_iterator {
        return m_begin<true>();
    }

    QC_MAP_TEMPLATE
    template <bool t_const>
    inline auto QC_MAP::m_begin() const noexcept -> Iterator<t_const> {
        if (!m_size) {
            return m_end<t_const>();
        }

        for (size_t i(0u); ; ++i) {
            if (m_buckets[i].dist) {
                return Iterator<t_const>(m_buckets + i);
            }
        }
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::end() noexcept -> iterator {
        return m_end<false>();
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::end() const noexcept -> const_iterator {
        return m_end<true>();
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::cend() const noexcept -> const_iterator {
        return m_end<true>();
    }

    QC_MAP_TEMPLATE
    template <bool t_const>
    inline auto QC_MAP::m_end() const noexcept -> Iterator<t_const> {
        return Iterator<t_const>(m_buckets + m_bucketCount);
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::find(const K & key) -> iterator {
        return find(key, m_hash(key));
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::find(const K & key) const -> const_iterator {
        return find(key, m_hash(key));
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::find(const K & key, size_t hash) -> iterator {
        return m_find<false>(key, hash);
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::find(const K & key, size_t hash) const -> const_iterator {
        return m_find<true>(key, hash);
    }

    QC_MAP_TEMPLATE
    template <bool t_const>
    inline auto QC_MAP::m_find(const K & key, size_t hash) const -> Iterator<t_const> {
        if (!m_buckets) {
            return m_end<t_const>();
        }

        size_t i(m_indexOf(hash));
        Dist dist(1u);

        while (true) {
            const Bucket & bucket(m_buckets[i]);

            if (bucket.dist < dist) {
                return m_end<t_const>();
            }

            if (m_equal(bucket.key, key)) {
                return Iterator<t_const>(&bucket);
            }

            ++i;
            ++dist;

            if (i >= m_bucketCount) i = 0u;
        };

        // Will never reach reach this return
        return m_end<t_const>();
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::equal_range(const K & key) -> std::pair<iterator, iterator> {
        return equal_range(key, m_hash(key));
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::equal_range(const K & key) const -> std::pair<const_iterator, const_iterator> {
        return equal_range(key, m_hash(key));
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::equal_range(const K & key, size_t hash) -> std::pair<iterator, iterator> {
        return m_equal_range<false>(key, hash);
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::equal_range(const K & key, size_t hash) const -> std::pair<const_iterator, const_iterator> {
        return m_equal_range<true>(key, hash);
    }

    QC_MAP_TEMPLATE
    template <bool t_const>
    inline auto QC_MAP::m_equal_range(const K & key, size_t hash) const -> std::pair<Iterator<t_const>, Iterator<t_const>> {
        Iterator<t_const> it(m_find<t_const>(key, hash));
        return { it, it };
    }

    QC_MAP_TEMPLATE
    inline void QC_MAP::reserve(size_t capacity) {
        rehash(capacity << 1);
    }

    QC_MAP_TEMPLATE
    inline void QC_MAP::rehash(size_t bucketCount) {
        bucketCount = detail::hash::ceil2(bucketCount);
        if (bucketCount < config::map::minBucketCount) bucketCount = config::map::minBucketCount;
        else if (bucketCount < (m_size << 1)) bucketCount = m_size << 1u;

        if (bucketCount != m_bucketCount) {
            if (m_buckets) {
                m_rehash(bucketCount);
            }
            else {
                m_bucketCount = bucketCount;
            }
        }
    }

    QC_MAP_TEMPLATE
    inline void QC_MAP::m_rehash(size_t bucketCount) {
        size_t oldSize(m_size);
        size_t oldBucketCount(m_bucketCount);
        Bucket * oldBuckets(m_buckets);

        m_size = 0u;
        m_bucketCount = bucketCount;
        m_allocate();

        for (size_t i(0u), n(0u); n < oldSize; ++i) {
            Bucket & bucket(oldBuckets[i]);
            if (bucket.dist) {
                emplace(std::move(bucket.entry()));
                bucket.entry().~T();
                ++n;
            }
        }

        AllocatorTraits::deallocate(m_alloc, oldBuckets, oldBucketCount + 1u);
    }

    QC_MAP_TEMPLATE
    inline void QC_MAP::swap(Map & other) {
        std::swap(m_size, other.m_size);
        std::swap(m_bucketCount, other.m_bucketCount);
        std::swap(m_buckets, other.m_buckets);
        std::swap(m_hash, other.m_hash);
        std::swap(m_equal, other.m_equal);
        if constexpr (AllocatorTraits::propagate_on_container_swap::value) {
            std::swap(m_alloc, other.m_alloc);
        }
    }

    QC_MAP_TEMPLATE
    inline bool QC_MAP::empty() const noexcept {
        return m_size == 0u;
    }

    QC_MAP_TEMPLATE
    inline size_t QC_MAP::size() const noexcept {
        return m_size;
    }

    QC_MAP_TEMPLATE
    inline size_t QC_MAP::max_size() const {
        return max_bucket_count() >> 1u;
    }

    QC_MAP_TEMPLATE
    inline size_t QC_MAP::capacity() const {
        return m_bucketCount >> 1u;
    }

    QC_MAP_TEMPLATE
    inline size_t QC_MAP::bucket_count() const {
        return m_bucketCount;
    }

    QC_MAP_TEMPLATE
    inline size_t QC_MAP::max_bucket_count() const {
        return std::numeric_limits<size_t>::max() - 1u;
    }

    QC_MAP_TEMPLATE
    inline size_t QC_MAP::bucket(const K & key) const {
        return m_indexOf(m_hash(key));
    }

    QC_MAP_TEMPLATE
    inline size_t QC_MAP::bucket_size(size_t i) const {
        if (i >= m_bucketCount || !m_buckets) {
            return 0u;
        }

        Dist dist(1u);
        while (m_buckets[i].dist > dist) {
            ++i;
            ++dist;

            if (i >= m_bucketCount) i = 0u;
        }

        size_t n(0u);
        while (m_buckets[i].dist == dist) {
            ++i;
            ++dist;
            ++n;

            if (i >= m_bucketCount) i = 0u;
        }

        return n;
    }

    QC_MAP_TEMPLATE
    inline float QC_MAP::load_factor() const {
        return float(m_size) / float(m_bucketCount);
    }

    QC_MAP_TEMPLATE
    inline float QC_MAP::max_load_factor() const {
        return 0.5f;
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::hash_function() const -> hasher {
        return m_hash;
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::key_eq() const -> key_equal {
        return m_equal;
    }

    QC_MAP_TEMPLATE
    inline auto QC_MAP::get_allocator() const -> allocator_type {
        return m_alloc;
    }

    QC_MAP_TEMPLATE
    inline bool QC_MAP::operator==(const QC_MAP & other) {
        if (m_size != other.m_size) {
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

    QC_MAP_TEMPLATE
    inline size_t QC_MAP::m_indexOf(size_t hash) const {
        return hash & (m_bucketCount - 1u);
    }

    QC_MAP_TEMPLATE
    inline void QC_MAP::m_allocate() {
        m_buckets = AllocatorTraits::allocate(m_alloc, m_bucketCount + 1u);
        m_zeroDists();
        m_buckets[m_bucketCount].dist = std::numeric_limits<Dist>::max();
    }

    QC_MAP_TEMPLATE
    inline void QC_MAP::m_deallocate() {
        AllocatorTraits::deallocate(m_alloc, m_buckets, m_bucketCount + 1u);
        m_buckets = nullptr;
    }

    QC_MAP_TEMPLATE
    inline void QC_MAP::m_zeroDists() {
        // If dist is smaller than a word, and the bucket is small, just zero everything
        if constexpr (sizeof(Dist) < sizeof(size_t) && sizeof(Bucket) <= sizeof(size_t) * 2) {
            std::fill_n(reinterpret_cast<size_t *>(m_buckets), (m_bucketCount * sizeof(Bucket)) / sizeof(size_t), 0u);
        }
        else {
            for (size_t i(0u); i < m_bucketCount; ++i) m_buckets[i].dist = 0u;
        }
    }

    QC_MAP_TEMPLATE
    inline void QC_MAP::m_copyBuckets(const Bucket * buckets) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            if (m_size) {
                std::copy_n(reinterpret_cast<const size_t *>(buckets), (m_bucketCount * sizeof(Bucket)) / sizeof(size_t), reinterpret_cast<size_t *>(m_buckets));
            }
        }
        else {
            for (size_t i(0u), n(0u); n < m_size; ++i) {
                if (m_buckets[i].dist = buckets[i].dist) {
                    AllocatorTraits::construct(m_alloc, &m_buckets[i].entry(), buckets[i].entry());
                    ++n;
                }
            }
        }
    }

    QC_MAP_TEMPLATE
    inline void QC_MAP::m_moveBuckets(Bucket * buckets) {
        if constexpr (std::is_trivially_copyable_v<T>) {
            if (m_size) {
                std::copy_n(reinterpret_cast<const size_t *>(buckets), (m_bucketCount * sizeof(Bucket)) / sizeof(size_t), reinterpret_cast<size_t *>(m_buckets));
            }
        }
        else {
            for (size_t i(0u), n(0u); n < m_size; ++i) {
                if (m_buckets[i].dist = buckets[i].dist) {
                    AllocatorTraits::construct(m_alloc, &m_buckets[i].entry(), std::move(buckets[i].entry()));
                    ++n;
                }
            }
        }
    }

    // Iterator ================================================================

    QC_MAP_TEMPLATE
    template <bool t_const>
    template <bool t_const_, typename>
    constexpr QC_MAP::Iterator<t_const>::Iterator(const Iterator<!t_const> & other) noexcept :
        m_bucket(other.m_bucket)
    {}

    QC_MAP_TEMPLATE
    template <bool t_const>
    template <typename Bucket_>
    constexpr QC_MAP::Iterator<t_const>::Iterator(Bucket_ * bucket) noexcept :
        m_bucket(const_cast<Bucket *>(bucket))
    {}

    QC_MAP_TEMPLATE
    template <bool t_const>
    inline auto QC_MAP::Iterator<t_const>::operator*() const -> value_type & {
        return m_bucket->entry();
    }

    QC_MAP_TEMPLATE
    template <bool t_const>
    inline auto QC_MAP::Iterator<t_const>::operator->() const -> value_type * {
        return &m_bucket->entry();
    }

    QC_MAP_TEMPLATE
    template <bool t_const>
    inline auto QC_MAP::Iterator<t_const>::operator++() -> Iterator & {
        do {
            ++m_bucket;
        } while (!m_bucket->dist);

        return *this;
    }

    QC_MAP_TEMPLATE
    template <bool t_const>
    inline auto QC_MAP::Iterator<t_const>::operator++(int) -> Iterator {
        Iterator temp(*this);
        operator++();
        return temp;
    }

    QC_MAP_TEMPLATE
    template <bool t_const>
    template <bool t_const_>
    inline bool QC_MAP::Iterator<t_const>::operator==(const Iterator<t_const_> & o) const {
        return m_bucket == o.m_bucket;
    }

}

namespace std {

    QC_MAP_TEMPLATE
    inline void swap(qc::QC_MAP & s1, qc::QC_MAP & s2) {
        s1.swap(s2);
    }

}
