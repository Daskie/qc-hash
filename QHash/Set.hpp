//==============================================================================
// Set.hpp /////////////////////////////////////////////////////////////////////
//==============================================================================
// Austin Quick, 2016 - 2019
//------------------------------------------------------------------------------
// https://github.com/Daskie/QHash
// ...
//------------------------------------------------------------------------------



#pragma once



#include <memory>

#include "Hash.hpp"



namespace qc {



using uchar = unsigned char;



namespace config {

    namespace set {

        constexpr unat minCapacity(16);
        constexpr unat minBucketCount(minCapacity * 2);

    }

}



// Set
//==============================================================================
// ...

template <typename V, typename H = Hash<V>, typename E = std::equal_to<V>, typename A = std::allocator<V>>
class Set {

    static_assert(std::is_move_constructible_v<V>, "Value type must be move constructable");
    static_assert(std::is_move_assignable_v<V>, "Value type must be move assignable");

    ////////////////////////////////////////////////////////////////////////////

    public:

    // Types ===================================================================

    template <bool t_const> class Iterator;
    friend Iterator;

    using key_type = V;
    using value_type = V;
    using hasher = H;
    using key_equal = E;
    using allocator_type = A;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using unatype = unat;
    using difference_type = ptrdiff_t;

    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;

    // Methods =================================================================

    // Set
    //--------------------------------------------------------------------------
    //

    explicit Set(unat minCapacity = config::set::minCapacity, const H & hash = H(), const E & equal = E(), const A & alloc = A());
    Set(unat minCapacity, const A & alloc);
    Set(unat minCapacity, const H & hash, const A & alloc);
    explicit Set(const A & alloc);
    Set(const Set & other);
    Set(const Set & other, const A & alloc);
    Set(Set && other);
    Set(Set && other, const A & alloc);
    Set(Set && other, A && alloc);
    template <typename It> Set(It first, It last, unat minCapacity = 0, const H & hash = H(), const E & equal = E(), const A & alloc = A());
    template <typename It> Set(It first, It last, unat minCapacity, const A & alloc);
    template <typename It> Set(It first, It last, unat minCapacity, const H & hash, const A & alloc);
    Set(std::initializer_list<V> values, unat minCapacity = 0, const H & hash = H(), const E & equal = E(), const A & alloc = A());
    Set(std::initializer_list<V> values, unat minCapacity, const A & alloc);
    Set(std::initializer_list<V> values, unat minCapacity, const H & hash, const A & alloc);

    // ~Set
    //--------------------------------------------------------------------------
    // 

    ~Set();

    // operator=
    //--------------------------------------------------------------------------
    //

    Set & operator=(const Set & other);
    Set & operator=(Set && other) noexcept;
    Set & operator=(std::initializer_list<V> values);

    // begin
    //--------------------------------------------------------------------------
    // 

    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    const_iterator cbegin() const noexcept;

    // end
    //--------------------------------------------------------------------------
    // 

    iterator end() noexcept;
    const_iterator end() const noexcept;
    const_iterator cend() const noexcept;

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

    unat bucket(const V & value) const;
    
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
    template <typename It> void insert(It first, It last);
    void insert(std::initializer_list<V> values);

    // emplace
    //--------------------------------------------------------------------------
    // ...
    // Invalidates iterators

    template <typename... Args> std::pair<iterator, bool> emplace(Args &&... args);
    std::pair<iterator, bool> emplace(V && value);

    // emplace_hint
    //--------------------------------------------------------------------------
    // Identical to emplace
    // Invalidates iterators

    template <typename... Args> std::pair<iterator, bool> emplace_hint(const_iterator hint, Args &&... args);

    // erase
    //--------------------------------------------------------------------------
    // ...
    // Invalidates iterators

    unat erase(const V & value);
    // Always returns end iterator as erase can trigger a rehash
    iterator erase(const_iterator position);
    // Always returns end iterator as erase can trigger a rehash
    iterator erase(const_iterator first, const_iterator last);

    // swap
    //--------------------------------------------------------------------------
    // ...
    // Invalidates iterators

    void swap(Set & other) noexcept;

    // count
    //--------------------------------------------------------------------------
    // 

    unat count(const V & value) const;

    // find
    //--------------------------------------------------------------------------
    // 

    iterator find(const V & value);
    const_iterator find(const V & value) const;
    const_iterator cfind(const V & value) const;

    // contains
    //--------------------------------------------------------------------------
    // 

    bool contains(const V & value) const;

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

    template <typename V, typename H, typename E, typename A> friend bool operator==(const Set<V, H, E, A> & s1, const Set<V, H, E, A> & s2);

    // operator==
    //--------------------------------------------------------------------------
    // Returns whether `s1` and `s2` have different values

    template <typename V, typename H, typename E, typename A> friend bool operator!=(const Set<V, H, E, A> & s1, const Set<V, H, E, A> & s2);

    // swap
    //--------------------------------------------------------------------------
    // 

    friend void swap(Set & s1, Set & s2) noexcept;

    ////////////////////////////////////////////////////////////////////////////

    private:

    // Types ===================================================================

    using Dist = detail::hash::utype<alignof(V)>;

    struct Bucket {
        V val;
        Dist dist;
    };

    using Allocator = typename std::allocator_traits<A>::template rebind_alloc<Bucket>;
    using AllocatorTraits = std::allocator_traits<Allocator>;

    // Variables ===============================================================

    unat m_size;
    unat m_bucketCount;
    Bucket * m_buckets;
    H m_hash;
    E m_equal;
    Allocator m_alloc;

    // Methods =================================================================

    std::pair<iterator, bool> emplace_private(V && value, unat hash);

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

    template <bool t_move = false> void copyBuckets(const Bucket * bucket);

};



// Iterator
//==============================================================================
// Forward iterator

template <typename V, typename H, typename E, typename A>
template <bool t_const>
class Set<V, H, E, A>::Iterator {

    friend Set;

    ////////////////////////////////////////////////////////////////////////////

    public:

    // Types ===================================================================

    using iterator_category = std::forward_iterator_tag;
    using value_type = std::conditional_t<t_const, const V, V>;
    using difference_type = ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

    // Iterator
    //--------------------------------------------------------------------------
    // 

    Iterator(const Iterator & other) noexcept = default;
    Iterator(const Iterator<!t_const> & other) noexcept;

    // ~Iterator
    //--------------------------------------------------------------------------
    // 

    ~Iterator() = default;

    // operator=
    //--------------------------------------------------------------------------
    // 

    Iterator & operator=(const Iterator & other) noexcept = default;
    Iterator & operator=(const Iterator<!t_const> & other) noexcept;

    // operator++
    //--------------------------------------------------------------------------
    // 

    Iterator<t_const> & operator++();

    // operator++ int
    //--------------------------------------------------------------------------
    // 

    Iterator<t_const> operator++(int);

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

    const V & operator*() const;

    // operator->
    //--------------------------------------------------------------------------
    // 

    const V * operator->() const;

    ////////////////////////////////////////////////////////////////////////////

    private:

    // Types ===================================================================

    using Bucket = Set::Bucket;
    using Dist = Set::Dist;

    // Variables ===============================================================

    const Bucket * m_bucket;

    // Methods =================================================================

    Iterator(const Bucket * bucket) noexcept;

};



}



#include "Set.tpp"