#pragma once

//
// QC Hash 2.2.3
//
// Austin Quick, 2016 - 2021
// https://github.com/Daskie/qc-hash
//
// Some nomenclature:
//   - Key: The data the maps a value
//   - Value: The data mapped by a key
//   - Element: A key-value pair, or just key in the case of a set. One "thing" in the map/set
//   - Slot: Purely conceptual. One "spot" in the backing array. Each slot may have an element or be empty
//   - Chunk: Conceptually a grouping of 8 slots. In memory, it is 8 distance bytes followed by 8 elements
//   - Bucket: Purely conceptual. This is a set of elements that all map to the same slot
//   - Size: The number of elements in the map/set
//   - Slot Count: The number of slots in the map/set. Is always at least twice size, and half of slot count
//   - Capacity: The number of elements that the map/set can currently hold without growing. Exactly half of slot count
//

#include <cstdint>
#include <cstring>

#include <bit>
#include <initializer_list>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

#include "qc-hash-chunk.hpp"

namespace qc_hash_chunk {

    namespace config {

        constexpr size_t minCapacity{16u}; // Must be at least 4
        constexpr size_t minSlotCount{minCapacity * 2u};

    }

    using u8 = uint8_t;
    using u64 = uint64_t;

    template <typename K, typename V> struct _Element {

        using E = std::pair<K, V>;

        K key;
        V val;

        // Ensure this cannot be constructed through normal means
        _Element() = delete;

        // Ensure this cannot be destructed through normal means
        ~_Element() = delete;

        E & get() noexcept { return reinterpret_cast<E &>(*this); }

        const E & get() const noexcept { return reinterpret_cast<const E &>(*this); }

    };

    template <typename K> struct _Element<K, void> {

        using E = K;

        K key;

        // Ensure this cannot be constructed through normal means
        _Element() = delete;

        // Ensure this cannot be destructed through normal means
        ~_Element() = delete;

        E & get() noexcept { return key; }

        const E & get() const noexcept { return key; }

    };

    template <typename K, typename V> struct _Chunk {
        union {
            u8 dists[8];
            u64 distsData;
        };
        _Element<K, V> elements[8];

        // Ensure this cannot be constructed through normal means
        _Chunk() = delete;

        // Ensure this cannot be destructed through normal means
        ~_Chunk() = delete;
    };

    template <typename T1, typename T2> concept IsSame = std::is_same_v<T1, T2>;
    template <typename H, typename K> concept IsHashCallable = requires (const K & key) { { H()(key) } -> IsSame<size_t>; };
    template <typename KE, typename K> concept IsKeyEqualCallable = requires (const K & key1, const K & key2) { { KE()(key1, key2) } -> IsSame<bool>; };

    //
    // ...
    //
    template <typename K, typename V, typename H = Hash<K>, typename KE = std::equal_to<K>, typename A = std::allocator<std::pair<K, V>>> class Map;

    //
    // ...
    // Defined as a `Map` whose mapped type is `void`.
    //
    template <typename K, typename H = Hash<K>, typename KE = std::equal_to<K>, typename A = std::allocator<K>> using Set = Map<K, void, H, KE, A>;

    template <typename K, typename V, typename H, typename KE, typename A> class Map {

        using E = typename _Element<K, V>::E;

        template <bool constant> class _Iterator;
        template <bool constant> friend class _Iterator;

        using _Chunk = _Chunk<K, V>;
        using _Element = _Element<K, V>;
        using _Allocator = typename std::allocator_traits<A>::template rebind_alloc<u64>;
        using _AllocatorTraits = std::allocator_traits<_Allocator>;

        public: //--------------------------------------------------------------

        using key_type = K;
        using mapped_type = V;
        using value_type = E;
        using hasher = H;
        using key_equal = KE;
        using allocator_type = A;
        using reference = E &;
        using const_reference = const E &;
        using pointer = typename std::allocator_traits<A>::pointer;
        using const_pointer = typename std::allocator_traits<A>::const_pointer;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        using iterator = _Iterator<false>;
        using const_iterator = _Iterator<true>;


        static_assert(IsHashCallable<H, K>);
        static_assert(IsKeyEqualCallable<KE, K>);
        static_assert(std::is_nothrow_move_constructible_v<E>);
        static_assert(std::is_nothrow_move_assignable_v<E>);
        static_assert(std::is_nothrow_swappable_v<E>);
        static_assert(std::is_nothrow_destructible_v<E>);
        static_assert(std::is_nothrow_default_constructible_v<H>);
        static_assert(std::is_nothrow_move_constructible_v<H>);
        static_assert(std::is_nothrow_move_assignable_v<H>);
        static_assert(std::is_nothrow_swappable_v<H>);
        static_assert(std::is_nothrow_destructible_v<H>);
        static_assert(std::is_nothrow_default_constructible_v<KE>);
        static_assert(std::is_nothrow_move_constructible_v<KE>);
        static_assert(std::is_nothrow_move_assignable_v<KE>);
        static_assert(std::is_nothrow_swappable_v<KE>);
        static_assert(std::is_nothrow_destructible_v<KE>);
        static_assert(std::is_nothrow_default_constructible_v<A>);
        static_assert(std::is_nothrow_move_constructible_v<A>);
        static_assert(std::is_nothrow_move_assignable_v<A> || !std::allocator_traits<A>::propagate_on_container_move_assignment::value);
        static_assert(std::is_nothrow_swappable_v<A> || !std::allocator_traits<A>::propagate_on_container_swap::value);
        static_assert(std::is_nothrow_destructible_v<A>);

        //
        // Memory is not allocated until the first element is inserted.
        //
        explicit Map(size_t minCapacity = config::minCapacity, const H & hash = {}, const KE & equal = {}, const A & alloc = {}) noexcept;
        Map(size_t minCapacity, const A & alloc) noexcept;
        Map(size_t minCapacity, const H & hash, const A & alloc) noexcept;
        explicit Map(const A & alloc) noexcept;
        template <typename It> Map(It first, It last, size_t minCapacity = 0u, const H & hash = {}, const KE & equal = {}, const A & alloc = {});
        template <typename It> Map(It first, It last, size_t minCapacity, const A & alloc);
        template <typename It> Map(It first, It last, size_t minCapacity, const H & hash, const A & alloc);
        Map(std::initializer_list<E> elements, size_t minCapacity = 0u, const H & hash = {}, const KE & equal = {}, const A & alloc = {});
        Map(std::initializer_list<E> elements, size_t minCapacity, const A & alloc);
        Map(std::initializer_list<E> elements, size_t minCapacity, const H & hash, const A & alloc);
        Map(const Map & other);
        Map(const Map & other, const A & alloc);
        Map(Map && other) noexcept;
        Map(Map && other, const A & alloc) noexcept;
        Map(Map && other, A && alloc) noexcept;

        //
        // ...
        //
        Map & operator=(std::initializer_list<E> elements);
        Map & operator=(const Map & other);
        Map & operator=(Map && other) noexcept;

        //
        // Destructs all elements and frees all memory allocated.
        //
        ~Map() noexcept;

        //
        // Prefer try_emplace over emplace over this.
        // Invalidates iterators.
        //
        std::pair<iterator, bool> insert(const E & element);
        std::pair<iterator, bool> insert(E && element);
        template <typename It> void insert(It first, It last);
        void insert(std::initializer_list<E> elements);

        //
        // Prefer try_emplace over this, but prefer this over insert.
        // Invalidates iterators.
        //
        std::pair<iterator, bool> emplace(const E & element);
        std::pair<iterator, bool> emplace(E && element);
        template <typename K_, typename T_> std::pair<iterator, bool> emplace(K_ && key, T_ && val) requires (!std::is_same_v<V, void>);
        template <typename... KArgs, typename... TArgs> std::pair<iterator, bool> emplace(std::piecewise_construct_t, std::tuple<KArgs...> && kArgs, std::tuple<TArgs...> && tArgs) requires (!std::is_same_v<V, void>);

        //
        // If there is no existing element for `key`, creates a new element in
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
        bool erase(const K & key);
        void erase(iterator position);

        //
        // All elements are removed and destructed.
        // Does not change capacity or free memory.
        // Invalidates iterators.
        //
        void clear() noexcept;

        //
        // Returns whether or not the map contains an element for `key`.
        //
        bool contains(const K & key) const;
        bool contains(const K & key, size_t hash) const;

        //
        // Returns `1` if the map contains an element for `key` and `0` if it does
        // not.
        //
        size_t count(const K & key) const;
        size_t count(const K & key, size_t hash) const;

        //
        // ...
        //
        std::add_lvalue_reference_t<V> at(const K & key) requires (!std::is_same_v<V, void>);
        std::add_lvalue_reference_t<const V> at(const K & key) const requires (!std::is_same_v<V, void>);

        //
        // ...
        // TODO: Use requires once MSVC gets its shit together
        //
        std::add_lvalue_reference_t<V> operator[](const K & key);
        std::add_lvalue_reference_t<V> operator[](K && key);

        //
        // Returns an iterator to the first element in the map.
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
        // Returns an iterator to the element for `key`, or the end iterator if no
        // such element exists.
        //
        iterator find(const K & key);
        const_iterator find(const K & key) const;
        iterator find(const K & key, size_t hash);
        const_iterator find(const K & key, size_t hash) const;

        //
        // As a key may correspond to as most one element, this method is
        // equivalent to `find`, except returning a pair of duplicate iterators.
        //
        std::pair<iterator, iterator> equal_range(const K & key);
        std::pair<const_iterator, const_iterator> equal_range(const K & key) const;
        std::pair<iterator, iterator> equal_range(const K & key, size_t hash);
        std::pair<const_iterator, const_iterator> equal_range(const K & key, size_t hash) const;

        //
        // Ensures the map is large enough to hold `capacity` elements without
        // rehashing.
        // Equivalent to `rehash(2 * capacity)`.
        // Invalidates iterators.
        //
        void reserve(size_t capacity);

        //
        // Ensures the number of slots is equal to the smallest power of two greater than or equal to both `slotCount`
        // and the current size, down to a minimum of `config::minSlotCount`
        //
        // Equivalent to `reserve(slotCount / 2)`
        //
        // Invalidates iterators
        //
        void rehash(size_t slotCount);

        //
        // Swaps the contents of this map and `other`'s
        // Invalidates iterators
        //
        void swap(Map & other) noexcept;

        //
        // Returns whether or not the map is empty
        //
        bool empty() const noexcept;

        //
        // Returns the number of elements in the map
        //
        size_t size() const noexcept;

        //
        // Equivalent to `max_slot_count() * 2`
        //
        size_t max_size() const noexcept;

        //
        // Equivalent to `slot_count() / 2`
        //
        size_t capacity() const noexcept;

        //
        // Will always be at least twice the number of elements.
        // Equivalent to `capacity() * 2`.
        //
        size_t slot_count() const noexcept;

        //
        // Equivalent to `max_size() / 2`.
        //
        size_t max_slot_count() const noexcept;

        //
        // Returns the slot index of the bucket into which `key` would fall.
        //
        size_t bucket(const K & key) const;

        //
        // How many elements are "in" the bucket at slot index `i`.
        //
        size_t bucket_size(size_t slotI) const noexcept;

        //
        // Returns the ratio of elements to slots.
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
        KE key_eq() const noexcept;

        //
        // Returns an instance of `allocator_type`.
        //
        A get_allocator() const noexcept;

        std::pair<u8, E> _elementAt(size_t slotI) const noexcept;

        private: //-------------------------------------------------------------

        static constexpr bool _isSet{std::is_same_v<V, void>};

        size_t _size;
        size_t _slotCount;
        _Chunk * _chunks;
        H _hash;
        KE _equal;
        _Allocator _alloc;

        template <typename KTuple, typename VTuple, size_t... kIndices, size_t... vIndices> std::pair<iterator, bool> _emplace(KTuple && kTuple, VTuple && vTuple, std::index_sequence<kIndices...>, std::index_sequence<vIndices...>);

        template <bool isNewElement, typename K_, typename... VArgs> std::pair<iterator, bool> _try_emplace(size_t hashOrDstSlotI, u8 bucketDist, K_ && key, VArgs &&... vArgs);

        void _erase(iterator position);

        template <bool zeroDists> void _clear() noexcept;

        template <bool constant> std::pair<_Iterator<constant>, _Iterator<constant>> _equal_range(const K & key, size_t hash) const;

        template <bool constant> _Iterator<constant> _begin() const noexcept;

        template <bool constant> _Iterator<constant> _end() const noexcept;

        template <bool constant> _Iterator<constant> _find(const K & key, size_t hash) const;

        void _rehash(size_t slotCount);

        void _allocate();

        void _deallocate();

        void _zeroDists() noexcept;

        template <bool move> void _forwardChunks(std::conditional_t<move, _Chunk, const _Chunk> * srcChunks);

    };

    template <typename K, typename V, typename H, typename KE, typename A> bool operator==(const Map<K, V, H, KE, A> & m1, const Map<K, V, H, KE, A> & m2);

    //
    // Forward iterator
    //
    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    class Map<K, V, H, KE, A>::_Iterator {

        friend Map;

        using E = std::conditional_t<constant, const Map::E, Map::E>;

        public: //--------------------------------------------------------------

        using iterator_category = std::forward_iterator_tag;
        using value_type = E;
        using difference_type = ptrdiff_t;
        using pointer = E *;
        using reference = E &;

        //
        // ...
        //
        constexpr _Iterator(const _Iterator & other) noexcept = default;
        template <bool constant_> requires (constant && !constant_) constexpr _Iterator(const _Iterator<constant_> & other) noexcept;

        //
        // ...
        //
        E & operator*() const noexcept;

        //
        // ...
        //
        E * operator->() const noexcept;

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

        using _Chunk = std::conditional_t<constant, const Map::_Chunk, Map::_Chunk>;

        size_t _pos;

        template <typename _Chunk_> constexpr _Iterator(_Chunk_ * chunk, size_t innerI) noexcept;

        _Chunk * _chunk() const noexcept;

        size_t _innerI() const noexcept;

    };

}

namespace std {

    template <typename K, typename V, typename H, typename KE, typename A> void swap(qc_hash_chunk::Map<K, V, H, KE, A> & a, qc_hash_chunk::Map<K, V, H, KE, A> & b) noexcept;

}

// INLINE IMPLEMENTATION ///////////////////////////////////////////////////////////////////////////////////////////////

namespace qc_hash_chunk {

    inline size_t _firstOccupiedIndexInChunk(const u64 distsData) noexcept {
        if constexpr (std::endian::native == std::endian::little) {
            return std::countr_zero(distsData) >> 3;
        }
        else {
            return std::countl_zero(distsData) >> 3;
        }
    }

    // Map =====================================================================

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::Map(const size_t minCapacity, const H & hash, const KE & equal, const A & alloc) noexcept:
        _size{},
        _slotCount{minCapacity <= config::minCapacity ? config::minSlotCount : std::bit_ceil(minCapacity << 1)},
        _chunks{},
        _hash{hash},
        _equal{equal},
        _alloc{alloc}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::Map(const size_t minCapacity, const A & alloc) noexcept :
        Map{minCapacity, H{}, KE{}, alloc}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::Map(const size_t minCapacity, const H & hash, const A & alloc) noexcept :
        Map{minCapacity, hash, KE{}, alloc}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::Map(const A & alloc) noexcept :
        Map{config::minCapacity, H{}, KE{}, alloc}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    template <typename It>
    inline Map<K, V, H, KE, A>::Map(const It first, const It last, const size_t minCapacity, const H & hash, const KE & equal, const A & alloc) :
        Map{config::minCapacity, hash, equal, alloc}
    {
        // Count number of elements to insert
        size_t n{0u};
        for (It it{first}; it != last; ++it, ++n);

        reserve(n > minCapacity ? n : minCapacity);

        insert(first, last);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <typename It>
    inline Map<K, V, H, KE, A>::Map(const It first, const It last, const size_t minCapacity, const A & alloc) :
        Map{first, last, minCapacity, H{}, KE{}, alloc}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    template <typename It>
    inline Map<K, V, H, KE, A>::Map(const It first, const It last, const size_t minCapacity, const H & hash, const A & alloc) :
        Map{first, last, minCapacity, hash, KE{}, alloc}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::Map(std::initializer_list<E> elements, size_t minCapacity, const H & hash, const KE & equal, const A & alloc) :
        Map{minCapacity ? minCapacity : elements.size(), hash, equal, alloc}
    {
        insert(elements);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::Map(const std::initializer_list<E> elements, const size_t minCapacity, const A & alloc) :
        Map{elements, minCapacity, H{}, KE{}, alloc}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::Map(const std::initializer_list<E> elements, const size_t minCapacity, const H & hash, const A & alloc) :
        Map{elements, minCapacity, hash, KE{}, alloc}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::Map(const Map & other) :
        Map{other, std::allocator_traits<A>::select_on_container_copy_construction(other._alloc)}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::Map(const Map & other, const A & alloc) :
        _size{other._size},
        _slotCount{other._slotCount},
        _chunks{},
        _hash{other._hash},
        _equal{other._equal},
        _alloc{alloc}
    {
        _allocate();
        _forwardChunks<false>(other._chunks);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::Map(Map && other) noexcept :
        Map{std::move(other), std::move(other._alloc)}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::Map(Map && other, const A & alloc) noexcept :
        Map{std::move(other), A(alloc)}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::Map(Map && other, A && alloc) noexcept :
        _size{std::exchange(other._size, 0u)},
        _slotCount{std::exchange(other._slotCount, 0u)},
        _chunks{std::exchange(other._chunks, nullptr)},
        _hash{std::move(other._hash)},
        _equal{std::move(other._equal)},
        _alloc{std::move(alloc)}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A> & Map<K, V, H, KE, A>::operator=(const std::initializer_list<E> elements) {
        clear();
        insert(elements);

        return *this;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A> & Map<K, V, H, KE, A>::operator=(const Map & other) {
        if (&other == this) {
            return *this;
        }

        if (_chunks) {
            _clear<false>();
            if (_slotCount != other._slotCount || _alloc != other._alloc) {
                _deallocate();
            }
        }

        _size = other._size;
        _slotCount = other._slotCount;
        _hash = other._hash;
        _equal = other._equal;
        if constexpr (std::allocator_traits<A>::propagate_on_container_copy_assignment::value) {
            _alloc = std::allocator_traits<A>::select_on_container_copy_construction(other._alloc);
        }

        if (other._chunks) {
            if (!_chunks) {
                _allocate();
            }

            _forwardChunks<false>(other._chunks);
        }

        return *this;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A> & Map<K, V, H, KE, A>::operator=(Map && other) noexcept {
        if (&other == this) {
            return *this;
        }

        if (_chunks) {
            _clear<false>();
            _deallocate();
        }

        _size = other._size;
        _slotCount = other._slotCount;
        _hash = std::move(other._hash);
        _equal = std::move(other._equal);
        if constexpr (std::allocator_traits<A>::propagate_on_container_move_assignment::value) {
            _alloc = std::move(other._alloc);
        }

        if (std::allocator_traits<A>::propagate_on_container_move_assignment::value || _alloc == other._alloc) {
            _chunks = other._chunks;
            other._chunks = nullptr;
        }
        else {
            _allocate();
            _forwardChunks<true>(other._chunks);
            other._clear<false>();
            other._deallocate();
        }

        other._size = 0u;
        other._slotCount = 0u;

        return *this;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::~Map() noexcept {
        if (_chunks) {
            _clear<false>();
            _deallocate();
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::insert(const E & element) -> std::pair<iterator, bool> {
        return emplace(element);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::insert(E && element) -> std::pair<iterator, bool> {
        return emplace(std::move(element));
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <typename It>
    inline void Map<K, V, H, KE, A>::insert(It first, const It last) {
        while (first != last) {
            emplace(*first);
            ++first;
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::insert(const std::initializer_list<E> elements) {
        for (const E & element : elements) {
            emplace(element);
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::emplace(const E & element) -> std::pair<iterator, bool> {
        if constexpr (_isSet) {
            return try_emplace(element);
        }
        else {
            return try_emplace(element.first, element.second);
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::emplace(E && element) -> std::pair<iterator, bool> {
        if constexpr (_isSet) {
            return try_emplace(std::move(element));
        }
        else {
            return try_emplace(std::move(element.first), std::move(element.second));
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <typename K_, typename T_>
    inline auto Map<K, V, H, KE, A>::emplace(K_ && key, T_ && val) -> std::pair<iterator, bool> requires (!std::is_same_v<V, void>) {
        return try_emplace(std::forward<K_>(key), std::forward<T_>(val));
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <typename... KArgs, typename... TArgs>
    inline auto Map<K, V, H, KE, A>::emplace(const std::piecewise_construct_t, std::tuple<KArgs...> && kArgs, std::tuple<TArgs...> && tArgs) -> std::pair<iterator, bool> requires (!std::is_same_v<V, void>) {
        return _emplace(std::move(kArgs), std::move(tArgs), std::index_sequence_for<KArgs...>(), std::index_sequence_for<TArgs...>());
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <typename KTuple, typename VTuple, size_t... kIndices, size_t... vIndices>
    inline auto Map<K, V, H, KE, A>::_emplace(KTuple && kTuple, VTuple && vTuple, const std::index_sequence<kIndices...>, const std::index_sequence<vIndices...>) -> std::pair<iterator, bool> {
        return try_emplace(K(std::move(std::get<kIndices>(kTuple))...), std::move(std::get<vIndices>(vTuple))...);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <typename... TArgs>
    inline auto Map<K, V, H, KE, A>::try_emplace(const K & key, TArgs &&... valArgs) -> std::pair<iterator, bool> {
        return _try_emplace<true>(_hash(key), 1u, key, std::forward<TArgs>(valArgs)...);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <typename... TArgs>
    inline auto Map<K, V, H, KE, A>::try_emplace(K && key, TArgs &&... valArgs) -> std::pair<iterator, bool> {
        return _try_emplace<true>(_hash(key), 1u, std::move(key), std::forward<TArgs>(valArgs)...);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool isNewElement, typename K_, typename... VArgs>
    inline auto Map<K, V, H, KE, A>::_try_emplace(const size_t hashOrDstSlotI, u8 bucketDist, K_ && key, VArgs &&... vArgs) -> std::pair<iterator, bool> {
        static_assert(!(!_isSet && !sizeof...(VArgs) && !std::is_default_constructible_v<V>), "The value type must be default constructible in order to pass no value arguments");
        static_assert(!(_isSet && sizeof...(VArgs)), "Sets do not have values");

        const size_t slotMask{_slotCount - 1u};

        // If we've yet to allocate memory, now is the time
        if constexpr (isNewElement) {
            if (!_chunks) {
                _allocate();
            }
        }

        size_t dstSlotI{isNewElement ? (hashOrDstSlotI & slotMask) : hashOrDstSlotI};
        _Chunk * dstChunk;
        size_t dstInnerI;

        // Find start of next bucket and see if element already exists
        while (true) {
            dstChunk = _chunks + (dstSlotI >> 3);
            dstInnerI = dstSlotI & 7u;

            // Found start of next bucket at `dstSlotI`
            if (dstChunk->dists[dstInnerI] < bucketDist) {
                break;
            }

            // Element already exists
            if constexpr (isNewElement) {
                if (_equal(dstChunk->elements[dstInnerI].key, key)) {
                    return {iterator{dstChunk, dstInnerI}, false};
                }
            }

            dstSlotI = (dstSlotI + 1u) & slotMask;
            ++bucketDist;
        }

        // Rehash if we're at capacity
        if constexpr (isNewElement) {
            if (_size >= (_slotCount >> 1)) {
                _rehash(_slotCount << 1);
                return _try_emplace<true>(hashOrDstSlotI, 1u, std::forward<K_>(key), std::forward<VArgs>(vArgs)...);
            }
        }

        u8 & dstDist{dstChunk->dists[dstInnerI]};
        _Element & dstElement{dstChunk->elements[dstInnerI]};

        // Slot occupied, propogate it back and insert new element
        if (dstDist) {
            if constexpr (_isSet) _try_emplace<false>((dstSlotI + 1u) & slotMask, dstDist + 1u, std::move(dstElement.key));
            else _try_emplace<false>((dstSlotI + 1u) & slotMask, dstDist + 1u, std::move(dstElement.key), std::move(dstElement.val));

            // If new element, destruct existing and construct new element in place
            if constexpr (isNewElement) {
                dstElement.get().~E();
                _AllocatorTraits::construct(_alloc, &dstElement.key, std::forward<K_>(key));
                if constexpr (!_isSet) _AllocatorTraits::construct(_alloc, &dstElement.val, std::forward<VArgs>(vArgs)...);
            }
            // Else if propagating existing element, just move it
            else {
                dstElement.key = std::forward<K_>(key);
                if constexpr (!_isSet) dstElement.val = [](V && val){ return val; }(std::forward<VArgs>(vArgs)...);
            }
        }
        // Slot is free, construct new element in place
        else {
            _AllocatorTraits::construct(_alloc, &dstElement.key, std::forward<K_>(key));
            if constexpr (!_isSet) _AllocatorTraits::construct(_alloc, &dstElement.val, std::forward<VArgs>(vArgs)...);
        }

        dstDist = bucketDist;
        if constexpr (isNewElement) {
            ++_size;
        }
        return {iterator{dstChunk, dstInnerI}, true};
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline bool Map<K, V, H, KE, A>::erase(const K & key) {
        const iterator it{find(key)};

        if (it == end()) {
            return false;
        }

        _erase(it);

        return true;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::erase(const iterator position) {
        _erase(position);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::_erase(iterator position) {
        const size_t slotMask{_slotCount - 1u};

        _Chunk * dstChunk{position._chunk()};
        size_t dstInnerI{position._innerI()};
        size_t dstSlotI{((dstChunk - _chunks) << 3) | dstInnerI};

        while (true) {
            size_t srcSlotI{(dstSlotI + 1u) & slotMask};
            _Chunk * srcChunk{_chunks + (srcSlotI >> 3)};
            size_t srcInnerI{srcSlotI & 7u};
            u8 srcDist{srcChunk->dists[srcInnerI]};

            // If there is no next bucket to shift back, break
            if (srcDist <= 1u) {
                break;
            }

            // Find end of bucket
            while (true) {
                const size_t nextSlotI{(srcSlotI + 1u) & slotMask};
                _Chunk * nextChunk{_chunks + (nextSlotI >> 3)};
                const size_t nextInnerI{nextSlotI & 7u};
                const size_t nextDist{nextChunk->dists[nextInnerI]};

                // Found end of bucket at `slotI`
                if (nextDist <= srcDist) {
                    break;
                }

                srcSlotI = nextSlotI;
                srcChunk = nextChunk;
                srcInnerI = nextInnerI;
                srcDist = nextDist;
            }

            // Move last element in bucket forward
            dstChunk->dists[dstInnerI] = srcDist - ((srcSlotI - dstSlotI + _slotCount) & slotMask);
            dstChunk->elements[dstInnerI].get() = std::move(srcChunk->elements[srcInnerI].get());

            dstSlotI = srcSlotI;
            dstChunk = srcChunk;
            dstInnerI = srcInnerI;
        }

        // Destruct (final) element
        dstChunk->dists[dstInnerI] = 0u;
        dstChunk->elements[dstInnerI].get().~E();
        --_size;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::clear() noexcept {
        _clear<true>();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool zeroDists>
    inline void Map<K, V, H, KE, A>::_clear() noexcept {
        if constexpr (std::is_trivially_destructible_v<E>) {
            if constexpr (zeroDists) {
                if (_size) _zeroDists();
            }
        }
        else {
            size_t erasedCount{0u};
            for (_Chunk * chunk{_chunks}; erasedCount < _size; ++chunk) {
                for (size_t innerI{0u}; innerI < 8u && erasedCount < _size; ++innerI) {
                    // There is an element in this slot. Erase it
                    if (chunk->dists[innerI]) {
                        chunk->elements[innerI].get().~E();

                        ++erasedCount;
                    }
                }

                if constexpr (zeroDists) {
                    chunk->distsData = 0u;
                }
            }
        }

        _size = 0u;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline bool Map<K, V, H, KE, A>::contains(const K & key) const {
        return contains(key, _hash(key));
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline bool Map<K, V, H, KE, A>::contains(const K & key, const size_t hash) const {
        return find(key, hash) != cend();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline size_t Map<K, V, H, KE, A>::count(const K & key) const {
        return contains(key);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline size_t Map<K, V, H, KE, A>::count(const K & key, const size_t hash) const {
        return contains(key, hash);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline std::add_lvalue_reference_t<V> Map<K, V, H, KE, A>::at(const K & key) requires (!std::is_same_v<V, void>) {
        return find(key)->second;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline std::add_lvalue_reference_t<const V> Map<K, V, H, KE, A>::at(const K & key) const requires (!std::is_same_v<V, void>) {
        return find(key)->second;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline std::add_lvalue_reference_t<V> Map<K, V, H, KE, A>::operator[](const K & key) {
        static_assert(!std::is_same_v<V, void>);
        return try_emplace(key).first->second;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline std::add_lvalue_reference_t<V> Map<K, V, H, KE, A>::operator[](K && key) {
        return try_emplace(std::move(key)).first->second;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::begin() noexcept -> iterator {
        return _begin<false>();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::begin() const noexcept -> const_iterator {
        return _begin<true>();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::cbegin() const noexcept -> const_iterator {
        return _begin<true>();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto Map<K, V, H, KE, A>::_begin() const noexcept -> _Iterator<constant> {
        if (!_size) {
            return _end<constant>();
        }

        for (const _Chunk * chunk{_chunks}; ; ++chunk) {
            if (chunk->distsData) {
                return _Iterator<constant>{chunk, _firstOccupiedIndexInChunk(chunk->distsData)};
            }
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::end() noexcept -> iterator {
        return _end<false>();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::end() const noexcept -> const_iterator {
        return _end<true>();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::cend() const noexcept -> const_iterator {
        return _end<true>();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto Map<K, V, H, KE, A>::_end() const noexcept -> _Iterator<constant> {
        return _Iterator<constant>{_chunks + (_slotCount >> 3), 0u};
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::find(const K & key) -> iterator {
        return find(key, _hash(key));
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::find(const K & key) const -> const_iterator {
        return find(key, _hash(key));
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::find(const K & key, const size_t hash) -> iterator {
        return _find<false>(key, hash);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::find(const K & key, const size_t hash) const -> const_iterator {
        return _find<true>(key, hash);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto Map<K, V, H, KE, A>::_find(const K & key, const size_t hash) const -> _Iterator<constant> {
        if (!_chunks) {
            return _end<constant>();
        }

        const size_t slotMask{_slotCount - 1u};

        size_t slotI{hash & slotMask};
        u8 dist{1u};

        while (true) {
            _Chunk & chunk{_chunks[slotI >> 3]};
            const size_t innerI{slotI & 7u};
            u8 & slotDist{chunk.dists[innerI]};
            _Element & slotElement{chunk.elements[innerI]};

            if (slotDist < dist) {
                return _end<constant>();
            }

            if (_equal(slotElement.key, key)) {
                return _Iterator<constant>{&chunk, innerI};
            }

            ++dist;

            // Increment slot index and wrap around to beginning if at end
            slotI = (slotI + 1u) & slotMask;
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::equal_range(const K & key) -> std::pair<iterator, iterator> {
        return equal_range(key, _hash(key));
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::equal_range(const K & key) const -> std::pair<const_iterator, const_iterator> {
        return equal_range(key, _hash(key));
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::equal_range(const K & key, const size_t hash) -> std::pair<iterator, iterator> {
        return _equal_range<false>(key, hash);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::equal_range(const K & key, const size_t hash) const -> std::pair<const_iterator, const_iterator> {
        return _equal_range<true>(key, hash);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto Map<K, V, H, KE, A>::_equal_range(const K & key, const size_t hash) const -> std::pair<_Iterator<constant>, _Iterator<constant>> {
        _Iterator<constant> it(_find<constant>(key, hash));
        return { it, it };
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::reserve(const size_t capacity) {
        rehash(capacity << 1);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::rehash(size_t slotCount) {
        if (slotCount < config::minSlotCount) {
            slotCount = config::minSlotCount;
        }
        else {
            if (slotCount < (_size << 1)) {
                slotCount = _size << 1;
            }

            slotCount = std::bit_ceil(slotCount);
        }

        if (slotCount != _slotCount) {
            if (_chunks) {
                _rehash(slotCount);
            }
            else {
                _slotCount = slotCount;
            }
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::_rehash(const size_t slotCount) {
        const size_t oldSize{_size};
        const size_t oldChunkCount{_slotCount >> 3};
        _Chunk * const oldChunks{_chunks};

        _size = 0u;
        _slotCount = slotCount;
        _allocate();

        size_t movedCount{0u};
        for (_Chunk * chunk{oldChunks}; movedCount < oldSize; ++chunk) {
            if (chunk->distsData) {
                for (size_t innerI{0u}; innerI < 8u; ++innerI) {
                    if (chunk->dists[innerI]) {
                        emplace(std::move(chunk->elements[innerI].get()));
                        ++movedCount;
                    }
                }
            }
        }

        _AllocatorTraits::deallocate(_alloc, reinterpret_cast<u64 *>(oldChunks), oldChunkCount * (sizeof(_Chunk) >> 3) + 1u);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::swap(Map & other) noexcept {
        std::swap(_size, other._size);
        std::swap(_slotCount, other._slotCount);
        std::swap(_chunks, other._chunks);
        std::swap(_hash, other._hash);
        std::swap(_equal, other._equal);
        if constexpr (std::allocator_traits<A>::propagate_on_container_swap::value) {
            std::swap(_alloc, other._alloc);
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline bool Map<K, V, H, KE, A>::empty() const noexcept {
        return _size == 0u;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline size_t Map<K, V, H, KE, A>::size() const noexcept {
        return _size;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline size_t Map<K, V, H, KE, A>::max_size() const noexcept {
        return max_slot_count() >> 1u;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline size_t Map<K, V, H, KE, A>::capacity() const noexcept {
        return _slotCount >> 1u;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline size_t Map<K, V, H, KE, A>::slot_count() const noexcept {
        return _slotCount;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline size_t Map<K, V, H, KE, A>::max_slot_count() const noexcept {
        return size_t(1u) << (std::numeric_limits<size_t>::digits - 1);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline size_t Map<K, V, H, KE, A>::bucket(const K & key) const {
        return _hash(key) & (_slotCount - 1u);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline size_t Map<K, V, H, KE, A>::bucket_size(size_t slotI) const noexcept {
        if (slotI >= _slotCount || !_chunks) {
            return 0u;
        }

        const size_t slotMask{_slotCount - 1u};

        // Seek to start of the bucket
        u8 dist{1u};
        while (true) {
            _Chunk * chunk{_chunks + (slotI >> 3)};
            size_t innerI{slotI & 7u};

            if (chunk->dists[innerI] <= dist) {
                break;
            }

            ++dist;

            slotI = (slotI + 1u) & slotMask;
        }

        // Count elements in bucket
        size_t n{0u};
        while (true) {
            _Chunk * chunk{_chunks + (slotI >> 3)};
            size_t innerI{slotI & 7u};

            if (chunk->dists[innerI] != dist) {
                break;
            }

            ++dist;
            ++n;

            slotI = (slotI + 1u) & slotMask;
        }

        return n;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline float Map<K, V, H, KE, A>::load_factor() const noexcept {
        return float(_size) / float(_slotCount);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline float Map<K, V, H, KE, A>::max_load_factor() const noexcept {
        return 0.5f;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline H Map<K, V, H, KE, A>::hash_function() const noexcept {
        return _hash;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline KE Map<K, V, H, KE, A>::key_eq() const noexcept {
        return _equal;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline A Map<K, V, H, KE, A>::get_allocator() const noexcept {
        return A(_alloc);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::_elementAt(const size_t slotI) const noexcept -> std::pair<u8, E> {
        static_assert(std::is_copy_constructible_v<E>);
        _Chunk * const chunk{_chunks + (slotI >> 3)};
        const size_t innerI{slotI & 7u};
        return {chunk->dists[innerI], chunk->elements[innerI].get()};
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::_allocate() {
        const size_t chunkCount{_slotCount >> 3};
        _chunks = reinterpret_cast<_Chunk *>(_AllocatorTraits::allocate(_alloc, chunkCount * (sizeof(_Chunk) >> 3) + 1u));
        _zeroDists();

        // Set the trailing dists all to 255 so iterators can know when to stop without needing to know the size of container
        // (really only need the first dist set, but I like this more)
        _chunks[chunkCount].distsData = ~size_t(0u);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::_deallocate() {
        const size_t chunkCount{_slotCount >> 3};
        _AllocatorTraits::deallocate(_alloc, reinterpret_cast<u64 *>(_chunks), chunkCount * (sizeof(_Chunk) >> 3) + 1u);
        _chunks = nullptr;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::_zeroDists() noexcept {
        for (_Chunk * chunk{_chunks}, * const endChunk{_chunks + (_slotCount >> 3)}; chunk != endChunk; ++chunk) {
            chunk->distsData = 0u;
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool move>
    inline void Map<K, V, H, KE, A>::_forwardChunks(std::conditional_t<move, _Chunk, const _Chunk> * srcChunks) {
        if constexpr (std::is_trivially_copyable_v<E>) {
            if (_size) {
                const size_t chunkCount{_slotCount >> 3};
                std::memcpy(_chunks, srcChunks, chunkCount * sizeof(_Chunk));
            }
        }
        else {
            size_t n{0u};
            auto srcChunk{srcChunks};
            _Chunk * dstChunk{_chunks};
            for (; n < _size; ++srcChunk, ++dstChunk) {
                if ((dstChunk->distsData = srcChunk->distsData)) {
                    for (size_t innerI{0u}; innerI < 8u; ++innerI) {
                        if (srcChunk->dists[innerI]) {
                            if constexpr (move) {
                                _AllocatorTraits::construct(_alloc, &dstChunk->elements[innerI].get(), std::move(srcChunk->elements[innerI].get()));
                            }
                            else {
                                _AllocatorTraits::construct(_alloc, &dstChunk->elements[innerI].get(), srcChunk->elements[innerI].get());
                            }
                            ++n;
                        }
                    }
                }
            }
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline bool operator==(const Map<K, V, H, KE, A> & m1, const Map<K, V, H, KE, A> & m2) {
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

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    template <bool constant_> requires (constant && !constant_)
    inline constexpr Map<K, V, H, KE, A>::_Iterator<constant>::_Iterator(const _Iterator<constant_> & other) noexcept:
        _pos{other._pos}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    template <typename _Chunk_>
    inline constexpr Map<K, V, H, KE, A>::_Iterator<constant>::_Iterator(_Chunk_ * const chunk, size_t innerI) noexcept :
        _pos{reinterpret_cast<size_t>(chunk) | innerI}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto Map<K, V, H, KE, A>::_Iterator<constant>::operator*() const noexcept -> E & {
        return _chunk()->elements[_innerI()].get();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto Map<K, V, H, KE, A>::_Iterator<constant>::operator->() const noexcept -> E * {
        return &operator*();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto Map<K, V, H, KE, A>::_Iterator<constant>::operator++() noexcept -> _Iterator & {
        _Chunk * chunk{_chunk()};
        size_t innerI{_innerI()};

        do {
#if 1
            if (innerI < 7u) {
                ++innerI;
            }
            else {
                do {
                    ++chunk;
                } while (!chunk->distsData);

                innerI = _firstOccupiedIndexInChunk(chunk->distsData);

                break;
            }

            // Alternative that is you'd think would be faster, but is ~3x slower
#else
            ++innerI;
            chunk += innerI >> 3;
            innerI &= 7u;
#endif
        } while (!chunk->dists[innerI]);

        _pos = reinterpret_cast<size_t>(chunk) | innerI;
        return *this;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto Map<K, V, H, KE, A>::_Iterator<constant>::operator++(int) noexcept -> _Iterator {
        _Iterator temp(*this);
        operator++();
        return temp;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    template <bool constant_>
    inline bool Map<K, V, H, KE, A>::_Iterator<constant>::operator==(const _Iterator<constant_> & it) const noexcept {
        return _pos == it._pos;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto Map<K, V, H, KE, A>::_Iterator<constant>::_chunk() const noexcept -> _Chunk * {
        return reinterpret_cast<_Chunk *>(_pos & ~size_t(7u));
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline size_t Map<K, V, H, KE, A>::_Iterator<constant>::_innerI() const noexcept {
        return _pos & 7u;
    }

}

namespace std {

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void swap(qc_hash_chunk::Map<K, V, H, KE, A> & a, qc_hash_chunk::Map<K, V, H, KE, A> & b) noexcept {
        a.swap(b);
    }

}
