//==============================================================================
// QMap ////////////////////////////////////////////////////////////////////////
//==============================================================================
// Austin Quick, 2017
//------------------------------------------------------------------------------



#pragma once



#include "Hash.hpp"
#include "QMU/Utils.hpp"



namespace qmu {



namespace config {

namespace map {

constexpr nat defNSlots = 16;

}

}



//======================================================================================================================
// Map /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================
// Basic unsorted hash map implementation using the murmur3 hashing algorithm.
// Setup as a vector of slots, each a linked list (not std::list) of nodes.
// Each node contains a pointer to the element, the hashkey, and a pointer
// to the next node.
// std::string comes implemented using c_str.
// Will always have a minimum of 1 slot, but may have 0 size.
//
// t_p indicates the precision of the hash. 4, 8, and 16 byte hash precision
// is supported.
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p = k_nat_p>
class Map {



    //--------------------------------------------------------------------------
    // Special Types

    private:

    using H = precision_ut<t_p>; // unsigned integral type of appropriate precision used for hash values



    //==========================================================================
    // Node
    //--------------------------------------------------------------------------
    // Serves as a node for Slot as a basic linked list.
    //--------------------------------------------------------------------------

    private:

    struct Node {

        H hashKey;
        E element;
        Node * next;

        Node(H hashKey, const E & element, Node * next);

    };



    //==========================================================================
    // Slot
    //--------------------------------------------------------------------------
    // Otherwise known as "bucket". A bare-bones linked-list used to store any
    // number of elements whose hashkey % nSlots == its index.
    //--------------------------------------------------------------------------

    private:

    struct Slot {

        Node * first;
        nat size;

        Slot();
        Slot(Node * first, nat size);

    };



    //==========================================================================
    // Iterator
    //--------------------------------------------------------------------------
    // Used to iterate through the map. Comes in mutable and const varieties.
    //--------------------------------------------------------------------------

    private:

    template <typename I_E = E> //E_ may be E or const E
    class TIterator;

    public:

    using Iterator = TIterator<E>;
    using CIterator = TIterator<const E>;

    // Creates an iterator at the beginning of the map.
    Iterator begin();
    // Creates a const iterator at the beginning of the map.
    CIterator cbegin() const;
    // Creates an iterator at one past the end of the map.
    Iterator end();
    // Creates a const iterator at one past the end of map.
    CIterator cend() const;



    //--------------------------------------------------------------------------
    // Static Variables

    public:

    static constexpr nat k_p = t_p; // precision of the map



    //--------------------------------------------------------------------------
    // Instance Variables

    private:

    nat m_size;							// total number of elements
    nat m_nSlots;						// number of slots
    std::unique_ptr<Slot[]> m_slots;	// the slots
    nat m_seed;							// the seed to use for hashing operations
    bool m_fixed;						// the map will automatically adjust its number of slots
    bool m_rehashing;					// the map is currently rehashing



    //==========================================================================
    // Map
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    Map();
    Map(const Map<K, E, t_p> & other);
    Map(Map<K, E, t_p> && other);
    explicit Map(nat nSlots, bool fixed = false);
    explicit Map(std::initializer_list<std::pair<const K &, const E &>> pairs, bool fixed = false);



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



    //==========================================================================
    // insert
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    std::pair<Iterator, bool> insert(const K & key, const E & element);
    void insert(std::initializer_list<std::pair<const K &, const E &>> pairs);

    std::pair<Iterator, bool> insert_h(const H & hashKey, const E & element);



    //==========================================================================
    // at
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    E & at(const K & key) const;

    E & at_h(const H & hashKey) const;



    //==========================================================================
    // iterator
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    Iterator iterator(const K & key);

    Iterator iterator_h(const H & hashKey);



    //==========================================================================
    // citerator
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    CIterator citerator(const K & key) const;

    CIterator citerator_h(const H & hashKey) const;



    //==========================================================================
    // operator[]
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    E & operator[](const K & key);

    E & access_h(const H & hashKey);



    //==========================================================================
    // erase
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    E erase(const K & key);

    E erase_h(const H & hashKey);



    //==========================================================================
    // has
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    nat count(const K & key) const;

    nat count_h(const H & hashKey) const;



    //==========================================================================
    // find
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    Iterator find(const E & element);



    //==========================================================================
    // cfind
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    public:

    CIterator cfind(const E & element) const;



    //==========================================================================
    // rehash
    //--------------------------------------------------------------------------
    // Resizes the map so that there are nSlots slots.
    // All elements are re-organized.
    // Relatively expensive method.
    //--------------------------------------------------------------------------

    public:
    void rehash(nat nSlots);



    //==========================================================================
    // clear
    //--------------------------------------------------------------------------
    // clears the map. all slots are cleared. when finished, size = 0
    //--------------------------------------------------------------------------

    public:

    void clear();



    //==========================================================================
    // equals
    //--------------------------------------------------------------------------
    // Returns whether the two maps are equivalent in size and content
    //--------------------------------------------------------------------------

    public:

    bool equals(const Map<K, E, t_p> & other) const;



    //--------------------------------------------------------------------------
    // Accessors

    public:

    nat size() const;
    bool empty() const;

    nat nSlots() const;

    nat seed() const;
    void seed(nat seed);

    bool fixed() const;
    void fixed(bool fixed);



    //==========================================================================
    // printContents
    //--------------------------------------------------------------------------
    // Calls slot.printContents for each slot, to in effect print the entire
    // contents of the map. NOT RECOMMENDED FOR LARGE MAPS
    //--------------------------------------------------------------------------

    void printContents(std::ostream & os, bool value, bool hash, bool address) const;



    //==========================================================================
    // stats
    //--------------------------------------------------------------------------
    // Run a basic statistical analysis on the map, filling up a juicy MapStats
    // struct.
    //--------------------------------------------------------------------------

    public:
    struct MapStats {
        nat min, max, median;
        double mean, stddev;
        std::shared_ptr<std::unique_ptr<nat[]>> histo;
    };

    MapStats stats() const;



    //==========================================================================
    // printHisto
    //--------------------------------------------------------------------------
    // Prints a statistical analysis of the map including nSlots, size, and
    // in regards to the size of each slot, the mean, upper and lower 10% mean,
    // median, max, min, standard deviation, variance, and a histogram.
    //--------------------------------------------------------------------------

    static void printHisto(const MapStats & stats, std::ostream & os);


};



//======================================================================================================================
// Iterator ////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================
// Basic iterator used to iterate forwards over the map.
// iterates forward over the slot, then moves to the next slot.
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename I_E> // may be E or const E
class Map<K, E, t_p>::TIterator {

    friend Map<K, E, t_p>;

    //--------------------------------------------------------------------------
    // Special Types

    using I_E_ref = typename std::add_lvalue_reference<I_E>::type;
    using I_E_ptr = typename std::add_pointer<I_E>::type;

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

    TIterator(const Map<K, E, t_p> & map);
    
    private:

    TIterator(const Map<K, E, t_p> & map, nat slot, typename Map<K, E, t_p>::Node * node);

    public:

    template <typename I_E_o>
    TIterator(const TIterator<I_E_o> & iterator);



    //==========================================================================
    // ~Iterator
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    ~TIterator() = default;



    //==========================================================================
    // operator=
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    template <typename I_E_o>
    TIterator<I_E> & operator=(const TIterator<I_E_o> & iterator);



    //==========================================================================
    // operator bool
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    operator bool() const;



    //==========================================================================
    // operator++
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    TIterator<I_E> & operator++();



    //==========================================================================
    // operator++ int
    //--------------------------------------------------------------------------
    // 

    //--------------------------------------------------------------------------

    TIterator<I_E> operator++(int);



    //==========================================================================
    // operator==
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    template <typename I_E_o>
    bool operator==(const TIterator<I_E_o> & o) const;



    //==========================================================================
    // operator!=
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    template <typename I_E_o>
    bool operator!=(const TIterator<I_E_o> & o) const;



    //==========================================================================
    // operator*
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    I_E_ref operator*() const;



    //==========================================================================
    // operator->
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    I_E_ptr operator->() const;



    //==========================================================================
    // hashKey
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    typename const Map<K, E, t_p>::H & hashKey() const;



    //==========================================================================
    // element
    //--------------------------------------------------------------------------
    // 
    //--------------------------------------------------------------------------

    I_E_ref element() const;

};



//======================================================================================================================
// DETAIL //////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



namespace detail {



//==============================================================================
// hashMod
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------

template <typename T>
constexpr nat hashMod(const T & h, nat v);
constexpr nat hashMod(const u128 & h, nat v);



//==============================================================================
// hashEqual
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------

template <typename T>
constexpr bool hashEqual(const T & h1, const T & h2);
constexpr bool hashEqual(const u128 & h1, const u128 & h2);



//==============================================================================
// hashLess
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------

template <typename T>
constexpr bool hashLess(const T & h1, const T & h2);
constexpr bool hashLess(const u128 & h1, const u128 & h2);



//==============================================================================
// hashGreater
//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------

template <typename T>
constexpr bool hashGreater(const T & h1, const T & h2);
constexpr bool hashGreater(const u128 & h1, const u128 & h2);



}



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
Map<K, E, t_p>::Node::Node(H hashKey, const E & element, Node * next) :
    hashKey(hashKey),
    element(element),
    next(next)
{}



//==============================================================================
// Slot
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
Map<K, E, t_p>::Slot::Slot() :
    first(nullptr),
    size(0)
{}

template <typename K, typename E, nat t_p>
Map<K, E, t_p>::Slot::Slot(Node * first, nat size) :
    first(first),
    size(size)
{}



//==============================================================================
// Map
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
Map<K, E, t_p>::Map() :
    m_size(0),
    m_nSlots(config::map::defNSlots),
    m_slots(new Slot[m_nSlots]),
    m_fixed(false),
    m_rehashing(false)
{
    memset(m_slots.get(), 0, m_nSlots * sizeof(Slot));
}

template <typename K, typename E, nat t_p>
Map<K, E, t_p>::Map(nat nSlots, bool fixed) :
    m_size(0),
    m_nSlots(nSlots < 1 ? 1 : nSlots),
    m_slots(new Slot[m_nSlots]),
    m_fixed(fixed),
    m_rehashing(false)
{
    memset(m_slots.get(), 0, m_nSlots * sizeof(Slot));
}

template <typename K, typename E, nat t_p>
Map<K, E, t_p>::Map(const Map<K, E, t_p> & map) :
    m_size(map.m_size),
    m_nSlots(map.m_nSlots),
    m_slots(new Slot[m_nSlots]),
    m_fixed(map.m_fixed),
    m_rehashing(false)
{
    for (nat i(0); i < m_nSlots; ++i) {
        m_slots[i] = map.m_slots[i];

        Node ** node = &m_slots[i].first;
        while (*node) {
            *node = new Node(**node);
            node = &(*node)->next;
        }
    }
}

template <typename K, typename E, nat t_p>
Map<K, E, t_p>::Map(Map<K, E, t_p> && map) :
    m_size(map.m_size),
    m_nSlots(map.m_nSlots),
    m_slots(std::move(map.m_slots)),
    m_fixed(map.m_fixed),
    m_rehashing(false)
{
    map.m_size = 0;
    map.m_nSlots = 0;
}

template <typename K, typename E, nat t_p>
Map<K, E, t_p>::Map(std::initializer_list<std::pair<const K &, const E &>> pairs, bool fixed) :
    m_size(0),
    m_nSlots(pairs.size()),
    m_slots(new Slot[m_nSlots]),
    m_fixed(fixed),
    m_rehashing(false)
{
    memset(m_slots.get(), 0, m_nSlots * sizeof(Slot));
    insert(pairs);
}



//==============================================================================
// ~Map
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
Map<K, E, t_p>::~Map() {
    for (nat i(0); i < m_nSlots; ++i) {
        Node * node(m_slots[i].first), * next;
        while (node) {
            next = node->next;
            delete node;
            node = next;
        }
    }
}



//==============================================================================
// operatator=
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
Map<K, E, t_p> & Map<K, E, t_p>::operator=(const Map<K, E, t_p> & map) {
    if (&map == this) {
        return *this;
    }

    m_size = map.m_size;
    m_nSlots = map.m_nSlots;
    m_slots = std::make_unique<Slot[]>(m_nSlots);

    for (nat i(0); i < m_nSlots; ++i) {
        m_slots[i] = map.m_slots[i];

        Node ** node = &m_slots[i].first;
        while (*node) {
            *node = new Node(**node);
            node = &(*node)->next;
        }
    }

    return *this;
}

template <typename K, typename E, nat t_p>
Map<K, E, t_p> & Map<K, E, t_p>::operator=(Map<K, E, t_p> && map) {
    m_size = map.m_size;
    m_nSlots = map.m_nSlots;
    m_slots = std::move(map.m_slots);

    map.m_size = 0;
    map.m_nSlots = 0;

    return *this;
}



//==============================================================================
// insert
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
std::pair<typename Map<K, E, t_p>::Iterator, bool> Map<K, E, t_p>::insert(const K & key, const E & element) {
    return insert_h(hash<t_p>(key, m_seed), element);
}

template <typename K, typename E, nat t_p>
void Map<K, E, t_p>::insert(std::initializer_list<std::pair<const K &, const E &>> pairs) {
    for (auto & pair : pairs) {
        insert_h(hash<t_p>(pair.first, m_seed), pair.second);
    }
}



//==============================================================================
// insert_h
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
std::pair<typename Map<K, E, t_p>::Iterator, bool> Map<K, E, t_p>::insert_h(const H & hashKey, const E & element) {
    nat slotI(detail::hashMod(hashKey, m_nSlots));

    Node ** node(&m_slots[slotI].first);
    while (*node && detail::hashLess((*node)->hashKey, hashKey)) {
        node = &(*node)->next;
    }
    if (*node && detail::hashEqual((*node)->hashKey, hashKey)) {
        return { Iterator(*this, slotI, *node), false };
    }
    *node = new Node(hashKey, element, *node);

    ++m_slots[slotI].size;
    ++m_size;
    if (!m_fixed && m_size > m_nSlots) {
        rehash(ceil2(m_size));
        return { iterator_h(hashKey), true };
    }
    else {
        return { Iterator(*this, slotI, *node), true };
    }
}



//==============================================================================
// at
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
E & Map<K, E, t_p>::at(const K & key) const {
    return at_h(hash<t_p>(key, m_seed));
}



//==============================================================================
// at_h
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
E & Map<K, E, t_p>::at_h(const H & hashKey) const {
    nat slotI(detail::hashMod(hashKey, m_nSlots));

    Node * node(m_slots[slotI].first);
    while (node && detail::hashLess(node->hashKey, hashKey)) {
        node = node->next;
    }
    if (node && detail::hashEqual(node->hashKey, hashKey)) {
        return node->element;
    }

    throw std::out_of_range("key not found");
}



//==============================================================================
// iterator
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::Iterator Map<K, E, t_p>::iterator(const K & key) {
    return iterator_h(hash<t_p>(key, m_seed));
}



//==============================================================================
// iterator_h
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::Iterator Map<K, E, t_p>::iterator_h(const H & hashKey) {
    return citerator_h(hashKey);
}



//==============================================================================
// citerator
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::CIterator Map<K, E, t_p>::citerator(const K & key) const {
    return citerator_h(hash<t_p>(key, m_seed));
}



//==============================================================================
// citerator_h
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::CIterator Map<K, E, t_p>::citerator_h(const H & hashKey) const {
    nat slotI(detail::hashMod(hashKey, m_nSlots));

    Node * node(m_slots[slotI].first);
    while (node && detail::hashLess(node->hashKey, hashKey)) {
        node = node->next;
    }
    if (node && detail::hashEqual(node->hashKey, hashKey)) {
        return CIterator(*this, slotI, node);
    }

    return cend();
}



//==============================================================================
// operator[]
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
E & Map<K, E, t_p>::operator[](const K & key) {
    return access_h(hash<t_p>(key, m_seed));
}



//==============================================================================
// access_h
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
E & Map<K, E, t_p>::access_h(const H & hashKey) {
    nat slotI(detail::hashMod(hashKey, m_nSlots));

    Node ** node(&m_slots[slotI].first);
    while (*node && detail::hashLess((*node)->hashKey, hashKey)) {
        node = &(*node)->next;
    }
    if (*node && detail::hashEqual((*node)->hashKey, hashKey)) {
        return (*node)->element;
    }
    *node = new Node{ hashKey, E(), *node };

    ++m_slots[slotI].size;
    ++m_size;
    if (!m_fixed && m_size > m_nSlots) {
        rehash(ceil2(m_size));
        return at_h(hashKey);
    }
    else {
        return (*node)->element;
    }
}



//==============================================================================
// erase
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
E Map<K, E, t_p>::erase(const K & key) {
    return erase_h(hash<t_p>(key, m_seed));
}



//==============================================================================
// erase_h
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
E Map<K, E, t_p>::erase_h(const H & hashKey) {
    nat slotI(detail::hashMod(hashKey, m_nSlots));

    E element;

    Node ** node(&m_slots[slotI].first);
    while (*node && detail::hashLess((*node)->hashKey, hashKey)) {
        node = &(*node)->next;
    }
    if (!*node || !detail::hashEqual((*node)->hashKey, hashKey)) {
        throw std::out_of_range("key not found");
    }
    element = (*node)->element;
    Node * next((*node)->next);
    delete *node;
    *node = next;

    --m_slots[slotI].size;
    --m_size;
    return element;
}



//==============================================================================
// count
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
nat Map<K, E, t_p>::count(const K & key) const {
    return count_h(hash<t_p>(key, m_seed));
}



//==============================================================================
// count_h
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
nat Map<K, E, t_p>::count_h(const H & hashKey) const {
    nat slotI(detail::hashMod(hashKey, m_nSlots));

    Node * node = m_slots[slotI].first;
    while (node && detail::hashLess(node->hashKey, hashKey)) {
        node = node->next;
    }
    if (node && detail::hashEqual(node->hashKey, hashKey)) {
        return true;
    }
    return false;
}



//==============================================================================
// find
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::Iterator Map<K, E, t_p>::find(const E & element) {
    return cfind(element);
}



//==============================================================================
// cfind
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::CIterator Map<K, E, t_p>::cfind(const E & element) const {
    for (CIterator it(cbegin()); it; ++it) {
        if (*it == element) {
            return it;
        }
    }

    return cend();
}



//==============================================================================
// rehash
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
void Map<K, E, t_p>::rehash(nat nSlots) {
    if (m_rehashing) {
        return;
    }
    if (nSlots < 1) {
        nSlots = 1;
    }
    if (nSlots == m_nSlots) {
        return;
    }

    Map<K, E, t_p> map(nSlots, m_fixed);
    m_rehashing = true;
    map.m_rehashing = true;

    for (CIterator it(cbegin()); it; ++it) {
        map.insert_h(it.hashKey(), it.element());
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
    for (nat i = 0; i < m_nSlots; ++i) {
        Node * node(m_slots[i].first), * next;
        while (node) {
            next = node->next;
            delete node;
            node = next;
        }

        m_slots[i].first = nullptr;
        m_slots[i].size = 0;
    }

    m_size = 0;
}



//==============================================================================
// equals
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
bool Map<K, E, t_p>::equals(const Map<K, E, t_p> & map) const {
    if (&map == this) {
        return true;
    }

    if (map.m_nSlots != m_nSlots || map.m_size != m_size) {
        return false;
    }

    CIterator it1(cbegin()), it2(map.cbegin());
    for (; it1 && it2; ++it1, ++it2) {
        if (*it1 != *it2) {
            return false;
        }
    }
    return it1 == it2;
}



//==============================================================================
// begin
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::Iterator Map<K, E, t_p>::begin() {
    return Iterator(*this);
}



//==============================================================================
// cbegin
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::CIterator Map<K, E, t_p>::cbegin() const {
    return CIterator(*this);
}



//==============================================================================
// end
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::Iterator Map<K, E, t_p>::end() {
    return Iterator(*this, m_nSlots, nullptr);
}



//==============================================================================
// cend
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::CIterator Map<K, E, t_p>::cend() const {
    return CIterator(*this, m_nSlots, nullptr);
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



//==============================================================================
// seed
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
nat Map<K, E, t_p>::seed() const {
    return m_seed;
}

template <typename K, typename E, nat t_p>
void Map<K, E, t_p>::seed(nat seed) {
    m_seed = seed;
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



//==============================================================================
// printContents
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
void Map<K, E, t_p>::printContents(std::ostream & os, bool value, bool hash, bool address) const {
    static const nat k_nSlotsThreshold(50);
    static const nat k_nElementsThreshold(10);

    if (m_nSlots > k_nSlotsThreshold) {
        os << "[S:" << m_nSlots << "][N:" << m_size << "](too large to print)";
        return;
    }

    for (nat i = 0; i < m_nSlots; ++i) {
        os << "[" << i << "]";

        Node * node = m_slots[i].first;

        os << "[N:" << m_slots[i].size << "]";

        if (m_slots[i].size > k_nElementsThreshold) {
            os << "(too large to print)" << std::endl;
            break;
        }

        while (node) {
            os << "(";
            if (value) {
                os << node->element;
            }
            if (hash) {
                if (value) {
                    os << ", ";
                }
                os << node->hashKey;
            }
            if (address) {
                if (value || hash) {
                    os << ", ";
                }
                os << std::hex << &node->element << std::dec;
            }
            os << ")";

            node = node->next;
        }

        os << std::endl;
    }
}



//==============================================================================
// stats
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
typename Map<K, E, t_p>::MapStats Map<K, E, t_p>::stats() const {
    nat min = m_slots[0].size;
    nat max = m_slots[0].size;
    nat median = m_slots[0].size;
    double mean = double(m_slots[0].size);
    double stddev = 0.0;

    nat total = 0;
    for (nat i = 0; i < m_nSlots; ++i) {
        if (m_slots[i].size < min) {
            min = m_slots[i].size;
        }
        else if (m_slots[i].size > max) {
            max = m_slots[i].size;
        }

        total += m_slots[i].size;
    }
    mean = (double)total / m_nSlots;

    nat * sizeCounts = new nat[max - min + 1];
    memset(sizeCounts, 0, (max - min + 1) * sizeof(nat));
    for (nat i = 0; i < m_nSlots; ++i) {
        ++sizeCounts[m_slots[i].size - min];

        stddev += (m_slots[i].size - mean) * (m_slots[i].size - mean);
    }
    stddev /= m_nSlots;
    stddev = std::sqrt(stddev);

    median = min;
    for (nat i = 1; i < max - min + 1; ++i) {
        if (sizeCounts[i] > sizeCounts[median - min]) {
            median = i + min;
        }
    }

    return{
        min, max, median,
        mean, stddev,
        std::make_shared<std::unique_ptr<nat[]>>(sizeCounts)
    };
}



//==============================================================================
// printHisto
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
void Map<K, E, t_p>::printHisto(const MapStats & stats, std::ostream & os) {
    nat sizeDigits = stats.max ? (nat)log10(stats.max) + 1 : 1;
    nat maxCount = (*stats.histo)[stats.median - stats.min];
    nat countDigits = maxCount ? (nat)log10(maxCount) + 1 : 1;
    nat maxLength = 80 - sizeDigits - countDigits - 5; // 5 is for "[][]" & \n
    nat length;
    for (nat i = stats.min; i < stats.max + 1; ++i) {
        os << "[";
        os.width(sizeDigits);
        os << i << "][";
        os.width(countDigits);
        os << (*stats.histo)[i - stats.min];
        os << "]";
        length = nat((double)maxLength * (*stats.histo)[i - stats.min] / maxCount + 0.5f);
        for (nat j = 0; j < length; ++j) {
            os << '-';
        }
        os << endl;
    }
}



//======================================================================================================================
// Iterator Implementation /////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// Iterator
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename I_E>
Map<K, E, t_p>::TIterator<I_E>::TIterator(const Map<K, E, t_p> & map) :
    m_map(&map),
    m_slot(0),
    m_node(nullptr)
{
    while (!m_map->m_slots[m_slot].first) ++m_slot;
    if (m_slot < m_map->m_nSlots) m_node = m_map->m_slots[m_slot].first;
}

template <typename K, typename E, nat t_p>
template <typename I_E>
Map<K, E, t_p>::TIterator<I_E>::TIterator(const Map<K, E, t_p> & map, nat slot, typename Map<K, E, t_p>::Node * node) :
    m_map(&map),
    m_slot(slot),
    m_node(node)
{}

template <typename K, typename E, nat t_p>
template <typename I_E>
template <typename I_E_o>
Map<K, E, t_p>::TIterator<I_E>::TIterator(typename const Map<K, E, t_p>::TIterator<I_E_o> & iterator) :
    m_map(iterator.m_map),
    m_slot(iterator.m_slot),
    m_node(iterator.m_node)
{}



//==============================================================================
// operator=
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename I_E>
template <typename I_E_o>
typename Map<K, E, t_p>::TIterator<I_E> & Map<K, E, t_p>::TIterator<I_E>::operator=(typename const Map<K, E, t_p>::TIterator<I_E_o> & iterator) {
    m_map = iterator.m_map;
    m_slot = iterator.m_slot;
    m_node = iterator.m_node;
    return *this;
}



//==============================================================================
// operator bool
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename I_E>
Map<K, E, t_p>::TIterator<I_E>::operator bool() const {
    return m_node != nullptr;
}



//==============================================================================
// operator++
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename I_E>
typename Map<K, E, t_p>::TIterator<I_E> & Map<K, E, t_p>::TIterator<I_E>::operator++() {
    m_node = m_node->next;
    if (!m_node) {
        while (++m_slot < m_map->m_nSlots) {
            if (m_map->m_slots[m_slot].size > 0) {
                m_node = m_map->m_slots[m_slot].first;
                break;
            }
        }
    }
    return *this;
}



//==============================================================================
// operator++ int
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename I_E>
typename Map<K, E, t_p>::TIterator<I_E> Map<K, E, t_p>::TIterator<I_E>::operator++(int) {
    Map<K, E, t_p>::TIterator<I_E> temp(*this);
    operator++();
    return temp;
}



//==============================================================================
// operator==
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename I_E>
template <typename I_E_o>
bool Map<K, E, t_p>::TIterator<I_E>::operator==(typename const Map<K, E, t_p>::TIterator<I_E_o> & o) const {
    return m_node == o.m_node;
}



//==============================================================================
// operator!=
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename I_E>
template <typename I_E_o>
bool Map<K, E, t_p>::TIterator<I_E>::operator!=(typename const Map<K, E, t_p>::TIterator<I_E_o> & o) const {
    return m_node != o.m_node;
}



//==============================================================================
// operator*
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename I_E>
typename Map<K, E, t_p>::TIterator<I_E>::I_E_ref Map<K, E, t_p>::TIterator<I_E>::operator*() const {
    return m_node->element;
}



//==============================================================================
// operator->
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename I_E>
typename Map<K, E, t_p>::TIterator<I_E>::I_E_ptr Map<K, E, t_p>::TIterator<I_E>::operator->() const {
    return &m_node->element;
}



//==============================================================================
// hashKey
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename I_E>
typename const Map<K, E, t_p>::H & Map<K, E, t_p>::TIterator<I_E>::hashKey() const {
    return m_node->hashKey;
}



//==============================================================================
// element
//------------------------------------------------------------------------------

template <typename K, typename E, nat t_p>
template <typename I_E>
typename Map<K, E, t_p>::TIterator<I_E>::I_E_ref Map<K, E, t_p>::TIterator<I_E>::element() const {
    return m_node->element;
}



//======================================================================================================================
// DETAIL IMPLEMENTATION ///////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================

namespace detail {



//==============================================================================
// hashMod
//------------------------------------------------------------------------------

template <typename T>
constexpr nat hashMod(const T & h, nat v) {
    return h % v;
}

constexpr nat hashMod(const u128 & h, nat v) {
    return h.h1 % v;
}



//==============================================================================
// hashEqual
//------------------------------------------------------------------------------

template <typename T>
constexpr bool hashEqual(const T & h1, const T & h2) {
    return h1 == h2;
}

constexpr bool hashEqual(const u128 & h1, const u128 & h2) {
    return h1.h1 == h2.h1 && h1.h2 == h2.h2;
}



//==============================================================================
// hashLess
//------------------------------------------------------------------------------

template <typename T>
constexpr bool hashLess(const T & h1, const T & h2) {
    return h1 < h2;
}

constexpr bool hashLess(const u128 & h1, const u128 & h2) {
    return h1.h2 == h2.h2 ? h1.h1 < h2.h1 : h1.h2 < h2.h2;
}



//==============================================================================
// hashGreater
//------------------------------------------------------------------------------

template <typename T>
constexpr bool hashGreater(const T & h1, const T & h2) {
    return h1 > h2;
}

constexpr bool hashGreater(const u128 & h1, const u128 & h2) {
    return h1.h2 == h2.h2 ? h1.h1 < h2.h1 : h1.h2 < h2.h2;
}



}



}



//==============================================================================