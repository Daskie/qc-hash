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

        constexpr unat defInitCapacity(16);

    }

}



// Set
//==============================================================================
// ...

template <typename V, typename H = Hash<V>, typename E = std::equal_to<V>>
class Set {

    static_assert(std::is_move_constructible_v<V>, "Value type must be move constructable");
    static_assert(std::is_move_assignable_v<V>, "Value type must be move assignable");

    ////////////////////////////////////////////////////////////////////////////

    public:

    // Types ===================================================================

    template <bool t_const> class Iterator;

    using key_type = V;
    using value_type = V;
    using hasher = H;
    using key_equal = E;
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

    explicit Set(unat minCapacity = config::set::defInitCapacity);
    Set(const Set<V, H, E> & other);
    Set(Set<V, H, E> && other);
    template <typename It> Set(It first, It last);
    explicit Set(std::initializer_list<V> values);

    // ~Set
    //--------------------------------------------------------------------------
    // 

    ~Set();

    // operator=
    //--------------------------------------------------------------------------
    // 

    Set & operator=(const Set<V, H, E> & other);
    Set & operator=(Set<V, H, E> && other);
    Set & operator=(std::initializer_list<V> values);

    // begin
    //--------------------------------------------------------------------------
    // 

    iterator begin();
    const_iterator begin() const;
    const_iterator cbegin() const;

    // end
    //--------------------------------------------------------------------------
    // 

    iterator end();
    const_iterator end() const;
    const_iterator cend() const;
    
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

    bool empty() const;
    
    // size
    //--------------------------------------------------------------------------
    // 

    unat size() const;
    
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
    template <typename InputIt> void insert(InputIt first, InputIt last);
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
    iterator erase(const_iterator position);

    // swap
    //--------------------------------------------------------------------------
    // ...
    // Invalidates iterators

    void swap(Set<V, H, E> & other);

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
    // Ensures capacity is at least `minCapacity`
    // If capacity is changed the set is regenerated making this a relatively
    // expensive method
    // Invalidates iterators

    void rehash(unat minCapacity);

    // reserve
    //--------------------------------------------------------------------------
    // Ensures capacity is ideal to hold <n> elements
    // Equivalent to `rehash(2 * n)`
    // If capacity is changed the set is regenerated making this a relatively
    // expensive method
    // Invalidates iterators

    void reserve(unat n);

    // hash_function
    //--------------------------------------------------------------------------
    // 

    hasher hash_function() const;

    // key_eq
    //--------------------------------------------------------------------------
    // 

    key_equal key_eq() const;
    
    // Functions ===============================================================

    // operator==
    //--------------------------------------------------------------------------
    // Returns whether `s1` and `s2` have the same values

    template <typename V, typename H, typename E> friend bool operator==(const Set<V, H, E> & s1, const Set<V, H, E> & s2);

    // operator==
    //--------------------------------------------------------------------------
    // Returns whether `s1` and `s2` have different values

    template <typename V, typename H, typename E> friend bool operator!=(const Set<V, H, E> & s1, const Set<V, H, E> & s2);

    // swap
    //--------------------------------------------------------------------------
    // 

    friend void swap(Set<V, H, E> & s1, Set<V, H, E> & s2);

    ////////////////////////////////////////////////////////////////////////////

    private:

    // Types ===================================================================

    struct PrivateTag {};

    struct Chunk {
        union {
            uchar dists[8]; // 0 means no element and distance starts at 1
            uint64_t distsVal;
        };
        V vals[8];
    };

    using ChunkIndex = std::pair<unat, unat>;

    // Variables ===============================================================

    unat m_size;
    unat m_capacity;
    Chunk * m_chunks;

    // Methods =================================================================

    Set(PrivateTag, unat capacity);

    std::pair<iterator, bool> emplace_h(V && value, unat hash);

    void propagate(V & value, ChunkIndex ci, uchar dist);

    unat detIndex(unat hash) const {
        return hash & (m_capacity - 1);
    }
    unat detIndex(const V & value) const {
        return detIndex(H()(value));
    }

    void rehash(PrivateTag, unat capacity);

    void allocate();

    void copyChunks(const Chunk * chunks);

    V & valAt(const ChunkIndex & ci) { return m_chunks[ci.first].vals[ci.second]; }
    const V & valAt(const ChunkIndex & ci) const { return m_chunks[ci.first].vals[ci.second]; }
    uchar & distAt(const ChunkIndex & ci) { return m_chunks[ci.first].dists[ci.second]; }
    const uchar & distAt(const ChunkIndex & ci) const { return m_chunks[ci.first].dists[ci.second]; }

    void increment(ChunkIndex & ci) const {
        // TODO: compare
        if (++ci.second >= 8) {
            ci.second = 0;
            if (++ci.first >= (m_capacity >> 3)) {
                ci.first = 0;
            }
        }

        //subI = (subI + 1) & 0b111;
        //chunkI += !subI;    
    }

    ChunkIndex toChunkIndex(unat i) const {
        return { i >> 3, i & 7 };
    }

    template <bool t_const>
    ChunkIndex toChunkIndex(Iterator<t_const> it) const {
        unat pos(it.m_dist.addr - reinterpret_cast<uintptr_t>(m_chunks));
        return { pos / sizeof(Chunk), pos & 7 };
    }

};



// Iterator
//==============================================================================
// Forward iterator

template <typename V, typename H, typename E>
template <bool t_const>
class Set<V, H, E>::Iterator {

    friend Set<V, H, E>;

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

    template <bool t_const_> Iterator(const Iterator<t_const_> & other);

    // ~Iterator
    //--------------------------------------------------------------------------
    // 

    ~Iterator() = default;

    // operator=
    //--------------------------------------------------------------------------
    // 

    template <bool t_const_> Iterator<t_const> & operator=(const Iterator<t_const_> & other);

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

    // Variables ===============================================================

    union {
        const uchar * ptr;
        uintptr_t addr;
    } m_dist;

    // Methods =================================================================

    Iterator(const uchar * distPtr);
    Iterator(uintptr_t distAddr) : Iterator(reinterpret_cast<const uchar *>(distAddr)) {}

};



}



#include "Set.tpp"