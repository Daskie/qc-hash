//==============================================================================
// Map /////////////////////////////////////////////////////////////////////////
//==============================================================================
// Austin Quick, 2016 - 2017
//------------------------------------------------------------------------------
// Unordered node-based hash map implementation with an emphasis on performance.
// Currently over 10x faster than std::unordered_map using qmu hashing,
// and ~1.5x faster when using std::hash.
//------------------------------------------------------------------------------



#pragma once



#include <memory>

#include "Hash.hpp"



namespace qmu {



namespace config {

namespace map {

constexpr nat defNSlots = 16;      // number of slots when unspecified

constexpr bool useStdHash = false; // toggles use of std::hash

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

template <typename K, typename E, nat t_p = k_nat_p>
class Map {

    static_assert(t_p == 4 || t_p == 8, "unsupported precision");

    //--------------------------------------------------------------------------
    // Types

    public:

    using H = precision_ut<t_p>; // unsigned integral type of appropriate precision used for hash values

    using key_type = K;
    using mapped_type = E;

    //--------------------------------------------------------------------------
    // Constants

    public:

    static constexpr nat precision = t_p;


    //==========================================================================
    // Node
    //--------------------------------------------------------------------------
    // Serves as a node for Slot as a basic linked list.
    //--------------------------------------------------------------------------

    private:

    struct Node {

        H hash;
        Node * next;
        E element;

        template <typename... ElementArgs>
        Node(H hash, Node * next, ElementArgs &&... args);

    };



    //==========================================================================
    // Iterator
    //--------------------------------------------------------------------------
    // Used to iterate through the map. Comes in mutable and const varieties.
    //--------------------------------------------------------------------------

    private:

    template <typename IE> //E_ may be E or const E
    class Iterator;

    public:

    using iterator = Iterator<E>;
    using const_iterator = Iterator<const E>;



    //--------------------------------------------------------------------------
    // Instance Variables

    private:

    nat m_size;							// total number of elements
    nat m_nSlots;						// number of slots
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

    explicit Map(nat minNSlots = config::map::defNSlots, bool fixed = false);
    Map(const Map<K, E, t_p> & other);
    Map(Map<K, E, t_p> && other);
    template <typename InputIT>
    Map(InputIT first, InputIT last, bool fixed = false);
    explicit Map(std::initializer_list<std::pair<K, E>> pairs, bool fixed = false);



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
    Map & operator=(std::initializer_list<std::pair<K, E>> pairs);



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
    void insert(std::initializer_list<std::pair<K, E>> pairs);

    std::pair<iterator, bool> insert_h(H hash, const E & element);



    //==========================================================================
    // emplace
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    template <typename... ElementArgs>
    std::pair<iterator, bool> emplace(const K & key, ElementArgs &&... elementArgs);

    template <typename... ElementArgs>
    std::pair<iterator, bool> emplace_h(H hash, ElementArgs &&... elementArgs);



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

    E & access_h(H hash);



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

    nat count(const K & key) const;

    nat count_h(H hash) const;



    //==========================================================================
    // rehash
    //--------------------------------------------------------------------------
    // Resizes the map so that there are at lease minNSlots slots.
    // All elements are re-organized.
    // Relatively expensive method.
    //--------------------------------------------------------------------------

    public:

    void rehash(nat minNSlots);



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

    nat size() const;

    bool empty() const;

    nat nSlots() const;
    nat bucket_count() const;

    nat slotSize(nat slotI) const;
    nat bucket_size(nat slotI) const;

    nat slot(const K & key) const;
    nat bucket(const K & key) const;

    bool fixed() const;
    void fixed(bool fixed);



    //--------------------------------------------------------------------------
    // Private Methods

    private:

    H detHash(const K & key) const;

    nat detSlotI(H hash) const;

};



//======================================================================================================================
// Iterator ////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================
// Basic iterator used to iterate forwards over the map.
// iterates forward over the slot, then moves to the next slot.
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename IE> // may be E or const E
class Map<K, E, t_p>::Iterator {

    friend Map<K, E, t_p>;

    //--------------------------------------------------------------------------
    // Types

    using iterator_category = std::forward_iterator_tag;
    using value_type = IE;
    using difference_type = ptrdiff_t;
    using pointer = IE *;
    using reference = IE &;

    //--------------------------------------------------------------------------
    // Instance Variables

    private:

    const Map<K, E, t_p> * m_map;
    nat m_slot;
    typename Map<K, E, t_p>::Node * m_node;



    //==========================================================================
    // Iterator
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    Iterator(const Map<K, E, t_p> & map);
    
    private:

    Iterator(const Map<K, E, t_p> & map, nat slot, typename Map<K, E, t_p>::Node * node);

    public:

    template <typename IE_>
    Iterator(const Iterator<IE_> & iterator);



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

    template <typename IE_>
    Iterator<IE> & operator=(const Iterator<IE_> & iterator);



    //==========================================================================
    // operator++
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    Iterator<IE> & operator++();



    //==========================================================================
    // operator++ int
    //--------------------------------------------------------------------------
    // 

    //--------------------------------------------------------------------------

    Iterator<IE> operator++(int);



    //==========================================================================
    // operator==
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    template <typename IE_>
    bool operator==(const Iterator<IE_> & it) const;



    //==========================================================================
    // operator!=
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    template <typename IE_>
    bool operator!=(const Iterator<IE_> & it) const;



    //==========================================================================
    // operator*
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    reference operator*() const;



    //==========================================================================
    // operator->
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    pointer operator->() const;



    //==========================================================================
    // hash
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    typename const Map<K, E, t_p>::H & hash() const;



    //==========================================================================
    // element
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    reference element() const;

};



//======================================================================================================================
// Functions ///////////////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// swap
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
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

template <typename K, typename E, nat t_p>
template <typename... ElementArgs>
Map<K, E, t_p>::Node::Node(H hash, Node * next, ElementArgs &&... elementArgs) :
    hash(hash),
    next(next),
    element(std::forward<ElementArgs>(elementArgs)...)
{}



//==============================================================================
// Map
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
Map<K, E, t_p>::Map(nat minNSlots, bool fixed) :
    m_size(0),
    m_nSlots(ceil2(max(minNSlots, 1_n))),
    m_slots(new Node *[m_nSlots]),
    m_nodeStore((Node *)std::malloc(m_nSlots * sizeof(Node))),
    m_fixed(fixed),
    m_rehashing(false)
{
    memset(m_slots.get(), 0, m_nSlots * sizeof(Node *));
}

template <typename K, typename E, nat t_p>
Map<K, E, t_p>::Map(const Map<K, E, t_p> & map) :
    m_size(map.m_size),
    m_nSlots(map.m_nSlots),
    m_slots(new Node *[m_nSlots]),
    m_nodeStore((Node *)std::malloc(m_nSlots * sizeof(Node))),
    m_fixed(map.m_fixed),
    m_rehashing(false)
{
    for (nat i(0); i < m_nSlots; ++i) {
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

template <typename K, typename E, nat t_p>
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

template <typename K, typename E, nat t_p>
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

template <typename K, typename E, nat t_p>
Map<K, E, t_p>::Map(std::initializer_list<std::pair<K, E>> pairs, bool fixed) :
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

template <typename K, typename E, nat t_p>
Map<K, E, t_p>::~Map() {
    clear();

    if (m_nodeStore) {
        std::free(m_nodeStore);
    }
}



//==============================================================================
// operatator=
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
Map<K, E, t_p> & Map<K, E, t_p>::operator=(const Map<K, E, t_p> & map) {
    return *this = std::move(Map<K, E, t_p>(map));
}

template <typename K, typename E, nat t_p>
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

template <typename K, typename E, nat t_p>
Map<K, E, t_p> & Map<K, E, t_p>::operator=(std::initializer_list<std::pair<K, E>> pairs) {
    return *this = std::move(Map<K, E, t_p>(pairs));
}



//==============================================================================
// swap
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
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

template <typename K, typename E, nat t_p>
std::pair<typename Map<K, E, t_p>::iterator, bool> Map<K, E, t_p>::insert(const K & key, const E & element) {
    return insert_h(detHash(key), element);
}

template <typename K, typename E, nat t_p>
template <typename InputIt>
void Map<K, E, t_p>::insert(InputIt first, InputIt last) {
    while (first != last) {
        insert_h(detHash(first->first), first->second);
        ++first;
    }
}

template <typename K, typename E, nat t_p>
void Map<K, E, t_p>::insert(std::initializer_list<std::pair<K, E>> pairs) {
    for (const auto & pair : pairs) {
        insert_h(detHash(pair.first), pair.second);
    }
}



//==============================================================================
// insert_h
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
std::pair<typename Map<K, E, t_p>::iterator, bool> Map<K, E, t_p>::insert_h(H hash, const E & element) {
    return emplace_h(hash, element);
}



//==============================================================================
// emplace
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename... ElementArgs>
std::pair<typename Map<K, E, t_p>::iterator, bool> Map<K, E, t_p>::emplace(const K & key, ElementArgs &&... elementArgs) {
    return emplace_h(detHash(key), std::forward<ElementArgs>(elementArgs)...);
}



//==============================================================================
// emplace_h
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename... ElementArgs>
std::pair<typename Map<K, E, t_p>::iterator, bool> Map<K, E, t_p>::emplace_h(H hash, ElementArgs &&... elementArgs) {
    if (!m_fixed && m_size >= m_nSlots) {
        rehash(m_nSlots * 2);
    }

    nat slotI(detSlotI(hash));

    Node ** node(&m_slots[slotI]);
    if (*node) {
        while (*node && (*node)->hash < hash) {
            node = &(*node)->next;
        }
        if (*node && (*node)->hash == hash) {
            return { iterator(*this, slotI, *node), false };
        }
        *node = new Node(hash, *node, std::forward<ElementArgs>(elementArgs)...);
    }
    else {
        *node = new (m_nodeStore + slotI) Node(hash, nullptr, std::forward<ElementArgs>(elementArgs)...);
    }

    ++m_size;
    return { iterator(*this, slotI, *node), true };
}



//==============================================================================
// at
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
E & Map<K, E, t_p>::at(const K & key) const {
    return at_h(detHash(key));
}



//==============================================================================
// at_h
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
E & Map<K, E, t_p>::at_h(H hash) const {
    nat slotI(detSlotI(hash));

    Node * node(m_slots[slotI]);
    while (node && node->hash < hash) {
        node = node->next;
    }
    if (node && node->hash == hash) {
        return node->element;
    }

    throw std::out_of_range("key not found");
}



//==============================================================================
// operator[]
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
E & Map<K, E, t_p>::operator[](const K & key) {
    return access_h(detHash(key));
}



//==============================================================================
// access_h
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
E & Map<K, E, t_p>::access_h(H hash) {
    return *emplace_h(hash).first;
}



//==============================================================================
// erase
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
bool Map<K, E, t_p>::erase(const K & key) {
    return erase_h(detHash(key));
}

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::iterator Map<K, E, t_p>::erase(const_iterator position) {
    if (position == cend()) {
        return position;
    }

    iterator next(position); ++next;
    
    return erase_h(position.hash()) ? next : position;
}

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::iterator Map<K, E, t_p>::erase(const_iterator first, const_iterator last) {
    while (first != last) {
        iterator next(first); ++next;
        if (!erase_h(first.hash())) {
            break;
        }
        ++first;
    }

    return first;
}



//==============================================================================
// erase_h
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
bool Map<K, E, t_p>::erase_h(H hash) {
    nat slotI(detSlotI(hash));

    Node ** node(&m_slots[slotI]);
    while (*node && (*node)->hash < hash) {
        node = &(*node)->next;
    }
    if (!*node || (*node)->hash != hash) {
        return false;
    }

    Node * next((*node)->next);
    if (*node < m_nodeStore || *node >= m_nodeStore + m_nSlots) delete *node;
    *node = next;

    --m_size;
    return true;
}



//==============================================================================
// count
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
nat Map<K, E, t_p>::count(const K & key) const {
    return count_h(detHash(key));
}



//==============================================================================
// count_h
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
nat Map<K, E, t_p>::count_h(H hash) const {
    nat slotI(detSlotI(hash));

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

template <typename K, typename E, nat t_p>
void Map<K, E, t_p>::rehash(nat minNSlots) {
    if (m_rehashing) {
        return;
    }

    Map<K, E, t_p> map(minNSlots, m_fixed);
    m_rehashing = true;
    map.m_rehashing = true;

    for (nat i(0); i < m_nSlots; ++i) {
        Node * node = m_slots[i]; 
        while (node) {
            map.emplace_h(node->hash, std::move(node->element));
            node = node->next;
        }
    }

    map.m_rehashing = false;
    m_rehashing = false;
    *this = std::move(map);
}



//==============================================================================
// clear
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
void Map<K, E, t_p>::clear() {
    Node * storeStart(m_nodeStore), * storeEnd(m_nodeStore + m_nSlots);
    for (nat i = 0; i < m_nSlots; ++i) {
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

template <typename K, typename E, nat t_p>
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

template <typename K, typename E, nat t_p>
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

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::iterator Map<K, E, t_p>::begin() {
    return iterator(*this);
}

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::const_iterator Map<K, E, t_p>::cbegin() const {
    return const_iterator(*this);
}



//==============================================================================
// end
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::iterator Map<K, E, t_p>::end() {
    return iterator(*this, m_nSlots, nullptr);
}

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::const_iterator Map<K, E, t_p>::cend() const {
    return const_iterator(*this, m_nSlots, nullptr);
}



//==============================================================================
// find
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::iterator Map<K, E, t_p>::find(const K & key) {
    return cfind(key);
}

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::const_iterator Map<K, E, t_p>::find(const K & key) const {
    return cfind(key);
}

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::const_iterator Map<K, E, t_p>::cfind(const K & key) const {
    return find_h(detHash(key));
}



//==============================================================================
// find_h
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::iterator Map<K, E, t_p>::find_h(H hash) {
    return cfind_h(hash);
}

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::const_iterator Map<K, E, t_p>::find_h(H hash) const {
    return cfind_h(hash);
}

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::const_iterator Map<K, E, t_p>::cfind_h(H hash) const {
    nat slotI(detSlotI(hash));

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

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::iterator Map<K, E, t_p>::find_e(const E & element) {
    return cfind_e(element);
}

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::const_iterator Map<K, E, t_p>::find_e(const E & element) const {
    return cfind_e(element);
}

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::const_iterator Map<K, E, t_p>::cfind_e(const E & element) const {
    for (const_iterator it(cbegin()); it != cend(); ++it) {
        if (*it == element) {
            return it;
        }
    }

    return cend();
}



//==============================================================================
// size
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
nat Map<K, E, t_p>::size() const {
    return m_size;
}



//==============================================================================
// empty
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
bool Map<K, E, t_p>::empty() const {
    return m_size == 0;
}

//==============================================================================
// nSlots
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
nat Map<K, E, t_p>::nSlots() const {
    return m_nSlots;
}

template <typename K, typename E, nat t_p>
nat Map<K, E, t_p>::bucket_count() const {
    return nSlots();
}

//==============================================================================
// slotSize
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
nat Map<K, E, t_p>::slotSize(nat slotI) const {
    if (slotI < 0 || slotI >= m_nSlots) {
        return 0;
    }

    nat size(0);

    for (Node * node(m_slots[slotI]); node; node = node->next) {
        ++size;
    }
    
    return size;
}

template <typename K, typename E, nat t_p>
nat Map<K, E, t_p>::bucket_size(nat slotI) const {
    return slotSize(slotI);
}

//==============================================================================
// slot
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
nat Map<K, E, t_p>::slot(const K & key) const {
    return detSlotI(detHash(key));
}

template <typename K, typename E, nat t_p>
nat Map<K, E, t_p>::bucket(const K & key) const {
    return slot(key);
}







//==============================================================================
// fixed
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
bool Map<K, E, t_p>::fixed() const {
    return m_fixed;
}

template <typename K, typename E, nat t_p>
void Map<K, E, t_p>::fixed(bool fixed) {
    m_fixed = fixed;
}



//------------------------------------------------------------------------------
// Private Methods

template <typename K, typename E, nat t_p>
inline typename Map<K, E, t_p>::H Map<K, E, t_p>::detHash(const K & key) const {
    if constexpr (!config::map::useStdHash) {
        return hash<t_p>(key);
    }
    if constexpr (config::map::useStdHash) {
        return std::hash<K>{}(key);
    }
}

template <typename K, typename E, nat t_p>
inline nat Map<K, E, t_p>::detSlotI(H hash) const {
    return hash & (m_nSlots - 1);
}



//======================================================================================================================
// Iterator Implementation /////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// Iterator
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename IE>
Map<K, E, t_p>::Iterator<IE>::Iterator(const Map<K, E, t_p> & map) :
    m_map(&map),
    m_slot(0),
    m_node(nullptr)
{
    while (!m_map->m_slots[m_slot]) ++m_slot;
    if (m_slot < m_map->m_nSlots) m_node = m_map->m_slots[m_slot];
}

template <typename K, typename E, nat t_p>
template <typename IE>
Map<K, E, t_p>::Iterator<IE>::Iterator(const Map<K, E, t_p> & map, nat slot, typename Map<K, E, t_p>::Node * node) :
    m_map(&map),
    m_slot(slot),
    m_node(node)
{}

template <typename K, typename E, nat t_p>
template <typename IE>
template <typename IE_>
Map<K, E, t_p>::Iterator<IE>::Iterator(typename const Map<K, E, t_p>::Iterator<IE_> & iterator) :
    m_map(iterator.m_map),
    m_slot(iterator.m_slot),
    m_node(iterator.m_node)
{}



//==============================================================================
// operator=
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename IE>
template <typename IE_>
typename Map<K, E, t_p>::Iterator<IE> & Map<K, E, t_p>::Iterator<IE>::operator=(typename const Map<K, E, t_p>::Iterator<IE_> & iterator) {
    m_map = iterator.m_map;
    m_slot = iterator.m_slot;
    m_node = iterator.m_node;
    return *this;
}



//==============================================================================
// operator++
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename IE>
typename Map<K, E, t_p>::Iterator<IE> & Map<K, E, t_p>::Iterator<IE>::operator++() {
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

template <typename K, typename E, nat t_p>
template <typename IE>
typename Map<K, E, t_p>::Iterator<IE> Map<K, E, t_p>::Iterator<IE>::operator++(int) {
    Map<K, E, t_p>::Iterator<IE> temp(*this);
    operator++();
    return temp;
}



//==============================================================================
// operator==
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename IE>
template <typename IE_>
bool Map<K, E, t_p>::Iterator<IE>::operator==(typename const Map<K, E, t_p>::Iterator<IE_> & o) const {
    return m_node == o.m_node;
}



//==============================================================================
// operator!=
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename IE>
template <typename IE_>
bool Map<K, E, t_p>::Iterator<IE>::operator!=(typename const Map<K, E, t_p>::Iterator<IE_> & o) const {
    return m_node != o.m_node;
}



//==============================================================================
// operator*
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename IE>
typename Map<K, E, t_p>::Iterator<IE>::reference Map<K, E, t_p>::Iterator<IE>::operator*() const {
    return m_node->element;
}



//==============================================================================
// operator->
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename IE>
typename Map<K, E, t_p>::Iterator<IE>::pointer Map<K, E, t_p>::Iterator<IE>::operator->() const {
    return &m_node->element;
}



//==============================================================================
// hash
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename IE>
typename const Map<K, E, t_p>::H & Map<K, E, t_p>::Iterator<IE>::hash() const {
    return m_node->hash;
}



//==============================================================================
// element
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename IE>
typename Map<K, E, t_p>::Iterator<IE>::reference Map<K, E, t_p>::Iterator<IE>::element() const {
    return m_node->element;
}



//======================================================================================================================
// Functions Implementation ////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// swap
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
inline void swap(Map<K, E, t_p> & m1, Map<K, E, t_p> & m2) {
    m1.swap(m2);
}



}