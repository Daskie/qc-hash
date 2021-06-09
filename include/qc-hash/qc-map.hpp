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
//   - Slot: Purely conceptual. One "spot" in the backing array. Comprised of a control and an element, and may be
//         present, empty, or a grave
//   - Control: A byte of data that stores info about the slot. The upper bit indicates if an element is present. The
//         lower seven bits are either the upper seven bits of the hash, zero if empty, or all 1's if a grave
//   - Grave: Means the slot used to have an element, but it was erased.
//   - Size: The number of elements in the map/set
//   - Slot Count: The number of slots in the map/set. Is always at least twice size, and half of slot count
//   - Capacity: The number of elements that the map/set can currently hold without growing. Exactly half of slot count
//

#include <cstdint>
#include <cstring>

#include <bit> // TODO: what do we need this for, can we implement ourselves?
#include <concepts>
#include <initializer_list>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

//
// Forward declaration of friend class used for testing
//
struct QcHashMapFriend;

namespace qc_hash {

    // This code assumes `size_t` is either 4 or 8 bytes
    static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8, "Unsupported architecture");

    // This code assumes that pointers are the same size as `size_t`
    static_assert(sizeof(intptr_t) == sizeof(size_t), "Unsupported architecture");

    namespace config {

        //
        // ...
        //
        // Must be at least 4 + 1
        //
        constexpr size_t minCapacity{16u + 1u};
        constexpr size_t minSlotCount{(minCapacity - 1u) * 2u + 1u};

    } // namespace config

    //
    // ...
    //
    template <typename T> concept Trivial = (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8) && alignof(T) <= sizeof(T);

    //
    // ...
    //
    template <typename K> struct TrivialHash;
    template <typename K> requires (std::is_integral_v<K> || std::is_enum_v<K>) struct TrivialHash<K>;
    template <typename K> requires std::is_pointer_v<K> struct TrivialHash<K>;

    template <typename K> concept TriviallyHashable = requires (K k) { { declval<TrivialHash<K>>(k) } -> std::convertible_to<size_t>; }; // TODO: maybe inline into assert? or remove

    //
    // ...
    //
    template <typename K, typename V, typename H = TrivialHash<K>, typename KE = std::equal_to<K>, typename A = std::allocator<std::pair<K, V>>> class Map;

    //
    // ...
    // Defined as a `Map` whose mapped type is `void`.
    //
    template <typename K, typename H = TrivialHash<K>, typename KE = std::equal_to<K>, typename A = std::allocator<K>> using Set = Map<K, void, H, KE, A>;

    //
    // ...
    // The hash function provided MUST have good entropy in both the lower and upper bits.
    // Therefore, AN IDENTITY HASH MUST NOT BE USED!
    //
    template <typename K, typename V, typename H, typename KE, typename A> class Map {

        friend QcHashMapFriend;

        static constexpr bool _isSet{std::is_same_v<V, void>};

        using E = std::conditional_t<_isSet, K, std::pair<K, V>>;

        template <bool constant> class _Iterator;
        template <bool constant> friend class _Iterator;

        public: //--------------------------------------------------------------

        static_assert(Trivial<K>);

        static_assert(alignof(E) <= 8u, "Element types with alignment greater than 8 currently unsupported");
        static_assert(std::is_nothrow_move_constructible_v<E>);
        static_assert(std::is_nothrow_move_assignable_v<E>);
        static_assert(std::is_nothrow_swappable_v<E>);
        static_assert(std::is_nothrow_destructible_v<E>);

        static_assert(std::is_same_v<H, TrivialHash<K>>);
        static_assert(std::is_nothrow_move_constructible_v<H>);
        static_assert(std::is_nothrow_move_assignable_v<H>);
        static_assert(std::is_nothrow_swappable_v<H>);
        static_assert(std::is_nothrow_destructible_v<H>);

        static_assert(std::is_nothrow_move_constructible_v<KE>);
        static_assert(std::is_nothrow_move_assignable_v<KE>);
        static_assert(std::is_nothrow_swappable_v<KE>);
        static_assert(std::is_nothrow_destructible_v<KE>);

        static_assert(std::is_nothrow_move_constructible_v<A>);
        static_assert(std::is_nothrow_move_assignable_v<A> || !std::allocator_traits<A>::propagate_on_container_move_assignment::value);
        static_assert(std::is_nothrow_swappable_v<A> || !std::allocator_traits<A>::propagate_on_container_swap::value);
        static_assert(std::is_nothrow_destructible_v<A>);

        using key_type = K;
        using mapped_type = V;
        using value_type = E;
        using hasher = H;
        using key_equal = KE;
        using allocator_type = A;
        using reference = E &;
        using const_reference = const E &;
        using pointer = E *;
        using const_pointer = const E *;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        using iterator = _Iterator<false>;
        using const_iterator = _Iterator<true>;

        //
        // Memory is not allocated until the first element is inserted.
        //
        explicit Map(size_t minCapacity = config::minCapacity, const H & hash = {}, const KE & equal = {}, const A & alloc = {}) noexcept;
        Map(size_t minCapacity, const A & alloc) noexcept;
        Map(size_t minCapacity, const H & hash, const A & alloc) noexcept;
        explicit Map(const A & alloc) noexcept;
        template <typename It> Map(It first, It last, size_t minCapacity = {}, const H & hash = {}, const KE & equal = {}, const A & alloc = {});
        template <typename It> Map(It first, It last, size_t minCapacity, const A & alloc);
        template <typename It> Map(It first, It last, size_t minCapacity, const H & hash, const A & alloc);
        Map(std::initializer_list<E> elements, size_t minCapacity = {}, const H & hash = {}, const KE & equal = {}, const A & alloc = {});
        Map(std::initializer_list<E> elements, size_t minCapacity, const A & alloc);
        Map(std::initializer_list<E> elements, size_t minCapacity, const H & hash, const A & alloc);
        Map(const Map & other);
        Map(Map && other) noexcept;

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
        template <typename K_, typename V_> std::pair<iterator, bool> emplace(K_ && key, V_ && val) requires (!std::is_same_v<V, void>);
        template <typename... KArgs> std::pair<iterator, bool> emplace(KArgs &&... keyArgs) requires (std::is_same_v<V, void>);
        template <typename... KArgs, typename... VArgs> std::pair<iterator, bool> emplace(std::piecewise_construct_t, std::tuple<KArgs...> && keyArgs, std::tuple<VArgs...> && valArgs) requires (!std::is_same_v<V, void>);

        //
        // If there is no existing element for `key`, creates a new element in
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

        //
        // Returns `1` if the map contains an element for `key` and `0` if it does
        // not.
        //
        size_t count(const K & key) const;

        //
        // ...
        // TODO: Use requires clause once MSVC supports it along with `std::add_lvalue_reference_t`
        //
        std::add_lvalue_reference_t<V> at(const K & key);
        std::add_lvalue_reference_t<const V> at(const K & key) const;

        //
        // ...
        // TODO: Use requires clause once MSVC supports it along with `std::add_lvalue_reference_t`
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

        //
        // As a key may correspond to as most one element, this method is
        // equivalent to `find`, except returning a pair of duplicate iterators.
        //
        std::pair<iterator, iterator> equal_range(const K & key);
        std::pair<const_iterator, const_iterator> equal_range(const K & key) const;

        //
        // Returns the index of the slot into which `key` would fall.
        //
        size_t slot(const K & key) const noexcept;

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
        // Returns the number of elements in the map
        //
        size_t size() const noexcept;

        //
        // Returns whether or not the map is empty
        //
        bool empty() const noexcept;

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

        private: //-------------------------------------------------------------

        using _RawKey = std::conditional_t<sizeof(K) == 1, uint8_t, std::conditional_t<sizeof(K) == 2, uint16_t, std::conditional_t<sizeof(K) == 4, uint32_t, uint64_t>>>;

        //
        // `vacant`   - The key is not in the map and the slot it would go in is empty
        // `present`  - The key is in the map
        // `occupied` - The key is not in the map and the slot it would go in is occupied by the next bucket
        //
        // This order is important - vacant must be '0' and present must be '1'
        //
        enum class _FindKeyResult { vacant = 0, present = 1, occupied = 2 };

        static constexpr _RawKey _specialRawKey{std::numeric_limits<_RawKey>::max()};

        static K & _key(E & element) noexcept;
        static const K & _key(const E & element) noexcept;

        static _RawKey & _raw(K & key) noexcept;
        static const _RawKey & _raw(const K & key) noexcept;

        size_t _size;
        size_t _slotCount;
        E * _elements;
        H _hash;
        KE _equal;
        A _alloc;

        template <typename KTuple, typename VTuple, size_t... kIndices, size_t... vIndices> std::pair<iterator, bool> _emplace(KTuple && kTuple, VTuple && vTuple, std::index_sequence<kIndices...>, std::index_sequence<vIndices...>);

        template <typename K_, typename... VArgs> std::pair<iterator, bool> _try_emplace(K_ && key, VArgs &&... vArgs);

        template <bool zeroKeys> void _clear() noexcept;

        //
        // Returns the index of the slot into which `key` would fall.
        //
        size_t _slot(const K & key) const noexcept;

        void _rehash(size_t slotCount);

        template <bool zeroControls> void _allocate();

        void _deallocate();

        void _zeroKeys() noexcept;

        template <bool move> void _forwardData(std::conditional_t<move, Map, const Map> & other);

        //
        // ...
        // If the key is not present, returns the element after the end of the key's bucket
        //
        std::pair<E *, _FindKeyResult> _findKey(const K & key) noexcept;
        std::pair<const E *, _FindKeyResult> _findKey(const K & key) const noexcept;

        void _shiftBucketBack(E * firstBucketElement) noexcept;

    };

    template <typename K, typename V, typename H, typename KE, typename A> bool operator==(const Map<K, V, H, KE, A> & m1, const Map<K, V, H, KE, A> & m2);

    //
    // Forward iterator
    //
    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    class Map<K, V, H, KE, A>::_Iterator {

        friend Map;
        friend QcHashMapFriend;

        using E = std::conditional_t<constant, const Map::E, Map::E>;

        public: //--------------------------------------------------------------

        using iterator_category = std::forward_iterator_tag;
        using value_type = E;
        using difference_type = ptrdiff_t;
        using pointer = E *;
        using reference = E &;

        //
        // ...
        // Default constructor provided to enable the class to be trivial
        //
        constexpr _Iterator() noexcept = default;
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

        E * _element;

        constexpr _Iterator(E * element) noexcept;

    };

} // namespace qc_hash

namespace std {

    template <typename K, typename V, typename H, typename KE, typename A> void swap(qc_hash::Map<K, V, H, KE, A> & a, qc_hash::Map<K, V, H, KE, A> & b) noexcept;

} // namespace std

// INLINE IMPLEMENTATION ///////////////////////////////////////////////////////////////////////////////////////////////

#pragma warning(push)
#pragma warning(disable:4706)

namespace qc_hash {

    template <typename K> requires (std::is_integral_v<K> || std::is_enum_v<K>)
    struct TrivialHash<K> {
        size_t operator()(const K k) const noexcept {
            return size_t(k);
        }
    };

    template <typename K> requires std::is_pointer_v<K>
    struct TrivialHash<K> {
        size_t operator()(const K k) const noexcept {
            constexpr int shift{int(std::bit_width(alignof(K)) - 1)};
            return reinterpret_cast<size_t>(k) >> shift;
        }
    };

    // Map =====================================================================

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::Map(const size_t minCapacity, const H & hash, const KE & equal, const A & alloc) noexcept:
        _size{},
        _slotCount{minCapacity <= config::minCapacity ? config::minSlotCount : std::bit_ceil((minCapacity - 1u) << 1) + 1u},
        _elements{},
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
        size_t n{};
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
        _size{other._size},
        _slotCount{other._slotCount},
        _elements{},
        _hash{other._hash},
        _equal{other._equal},
        _alloc{std::allocator_traits<A>::select_on_container_copy_construction(other._alloc)}
    {
        if (_size) {
            _allocate<false>();
            _forwardData<false>(other);
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::Map(Map && other) noexcept :
        _size{std::exchange(other._size, 0u)},
        _slotCount{std::exchange(other._slotCount, config::minSlotCount)},
        _elements{std::exchange(other._elements, nullptr)},
        _hash{std::move(other._hash)},
        _equal{std::move(other._equal)},
        _alloc{std::move(other._alloc)}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A> & Map<K, V, H, KE, A>::operator=(const std::initializer_list<E> elements) {
        return *this = Map(elements);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A> & Map<K, V, H, KE, A>::operator=(const Map & other) {
        if (&other == this) {
            return *this;
        }

        if (_elements) {
            _clear<false>();
            if (!other._size || _slotCount != other._slotCount || _alloc != other._alloc) {
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

        if (_size) {
            if (!_elements) {
                _allocate<false>();
            }

            _forwardData<false>(other);
        }

        return *this;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A> & Map<K, V, H, KE, A>::operator=(Map && other) noexcept {
        if (&other == this) {
            return *this;
        }

        if (_elements) {
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

#pragma warning(suppress:4127) // TODO: MSVC erroneously thinks this should be constexpr
        if (std::allocator_traits<A>::propagate_on_container_move_assignment::value || _alloc == other._alloc) {
            _elements = std::exchange(other._elements, nullptr);
            other._size = {};
        }
        else {
            if (_size) {
                _allocate<false>();
                _forwardData<true>(other);
                other._clear<false>();
            }
            if (other._elements) {
                other._deallocate();
            }
        }

        other._slotCount = config::minSlotCount;

        return *this;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::~Map() noexcept {
        if (_elements) {
            _clear<false>();
            _deallocate();
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::insert(const E & element) -> std::pair<iterator, bool> {
        static_assert(std::is_copy_constructible_v<E>);

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
        static_assert(std::is_copy_constructible_v<E>);

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
    template <typename K_, typename V_>
    inline auto Map<K, V, H, KE, A>::emplace(K_ && key, V_ && val) -> std::pair<iterator, bool> requires (!std::is_same_v<V, void>) {
        return try_emplace(std::forward<K_>(key), std::forward<V_>(val));
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <typename... KArgs>
    inline auto Map<K, V, H, KE, A>::emplace(KArgs &&... keyArgs) -> std::pair<iterator, bool> requires (std::is_same_v<V, void>) {
        return try_emplace(K{std::forward<KArgs>(keyArgs)...});
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <typename... KArgs, typename... VArgs>
    inline auto Map<K, V, H, KE, A>::emplace(const std::piecewise_construct_t, std::tuple<KArgs...> && keyArgs, std::tuple<VArgs...> && valArgs) -> std::pair<iterator, bool> requires (!std::is_same_v<V, void>) {
        return _emplace(std::move(keyArgs), std::move(valArgs), std::index_sequence_for<KArgs...>(), std::index_sequence_for<VArgs...>());
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <typename KTuple, typename VTuple, size_t... kIndices, size_t... vIndices>
    inline auto Map<K, V, H, KE, A>::_emplace(KTuple && kTuple, VTuple && vTuple, const std::index_sequence<kIndices...>, const std::index_sequence<vIndices...>) -> std::pair<iterator, bool> {
        return try_emplace(K{std::move(std::get<kIndices>(kTuple))...}, std::move(std::get<vIndices>(vTuple))...);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <typename... VArgs>
    inline auto Map<K, V, H, KE, A>::try_emplace(const K & key, VArgs &&... valArgs) -> std::pair<iterator, bool> {
        static_assert(std::is_copy_constructible_v<K>);

        return _try_emplace(key, std::forward<VArgs>(valArgs)...);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <typename... VArgs>
    inline auto Map<K, V, H, KE, A>::try_emplace(K && key, VArgs &&... valArgs) -> std::pair<iterator, bool> {
        return _try_emplace(std::move(key), std::forward<VArgs>(valArgs)...);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <typename K_, typename... VArgs>
    inline auto Map<K, V, H, KE, A>::_try_emplace(K_ && key, VArgs &&... vArgs) -> std::pair<iterator, bool> {
        static_assert(!(!_isSet && !sizeof...(VArgs) && !std::is_default_constructible_v<V>), "The value type must be default constructible in order to pass no value arguments");
        static_assert(!(_isSet && sizeof...(VArgs)), "Sets do not have values");

        // If we've yet to allocate memory, now is the time
        if (!_elements) {
            _allocate<true>();
        }

        auto [slotElement, result]{_findKey(key)};

        // Key is already present
        if (result == _FindKeyResult::present) {
            return {iterator{slotElement}, false};
        }

        // Rehash if we're at capacity
        if (_size >= (_slotCount >> 1) + 1u) [[unlikely]] {
            _rehash((_slotCount << 1) - 1u);
            std::tie(slotElement, result) = _findKey(key);
        }

        // Shift back next bucket if necessary
        if (result == _FindKeyResult::occupied) {
            _shiftBucketBack(slotElement);
        }

        if constexpr (_isSet) {
            std::allocator_traits<A>::construct(_alloc, slotElement, std::forward<K_>(key));
        }
        else {
            std::allocator_traits<A>::construct(_alloc, &slotElement->first, std::forward<K_>(key));
            std::allocator_traits<A>::construct(_alloc, &slotElement->second, std::forward<VArgs>(vArgs)...);
        }

        ++_size;

        return {iterator{slotElement}, true};
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline bool Map<K, V, H, KE, A>::erase(const K & key) {
        if (!_size) {
            return false;
        }

        const auto [slotElement, result]{_findKey(key)};

        if (result == _FindKeyResult::present) {
            erase(iterator{slotElement});
            return true;
        }
        else {
            return false;
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::erase(const iterator position) {
        E * eraseElement{position._element};
        std::allocator_traits<A>::destroy(_alloc, eraseElement);

        // Special zero key case
        if (eraseElement == _elements) {
            _raw(_key(*_elements)) = _specialRawKey;
            --_size;
            return;
        }

        // General case

        const E * const lastElement{_elements + _slotCount};
        size_t eraseBucketSlotI{_slot(_key(*eraseElement))};

        while (true) {
            E * lastBucketElement{eraseElement};

            // Find the last element in the bucket
            while (true) {
                E * nextSlotElement{lastBucketElement + 1};
                if (nextSlotElement == lastElement) [[unlikely]] {
                    nextSlotElement = _elements + 1;
                }

                if (!_raw(_key(*nextSlotElement))) {
                    break;
                }

                const size_t nextBucketSlotI{_slot(_key(*nextSlotElement))};
                if (nextBucketSlotI > eraseBucketSlotI) {
                    break;
                }

                lastBucketElement = nextSlotElement;
            }

            if (eraseElement == lastBucketElement) {
                _raw(_key(*eraseElement)) = {};
                break;
            }
            else {
                std::allocator_traits<A>::construct(_alloc, eraseElement, std::move(*lastBucketElement));
                std::allocator_traits<A>::destroy(_alloc, lastBucketElement);

                eraseElement = lastBucketElement;
                eraseBucketSlotI = lastBucketElement - _elements;
            }
        }

        --_size;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::clear() noexcept {
        _clear<true>();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool zeroKeys>
    inline void Map<K, V, H, KE, A>::_clear() noexcept {
        if constexpr (std::is_trivially_destructible_v<E>) {
            if constexpr (zeroKeys) {
                if (_size) {
                    _zeroKeys();
                    _size = {};
                }
            }
        }
        else {
            if (_size) {
                size_t n{};

                // Special zero key case
                if (_RawKey & rawKey{_raw(_key(*_elements))}; !rawKey) {
                    std::allocator_traits<A>::destroy(_alloc, _elements);

                    if constexpr (zeroKeys) {
                        rawKey = _specialRawKey;
                    }

                    ++n;
                }

                // General case
                for (E * element{_elements + 1}; n < _size; ++element) {
                    if (_RawKey & rawKey{_raw(_key(*element))}; rawKey) {
                        std::allocator_traits<A>::destroy(_alloc, element);

                        if constexpr (zeroKeys) {
                            rawKey = {};
                        }

                        ++n;
                    }
                }

                _size = {};
            }
        }

    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline bool Map<K, V, H, KE, A>::contains(const K & key) const {
        return _size ? _findKey(key).second == _FindKeyResult::present : false;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline size_t Map<K, V, H, KE, A>::count(const K & key) const {
        return contains(key);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline std::add_lvalue_reference_t<V> Map<K, V, H, KE, A>::at(const K & key) {
        // TODO: Remove once MSVC supports `std::add_lvalue_reference_t` along with requires clause
        static_assert(!_isSet, "Sets do not have mapped values");

        return const_cast<V &>(const_cast<const Map *>(this)->at(key));
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline std::add_lvalue_reference_t<const V> Map<K, V, H, KE, A>::at(const K & key) const {
        // TODO: Remove once MSVC supports `std::add_lvalue_reference_t` along with requires clause
        static_assert(!_isSet, "Sets do not have mapped values");

        if (!_size) {
            throw std::out_of_range{"Map is empty"};
        }

        const auto [slotElement, result]{_findKey(key)};

        if (result != _FindKeyResult::present) {
            throw std::out_of_range{"Element not found"};
        }

        return slotElement->second;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline std::add_lvalue_reference_t<V> Map<K, V, H, KE, A>::operator[](const K & key) {
        // TODO: Remove once MSVC supports `std::add_lvalue_reference_t` along with requires clause
        static_assert(!_isSet, "Sets do not have mapped values");

        return try_emplace(key).first->second;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline std::add_lvalue_reference_t<V> Map<K, V, H, KE, A>::operator[](K && key) {
        // TODO: Remove once MSVC supports `std::add_lvalue_reference_t` along with requires clause
        static_assert(!_isSet, "Sets do not have mapped values");

        return try_emplace(std::move(key)).first->second;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::begin() noexcept -> iterator {
        // Separated to placate IntelliSense
        const const_iterator cit{const_cast<const Map *>(this)->begin()};
        return reinterpret_cast<const iterator &>(cit);
    }

    // TODO: try caching this
    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::begin() const noexcept -> const_iterator {
        if (!_size) {
            return end();
        }

        // Special zero key case
        if (!_raw(_key(*_elements))) {
            return const_iterator{_elements};
        }

        // General case
        for (const E * slotElement{_elements + 1}; ; ++slotElement) {
            if (_raw(_key(*slotElement))) {
                return const_iterator{slotElement};
            }
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::cbegin() const noexcept -> const_iterator {
        return begin();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline typename Map<K, V, H, KE, A>::iterator Map<K, V, H, KE, A>::end() noexcept {
        // Separated to placate IntelliSense
        const const_iterator cit{const_cast<const Map *>(this)->end()};
        return reinterpret_cast<const iterator &>(cit);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::end() const noexcept -> const_iterator {
        return const_iterator{_elements + _slotCount};
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::cend() const noexcept -> const_iterator {
        return end();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::find(const K & key) -> iterator {
        // Separated to dodge a warning
        const const_iterator temp{const_cast<const Map *>(this)->find(key)};
        return reinterpret_cast<const iterator &>(temp);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::find(const K & key) const -> const_iterator {
        if (!_size) {
            return cend();
        }

        const auto [slotElement, result]{_findKey(key)};
        return result == _FindKeyResult::present ? const_iterator{slotElement} : cend();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::equal_range(const K & key) -> std::pair<iterator, iterator> {
        const iterator it{find(key)};
        return {it, it};
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::equal_range(const K & key) const -> std::pair<const_iterator, const_iterator> {
        const const_iterator it{find(key)};
        return {it, it};
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline size_t Map<K, V, H, KE, A>::slot(const K & key) const noexcept {
        return _raw(key) ? _slot(key) : 0u;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline size_t Map<K, V, H, KE, A>::_slot(const K & key) const noexcept {
        return (_hash(key) & (_slotCount - 2u)) + 1u;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::reserve(const size_t capacity) {
        rehash((capacity << 1) - 1u);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::rehash(size_t slotCount) {
        if (slotCount <= config::minSlotCount) {
            slotCount = config::minSlotCount;
        }
        else {
            if ((slotCount >> 1) + 1u < _size) {
                slotCount = (_size << 1) - 1u;
            }

            slotCount = std::bit_ceil(slotCount - 1u) + 1u;
        }

        if (slotCount != _slotCount) {
            if (_elements) {
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
        const size_t oldSlotCount{_slotCount};
        E * const oldElements{_elements};

        _size = {};
        _slotCount = slotCount;
        _allocate<true>();

        size_t n{};

        // Special zero key case
        if (!_raw(_key(*oldElements))) {
            emplace(std::move(*oldElements));
            std::allocator_traits<A>::destroy(_alloc, oldElements);
            ++n;
        }

        // General case
        for (E * element{oldElements + 1}; n < oldSize; ++element) {
            if (_raw(_key(*element))) {
                emplace(std::move(*element));
                std::allocator_traits<A>::destroy(_alloc, element);
                ++n;
            }
        }

        std::allocator_traits<A>::deallocate(_alloc, oldElements, oldSlotCount + 1u);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::swap(Map & other) noexcept {
        std::swap(_size, other._size);
        std::swap(_slotCount, other._slotCount);
        std::swap(_elements, other._elements);
        std::swap(_hash, other._hash);
        std::swap(_equal, other._equal);
        if constexpr (std::allocator_traits<A>::propagate_on_container_swap::value) {
            std::swap(_alloc, other._alloc);
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline size_t Map<K, V, H, KE, A>::size() const noexcept {
        return _size;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline bool Map<K, V, H, KE, A>::empty() const noexcept {
        return !_size;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline size_t Map<K, V, H, KE, A>::max_size() const noexcept {
        return (max_slot_count() + 1u) >> 1;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline size_t Map<K, V, H, KE, A>::capacity() const noexcept {
        return (_slotCount + 1u) >> 1;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline size_t Map<K, V, H, KE, A>::slot_count() const noexcept {
        return _slotCount;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline size_t Map<K, V, H, KE, A>::max_slot_count() const noexcept {
        return (size_t(1u) << (std::numeric_limits<size_t>::digits - 1)) + 1u;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline float Map<K, V, H, KE, A>::load_factor() const noexcept {
        return float(_size) / float(_slotCount);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline float Map<K, V, H, KE, A>::max_load_factor() const noexcept {
        return float(config::minCapacity) / float(config::minSlotCount);
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
    inline K & Map<K, V, H, KE, A>::_key(E & element) noexcept {
        if constexpr (_isSet) return element;
        else return element.first;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline const K & Map<K, V, H, KE, A>::_key(const E & element) noexcept {
        if constexpr (_isSet) return element;
        else return element.first;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::_raw(K & key) noexcept -> _RawKey & {
        return reinterpret_cast<_RawKey &>(key);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::_raw(const K & key) noexcept -> const _RawKey & {
        return reinterpret_cast<const _RawKey &>(key);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool zeroKeys>
    inline void Map<K, V, H, KE, A>::_allocate() {
        _elements = std::allocator_traits<A>::allocate(_alloc, _slotCount + 1u);

        if constexpr (zeroKeys) {
            _zeroKeys();
        }

        // Set the trailing key to non-zero value so iterators know when to stop without needing to know the slot count
        _raw(_key(_elements[_slotCount])) = _specialRawKey;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::_deallocate() {
        std::allocator_traits<A>::deallocate(_alloc, _elements, _slotCount + 1u);
        _elements = nullptr;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::_zeroKeys() noexcept {
        // Special zero key case
        _raw(_key(*_elements)) = _specialRawKey;

        // General case
        // TODO: compare to memset
        for (size_t slotI{1u}; slotI < _slotCount; ++slotI) {
            _raw(_key(_elements[slotI])) = {};
        }

    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool move>
    inline void Map<K, V, H, KE, A>::_forwardData(std::conditional_t<move, Map, const Map> & other) {
        if constexpr (std::is_trivially_copyable_v<E>) {
            std::memcpy(_elements, other._elements, _slotCount * sizeof(E));
        }
        else {
            using ElementForwardType = std::conditional_t<move, E &&, const E &>;

            // Special zero key case
            if (!_raw(_key(*other._elements))) {
                std::allocator_traits<A>::construct(_alloc, _elements, static_cast<ElementForwardType>(*other._elements));
            }
            else {
                _raw(_key(*_elements)) = _specialRawKey;
            }

            // General case
            std::conditional_t<move, E, const E> * srcElement{other._elements + 1};
            const E * const srcEndElement{other._elements + _slotCount};
            E * dstElement{_elements + 1};
            for (; srcElement < srcEndElement; ++srcElement, ++dstElement) {
                if (_raw(_key(*srcElement))) {
                    std::allocator_traits<A>::construct(_alloc, dstElement, static_cast<ElementForwardType>(*srcElement));
                }
                else {
                    _raw(_key(*dstElement)) = {};
                }
            }
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::_findKey(const K & key) noexcept -> std::pair<E *, _FindKeyResult> {
        // Separated to dodge a compiler warning
        const std::pair<const E *, _FindKeyResult> temp{const_cast<const Map *>(this)->_findKey(key)};
        return reinterpret_cast<const std::pair<E *, _FindKeyResult> &>(temp);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::_findKey(const K & key) const noexcept -> std::pair<const E *, _FindKeyResult> {
        // Special zero key case
        if (!_raw(key)) {
            // True/false implicitly converts to 1/0 which matches present/vacant enum values
            return {_elements, _FindKeyResult(!_raw(_key(*_elements)))};
        }

        // General case

        const size_t bucketSlotI{_slot(key)};
        const E * slotElement{_elements + bucketSlotI};
        const E * const lastElement{_elements + _slotCount};
        bool present{};
        bool occupied{};

        // Seek to the key if is is present, or one past the end of the bucket
        if (_raw(_key(*slotElement)) && !(present = _equal(_key(*slotElement), key))) {
            do {
                ++slotElement;

                if (slotElement == lastElement) [[unlikely]] {
                    slotElement = _elements + 1;
                }
            } while (_raw(_key(*slotElement)) && !(present = _equal(_key(*slotElement), key)) && !(occupied = _slot(_key(*slotElement)) != bucketSlotI));
        }

        // Present/occupied boolean to enum branchless conversion
        return {slotElement, _FindKeyResult(present + occupied * 2)};
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::_shiftBucketBack(E * const firstBucketElement) noexcept {
        const E * const lastElement{_elements + _slotCount};
        const size_t bucketSlotI{_slot(_key(*firstBucketElement))};

        // Find slot after bucket
        E * slotElement{firstBucketElement};
        do {
            ++slotElement;

            if (slotElement == lastElement) [[unlikely]] {
                slotElement = _elements + 1;
            }
        } while (_raw(_key(*slotElement)) && _slot(_key(*slotElement)) == bucketSlotI);

        // The slot is occupied by the next bucket - need to push it back first
        if (_raw(_key(*slotElement))) {
            _shiftBucketBack(slotElement);
        }

        // Move the first bucket element to the now-end of the bucket
        std::allocator_traits<A>::construct(_alloc, slotElement, std::move(*firstBucketElement));
        std::allocator_traits<A>::destroy(_alloc, firstBucketElement);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline bool operator==(const Map<K, V, H, KE, A> & m1, const Map<K, V, H, KE, A> & m2) {
        if (m1.size() != m2.size()) {
            return false;
        }

        if (&m1 == &m2) {
            return true;
        }

        const auto endIt{m2.cend()};

        for (const auto & e : m1) {
            if constexpr (std::is_same_v<V, void>) {
                if (!m2.contains(e)) {
                    return false;
                }
            }
            else {
                const auto it{m2.find(e.first)};
                if (it == endIt || it->second != e.second) {
                    return false;
                }
            }
        }

        return true;
    }

    // Iterator ================================================================

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    template <bool constant_> requires (constant && !constant_)
    inline constexpr Map<K, V, H, KE, A>::_Iterator<constant>::_Iterator(const _Iterator<constant_> & other) noexcept:
        _element{other._element}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline constexpr Map<K, V, H, KE, A>::_Iterator<constant>::_Iterator(E * const element) noexcept :
        _element{element}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto Map<K, V, H, KE, A>::_Iterator<constant>::operator*() const noexcept -> E & {
        return *_element;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto Map<K, V, H, KE, A>::_Iterator<constant>::operator->() const noexcept -> E * {
        return _element;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto Map<K, V, H, KE, A>::_Iterator<constant>::operator++() noexcept -> _Iterator & {
        do {
            ++_element;
        } while (!_raw(_key(*_element)));

        return *this;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto Map<K, V, H, KE, A>::_Iterator<constant>::operator++(int) noexcept -> _Iterator {
        const _Iterator temp{*this};
        operator++();
        return temp;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    template <bool constant_>
    inline bool Map<K, V, H, KE, A>::_Iterator<constant>::operator==(const _Iterator<constant_> & it) const noexcept {
        return _element == it._element;
    }

} // namespace qc_hash

namespace std {

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void swap(qc_hash::Map<K, V, H, KE, A> & a, qc_hash::Map<K, V, H, KE, A> & b) noexcept {
        a.swap(b);
    }

} // namespace std

#pragma warning(pop)
