//==============================================================================
// Map /////////////////////////////////////////////////////////////////////////
//==============================================================================
// Austin Quick, 2016 - 2018
//------------------------------------------------------------------------------
// Unordered node-based hash map implementation with an emphasis on performance.
// Currently over 10x faster than std::unordered_map using qmu hashing,
// and ~1.5x faster when using std::hash.
//------------------------------------------------------------------------------



#pragma once



#include <memory>

#include "Hash.hpp"



namespace qhm {



namespace config {

namespace map {

constexpr unat defNSlots = 16;      // number of slots when unspecified

}

}



//======================================================================================================================
// Map /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================
// Setup as a array of slots (buckets), each having a linked list (not
// std::list) of nodes, each containing a hash, a pointer to the next node, and
// an element value.
// Will always have a minimum of 1 slot, but may have 0 size.
// Memory for the number of slot's worth of nodes is pre-allocated. This is a
// huge performance boost with the cost of extra memory usage for un-full maps.
// t_p indicates the precision of the hash. 4 and 8 byte hash precision is
// supported.
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p = k_nat_p>
class Map {

    static_assert(t_p == 4 || t_p == 8, "unsupported precision");
    static_assert(std::is_copy_constructible_v<K>, "key type must be copy constructable");
    static_assert(std::is_move_constructible_v<E>, "element type must by move constructable");

    //--------------------------------------------------------------------------
    // Types

    public:

    using H = precision_ut<t_p>; // unsigned integral type of appropriate precision used for hash values
    using V = std::pair<K, E>;

    using key_type = K;
    using mapped_type = E;
    using value_type = V;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using size_type = unat;
    using difference_type = nat;

    //--------------------------------------------------------------------------
    // Constants

    public:

    static constexpr int precision = t_p;


    //==========================================================================
    // Node
    //--------------------------------------------------------------------------
    // Serves as a node for Slot as a basic linked list.
    //--------------------------------------------------------------------------

    private:

    struct Node {

        H hash;
        Node * next;
        V value;

        template <typename K_, typename... ElementArgs>
        Node(H hash, Node * next, K_ && key, ElementArgs &&... args);

    };



    //==========================================================================
    // Iterator
    //--------------------------------------------------------------------------
    // Used to iterate through the map. Comes in mutable and const varieties.
    //--------------------------------------------------------------------------

    private:

    template <bool t_const>
    class Iterator;

    public:

    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;



    //--------------------------------------------------------------------------
    // Instance Variables

    private:

    unat m_size;				        // total number of elements
    unat m_nSlots;						// number of slots
    std::unique_ptr<Node *[]> m_slots;  // the slots
    Node * m_nodeStore;                 // a supply of preallocated nodes (an optimization)
    bool m_fixed;						// the map will automatically adjust its number of slots
    bool m_rehashing;					// the map is currently rehashing



    //==========================================================================
    // Map
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    explicit Map(unat minNSlots = config::map::defNSlots, bool fixed = false);
    Map(const Map<K, E, t_p> & other);
    Map(Map<K, E, t_p> && other);
    template <typename InputIt>
    Map(InputIt first, InputIt last, bool fixed = false);
    explicit Map(std::initializer_list<V> values, bool fixed = false);



    //==========================================================================
    // ~Map
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    ~Map();



    //==========================================================================
    // operator=
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    Map & operator=(const Map<K, E, t_p> & other);
    Map & operator=(Map<K, E, t_p> && other);
    Map & operator=(std::initializer_list<V> pairs);



    //==========================================================================
    // swap
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    void swap(Map<K, E, t_p> & map);



    //==========================================================================
    // insert
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    std::pair<iterator, bool> insert(const K & key, const E & element);
    template <typename InputIt>
    void insert(InputIt first, InputIt last);
    void insert(std::initializer_list<V> pairs);

    std::pair<iterator, bool> insert_h(H hash, const K & key, const E & element);



    //==========================================================================
    // emplace
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    template <typename K_, typename... ElementArgs>
    std::pair<iterator, bool> emplace(K_ && key, ElementArgs &&... elementArgs);

    template <typename K_, typename... ElementArgs>
    std::pair<iterator, bool> emplace_h(H hash, K_ && key, ElementArgs &&... elementArgs);



    //==========================================================================
    // at
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    E & at(const K & key) const;

    E & at_h(H hash) const;



    //==========================================================================
    // begin
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    iterator begin();
    const_iterator cbegin() const;



    //==========================================================================
    // end
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    iterator end();
    const_iterator cend() const;



    //==========================================================================
    // find
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    iterator find(const K & key);
    const_iterator find(const K & key) const;
    const_iterator cfind(const K & key) const;

    iterator find_h(H hash);
    const_iterator find_h(H hash) const;
    const_iterator cfind_h(H hash) const;



    //==========================================================================
    // find_e
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    iterator find_e(const E & element);
    const_iterator find_e(const E & element) const;
    const_iterator cfind_e(const E & element) const;



    //==========================================================================
    // operator[]
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    E & operator[](const K & key);

    E & access_h(H hash, const K & key);



    //==========================================================================
    // erase
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    bool erase(const K & key);
    iterator erase(const_iterator position);
    iterator erase(const_iterator first, const_iterator last);

    bool erase_h(H hash);



    //==========================================================================
    // count
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    unat count(const K & key) const;

    unat count_h(H hash) const;



    //==========================================================================
    // rehash
    //--------------------------------------------------------------------------
    // Resizes the map so that there are at lease minNSlots slots.
    // All elements are re-organized.
    // Relatively expensive method.
    //--------------------------------------------------------------------------

    public:

    void rehash(unat minNSlots);



    //==========================================================================
    // reserve
    //--------------------------------------------------------------------------
    // Ensures at least nSlots are already allocated.
    //--------------------------------------------------------------------------

    public:

    void reserve(unat nSlots);



    //==========================================================================
    // clear
    //--------------------------------------------------------------------------
    // clears the map. all slots are cleared. when finished, size = 0
    //--------------------------------------------------------------------------

    public:

    void clear();



    //==========================================================================
    // operator==
    //--------------------------------------------------------------------------
    // Returns whether the elements of the two maps are the same
    //--------------------------------------------------------------------------

    public:

    bool operator==(const Map<K, E, t_p> & m) const;



    //==========================================================================
    // operator!=
    //--------------------------------------------------------------------------
    // Returns whether the elements of the two maps are different
    //--------------------------------------------------------------------------

    public:

    bool operator!=(const Map<K, E, t_p> & m) const;



    //--------------------------------------------------------------------------
    // Accessors

    public:

    unat size() const;

    bool empty() const;

    unat nSlots() const;
    unat bucket_count() const;

    unat slotSize(unat slotI) const;
    unat bucket_size(unat slotI) const;

    unat slot(const K & key) const;
    unat bucket(const K & key) const;

    bool fixed() const;
    void fixed(bool fixed);



    //--------------------------------------------------------------------------
    // Private Methods

    private:

    H detHash(const K & key) const;

    unat detSlotI(H hash) const;

};



//======================================================================================================================
// Iterator ////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================
// Basic iterator used to iterate forwards over the map.
// iterates forward over the slot, then moves to the next slot.
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
template <bool t_const> // may be E or const E
class Map<K, E, t_p>::Iterator {

    friend Map<K, E, t_p>;

    //--------------------------------------------------------------------------
    // Types

    using IE = std::conditional_t<t_const, const E, E>;
    using IV = std::conditional_t<t_const, const typename Map<K, E, t_p>::V, typename Map<K, E, t_p>::V>;

    using iterator_category = std::forward_iterator_tag;
    using value_type = IV;
    using difference_type = ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type &;

    //--------------------------------------------------------------------------
    // Instance Variables

    private:

    const Map<K, E, t_p> * m_map;
    unat m_slot;
    typename Map<K, E, t_p>::Node * m_node;



    //==========================================================================
    // Iterator
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    Iterator(const Map<K, E, t_p> & map);
    
    private:

    Iterator(const Map<K, E, t_p> & map, unat slot, typename Map<K, E, t_p>::Node * node);

    public:

    template <bool t_const_>
    Iterator(const Iterator<t_const_> & iterator);



    //==========================================================================
    // ~Iterator
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    ~Iterator() = default;



    //==========================================================================
    // operator=
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    template <bool t_const_>
    Iterator<t_const> & operator=(const Iterator<t_const_> & iterator);



    //==========================================================================
    // operator++
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    Iterator<t_const> & operator++();



    //==========================================================================
    // operator++ int
    //--------------------------------------------------------------------------
    // 

    //--------------------------------------------------------------------------

    Iterator<t_const> operator++(int);



    //==========================================================================
    // operator==
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    template <bool t_const_>
    bool operator==(const Iterator<t_const_> & it) const;



    //==========================================================================
    // operator!=
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    template <bool t_const_>
    bool operator!=(const Iterator<t_const_> & it) const;



    //==========================================================================
    // operator*
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    IV & operator*() const;



    //==========================================================================
    // operator->
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    IV * operator->() const;



    //==========================================================================
    // hash
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    typename Map<K, E, t_p>::H hash() const;



    //==========================================================================
    // key
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    const K & key() const;



    //==========================================================================
    // element
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    IE & element() const;

};



//======================================================================================================================
// Functions ///////////////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// swap
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
void swap(Map<K, E, t_p> & m1, Map<K, E, t_p> & m2);



//==============================================================================================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//==============================================================================================================================================================



//======================================================================================================================
// MAP IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// Node
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
template <typename K_, typename... ElementArgs>
Map<K, E, t_p>::Node::Node(H hash, Node * next, K_ && key, ElementArgs &&... elementArgs) :
    hash(hash),
    next(next),
    value(std::piecewise_construct, std::tuple<K_ &&>(std::forward<K_>(key)), std::tuple<ElementArgs &&...>(std::forward<ElementArgs>(elementArgs)...))
{}



//==============================================================================
// Map
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
Map<K, E, t_p>::Map(unat minNSlots, bool fixed) :
    m_size(0),
    m_nSlots(ceil2(max(minNSlots, 1_un))),
    m_slots(new Node *[m_nSlots]),
    m_nodeStore((Node *)std::malloc(m_nSlots * sizeof(Node))),
    m_fixed(fixed),
    m_rehashing(false)
{
    memset(m_slots.get(), 0, m_nSlots * sizeof(Node *));
}

template <typename K, typename E, int t_p>
Map<K, E, t_p>::Map(const Map<K, E, t_p> & map) :
    m_size(map.m_size),
    m_nSlots(map.m_nSlots),
    m_slots(new Node *[m_nSlots]),
    m_nodeStore((Node *)std::malloc(m_nSlots * sizeof(Node))),
    m_fixed(map.m_fixed),
    m_rehashing(false)
{
    for (unat i(0); i < m_nSlots; ++i) {
        m_slots[i] = map.m_slots[i];

        if (m_slots[i]) {
            m_slots[i] = new (m_nodeStore + i) Node(*m_slots[i]);
            Node ** node(&m_slots[i]->next);
            while (*node) {
                *node = new Node(**node);
                node = &(*node)->next;
            }
        }
    }
}

template <typename K, typename E, int t_p>
Map<K, E, t_p>::Map(Map<K, E, t_p> && map) :
    m_size(map.m_size),
    m_nSlots(map.m_nSlots),
    m_slots(std::move(map.m_slots)),
    m_nodeStore(map.m_nodeStore),
    m_fixed(map.m_fixed),
    m_rehashing(false)
{
    map.m_size = 0;
    map.m_nSlots = 0;
    map.m_nodeStore = nullptr;
}

template <typename K, typename E, int t_p>
template <typename InputIt>
Map<K, E, t_p>::Map(InputIt first, InputIt last, bool fixed) :
    m_size(0),
    m_nSlots(ceil2(std::distance(first, last))),
    m_slots(new Node *[m_nSlots]),
    m_nodeStore((Node *)std::malloc(m_nSlots * sizeof(Node))),
    m_fixed(fixed),
    m_rehashing(false)
{
    memset(m_slots.get(), 0, m_nSlots * sizeof(Node *));
    insert(first, last);
}

template <typename K, typename E, int t_p>
Map<K, E, t_p>::Map(std::initializer_list<V> pairs, bool fixed) :
    m_size(0),
    m_nSlots(ceil2(pairs.size())),
    m_slots(new Node *[m_nSlots]),
    m_nodeStore((Node *)std::malloc(m_nSlots * sizeof(Node))),
    m_fixed(fixed),
    m_rehashing(false)
{
    memset(m_slots.get(), 0, m_nSlots * sizeof(Node *));
    insert(pairs);
}



//==============================================================================
// ~Map
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
Map<K, E, t_p>::~Map() {
    clear();

    if (m_nodeStore) {
        std::free(m_nodeStore);
    }
}



//==============================================================================
// operatator=
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
Map<K, E, t_p> & Map<K, E, t_p>::operator=(const Map<K, E, t_p> & map) {
    return *this = std::move(Map<K, E, t_p>(map));
}

template <typename K, typename E, int t_p>
Map<K, E, t_p> & Map<K, E, t_p>::operator=(Map<K, E, t_p> && map) {
    if (&map == this) {
        return *this;
    }

    clear();
    std::free(m_nodeStore);

    m_size = map.m_size;
    m_nSlots = map.m_nSlots;
    m_slots = std::move(map.m_slots);
    m_nodeStore = map.m_nodeStore;

    map.m_size = 0;
    map.m_nSlots = 0;
    map.m_nodeStore = nullptr;

    return *this;
}

template <typename K, typename E, int t_p>
Map<K, E, t_p> & Map<K, E, t_p>::operator=(std::initializer_list<V> pairs) {
    return *this = std::move(Map<K, E, t_p>(pairs));
}



//==============================================================================
// swap
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
void Map<K, E, t_p>::swap(Map<K, E, t_p> & map) {
    std::swap(m_size, map.m_size);
    std::swap(m_nSlots, map.m_nSlots);
    std::swap(m_slots, map.m_slots);
    std::swap(m_nodeStore, map.m_nodeStore);
    std::swap(m_fixed, map.m_fixed);
}



//==============================================================================
// insert
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
std::pair<typename Map<K, E, t_p>::iterator, bool> Map<K, E, t_p>::insert(const K & key, const E & element) {
    return insert_h(detHash(key), key, element);
}

template <typename K, typename E, int t_p>
template <typename InputIt>
void Map<K, E, t_p>::insert(InputIt first, InputIt last) {
    while (first != last) {
        insert_h(detHash(first->first), first->first, first->second);
        ++first;
    }
}

template <typename K, typename E, int t_p>
void Map<K, E, t_p>::insert(std::initializer_list<V> pairs) {
    for (const auto & pair : pairs) {
        insert_h(detHash(pair.first), pair.first, pair.second);
    }
}



//==============================================================================
// insert_h
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
std::pair<typename Map<K, E, t_p>::iterator, bool> Map<K, E, t_p>::insert_h(H hash, const K & key, const E & element) {
    return emplace_h(hash, key, element);
}



//==============================================================================
// emplace
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
template <typename K_, typename... ElementArgs>
std::pair<typename Map<K, E, t_p>::iterator, bool> Map<K, E, t_p>::emplace(K_ && key, ElementArgs &&... elementArgs) {
    return emplace_h(detHash(key), std::forward<K_>(key), std::forward<ElementArgs>(elementArgs)...);
}



//==============================================================================
// emplace_h
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
template <typename K_, typename... ElementArgs>
std::pair<typename Map<K, E, t_p>::iterator, bool> Map<K, E, t_p>::emplace_h(H hash, K_ && key, ElementArgs &&... elementArgs) {
    if (!m_fixed && m_size >= m_nSlots) {
        rehash(m_nSlots * 2);
    }

    unat slotI(detSlotI(hash));

    Node ** node(&m_slots[slotI]);
    if (*node) {
        while (*node && (*node)->hash < hash) {
            node = &(*node)->next;
        }
        if (*node && (*node)->hash == hash) {
            return { iterator(*this, slotI, *node), false };
        }
        *node = new Node(hash, *node, std::forward<K_>(key), std::forward<ElementArgs>(elementArgs)...);
    }
    else {
        *node = new (m_nodeStore + slotI) Node(hash, nullptr, std::forward<K_>(key), std::forward<ElementArgs>(elementArgs)...);
    }

    ++m_size;
    return { iterator(*this, slotI, *node), true };
}



//==============================================================================
// at
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
E & Map<K, E, t_p>::at(const K & key) const {
    return at_h(detHash(key));
}



//==============================================================================
// at_h
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
E & Map<K, E, t_p>::at_h(H hash) const {
    unat slotI(detSlotI(hash));

    Node * node(m_slots[slotI]);
    while (node && node->hash < hash) {
        node = node->next;
    }
    if (node && node->hash == hash) {
        return node->value.second;
    }

    throw std::out_of_range("key not found");
}



//==============================================================================
// operator[]
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
E & Map<K, E, t_p>::operator[](const K & key) {
    return access_h(detHash(key), key);
}



//==============================================================================
// access_h
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
E & Map<K, E, t_p>::access_h(H hash, const K & key) {
    return emplace_h(hash, key).first->second;
}



//==============================================================================
// erase
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
bool Map<K, E, t_p>::erase(const K & key) {
    return erase_h(detHash(key));
}

template <typename K, typename E, int t_p>
typename Map<K, E, t_p>::iterator Map<K, E, t_p>::erase(const_iterator position) {
    if (position == cend()) {
        return position;
    }

    iterator next(position); ++next;
    
    return erase_h(position.hash()) ? next : position;
}

template <typename K, typename E, int t_p>
typename Map<K, E, t_p>::iterator Map<K, E, t_p>::erase(const_iterator first, const_iterator last) {
    while (first != last) {
        if (!erase_h((first++).hash())) {
            break;
        }
    }

    return first;
}



//==============================================================================
// erase_h
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
bool Map<K, E, t_p>::erase_h(H hash) {
    unat slotI(detSlotI(hash));

    Node ** node(&m_slots[slotI]);
    while (*node && (*node)->hash < hash) {
        node = &(*node)->next;
    }
    if (!*node || (*node)->hash != hash) {
        return false;
    }

    Node * next((*node)->next);
    if (*node < m_nodeStore || *node >= m_nodeStore + m_nSlots) {
        delete *node;
    }
    *node = next;

    --m_size;
    return true;
}



//==============================================================================
// count
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
unat Map<K, E, t_p>::count(const K & key) const {
    return count_h(detHash(key));
}



//==============================================================================
// count_h
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
unat Map<K, E, t_p>::count_h(H hash) const {
    unat slotI(detSlotI(hash));

    Node * node = m_slots[slotI];
    while (node && node->hash < hash) {
        node = node->next;
    }
    if (node && node->hash == hash) {
        return 1;
    }

    return 0;
}



//==============================================================================
// rehash
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
void Map<K, E, t_p>::rehash(unat minNSlots) {
    if (m_rehashing) {
        return;
    }

    Map<K, E, t_p> map(minNSlots, m_fixed);
    m_rehashing = true;
    map.m_rehashing = true;

    for (unat i(0); i < m_nSlots; ++i) {
        Node * node = m_slots[i]; 
        while (node) {
            map.emplace_h(node->hash, std::move(node->value.first), std::move(node->value.second));
            node = node->next;
        }
    }

    map.m_rehashing = false;
    m_rehashing = false;
    *this = std::move(map);
}



//==============================================================================
// reserve
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
void Map<K, E, t_p>::reserve(unat nSlots) {
    if (nSlots <= m_nSlots) {
        return;
    }

    rehash(nSlots);
}



//==============================================================================
// clear
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
void Map<K, E, t_p>::clear() {
    Node * storeStart(m_nodeStore), * storeEnd(m_nodeStore + m_nSlots);
    for (unat i = 0; i < m_nSlots; ++i) {
        Node * node(m_slots[i]), * next;
        while (node) {
            next = node->next;
            if (node < storeStart || node >= storeEnd) delete node;
            node = next;
        }

        m_slots[i] = nullptr;
    }

    m_size = 0;
}



//==============================================================================
// operator==
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
bool Map<K, E, t_p>::operator==(const Map<K, E, t_p> & map) const {
    if (&map == this) {
        return true;
    }

    if (m_size != map.m_size) {
        return false;
    }

    const_iterator it1(cbegin()), it2(map.cbegin());
    for (; it1 != cend() && it2 != map.cend(); ++it1, ++it2) {
        if (*it1 != *it2) {
            return false;
        }
    }
    return it1 == cend() && it2 == map.cend();
}



//==============================================================================
// operator!=
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
bool Map<K, E, t_p>::operator!=(const Map<K, E, t_p> & map) const {
    if (&map == this) {
        return false;
    }

    if (m_size != map.m_size) {
        return true;
    }

    const_iterator it1(cbegin()), it2(map.cbegin());
    for (; it1 != cend() && it2 != map.cend(); ++it1, ++it2) {
        if (*it1 == *it2) {
            return false;
        }
    }
    return it1 != cend() || it2 != map.cend();
}



//==============================================================================
// begin
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
typename Map<K, E, t_p>::iterator Map<K, E, t_p>::begin() {
    return iterator(*this);
}

template <typename K, typename E, int t_p>
typename Map<K, E, t_p>::const_iterator Map<K, E, t_p>::cbegin() const {
    return const_iterator(*this);
}



//==============================================================================
// end
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
typename Map<K, E, t_p>::iterator Map<K, E, t_p>::end() {
    return iterator(*this, m_nSlots, nullptr);
}

template <typename K, typename E, int t_p>
typename Map<K, E, t_p>::const_iterator Map<K, E, t_p>::cend() const {
    return const_iterator(*this, m_nSlots, nullptr);
}



//==============================================================================
// find
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
typename Map<K, E, t_p>::iterator Map<K, E, t_p>::find(const K & key) {
    return cfind(key);
}

template <typename K, typename E, int t_p>
typename Map<K, E, t_p>::const_iterator Map<K, E, t_p>::find(const K & key) const {
    return cfind(key);
}

template <typename K, typename E, int t_p>
typename Map<K, E, t_p>::const_iterator Map<K, E, t_p>::cfind(const K & key) const {
    return find_h(detHash(key));
}



//==============================================================================
// find_h
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
typename Map<K, E, t_p>::iterator Map<K, E, t_p>::find_h(H hash) {
    return cfind_h(hash);
}

template <typename K, typename E, int t_p>
typename Map<K, E, t_p>::const_iterator Map<K, E, t_p>::find_h(H hash) const {
    return cfind_h(hash);
}

template <typename K, typename E, int t_p>
typename Map<K, E, t_p>::const_iterator Map<K, E, t_p>::cfind_h(H hash) const {
    unat slotI(detSlotI(hash));

    Node * node(m_slots[slotI]);
    while (node && node->hash < hash) {
        node = node->next;
    }
    if (node && node->hash == hash) {
        return const_iterator(*this, slotI, node);
    }

    return cend();
}



//==============================================================================
// find_e
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
typename Map<K, E, t_p>::iterator Map<K, E, t_p>::find_e(const E & element) {
    return cfind_e(element);
}

template <typename K, typename E, int t_p>
typename Map<K, E, t_p>::const_iterator Map<K, E, t_p>::find_e(const E & element) const {
    return cfind_e(element);
}

template <typename K, typename E, int t_p>
typename Map<K, E, t_p>::const_iterator Map<K, E, t_p>::cfind_e(const E & element) const {
    for (const_iterator it(cbegin()); it != cend(); ++it) {
        if (it->second == element) {
            return it;
        }
    }

    return cend();
}



//==============================================================================
// size
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
unat Map<K, E, t_p>::size() const {
    return m_size;
}



//==============================================================================
// empty
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
bool Map<K, E, t_p>::empty() const {
    return m_size == 0;
}

//==============================================================================
// nSlots
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
unat Map<K, E, t_p>::nSlots() const {
    return m_nSlots;
}

template <typename K, typename E, int t_p>
unat Map<K, E, t_p>::bucket_count() const {
    return nSlots();
}

//==============================================================================
// slotSize
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
unat Map<K, E, t_p>::slotSize(unat slotI) const {
    if (slotI < 0 || slotI >= m_nSlots) {
        return 0;
    }

    unat size(0);

    for (Node * node(m_slots[slotI]); node; node = node->next) {
        ++size;
    }
    
    return size;
}

template <typename K, typename E, int t_p>
unat Map<K, E, t_p>::bucket_size(unat slotI) const {
    return slotSize(slotI);
}

//==============================================================================
// slot
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
unat Map<K, E, t_p>::slot(const K & key) const {
    return detSlotI(detHash(key));
}

template <typename K, typename E, int t_p>
unat Map<K, E, t_p>::bucket(const K & key) const {
    return slot(key);
}







//==============================================================================
// fixed
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
bool Map<K, E, t_p>::fixed() const {
    return m_fixed;
}

template <typename K, typename E, int t_p>
void Map<K, E, t_p>::fixed(bool fixed) {
    m_fixed = fixed;
}



//------------------------------------------------------------------------------
// Private Methods

template <typename K, typename E, int t_p>
inline typename Map<K, E, t_p>::H Map<K, E, t_p>::detHash(const K & key) const {
    return hash<t_p>(key);
}

template <typename K, typename E, int t_p>
inline unat Map<K, E, t_p>::detSlotI(H hash) const {
    return hash & (m_nSlots - 1);
}



//======================================================================================================================
// Iterator Implementation /////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// Iterator
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
template <bool t_const>
Map<K, E, t_p>::Iterator<t_const>::Iterator(const Map<K, E, t_p> & map) :
    m_map(&map),
    m_slot(0),
    m_node(nullptr)
{
    while (!m_map->m_slots[m_slot]) ++m_slot;
    if (m_slot < m_map->m_nSlots) m_node = m_map->m_slots[m_slot];
}

template <typename K, typename E, int t_p>
template <bool t_const>
Map<K, E, t_p>::Iterator<t_const>::Iterator(const Map<K, E, t_p> & map, unat slot, typename Map<K, E, t_p>::Node * node) :
    m_map(&map),
    m_slot(slot),
    m_node(node)
{}

template <typename K, typename E, int t_p>
template <bool t_const>
template <bool t_const_>
Map<K, E, t_p>::Iterator<t_const>::Iterator(typename const Map<K, E, t_p>::Iterator<t_const_> & iterator) :
    m_map(iterator.m_map),
    m_slot(iterator.m_slot),
    m_node(iterator.m_node)
{}



//==============================================================================
// operator=
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
template <bool t_const>
template <bool t_const_>
typename Map<K, E, t_p>::Iterator<t_const> & Map<K, E, t_p>::Iterator<t_const>::operator=(typename const Map<K, E, t_p>::Iterator<t_const_> & iterator) {
    m_map = iterator.m_map;
    m_slot = iterator.m_slot;
    m_node = iterator.m_node;
    return *this;
}



//==============================================================================
// operator++
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
template <bool t_const>
typename Map<K, E, t_p>::Iterator<t_const> & Map<K, E, t_p>::Iterator<t_const>::operator++() {
    m_node = m_node->next;
    if (!m_node) {
        while (++m_slot < m_map->m_nSlots) {
            if (m_node = m_map->m_slots[m_slot]) break;
        }
    }
    return *this;
}



//==============================================================================
// operator++ int
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
template <bool t_const>
typename Map<K, E, t_p>::Iterator<t_const> Map<K, E, t_p>::Iterator<t_const>::operator++(int) {
    Map<K, E, t_p>::Iterator<t_const> temp(*this);
    operator++();
    return temp;
}



//==============================================================================
// operator==
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
template <bool t_const>
template <bool t_const_>
bool Map<K, E, t_p>::Iterator<t_const>::operator==(typename const Map<K, E, t_p>::Iterator<t_const_> & o) const {
    return m_node == o.m_node;
}



//==============================================================================
// operator!=
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
template <bool t_const>
template <bool t_const_>
bool Map<K, E, t_p>::Iterator<t_const>::operator!=(typename const Map<K, E, t_p>::Iterator<t_const_> & o) const {
    return m_node != o.m_node;
}



//==============================================================================
// operator*
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
template <bool t_const>
typename Map<K, E, t_p>::Iterator<t_const>::reference Map<K, E, t_p>::Iterator<t_const>::operator*() const {
    return m_node->value;
}



//==============================================================================
// operator->
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
template <bool t_const>
typename Map<K, E, t_p>::Iterator<t_const>::pointer Map<K, E, t_p>::Iterator<t_const>::operator->() const {
    return &m_node->value;
}



//==============================================================================
// hash
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
template <bool t_const>
typename Map<K, E, t_p>::H Map<K, E, t_p>::Iterator<t_const>::hash() const {
    return m_node->hash;
}



//==============================================================================
// key
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
template <bool t_const>
const K & Map<K, E, t_p>::Iterator<t_const>::key() const {
    return m_node->value.first;
}



//==============================================================================
// element
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
template <bool t_const>
typename Map<K, E, t_p>::Iterator<t_const>::IE & Map<K, E, t_p>::Iterator<t_const>::element() const {
    return m_node->value.second;
}



//======================================================================================================================
// Functions Implementation ////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// swap
//------------------------------------------------------------------------------

template <typename K, typename E, int t_p>
inline void swap(Map<K, E, t_p> & m1, Map<K, E, t_p> & m2) {
    m1.swap(m2);
}



}