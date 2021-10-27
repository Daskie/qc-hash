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
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <qc-hash/fasthash.hpp>

//
// Forward declaration of friend class used for testing
//
struct QcHashMapFriend;

namespace qc_hash_flat {

    using namespace qc_hash;

    using u8 = uint8_t;
    using u64 = uint64_t;

    namespace config {

        //
        // ...
        //
        // Must be at least 4
        //
        constexpr size_t minCapacity{16u};
        constexpr size_t minSlotCount{minCapacity * 2u};

        //
        // Must give good distribution of both low and high bits
        //
        template <typename K> using DefaultHash = fasthash::Hash<K>;

    }

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

    template <typename T1, typename T2> concept IsSame = std::is_same_v<T1, T2>;
    template <typename H, typename K> concept IsHashCallable = requires (H & hash, const K & key) { { hash(key) } -> IsSame<size_t>; };
    template <typename KE, typename K> concept IsKeyEqualCallable = requires (KE & keyEqual, const K & key1, const K & key2) { { keyEqual(key1, key2) } -> IsSame<bool>; };

    //
    // ...
    //
    template <typename K, typename V, typename H = config::DefaultHash<K>, typename KE = std::equal_to<K>, typename A = std::allocator<std::pair<K, V>>> class Map;

    //
    // ...
    // Defined as a `Map` whose mapped type is `void`.
    //
    template <typename K, typename H = config::DefaultHash<K>, typename KE = std::equal_to<K>, typename A = std::allocator<K>> using Set = Map<K, void, H, KE, A>;

    //
    // ...
    // The hash function provided MUST have good entropy in both the lower and upper bits.
    // Therefore, AN IDENTITY HASH MUST NOT BE USED!
    //
    template <typename K, typename V, typename H, typename KE, typename A> class Map {

        using E = typename _Element<K, V>::E;

        template <bool constant> class _Iterator;
        template <bool constant> friend class _Iterator;
        friend QcHashMapFriend;

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
        using pointer = E *;
        using const_pointer = const E *;
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
        static_assert(alignof(E) <= 8u, "Element types with alignment greater than 8 currently unsupported");

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

        static constexpr bool _isSet{std::is_same_v<V, void>};

        size_t _size;
        size_t _slotCount;
        u8 * _controls;
        H _hash;
        KE _equal;
        _Allocator _alloc;

        _Element * _elements() noexcept;
        const _Element * _elements() const noexcept;

        template <typename KTuple, typename VTuple, size_t... kIndices, size_t... vIndices> std::pair<iterator, bool> _emplace(KTuple && kTuple, VTuple && vTuple, std::index_sequence<kIndices...>, std::index_sequence<vIndices...>);

        template <typename K_, typename... VArgs> std::pair<iterator, bool> _try_emplace(K_ && key, VArgs &&... vArgs);

        template <bool zeroControls> void _clear() noexcept;

        //
        // ...
        // The second return value is zero if the key was found, or what the key's control byte would be otherwise
        //
        template <bool passGraves> std::pair<size_t, u8> _findKeyOrFirstNotPresent(const K & key, size_t hash) const;

        void _rehash(size_t slotCount);

        template <bool zeroControls> void _allocate();

        void _deallocate();

        void _zeroControls(size_t startBlockSlotI) noexcept;

        template <bool move> void _forwardData(std::conditional_t<move, Map, const Map> & other);

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

        std::conditional_t<constant, const u8, u8> * _control;
        std::conditional_t<constant, const Map::_Element, Map::_Element> * _element;

        constexpr _Iterator(decltype(_control) control, decltype(_element) element) noexcept;

    };

} // namespace qc_hash_flat

namespace std {

    template <typename K, typename V, typename H, typename KE, typename A> void swap(qc_hash_flat::Map<K, V, H, KE, A> & a, qc_hash_flat::Map<K, V, H, KE, A> & b) noexcept;

} // namespace std

// INLINE IMPLEMENTATION ///////////////////////////////////////////////////////////////////////////////////////////////

namespace qc_hash_flat {

    constexpr u8 _presentMask{0b10000000u};
    constexpr u64 _presentBlockMask{0b10000000'10000000'10000000'10000000'10000000'10000000'10000000'10000000u};
    constexpr u8 _graveControl{0b01111111u};

    inline size_t _firstPresentIndexInBlock(const u64 controlBlock) noexcept {
        if constexpr (std::endian::native == std::endian::little) {
            return std::countr_zero(controlBlock & _presentBlockMask) >> 3;
        }
        else {
            return std::countl_zero(controlBlock & _presentBlockMask) >> 3;
        }
    }

    // Map =====================================================================

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::Map(const size_t minCapacity, const H & hash, const KE & equal, const A & alloc) noexcept:
        _size{},
        _slotCount{minCapacity <= config::minCapacity ? config::minSlotCount : std::bit_ceil(minCapacity << 1)},
        _controls{},
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
        _size{other._size},
        _slotCount{other._slotCount},
        _controls{},
        _hash{other._hash},
        _equal{other._equal},
        _alloc{_AllocatorTraits::select_on_container_copy_construction(other._alloc)}
    {
        _allocate<false>();
        _forwardData<false>(other);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::Map(Map && other) noexcept :
        _size{std::exchange(other._size, 0u)},
        _slotCount{std::exchange(other._slotCount, config::minSlotCount)},
        _controls{std::exchange(other._controls, nullptr)},
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

        if (_controls) {
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
            _alloc = _AllocatorTraits::select_on_container_copy_construction(other._alloc);
        }

        if (other._controls) {
            if (!_controls) {
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

        if (_controls) {
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
            _controls = std::exchange(other._controls, nullptr);
        }
        else {
            _allocate<false>();
            _forwardData<true>(other);
            other._clear<false>();
            other._deallocate();
        }

        other._size = 0u;
        other._slotCount = config::minSlotCount;

        return *this;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline Map<K, V, H, KE, A>::~Map() noexcept {
        if (_controls) {
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
        if (!_controls) {
            _allocate<true>();
        }

        const size_t hash{_hash(key)};
        auto [slotI, keyControl]{_findKeyOrFirstNotPresent<false>(key, hash)};
        u8 * slotControl{_controls + slotI};
        _Element * slotElement{_elements() + slotI};

        // Element already exists
        if (!keyControl) {
            return {iterator{slotControl, slotElement}, false};
        }

        // Rehash if we're at capacity
        if (_size >= (_slotCount >> 1)) {
            _rehash(_slotCount << 1);
            std::tie(slotI, keyControl) = _findKeyOrFirstNotPresent<false>(key, hash);
            slotControl = _controls + slotI;
            slotElement = _elements() + slotI;
        }

        // Construct new element in place
        *slotControl = keyControl;
        _AllocatorTraits::construct(_alloc, &slotElement->key, std::forward<K_>(key));
        if constexpr (!_isSet) _AllocatorTraits::construct(_alloc, &slotElement->val, std::forward<VArgs>(vArgs)...);
        ++_size;

        return {iterator{slotControl, slotElement}, true};
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline bool Map<K, V, H, KE, A>::erase(const K & key) {
        if (!_size) {
            return false;
        }

        const auto [slotI, keyControl]{_findKeyOrFirstNotPresent<true>(key, _hash(key))};

        if (keyControl) {
            return false;
        }
        else {
            erase(iterator{_controls + slotI, _elements() + slotI});
            return true;
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::erase(const iterator position) {
        *position._control = _graveControl;
        if constexpr (!std::is_trivially_destructible_v<E>) {
            position._element->get().~E();
        }
        --_size;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::clear() noexcept {
        _clear<true>();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool zeroControls>
    inline void Map<K, V, H, KE, A>::_clear() noexcept {
        if constexpr (std::is_trivially_destructible_v<E>) {
            if constexpr (zeroControls) {
                if (_slotCount) {
                    _zeroControls(0u);
                }
            }
        }
        else {
            _Element * const elements{_elements()};

            size_t blockSlotI{0u};
            for (size_t n{0u}; n < _size; blockSlotI += 8u) {
                u8 * const blockControls{_controls + blockSlotI};
                u64 & controlBlock{reinterpret_cast<u64 &>(*blockControls)};

                if (zeroControls ? controlBlock : (controlBlock & _presentBlockMask)) {
                    _Element * const blockElements{elements + blockSlotI};

                    for (size_t innerI{0u}; innerI < 8u; ++innerI) {
                        if (blockControls[innerI] & _presentMask) {
                            blockElements[innerI].get().~E();
                            ++n;
                        }
                    }

                    if constexpr (zeroControls) {
                        controlBlock = 0u;
                    }
                }
            }

            // Quickly clear any additional graves
            if constexpr (zeroControls) {
                _zeroControls(blockSlotI);
            }
        }

        _size = 0u;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline bool Map<K, V, H, KE, A>::contains(const K & key) const {
        return _size ? !_findKeyOrFirstNotPresent<true>(key, _hash(key)).second : false;
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

        const auto [slotI, keyControl]{_findKeyOrFirstNotPresent<true>(key, _hash(key))};

        if (keyControl) {
            throw std::out_of_range{"Element not found"};
        }

        return _elements()[slotI].val;
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

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::begin() const noexcept -> const_iterator {
        if (!_size) {
            return end();
        }

        for (size_t blockSlotI{0u}; ; blockSlotI += 8u) {
            const u64 controlBlock{reinterpret_cast<const u64 &>(_controls[blockSlotI])};
            if (controlBlock) {
                const size_t slotI{blockSlotI + _firstPresentIndexInBlock(controlBlock)};
                return const_iterator{_controls + slotI, _elements() + slotI};
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
        return const_iterator{_controls + _slotCount, nullptr};
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::cend() const noexcept -> const_iterator {
        return end();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::find(const K & key) -> iterator {
        // Separated to placate IntelliSense
        const const_iterator cit{const_cast<const Map *>(this)->find(key)};
        return reinterpret_cast<const iterator &>(cit);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::find(const K & key) const -> const_iterator {
        if (!_size) {
            return cend();
        }

        const auto [slotI, keyControl]{_findKeyOrFirstNotPresent<true>(key, _hash(key))};
        return keyControl ? cend() : const_iterator{_controls + slotI, _elements() + slotI};
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool passGraves>
    inline auto Map<K, V, H, KE, A>::_findKeyOrFirstNotPresent(const K & key, const size_t hash) const -> std::pair<size_t, u8>{
        const size_t slotMask{_slotCount - 1u};
        const u8 keyControl{u8(_presentMask | (hash >> (std::numeric_limits<size_t>::digits - 7)))};
        const _Element * const elements{_elements()};

        for (size_t slotI{hash & slotMask}; ; slotI = (slotI + 1u) & slotMask) {
            const u8 & slotControl{_controls[slotI]};
            if (slotControl == keyControl) {
                if (_equal(elements[slotI].key, key)) {
                    return {slotI, u8(0u)};
                }
            }
            else if (passGraves ? !slotControl : !(slotControl & _presentMask)) {
                return {slotI, keyControl};
            }
        }
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
        return _hash(key) & (_slotCount - 1u);
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
            if (_controls) {
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
        u8 * const oldControls{_controls};
        _Element * const oldElements{_elements()};

        _size = 0u;
        _slotCount = slotCount;
        _allocate<true>();

        for (size_t blockSlotI{0u}, movedCount{0u}; movedCount < oldSize; blockSlotI += 8u) {
            const u8 * const blockControls{oldControls + blockSlotI};
            const u64 controlBlock{reinterpret_cast<const u64 &>(*blockControls)};

            if (controlBlock & _presentBlockMask) {
                _Element * const blockElements{oldElements + blockSlotI};

                for (size_t innerI{0u}; innerI < 8u; ++innerI) {
                    if (blockControls[innerI] & _presentMask) {
                        E & element{blockElements[innerI].get()};
                        emplace(std::move(element));
                        element.~E();
                        ++movedCount;
                    }
                }
            }
        }

        const size_t oldMemorySize{oldSlotCount + 8u + oldSlotCount * sizeof(_Element)};
        _AllocatorTraits::deallocate(_alloc, reinterpret_cast<u64 *>(oldControls), oldMemorySize >> 3);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::swap(Map & other) noexcept {
        std::swap(_size, other._size);
        std::swap(_slotCount, other._slotCount);
        std::swap(_controls, other._controls);
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
        return _size == 0u;
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
    inline auto Map<K, V, H, KE, A>::_elements() noexcept -> _Element * {
        return const_cast<_Element *>(const_cast<const Map *>(this)->_elements());
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline auto Map<K, V, H, KE, A>::_elements() const noexcept -> const _Element * {
        return reinterpret_cast<const _Element *>(_controls + _slotCount + 8u);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool zeroControls>
    inline void Map<K, V, H, KE, A>::_allocate() {
        const size_t memorySize{_slotCount + 8u + _slotCount * sizeof(_Element)};
        _controls = reinterpret_cast<u8 *>(_AllocatorTraits::allocate(_alloc, memorySize >> 3));
        if constexpr (zeroControls) {
            _zeroControls(0u);
        }

        // Set the trailing control block to all present so iterators can know when to stop without needing to know the size of container
        // (really only need the first control set, but I like this more)
        reinterpret_cast<u64 &>(_controls[_slotCount]) = ~u64(0u);
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::_deallocate() {
        const size_t memorySize{_slotCount + 8u + _slotCount * sizeof(_Element)};
        _AllocatorTraits::deallocate(_alloc, reinterpret_cast<u64 *>(_controls), memorySize >> 3);
        _controls = nullptr;
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void Map<K, V, H, KE, A>::_zeroControls(const size_t startBlockSlotI) noexcept {
        u64 * const startControlBlock{reinterpret_cast<u64 *>(_controls + startBlockSlotI)};
        const u64 * const endControlBlock{reinterpret_cast<const u64 *>(_controls + _slotCount)};
        for (u64 * controlBlock{startControlBlock}; controlBlock < endControlBlock; ++controlBlock) {
            // TODO: compare this with only setting if it's not already zero (to avoid writes to cache)
            *controlBlock = 0u;
        }
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool move>
    inline void Map<K, V, H, KE, A>::_forwardData(std::conditional_t<move, Map, const Map> & other) {
        const u8 * const srcControls{other._controls};
        if constexpr (std::is_trivially_copyable_v<E>) {
            const size_t blockCount{(_slotCount + 8u + sizeof(_Element) * _slotCount) >> 3};
            const u64 * srcBlock{reinterpret_cast<const u64 *>(srcControls)};
            const u64 * const srcEndBlock{srcBlock + blockCount};
            u64 * dstBlock{reinterpret_cast<u64 *>(_controls)};
            for (; srcBlock < srcEndBlock; ++srcBlock, ++dstBlock) {
                *dstBlock = *srcBlock;
            }
        }
        else {
            std::conditional_t<move, _Element, const _Element> * const srcElements{other._elements()};
            _Element * const dstElements{_elements()};
            for (size_t blockSlotI{0u}, n{0u}; n < _size; blockSlotI += 8u) {
                const u8 * const srcBlockControls{srcControls + blockSlotI};
                u8 * const dstBlockControls{_controls + blockSlotI};
                const u64 srcControlBlock{reinterpret_cast<const u64 &>(*srcBlockControls)};
                u64 & dstControlBlock{reinterpret_cast<u64 &>(*dstBlockControls)};

                if ((dstControlBlock = srcControlBlock) & _presentBlockMask) {
                    const _Element * const srcBlockElements{srcElements + blockSlotI};
                    _Element * const dstBlockElements{dstElements + blockSlotI};

                    for (size_t innerI{0u}; innerI < 8u; ++innerI) {
                        if (srcBlockControls[innerI] & _presentMask) {
                            if constexpr (move) {
                                _AllocatorTraits::construct(_alloc, &dstBlockElements[innerI].get(), std::move(srcBlockElements[innerI].get()));
                            }
                            else {
                                _AllocatorTraits::construct(_alloc, &dstBlockElements[innerI].get(), srcBlockElements[innerI].get());
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
        _control{other._control},
        _element{other._element}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline constexpr Map<K, V, H, KE, A>::_Iterator<constant>::_Iterator(const decltype(_control) control, const decltype(_element) element) noexcept :
        _control{control},
        _element{element}
    {}

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto Map<K, V, H, KE, A>::_Iterator<constant>::operator*() const noexcept -> E & {
        return _element->get();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto Map<K, V, H, KE, A>::_Iterator<constant>::operator->() const noexcept -> E * {
        return &_element->get();
    }

    template <typename K, typename V, typename H, typename KE, typename A>
    template <bool constant>
    inline auto Map<K, V, H, KE, A>::_Iterator<constant>::operator++() noexcept -> _Iterator & {
        const u8 * const origControl{_control};
        ++_control;

        size_t innerI{reinterpret_cast<const size_t &>(_control) & 7u};

        // Seek up to the next start of block
        if (innerI != 0u) {
            do {
                if (*_control & _presentMask) {
                    _element += _control - origControl;
                    return *this;
                }

                ++innerI;
                ++_control;
            } while (innerI < 8u);
        }

        const u64 * controlBlock{reinterpret_cast<const u64 *>(_control)};

        // Seek to next occupied block
        while (!(*controlBlock & _presentBlockMask)) {
            ++controlBlock;
        }

        // Seek to first present element within block
        innerI = _firstPresentIndexInBlock(*controlBlock);
        _control = const_cast<decltype(_control)>(reinterpret_cast<const u8 *>(controlBlock)) + innerI;
        _element += _control - origControl;

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
        return _control == it._control;
    }

} // namespace qc_hash_flat

namespace std {

    template <typename K, typename V, typename H, typename KE, typename A>
    inline void swap(qc_hash_flat::Map<K, V, H, KE, A> & a, qc_hash_flat::Map<K, V, H, KE, A> & b) noexcept {
        a.swap(b);
    }

} // namespace std
