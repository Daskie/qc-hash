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

#include <bit>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace qc::hash
{
    // This code assumes `size_t` is either 4 or 8 bytes
    static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8, "Unsupported architecture");

    // This code assumes that pointers are the same size as `size_t`
    static_assert(sizeof(void *) == sizeof(size_t), "Unsupported architecture");

    namespace config
    {
        ///
        /// The capacity new maps/sets will be initialized with, once memory is allocated. The capacity will never be
        /// rehashed below this value. Does not include the two special elements, as they do not count against the load
        /// factor.
        ///
        /// Must be a power of two.
        ///
        constexpr size_t minCapacity{16u};
    }

    template <typename T> concept NativeInteger = std::is_integral_v<T> && !std::is_same_v<T, bool> && sizeof(T) <= sizeof(size_t);
    template <typename T> concept NativeSignedInteger = NativeInteger<T> && std::is_signed_v<T>;
    template <typename T> concept NativeUnsignedInteger = NativeInteger<T> && std::is_unsigned_v<T>;
    template <typename T> concept NativeEnum = std::is_enum_v<T> && sizeof(T) <= sizeof(size_t);
    template <typename T> concept Pointer = std::is_pointer_v<T>;

    template <typename T> struct _UTypeHelper;
    template <typename T> requires (sizeof(T) == 1u && alignof(T) >= 1u) struct _UTypeHelper<T> { using type = uint8_t; };
    template <typename T> requires (sizeof(T) == 2u && alignof(T) >= 2u) struct _UTypeHelper<T> { using type = uint16_t; };
    template <typename T> requires (sizeof(T) == 4u && alignof(T) >= 4u) struct _UTypeHelper<T> { using type = uint32_t; };
    template <typename T> requires (sizeof(T) == 8u && alignof(T) >= 8u) struct _UTypeHelper<T> { using type = uint64_t; };

    template <typename T> using UType = typename _UTypeHelper<T>::type;

    template <typename T> struct HasUniqueRepresentation : std::false_type {};
    template <typename T> struct HasUniqueRepresentation<std::unique_ptr<T>> : std::true_type {};

    template <typename T> concept Rawable = (sizeof(T) <= sizeof(size_t)) && (alignof(T) >= sizeof(T)) && (std::has_unique_object_representations_v<T> || HasUniqueRepresentation<T>::value);

    //
    // ...
    // Must provide specializations for heterogeneous lookup!
    //
    template <typename K> struct RawHash;
    template <NativeUnsignedInteger K> struct RawHash<K>;
    template <NativeSignedInteger K> struct RawHash<K>;
    template <NativeEnum K> struct RawHash<K>;
    template <Pointer K> struct RawHash<K>;
    template <typename T> struct RawHash<std::unique_ptr<T>>;

    template <typename T, typename H> concept Comparable = Rawable<T> && requires (const H h, const T v) { { h(v) } -> NativeUnsignedInteger; };

    //
    // TODO: Only needed due to limited MSVC `requires` keyword support. This should be inlined.
    //
    template <typename H, typename K> concept _IsValidHasher = requires (const H h, const K k) { { h(k) } -> NativeUnsignedInteger; };

    //
    // Forward declaration of friend type used for testing
    //
    struct RawFriend;

    //
    // ...
    //
    template <Rawable K, typename V, typename H = RawHash<K>, typename KE = void, typename A = std::allocator<std::pair<K, V>>> class RawMap;

    //
    // ...
    // Defined as a `RawMap` whose mapped type is `void`.
    //
    template <Rawable K, typename H = RawHash<K>, typename KE = void, typename A = std::allocator<K>> using RawSet = RawMap<K, void, H, KE, A>;

    //
    // ...
    //
    template <Rawable K, typename V, typename H, typename KE, typename A> class RawMap
    {
        static constexpr bool _isSet{std::is_same_v<V, void>};
        static constexpr bool _isMap{!_isSet};

        using E = std::conditional_t<_isSet, K, std::pair<K, V>>;

        template <bool constant> class _Iterator;

        friend ::qc::hash::RawFriend;

        public: //--------------------------------------------------------------

        static_assert(std::is_nothrow_move_constructible_v<E>);
        static_assert(std::is_nothrow_move_assignable_v<E>);
        static_assert(std::is_nothrow_swappable_v<E>);
        static_assert(std::is_nothrow_destructible_v<E>);

        // TODO: Make hasher requriements clear in docs
        static_assert(_IsValidHasher<H, K>);
        static_assert(std::is_nothrow_move_constructible_v<H>);
        static_assert(std::is_nothrow_move_assignable_v<H>);
        static_assert(std::is_nothrow_swappable_v<H>);
        static_assert(std::is_nothrow_destructible_v<H>);

        static_assert(std::is_same_v<KE, void>);

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
        explicit RawMap(size_t minCapacity = config::minCapacity, const H & hash = {}, const A & alloc = {}) noexcept;
        RawMap(size_t minCapacity, const A & alloc) noexcept;
        explicit RawMap(const A & alloc) noexcept;
        template <typename It> RawMap(It first, It last, size_t minCapacity = {}, const H & hash = {}, const A & alloc = {});
        template <typename It> RawMap(It first, It last, size_t minCapacity, const A & alloc);
        RawMap(std::initializer_list<E> elements, size_t minCapacity = {}, const H & hash = {}, const A & alloc = {});
        RawMap(std::initializer_list<E> elements, size_t minCapacity, const A & alloc);
        RawMap(const RawMap & other);
        RawMap(RawMap && other) noexcept;

        //
        // ...
        //
        RawMap & operator=(std::initializer_list<E> elements);
        RawMap & operator=(const RawMap & other);
        RawMap & operator=(RawMap && other) noexcept;

        //
        // Destructs all elements and frees all memory allocated.
        //
        ~RawMap() noexcept;

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
        template <Comparable<H> K_> bool contains(const K_ & key) const;

        //
        // Returns `1` if the map contains an element for `key` and `0` if it does
        // not.
        //
        template <Comparable<H> K_> size_t count(const K_ & key) const;

        //
        // ...
        //
        template <Comparable<H> K_> std::add_lvalue_reference_t<V> at(const K_ & key) requires (!std::is_same_v<V, void>);
        template <Comparable<H> K_> std::add_lvalue_reference_t<const V> at(const K_ & key) const requires (!std::is_same_v<V, void>);

        //
        // ...
        //
        std::add_lvalue_reference_t<V> operator[](const K & key) requires (!std::is_same_v<V, void>);
        std::add_lvalue_reference_t<V> operator[](K && key) requires (!std::is_same_v<V, void>);

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
        template <Comparable<H> K_> iterator find(const K_ & key);
        template <Comparable<H> K_> const_iterator find(const K_ & key) const;

        //
        // As a key may correspond to as most one element, this method is
        // equivalent to `find`, except returning a pair of duplicate iterators.
        //
        template <Comparable<H> K_> std::pair<iterator, iterator> equal_range(const K_ & key);
        template <Comparable<H> K_> std::pair<const_iterator, const_iterator> equal_range(const K_ & key) const;

        //
        // Returns the index of the slot into which `key` would fall.
        //
        template <Comparable<H> K_> size_t slot(const K_ & key) const noexcept;

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
        void swap(RawMap & other) noexcept;

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
        const H & hash_function() const noexcept;

        //
        // Returns an instance of `key_equal`.
        //
        void key_eq() const noexcept {};

        //
        // Returns an instance of `allocator_type`.
        //
        const A & get_allocator() const noexcept;

        private: //-------------------------------------------------------------

        using _RawKey = UType<K>;

        static constexpr _RawKey _vacantKey{std::numeric_limits<_RawKey>::max()};
        static constexpr _RawKey _graveKey{_RawKey(_vacantKey - 1u)};
        static constexpr _RawKey _specialKeys[2]{_graveKey, _vacantKey};
        static constexpr _RawKey _vacantGraveKey{_vacantKey};
        static constexpr _RawKey _vacantVacantKey{_graveKey};
        static constexpr _RawKey _vacantSpecialKeys[2]{_vacantGraveKey, _vacantVacantKey};
        static constexpr _RawKey _terminalKey{_RawKey(0u)};

        static K & _key(E & element) noexcept;
        static const K & _key(const E & element) noexcept;

        template <Rawable K_> static UType<K_> & _raw(K_ & key) noexcept;
        template <Rawable K_> static const UType<K_> & _raw(const K_ & key) noexcept;

        static bool _isPresent(_RawKey key) noexcept;

        static bool _isSpecial(_RawKey key) noexcept;

        size_t _size;
        size_t _slotCount; // Does not include special elements
        E * _elements;
        bool _haveSpecial[2];
        H _hash;
        A _alloc;

        template <typename KTuple, typename VTuple, size_t... kIndices, size_t... vIndices> std::pair<iterator, bool> _emplace(KTuple && kTuple, VTuple && vTuple, std::index_sequence<kIndices...>, std::index_sequence<vIndices...>);

        template <typename K_, typename... VArgs> std::pair<iterator, bool> _try_emplace(K_ && key, VArgs &&... vArgs);

        template <bool preserveInvariants> void _clear() noexcept;

        //
        // Returns the index of the slot into which `key` would fall.
        //
        template <Comparable<H> K_> size_t _slot(const K_ & key) const noexcept;

        void _rehash(size_t slotCount);

        template <bool zeroControls> void _allocate();

        void _deallocate();

        void _clearKeys() noexcept;

        template <bool move> void _forwardData(std::conditional_t<move, RawMap, const RawMap> & other);

        template <bool insertionForm> struct _FindKeyResult;
        template <> struct _FindKeyResult<false> { E * element; bool isPresent; };
        template <> struct _FindKeyResult<true> { E * element; bool isPresent; bool isSpecial; uint8_t specialI; };

        //
        // ...
        // If the key is not present, returns the element after the end of the key's bucket
        //
        template <bool insertionForm, Comparable<H> K_> _FindKeyResult<insertionForm> _findKey(const K_ & key) const noexcept;
    };

    template <Rawable K, typename V, typename H, typename KE, typename A> bool operator==(const RawMap<K, V, H, KE, A> & m1, const RawMap<K, V, H, KE, A> & m2);

    //
    // Forward iterator
    //
    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    class RawMap<K, V, H, KE, A>::_Iterator
    {
        friend ::qc::hash::RawMap<K, V, H, KE, A>;
        friend ::qc::hash::RawFriend;

        using E = std::conditional_t<constant, const RawMap::E, RawMap::E>;

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
}

namespace std
{
    template <typename K, typename V, typename H, typename KE, typename A> void swap(qc::hash::RawMap<K, V, H, KE, A> & a, qc::hash::RawMap<K, V, H, KE, A> & b) noexcept;
}

// INLINE IMPLEMENTATION ///////////////////////////////////////////////////////////////////////////////////////////////

namespace qc::hash
{

    constexpr size_t _minSlotCount{config::minCapacity * 2u};

    template <NativeUnsignedInteger K>
    struct RawHash<K>
    {
        template <NativeUnsignedInteger K_> requires (sizeof(K_) <= sizeof(K))
        size_t operator()(const K_ k) const noexcept
        {
            return k;
        }
    };

    template <NativeSignedInteger K>
    struct RawHash<K>
    {
        template <NativeSignedInteger K_> requires (sizeof(K_) <= sizeof(K))
        size_t operator()(const K_ k) const noexcept
        {
            return UType<K_>(k);
        }

        template <NativeUnsignedInteger K_> requires (sizeof(K_) < sizeof(K))
        size_t operator()(const K_ k) const noexcept
        {
            return k;
        }
    };

    template <NativeEnum K>
    struct RawHash<K>
    {
        size_t operator()(const K k) const noexcept
        {
            return UType<K>(k);
        }
    };

    template <Pointer K>
    struct RawHash<K>
    {
        using T = std::remove_pointer_t<K>;

        size_t operator()(const T * const k) const noexcept
        {
            constexpr int shift{int(std::bit_width(alignof(T)) - 1u)};
            return reinterpret_cast<size_t>(k) >> shift;
        }
    };

    template <typename T>
    struct RawHash<std::unique_ptr<T>> : RawHash<T *>
    {
        using RawHash<T *>::operator();

        size_t operator()(const std::unique_ptr<T> & k) const noexcept
        {
            return operator()(k.get());
        }
    };

    // RawMap ==================================================================

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline RawMap<K, V, H, KE, A>::RawMap(const size_t minCapacity, const H & hash, const A & alloc) noexcept:
        _size{},
        _slotCount{minCapacity <= config::minCapacity ? _minSlotCount : std::bit_ceil(minCapacity << 1)},
        _elements{},
        _haveSpecial{},
        _hash{hash},
        _alloc{alloc}
    {}

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline RawMap<K, V, H, KE, A>::RawMap(const size_t minCapacity, const A & alloc) noexcept :
        RawMap{minCapacity, H{}, alloc}
    {}

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline RawMap<K, V, H, KE, A>::RawMap(const A & alloc) noexcept :
        RawMap{config::minCapacity, H{}, alloc}
    {}

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <typename It>
    inline RawMap<K, V, H, KE, A>::RawMap(const It first, const It last, const size_t minCapacity, const H & hash, const A & alloc) :
        RawMap{minCapacity, hash, alloc}
    {
        // Count number of elements to insert
        size_t n{};
        for (It it{first}; it != last; ++it, ++n);

        reserve(n);

        insert(first, last);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <typename It>
    inline RawMap<K, V, H, KE, A>::RawMap(const It first, const It last, const size_t minCapacity, const A & alloc) :
        RawMap{first, last, minCapacity, H{}, alloc}
    {}

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline RawMap<K, V, H, KE, A>::RawMap(std::initializer_list<E> elements, size_t minCapacity, const H & hash, const A & alloc) :
        RawMap{minCapacity ? minCapacity : elements.size(), hash, alloc}
    {
        insert(elements);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline RawMap<K, V, H, KE, A>::RawMap(const std::initializer_list<E> elements, const size_t minCapacity, const A & alloc) :
        RawMap{elements, minCapacity, H{}, alloc}
    {}

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline RawMap<K, V, H, KE, A>::RawMap(const RawMap & other) :
        _size{other._size},
        _slotCount{other._slotCount},
        _elements{},
        _haveSpecial{other._haveSpecial[0], other._haveSpecial[1]},
        _hash{other._hash},
        _alloc{std::allocator_traits<A>::select_on_container_copy_construction(other._alloc)}
    {
        if (_size) {
            _allocate<false>();
            _forwardData<false>(other);
        }
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline RawMap<K, V, H, KE, A>::RawMap(RawMap && other) noexcept :
        _size{std::exchange(other._size, 0u)},
        _slotCount{std::exchange(other._slotCount, _minSlotCount)},
        _elements{std::exchange(other._elements, nullptr)},
        _hash{std::move(other._hash)},
        _haveSpecial{std::exchange(other._haveSpecial[0], false), std::exchange(other._haveSpecial[1], false)},
        _alloc{std::move(other._alloc)}
    {}

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline RawMap<K, V, H, KE, A> & RawMap<K, V, H, KE, A>::operator=(const std::initializer_list<E> elements)
    {
        return *this = RawMap(elements);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline RawMap<K, V, H, KE, A> & RawMap<K, V, H, KE, A>::operator=(const RawMap & other)
    {
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
        _haveSpecial[0] = other._haveSpecial[0];
        _haveSpecial[1] = other._haveSpecial[1];
        _hash = other._hash;
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

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline RawMap<K, V, H, KE, A> & RawMap<K, V, H, KE, A>::operator=(RawMap && other) noexcept
    {
        if (&other == this) {
            return *this;
        }

        if (_elements) {
            _clear<false>();
            _deallocate();
        }

        _size = other._size;
        _slotCount = other._slotCount;
        _haveSpecial[0] = other._haveSpecial[0];
        _haveSpecial[1] = other._haveSpecial[1];
        _hash = std::move(other._hash);
        if constexpr (std::allocator_traits<A>::propagate_on_container_move_assignment::value) {
            _alloc = std::move(other._alloc);
        }

        if (_alloc == other._alloc || std::allocator_traits<A>::propagate_on_container_move_assignment::value) {
            _elements = std::exchange(other._elements, nullptr);
            other._size = {};
        }
        else {
            if (_size) {
                _allocate<false>();
                _forwardData<true>(other);
                other._clear<false>();
                other._size = 0u;
            }
            if (other._elements) {
                other._deallocate();
            }
        }

        other._slotCount = _minSlotCount;
        other._haveSpecial[0] = false;
        other._haveSpecial[1] = false;

        return *this;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline RawMap<K, V, H, KE, A>::~RawMap() noexcept
    {
        if (_elements) {
            _clear<false>();
            _deallocate();
        }
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline auto RawMap<K, V, H, KE, A>::insert(const E & element) -> std::pair<iterator, bool>
    {
        static_assert(std::is_copy_constructible_v<E>);

        return emplace(element);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline auto RawMap<K, V, H, KE, A>::insert(E && element) -> std::pair<iterator, bool>
    {
        return emplace(std::move(element));
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <typename It>
    inline void RawMap<K, V, H, KE, A>::insert(It first, const It last)
    {
        while (first != last) {
            emplace(*first);
            ++first;
        }
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline void RawMap<K, V, H, KE, A>::insert(const std::initializer_list<E> elements)
    {
        for (const E & element : elements) {
            emplace(element);
        }
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline auto RawMap<K, V, H, KE, A>::emplace(const E & element) -> std::pair<iterator, bool>
    {
        static_assert(std::is_copy_constructible_v<E>);

        if constexpr (_isSet) {
            return try_emplace(element);
        }
        else {
            return try_emplace(element.first, element.second);
        }
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline auto RawMap<K, V, H, KE, A>::emplace(E && element) -> std::pair<iterator, bool>
    {
        if constexpr (_isSet) {
            return try_emplace(std::move(element));
        }
        else {
            return try_emplace(std::move(element.first), std::move(element.second));
        }
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <typename K_, typename V_>
    inline auto RawMap<K, V, H, KE, A>::emplace(K_ && key, V_ && val) -> std::pair<iterator, bool> requires (!std::is_same_v<V, void>)
    {
        return try_emplace(std::forward<K_>(key), std::forward<V_>(val));
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <typename... KArgs>
    inline auto RawMap<K, V, H, KE, A>::emplace(KArgs &&... keyArgs) -> std::pair<iterator, bool> requires (std::is_same_v<V, void>)
    {
        return try_emplace(K{std::forward<KArgs>(keyArgs)...});
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <typename... KArgs, typename... VArgs>
    inline auto RawMap<K, V, H, KE, A>::emplace(const std::piecewise_construct_t, std::tuple<KArgs...> && keyArgs, std::tuple<VArgs...> && valArgs) -> std::pair<iterator, bool> requires (!std::is_same_v<V, void>)
    {
        return _emplace(std::move(keyArgs), std::move(valArgs), std::index_sequence_for<KArgs...>(), std::index_sequence_for<VArgs...>());
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <typename KTuple, typename VTuple, size_t... kIndices, size_t... vIndices>
    inline auto RawMap<K, V, H, KE, A>::_emplace(KTuple && kTuple, VTuple && vTuple, const std::index_sequence<kIndices...>, const std::index_sequence<vIndices...>) -> std::pair<iterator, bool>
    {
        return try_emplace(K{std::move(std::get<kIndices>(kTuple))...}, std::move(std::get<vIndices>(vTuple))...);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <typename... VArgs>
    inline auto RawMap<K, V, H, KE, A>::try_emplace(const K & key, VArgs &&... valArgs) -> std::pair<iterator, bool>
    {
        static_assert(std::is_copy_constructible_v<K>);

        return _try_emplace(key, std::forward<VArgs>(valArgs)...);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <typename... VArgs>
    inline auto RawMap<K, V, H, KE, A>::try_emplace(K && key, VArgs &&... valArgs) -> std::pair<iterator, bool>
    {
        return _try_emplace(std::move(key), std::forward<VArgs>(valArgs)...);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <typename K_, typename... VArgs>
    inline auto RawMap<K, V, H, KE, A>::_try_emplace(K_ && key, VArgs &&... vArgs) -> std::pair<iterator, bool>
    {
        static_assert(!(_isMap && !sizeof...(VArgs) && !std::is_default_constructible_v<V>), "The value type must be default constructible in order to pass no value arguments");
        static_assert(!(_isSet && sizeof...(VArgs)), "Sets do not have values");

        // If we've yet to allocate memory, now is the time
        if (!_elements) {
            _allocate<true>();
        }

        _FindKeyResult<true> findResult{_findKey<true>(key)};

        // Key is already present
        if (findResult.isPresent) {
            return {iterator{findResult.element}, false};
        }

        if (findResult.isSpecial) [[unlikely]] {
            _haveSpecial[findResult.specialI] = true;
        }
        else {
            // Rehash if we're at capacity
            if ((_size - _haveSpecial[0] - _haveSpecial[1]) >= (_slotCount >> 1)) [[unlikely]] {
                _rehash(_slotCount << 1);
                findResult = _findKey<true>(key);
            }
        }

        if constexpr (_isSet) {
            std::allocator_traits<A>::construct(_alloc, findResult.element, std::forward<K_>(key));
        }
        else {
            std::allocator_traits<A>::construct(_alloc, &findResult.element->first, std::forward<K_>(key));
            std::allocator_traits<A>::construct(_alloc, &findResult.element->second, std::forward<VArgs>(vArgs)...);
        }

        ++_size;

        return {iterator{findResult.element}, true};
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline bool RawMap<K, V, H, KE, A>::erase(const K & key)
    {
        if (!_size) {
            return false;
        }

        const auto [element, isPresent]{_findKey<false>(key)};

        if (isPresent) {
            erase(iterator{element});
            return true;
        }
        else {
            return false;
        }
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline void RawMap<K, V, H, KE, A>::erase(const iterator position)
    {
        E * const eraseElement{position._element};
        _RawKey & rawKey{_raw(_key(*eraseElement))};
        E * const specialElements{_elements + _slotCount};

        std::allocator_traits<A>::destroy(_alloc, eraseElement);

        // General case
        if (eraseElement < specialElements) {
            rawKey = _graveKey;
        }
        else [[unlikely]] {
            const auto specialI{eraseElement - specialElements};
            _raw(_key(specialElements[specialI])) = _vacantSpecialKeys[specialI];
            _haveSpecial[specialI] = false;
        }

        --_size;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline void RawMap<K, V, H, KE, A>::clear() noexcept
    {
        _clear<true>();
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <bool preserveInvariants>
    inline void RawMap<K, V, H, KE, A>::_clear() noexcept
    {
        if constexpr (std::is_trivially_destructible_v<E>) {
            if constexpr (preserveInvariants) {
                if (_size) {
                    _clearKeys();
                    _size = {};
                    _haveSpecial[0] = false;
                    _haveSpecial[1] = false;
                }
            }
        }
        else {
            if (_size) {
                // General case
                E * element{_elements};
                size_t n{};
                const size_t regularElementCount{_size - _haveSpecial[0] - _haveSpecial[1]};
                for (; n < regularElementCount; ++element) {
                    _RawKey & rawKey{_raw(_key(*element))};
                    if (_isPresent(rawKey)) {
                        std::allocator_traits<A>::destroy(_alloc, element);
                        ++n;
                    }
                    if constexpr (preserveInvariants) {
                        rawKey = _vacantKey;
                    }
                }
                // Clear remaining graves
                if constexpr (preserveInvariants) {
                    const E * const endRegularElement{_elements + _slotCount};
                    for (; element < endRegularElement; ++element) {
                        _raw(_key(*element)) = _vacantKey;
                    }
                }

                // Special keys case
                if (_haveSpecial[0]) [[unlikely]] {
                    element = _elements + _slotCount;
                    std::allocator_traits<A>::destroy(_alloc, element);
                    if constexpr (preserveInvariants) {
                        _raw(_key(*element)) = _vacantGraveKey;
                        _haveSpecial[0] = false;
                    }
                }
                if (_haveSpecial[1]) [[unlikely]] {
                    element = _elements + _slotCount + 1;
                    std::allocator_traits<A>::destroy(_alloc, element);
                    if constexpr (preserveInvariants) {
                        _raw(_key(*element)) = _vacantVacantKey;
                        _haveSpecial[1] = false;
                    }
                }

                if constexpr (preserveInvariants) {
                    _size = {};
                }
            }
        }

    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <Comparable<H> K_>
    inline bool RawMap<K, V, H, KE, A>::contains(const K_ & key) const
    {
        return _size ? _findKey<false>(key).isPresent : false;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <Comparable<H> K_>
    inline size_t RawMap<K, V, H, KE, A>::count(const K_ & key) const
    {
        return contains(key);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <Comparable<H> K_>
    inline std::add_lvalue_reference_t<V> RawMap<K, V, H, KE, A>::at(const K_ & key) requires (!std::is_same_v<V, void>)
    {
        return const_cast<V &>(static_cast<const RawMap *>(this)->at(key));
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <Comparable<H> K_>
    inline std::add_lvalue_reference_t<const V> RawMap<K, V, H, KE, A>::at(const K_ & key) const requires (!std::is_same_v<V, void>)
    {
        if (!_size) {
            throw std::out_of_range{"Map is empty"};
        }

        const auto [element, isPresent]{_findKey<false>(key)};

        if (!isPresent) {
            throw std::out_of_range{"Element not found"};
        }

        return element->second;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline std::add_lvalue_reference_t<V> RawMap<K, V, H, KE, A>::operator[](const K & key) requires (!std::is_same_v<V, void>)
    {
        return try_emplace(key).first->second;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline std::add_lvalue_reference_t<V> RawMap<K, V, H, KE, A>::operator[](K && key) requires (!std::is_same_v<V, void>)
    {
        return try_emplace(std::move(key)).first->second;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline auto RawMap<K, V, H, KE, A>::begin() noexcept -> iterator
    {
        // Separated to dodge a compiler warning
        const const_iterator cit{static_cast<const RawMap *>(this)->begin()};
        return reinterpret_cast<const iterator &>(cit);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline auto RawMap<K, V, H, KE, A>::begin() const noexcept -> const_iterator
    {
        // General case
        if (_size - _haveSpecial[0] - _haveSpecial[1]) [[likely]] {
            for (const E * element{_elements}; ; ++element) {
                if (_isPresent(_raw(_key(*element)))) {
                    return const_iterator{element};
                }
            }
        }

        // Special key cases
        if (_haveSpecial[0]) [[unlikely]] {
            return const_iterator{_elements + _slotCount};
        }
        if (_haveSpecial[1]) [[unlikely]] {
            return const_iterator{_elements + _slotCount + 1};
        }

        return end();
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline auto RawMap<K, V, H, KE, A>::cbegin() const noexcept -> const_iterator
    {
        return begin();
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline typename RawMap<K, V, H, KE, A>::iterator RawMap<K, V, H, KE, A>::end() noexcept
    {
        return iterator{};
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline auto RawMap<K, V, H, KE, A>::end() const noexcept -> const_iterator
    {
        return const_iterator{};
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline auto RawMap<K, V, H, KE, A>::cend() const noexcept -> const_iterator
    {
        return const_iterator{};
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <Comparable<H> K_>
    inline auto RawMap<K, V, H, KE, A>::find(const K_ & key) -> iterator
    {
        // Separated to dodge a compiler warning
        const const_iterator temp{static_cast<const RawMap *>(this)->find(key)};
        return reinterpret_cast<const iterator &>(temp);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <Comparable<H> K_>
    inline auto RawMap<K, V, H, KE, A>::find(const K_ & key) const -> const_iterator
    {
        if (!_size) {
            return cend();
        }

        const auto [element, isPresent]{_findKey<false>(key)};
        return isPresent ? const_iterator{element} : cend();
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <Comparable<H> K_>
    inline auto RawMap<K, V, H, KE, A>::equal_range(const K_ & key) -> std::pair<iterator, iterator>
    {
        const iterator it{find(key)};
        return {it, it};
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <Comparable<H> K_>
    inline auto RawMap<K, V, H, KE, A>::equal_range(const K_ & key) const -> std::pair<const_iterator, const_iterator>
    {
        const const_iterator it{find(key)};
        return {it, it};
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <Comparable<H> K_>
    inline size_t RawMap<K, V, H, KE, A>::slot(const K_ & key) const noexcept
    {
        if (_isSpecial(_raw(key))) [[unlikely]] {
            return _slotCount + (_raw(key) & 1u);
        }
        else {
            return _slot(key);
        }
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <Comparable<H> K_>
    inline size_t RawMap<K, V, H, KE, A>::_slot(const K_ & key) const noexcept
    {
        return _hash(key) & (_slotCount - 1u);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline void RawMap<K, V, H, KE, A>::reserve(const size_t capacity)
    {
        rehash(capacity << 1);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline void RawMap<K, V, H, KE, A>::rehash(size_t slotCount)
    {
        const size_t currentMinSlotCount{_size <= config::minCapacity ? _minSlotCount : ((_size - _haveSpecial[0] - _haveSpecial[1]) << 1)};
        if (slotCount < currentMinSlotCount) {
            slotCount = currentMinSlotCount;
        }
        else {
            slotCount = std::bit_ceil(slotCount);
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

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline void RawMap<K, V, H, KE, A>::_rehash(const size_t slotCount)
    {
        const size_t oldSize{_size};
        const size_t oldSlotCount{_slotCount};
        E * const oldElements{_elements};
        const bool oldHaveSpecial[2]{_haveSpecial[0], _haveSpecial[1]};

        _size = {};
        _slotCount = slotCount;
        _allocate<true>();
        _haveSpecial[0] = false;
        _haveSpecial[1] = false;

        // General case
        size_t n{};
        const size_t regularElementCount{oldSize - oldHaveSpecial[0] - oldHaveSpecial[1]};
        for (E * element{oldElements}; n < regularElementCount; ++element) {
            if (_isPresent(_raw(_key(*element)))) {
                emplace(std::move(*element));
                std::allocator_traits<A>::destroy(_alloc, element);
                ++n;
            }
        }

        // Special keys case
        if (oldHaveSpecial[0]) [[unlikely]] {
            E * const oldElement{oldElements + oldSlotCount};
            std::allocator_traits<A>::construct(_alloc, _elements + _slotCount, std::move(*oldElement));
            std::allocator_traits<A>::destroy(_alloc, oldElement);
            ++_size;
            _haveSpecial[0] = true;
        }
        if (oldHaveSpecial[1]) [[unlikely]] {
            E * const oldElement{oldElements + oldSlotCount + 1};
            std::allocator_traits<A>::construct(_alloc, _elements + _slotCount + 1, std::move(*oldElement));
            std::allocator_traits<A>::destroy(_alloc, oldElement);
            ++_size;
            _haveSpecial[1] = true;
        }

        std::allocator_traits<A>::deallocate(_alloc, oldElements, oldSlotCount + (2u + 3u));
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline void RawMap<K, V, H, KE, A>::swap(RawMap & other) noexcept
    {
        std::swap(_size, other._size);
        std::swap(_slotCount, other._slotCount);
        std::swap(_elements, other._elements);
        std::swap(_haveSpecial, other._haveSpecial);
        std::swap(_hash, other._hash);
        if constexpr (std::allocator_traits<A>::propagate_on_container_swap::value) {
            std::swap(_alloc, other._alloc);
        }
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline size_t RawMap<K, V, H, KE, A>::size() const noexcept
    {
        return _size;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline bool RawMap<K, V, H, KE, A>::empty() const noexcept
    {
        return !_size;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline size_t RawMap<K, V, H, KE, A>::max_size() const noexcept
    {
        return (max_slot_count() >> 1) + 2u;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline size_t RawMap<K, V, H, KE, A>::capacity() const noexcept
    {
        return _slotCount >> 1;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline size_t RawMap<K, V, H, KE, A>::slot_count() const noexcept
    {
        return _slotCount;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline size_t RawMap<K, V, H, KE, A>::max_slot_count() const noexcept
    {
        return size_t(1u) << (std::numeric_limits<size_t>::digits - 1);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline float RawMap<K, V, H, KE, A>::load_factor() const noexcept
    {
        return float(_size) / float(_slotCount);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline float RawMap<K, V, H, KE, A>::max_load_factor() const noexcept
    {
        return float(config::minCapacity) / float(_minSlotCount);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline const H & RawMap<K, V, H, KE, A>::hash_function() const noexcept
    {
        return _hash;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline const A & RawMap<K, V, H, KE, A>::get_allocator() const noexcept
    {
        return _alloc;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline K & RawMap<K, V, H, KE, A>::_key(E & element) noexcept
    {
        if constexpr (_isSet) return element;
        else return element.first;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline const K & RawMap<K, V, H, KE, A>::_key(const E & element) noexcept
    {
        if constexpr (_isSet) return element;
        else return element.first;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <Rawable K_>
    inline UType<K_> & RawMap<K, V, H, KE, A>::_raw(K_ & key) noexcept
    {
        return reinterpret_cast<_RawKey &>(key);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <Rawable K_>
    inline const UType<K_> & RawMap<K, V, H, KE, A>::_raw(const K_ & key) noexcept
    {
        return reinterpret_cast<const UType<K_> &>(key);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline bool RawMap<K, V, H, KE, A>::_isPresent(const _RawKey key) noexcept
    {
        return !_isSpecial(key);
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline bool RawMap<K, V, H, KE, A>::_isSpecial(const _RawKey key) noexcept
    {
        return (key | 1u) == _vacantKey;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <bool zeroKeys>
    inline void RawMap<K, V, H, KE, A>::_allocate()
    {
        _elements = std::allocator_traits<A>::allocate(_alloc, _slotCount + (2u + 3u));

        if constexpr (zeroKeys) {
            _clearKeys();
        }

        // Set the trailing keys to special terminal values so iterators know when to stop
        _raw(_key(_elements[_slotCount + 2])) = _terminalKey;
        _raw(_key(_elements[_slotCount + 3])) = _terminalKey;
        _raw(_key(_elements[_slotCount + 4])) = _terminalKey;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline void RawMap<K, V, H, KE, A>::_deallocate()
    {
        std::allocator_traits<A>::deallocate(_alloc, _elements, _slotCount + (2u + 3u));
        _elements = nullptr;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline void RawMap<K, V, H, KE, A>::_clearKeys() noexcept
    {
        // General case
        E * const specialElements{_elements + _slotCount};
        for (E * element{_elements}; element < specialElements; ++element) {
            _raw(_key(*element)) = _vacantKey;
        }

        // Special key case
        _raw(_key(specialElements[0])) = _vacantGraveKey;
        _raw(_key(specialElements[1])) = _vacantVacantKey;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <bool move>
    inline void RawMap<K, V, H, KE, A>::_forwardData(std::conditional_t<move, RawMap, const RawMap> & other)
    {
        if constexpr (std::is_trivially_copyable_v<E>) {
            std::memcpy(_elements, other._elements, (_slotCount + 2u) * sizeof(E));
        }
        else {
            using ElementForwardType = std::conditional_t<move, E &&, const E &>;

            // General case
            std::conditional_t<move, E, const E> * srcElement{other._elements};
            const E * const srcEndElement{other._elements + _slotCount};
            E * dstElement{_elements};
            for (; srcElement < srcEndElement; ++srcElement, ++dstElement) {
                const _RawKey rawSrcKey{_raw(_key(*srcElement))};
                if (_isPresent(rawSrcKey)) {
                    std::allocator_traits<A>::construct(_alloc, dstElement, static_cast<ElementForwardType>(*srcElement));
                }
                else {
                    _raw(_key(*dstElement)) = rawSrcKey;
                }
            }

            // Special keys case
            if (_haveSpecial[0]) {
                std::allocator_traits<A>::construct(_alloc, _elements + _slotCount, static_cast<ElementForwardType>(other._elements[_slotCount]));
            }
            else {
                _raw(_key(_elements[_slotCount])) = _vacantGraveKey;
            }
            if (_haveSpecial[1]) {
                std::allocator_traits<A>::construct(_alloc, _elements + _slotCount + 1, static_cast<ElementForwardType>(other._elements[_slotCount + 1]));
            }
            else {
                _raw(_key(_elements[_slotCount + 1])) = _vacantVacantKey;
            }
        }
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <bool insertionForm, Comparable<H> K_>
    inline auto RawMap<K, V, H, KE, A>::_findKey(const K_ & key) const noexcept -> _FindKeyResult<insertionForm>
    {
        // Special key case
        if (const _RawKey rawKey{_raw(key)}; _isSpecial(rawKey)) [[unlikely]] {
            const uint8_t specialI{uint8_t(rawKey & 1u)};
            if constexpr (insertionForm) {
                return _FindKeyResult<insertionForm>{.element = _elements + _slotCount + specialI, .isPresent = _haveSpecial[specialI], .isSpecial = true, .specialI = specialI};
            }
            else {
                return _FindKeyResult<insertionForm>{.element = _elements + _slotCount + specialI, .isPresent = _haveSpecial[specialI]};
            }
        }

        // General case

        const E * const lastElement{_elements + _slotCount};

        E * element{_elements + _slot(key)};
        E * grave{};

        while (true) {
            const _RawKey rawKey{_raw(_key(*element))};

            if (rawKey == _raw(key)) {
                return {.element = element, .isPresent = true};
            }

            if (rawKey == _vacantKey) {
                if constexpr (insertionForm) {
                    return {.element = grave ? grave : element, .isPresent = false};
                }
                else {
                    return {.element = element, .isPresent = false};
                }
            }

            if constexpr (insertionForm) {
                if (rawKey == _graveKey) {
                    grave = element;
                }
            }

            ++element;
            if (element == lastElement) [[unlikely]] {
                element = _elements;
            }
        }
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    inline bool operator==(const RawMap<K, V, H, KE, A> & m1, const RawMap<K, V, H, KE, A> & m2)
    {
        if (m1.size() != m2.size()) {
            return false;
        }

        if (&m1 == &m2) {
            return true;
        }

        const auto endIt{m2.cend()};

        for (const auto & element : m1) {
            if constexpr (std::is_same_v<V, void>) {
                if (!m2.contains(element)) {
                    return false;
                }
            }
            else {
                const auto it{m2.find(element.first)};
                if (it == endIt || it->second != element.second) {
                    return false;
                }
            }
        }

        return true;
    }

    // Iterator ================================================================

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    template <bool constant_> requires (constant && !constant_)
    inline constexpr RawMap<K, V, H, KE, A>::_Iterator<constant>::_Iterator(const _Iterator<constant_> & other) noexcept:
        _element{other._element}
    {}

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline constexpr RawMap<K, V, H, KE, A>::_Iterator<constant>::_Iterator(E * const element) noexcept :
        _element{element}
    {}

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto RawMap<K, V, H, KE, A>::_Iterator<constant>::operator*() const noexcept -> E &
    {
        return *_element;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto RawMap<K, V, H, KE, A>::_Iterator<constant>::operator->() const noexcept -> E *
    {
        return _element;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto RawMap<K, V, H, KE, A>::_Iterator<constant>::operator++() noexcept -> _Iterator &
    {
        while (true) {
            ++_element;
            _RawKey rawKey{_raw(_key(*_element))};

            // General present case
            if (_isPresent(rawKey) && rawKey != _terminalKey) {
                return *this;
            }

            // We've made it to the special keys
            if (_raw(_key(_element[2])) == _terminalKey) [[unlikely]] {
                const int specialI{int(_raw(_key(_element[1])) == _terminalKey) + int(rawKey == _terminalKey)};
                switch (specialI) {
                    case 0:
                        if (rawKey == _graveKey) {
                            break;
                        }
                        else {
                            ++_element;
                            rawKey = _raw(_key(*_element));
                            [[fallthrough]];
                        }
                    case 1:
                        if (rawKey == _vacantKey) {
                            break;
                        }
                        else {
                            [[fallthrough]];
                        }
                    case 2:
                        _element = nullptr;
                }

                return *this;
            }
        }
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto RawMap<K, V, H, KE, A>::_Iterator<constant>::operator++(int) noexcept -> _Iterator
    {
        const _Iterator temp{*this};
        operator++();
        return temp;
    }

    template <Rawable K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    template <bool constant_>
    inline bool RawMap<K, V, H, KE, A>::_Iterator<constant>::operator==(const _Iterator<constant_> & it) const noexcept
    {
        return _element == it._element;
    }
}

namespace std
{
    template <typename K, typename V, typename H, typename KE, typename A>
    inline void swap(qc::hash::RawMap<K, V, H, KE, A> & a, qc::hash::RawMap<K, V, H, KE, A> & b) noexcept
    {
        a.swap(b);
    }
}
