//==============================================================================
// QMap ////////////////////////////////////////////////////////////////////////
//==============================================================================
// Austin Quick, 2017
//------------------------------------------------------------------------------



#pragma once



#include "Hash.hpp"
#include "QMU/Utils.hpp"



namespace qmu {



constexpr u32 k_defSeed = 0;



//======================================================================================================================
// Map /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================
// Basic unsorted hash map implementation using the murmur3 hashing algorithm.
// Setup as a vector of slots, each a linked list (not std::list) of nodes.
// Each node contains a pointer to the element, the hashkey, and a pointer
// to the next node.
// Can store only a single type, but data can be accessed with any type of key
// or a pointer to any raw data.
// std::string comes implemented using c_str.
// Will always have a minimum of 1 slot, but may have 0 size.
//
// t_p indicates the precision of the hash. 32, 64, and 128 bit hash precision
// is supported.
//------------------------------------------------------------------------------

template <typename E, nat t_p = k_nat_p>
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
	static constexpr nat k_defNSlots = 8;
	static constexpr double k_defFactor = 1;



	//--------------------------------------------------------------------------
	// Instance Variables

	private:

	nat m_size;							// total number of elements
	nat m_nSlots;						// number of slots
	std::unique_ptr<Slot[]> m_slots;	// the slots
	nat m_seed;							// the seed to use for hashing operations
	bool m_fixed;						// the map will automatically adjust its number of slots
	double m_factor;						// the ideal number of elements per slot
	bool m_rehashing;					// the map is currently rehashing



	//==========================================================================
	// Map
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------

	public:

	Map();
	Map(const Map & other);
	Map(Map && other);
	explicit Map(nat nSlots, bool fixed = false);
	/*template <typename K, typename... Pairs>
	Map(const std::pair<const K &, const E &> & pair, const Pairs &... pairs);*/
	template <typename K>
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

	Map & operator=(const Map & other);
	Map & operator=(Map && other);



	//==========================================================================
	// insert
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------

	public:

	template <typename K>
	std::pair<Iterator, bool> insert(const K & key, const E & element);
	template <typename K>
	std::pair<Iterator, bool> insert(const K * key, nat nElements, const E & element);
	std::pair<Iterator, bool> insert(const std::string & key, const E & element);
	template <typename K>
	void insert(std::initializer_list<std::pair<const K &, const E &>> pairs);

	std::pair<Iterator, bool> insertByHash(const H & hashKey, const E & element);



	//==========================================================================
	// at
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------

	public:

	template <typename K>
	E & at(const K & key) const;
	template <typename K>
	E & at(const K * key, nat nElements) const;
	E & at(const std::string & key) const;

	E & atByHash(const H & hashKey) const;



	//==========================================================================
	// iterator
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------

	public:

	template <typename K>
	Iterator iterator(const K & key);
	template <typename K>
	Iterator iterator(const K * key, nat nElements);
	Iterator iterator(const std::string & key);

	Iterator iteratorByHash(const H & hashKey);



	//==========================================================================
	// citerator
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------

	public:

	template <typename K>
	CIterator citerator(const K & key) const;
	template <typename K>
	CIterator citerator(const K * key, nat nElements) const;
	CIterator citerator(const std::string & key) const;

	CIterator citeratorByHash(const H & hashKey) const;



	//==========================================================================
	// operator[]
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------

	public:

	template <typename K>
	E & operator[](const K & key);
	E & operator[](const std::string & key);

	E & accessByHash(const H & hashKey);



	//==========================================================================
	// erase
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------

	public:

	template <typename K>
	E erase(const K & key);
	template <typename K>
	E erase(const K * key, nat nElements);
	E erase(const std::string & key);

	E eraseByHash(const H & hashKey);



	//==========================================================================
	// has
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------

	public:

	template <typename K>
	nat count(const K & key) const;
	template <typename K>
	nat count(const K * key, nat nElements) const;
	nat count(const std::string & key) const;

	nat countByHash(const H & hashKey) const;



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

	bool equals(const Map<E, t_p> & other) const;



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

	double factor() const;
	void factor(double factor);



	//==========================================================================
	// operator<<
	//--------------------------------------------------------------------------
	// Mainly used for cout << map;
	// generates string with nSlots and size.
	//
	// *note: defined here because linking errors
	//--------------------------------------------------------------------------

	public:

	friend std::ostream & operator<<(std::ostream & os, const Map & hashMap) {
		return os << "nSlots:" << hashMap.m_nSlots << ", nElements:" << hashMap.m_size;
	}



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

template <typename E, nat t_p>
template <typename I_E> // may be E or const E
class Map<E, t_p>::TIterator {

	friend Map<E, t_p>;

	//--------------------------------------------------------------------------
	// Special Types

	using I_E_ref = typename std::add_lvalue_reference<I_E>::type;
	using I_E_ptr = typename std::add_pointer<I_E>::type;

	//--------------------------------------------------------------------------
	// Instance Variables

	private:

	const Map<E, t_p> * m_map;
	nat m_slot;
	typename Map<E, t_p>::Node * m_node;



	//==========================================================================
	// Iterator
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------

	public:

	TIterator(const Map<E, t_p> & map);
	
	private:

	TIterator(const Map<E, t_p> & map, nat slot, typename Map<E, t_p>::Node * node);

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

	typename const Map<E, t_p>::H & hashKey() const;



	//==========================================================================
	// element
	//--------------------------------------------------------------------------
	// 
	//--------------------------------------------------------------------------

	I_E_ref element() const;

};



//======================================================================================================================
// TECH ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



namespace tech {



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
// IMPLEMENTATION ==============================================================================================================================================
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//==============================================================================================================================================================



//======================================================================================================================
// MAP IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================



//==============================================================================
// Map
//------------------------------------------------------------------------------

template <typename E, nat t_p>
Map<E, t_p>::Map() :
	m_size(0),
	m_nSlots(k_defNSlots),
	m_slots(new Slot[m_nSlots]),
	m_fixed(false),
	m_factor(k_defFactor),
	m_rehashing(false)
{
	memset(m_slots.get(), 0, m_nSlots * sizeof(Slot));
}

template <typename E, nat t_p>
Map<E, t_p>::Map(nat nSlots, bool fixed) :
	m_size(0),
	m_nSlots(nSlots < 1 ? 1 : nSlots),
	m_slots(new Slot[m_nSlots]),
	m_fixed(fixed),
	m_factor(k_defFactor),
	m_rehashing(false)
{
	memset(m_slots.get(), 0, m_nSlots * sizeof(Slot));
}

template <typename E, nat t_p>
Map<E, t_p>::Map(const Map<E, t_p> & map) :
	m_size(map.m_size),
	m_nSlots(map.m_nSlots),
	m_slots(new Slot[m_nSlots]),
	m_fixed(map.m_fixed),
	m_factor(map.m_factor),
	m_rehashing(false)
{
	for (nat i(0); i < m_nSlots; ++i) {
		m_slots[i].size = map.m_slots[i].size;
		m_slots[i].first = map.m_slots[i].first;

		Node ** node = &m_slots[i].first;
		while (*node) {
			*node = new Node(**node);
			node = &(*node)->next;
		}
	}
}

template <typename E, nat t_p>
Map<E, t_p>::Map(Map<E, t_p> && map) :
	m_size(map.m_size),
	m_nSlots(map.m_nSlots),
	m_slots(std::move(map.m_slots)),
	m_fixed(map.m_fixed),
	m_factor(map.m_factor),
	m_rehashing(false)
{
	map.m_size = 0;
	map.m_nSlots = 0;
}

template <typename E, nat t_p>
template <typename K>
Map<E, t_p>::Map(std::initializer_list<std::pair<const K &, const E &>> pairs, bool fixed) :
	m_size(0),
	m_nSlots(pairs.size()),
	m_slots(new Slot[m_nSlots]),
	m_fixed(fixed),
	m_factor(k_defFactor),
	m_rehashing(false)
{
	memset(m_slots.get(), 0, m_nSlots * sizeof(Slot));
	insert(pairs);
}



//==============================================================================
// ~Map
//------------------------------------------------------------------------------

template <typename E, nat t_p>
Map<E, t_p>::~Map() {
	for (nat i(0); i < m_nSlots; ++i) {
		Node * node = m_slots[i].first, * next;
		while (node) {
			next = node->next;
			delete node;
			node = next;
		}
	}

	m_slots.reset();
}



//==============================================================================
// operatator=
//------------------------------------------------------------------------------

template <typename E, nat t_p>
Map<E, t_p> & Map<E, t_p>::operator=(const Map<E, t_p> & map) {
	if (&map == this) {
		return *this;
	}

	m_size = map.m_size;
	m_nSlots = map.m_nSlots;
	m_slots = std::make_unique<Slot[]>(m_nSlots);

	for (nat i(0); i < m_nSlots; ++i) {
		m_slots[i].size = map.m_slots[i].size;
		m_slots[i].first = map.m_slots[i].first;

		Node ** node = &m_slots[i].first;
		while (*node) {
			*node = new Node(**node);
			node = &(*node)->next;
		}
	}

	return *this;
}

template <typename E, nat t_p>
Map<E, t_p> & Map<E, t_p>::operator=(Map<E, t_p> && map) {
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

template <typename E, nat t_p> template <typename K>
std::pair<typename Map<E, t_p>::Iterator, bool> Map<E, t_p>::insert(const K & key, const E & element) {
	return insertByHash(hash<t_p>(key, m_seed), element);
}

template <typename E, nat t_p> template <typename K>
std::pair<typename Map<E, t_p>::Iterator, bool> Map<E, t_p>::insert(const K * key, nat nElements, const E & element) {
	return insertByHash(hash<t_p>(key, nElements, m_seed), element);
}

template <typename E, nat t_p>
std::pair<typename Map<E, t_p>::Iterator, bool> Map<E, t_p>::insert(const std::string & key, const E & element) {
	return insertByHash(hash<t_p>(key, m_seed), element);
}

template <typename E, nat t_p>
template <typename K>
void Map<E, t_p>::insert(std::initializer_list<std::pair<const K &, const E &>> pairs) {
	for (auto & pair : pairs) {
		insertByHash(hash<t_p>(pair.first, m_seed), pair.second);
	}
}



//==============================================================================
// insertByHash
//------------------------------------------------------------------------------

template <typename E, nat t_p>
std::pair<typename Map<E, t_p>::Iterator, bool> Map<E, t_p>::insertByHash(const H & hashKey, const E & element) {
	nat slotI(tech::hashMod(hashKey, m_nSlots));

	Node ** node(&m_slots[slotI].first);
	while (*node && tech::hashLess((*node)->hashKey, hashKey)) {
		node = &(*node)->next;
	}
	if (*node && tech::hashEqual((*node)->hashKey, hashKey)) {
		return { Iterator(*this, slotI, *node), false };
	}
	*node = new Node{ hashKey, element, *node };

	++m_slots[slotI].size;
	++m_size;
	if (m_size > m_nSlots * m_factor && !m_fixed) {
		rehash(ceil2(static_cast<nat>(ceil(m_size / m_factor))));
		return { iteratorByHash(hashKey), true };
	}
	else {
		return { Iterator(*this, slotI, *node), true };
	}
}



//==============================================================================
// at
//------------------------------------------------------------------------------

template <typename E, nat t_p> template <typename K>
E & Map<E, t_p>::at(const K & key) const {
	return atByHash(hash<t_p>(key, m_seed));
}

template <typename E, nat t_p> template <typename K>
E & Map<E, t_p>::at(const K * key, nat nElements) const {
	return atByHash(hash<t_p>(key, nElements, m_seed));
}

template <typename E, nat t_p>
E & Map<E, t_p>::at(const std::string & key) const {
	return atByHash(hash<t_p>(key, m_seed));
}



//==============================================================================
// atByHash
//------------------------------------------------------------------------------

template <typename E, nat t_p>
E & Map<E, t_p>::atByHash(const H & hashKey) const {
	nat slotI(tech::hashMod(hashKey, m_nSlots));

	Node * node(m_slots[slotI].first);
	while (node && tech::hashLess(node->hashKey, hashKey)) {
		node = node->next;
	}
	if (node && tech::hashEqual(node->hashKey, hashKey)) {
		return node->element;
	}

	throw std::out_of_range("key not found");
}



//==============================================================================
// iterator
//------------------------------------------------------------------------------

template <typename E, nat t_p> template <typename K>
typename Map<E, t_p>::Iterator Map<E, t_p>::iterator(const K & key) {
	return iteratorByHash(hash<t_p>(key, m_seed));
}

template <typename E, nat t_p> template <typename K>
typename Map<E, t_p>::Iterator Map<E, t_p>::iterator(const K * key, nat nElements) {
	return iteratorByHash(hash<t_p>(key, nElements, m_seed));
}

template <typename E, nat t_p>
typename Map<E, t_p>::Iterator Map<E, t_p>::iterator(const std::string & key) {
	return iteratorByHash(hash<t_p>(key, m_seed));
}



//==============================================================================
// iteratorByHash
//------------------------------------------------------------------------------

template <typename E, nat t_p>
typename Map<E, t_p>::Iterator Map<E, t_p>::iteratorByHash(const H & hashKey) {
	return citeratorByHash(hashKey);
}



//==============================================================================
// citerator
//------------------------------------------------------------------------------

template <typename E, nat t_p> template <typename K>
typename Map<E, t_p>::CIterator Map<E, t_p>::citerator(const K & key) const {
	return citeratorByHash(hash<t_p>(key, m_seed));
}

template <typename E, nat t_p> template <typename K>
typename Map<E, t_p>::CIterator Map<E, t_p>::citerator(const K * key, nat nElements) const {
	return citeratorByHash(hash<t_p>(key, nElements, m_seed));
}

template <typename E, nat t_p>
typename Map<E, t_p>::CIterator Map<E, t_p>::citerator(const std::string & key) const {
	return citeratorByHash(hash<t_p>(key, m_seed));
}



//==============================================================================
// citeratorByHash
//------------------------------------------------------------------------------

template <typename E, nat t_p>
typename Map<E, t_p>::CIterator Map<E, t_p>::citeratorByHash(const H & hashKey) const {
	nat slotI(tech::hashMod(hashKey, m_nSlots));

	Node * node(m_slots[slotI].first);
	while (node && tech::hashLess(node->hashKey, hashKey)) {
		node = node->next;
	}
	if (node && tech::hashEqual(node->hashKey, hashKey)) {
		return CIterator(*this, slotI, node);
	}

	return cend();
}



//==============================================================================
// operator[]
//------------------------------------------------------------------------------

template <typename E, nat t_p> template <typename K>
E & Map<E, t_p>::operator[](const K & key) {
	return accessByHash(hash<t_p>(key, m_seed));
}

template <typename E, nat t_p>
E & Map<E, t_p>::operator[](const std::string & key) {
	return accessByHash(hash<t_p>(key, m_seed));
}



//==============================================================================
// accessByHash
//------------------------------------------------------------------------------

template <typename E, nat t_p>
E & Map<E, t_p>::accessByHash(const H & hashKey) {
	nat slotI(tech::hashMod(hashKey, m_nSlots));

	Node ** node(&m_slots[slotI].first);
	while (*node && tech::hashLess((*node)->hashKey, hashKey)) {
		node = &(*node)->next;
	}
	if (*node && tech::hashEqual((*node)->hashKey, hashKey)) {
		return (*node)->element;
	}
	*node = new Node{ hashKey, E(), *node };

	++m_slots[slotI].size;
	++m_size;
	if (m_size > m_nSlots * m_factor && !m_fixed) {
		rehash(ceil2(static_cast<nat>(ceil(m_size / m_factor))));
		return atByHash(hashKey);
	}
	else {
		return (*node)->element;
	}
}



//==============================================================================
// erase
//------------------------------------------------------------------------------

template <typename E, nat t_p> template <typename K>
E Map<E, t_p>::erase(const K & key) {
	return eraseByHash(hash<t_p>(key, m_seed));
}

template <typename E, nat t_p> template <typename K>
E Map<E, t_p>::erase(const K * key, nat nElements) {
	return eraseByHash(hash<t_p>(key, nElements, m_seed));
}

template <typename E, nat t_p>
E Map<E, t_p>::erase(const std::string & key) {
	return eraseByHash(hash<t_p>(key, m_seed));
}



//==============================================================================
// eraseByHash
//------------------------------------------------------------------------------

template <typename E, nat t_p>
E Map<E, t_p>::eraseByHash(const H & hashKey) {
	nat slotI(tech::hashMod(hashKey, m_nSlots));

	E element;

	Node ** node(&m_slots[slotI].first);
	while (*node && tech::hashLess((*node)->hashKey, hashKey)) {
		node = &(*node)->next;
	}
	if (!*node || !tech::hashEqual((*node)->hashKey, hashKey)) {
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

template <typename E, nat t_p> template <typename K>
nat Map<E, t_p>::count(const K & key) const {
	return countByHash(hash<t_p>(key, m_seed));
}

template <typename E, nat t_p> template <typename K>
nat Map<E, t_p>::count(const K * key, nat nElements) const {
	return countByHash(hash<t_p>(key, nElements, m_seed));
}

template <typename E, nat t_p>
nat Map<E, t_p>::count(const std::string & key) const {
	return countByHash(hash<t_p>(key, m_seed));
}



//==============================================================================
// countByHash
//------------------------------------------------------------------------------

template <typename E, nat t_p>
nat Map<E, t_p>::countByHash(const H & hashKey) const {
	nat slotI(tech::hashMod(hashKey, m_nSlots));

	Node * node = m_slots[slotI].first;
	while (node && tech::hashLess(node->hashKey, hashKey)) {
		node = node->next;
	}
	if (node && tech::hashEqual(node->hashKey, hashKey)) {
		return true;
	}
	return false;
}



//==============================================================================
// find
//------------------------------------------------------------------------------

template <typename E, nat t_p>
typename Map<E, t_p>::Iterator Map<E, t_p>::find(const E & element) {
	return cfind(element);
}



//==============================================================================
// cfind
//------------------------------------------------------------------------------

template <typename E, nat t_p>
typename Map<E, t_p>::CIterator Map<E, t_p>::cfind(const E & element) const {
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

template <typename E, nat t_p>
void Map<E, t_p>::rehash(nat nSlots) {
	if (m_rehashing) {
		return;
	}
	if (nSlots < 1) {
		nSlots = 1;
	}
	if (nSlots == m_nSlots) {
		return;
	}

	Map<E, t_p> map(nSlots, m_fixed);
	m_rehashing = true;
	map.m_rehashing = true;

	for (CIterator it(cbegin()); it; ++it) {
		map.insertByHash(it.hashKey(), it.element());
	}

	map.m_rehashing = false;
	m_rehashing = false;
	*this = std::move(map);
}



//==============================================================================
// clear
//------------------------------------------------------------------------------

template <typename E, nat t_p>
void Map<E, t_p>::clear() {
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

template <typename E, nat t_p>
bool Map<E, t_p>::equals(const Map<E, t_p> & map) const {
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

template <typename E, nat t_p>
typename Map<E, t_p>::Iterator Map<E, t_p>::begin() {
	return Iterator(*this);
}



//==============================================================================
// cbegin
//------------------------------------------------------------------------------

template <typename E, nat t_p>
typename Map<E, t_p>::CIterator Map<E, t_p>::cbegin() const {
	return CIterator(*this);
}



//==============================================================================
// end
//------------------------------------------------------------------------------

template <typename E, nat t_p>
typename Map<E, t_p>::Iterator Map<E, t_p>::end() {
	return Iterator(*this, m_nSlots, nullptr);
}



//==============================================================================
// cend
//------------------------------------------------------------------------------

template <typename E, nat t_p>
typename Map<E, t_p>::CIterator Map<E, t_p>::cend() const {
	return CIterator(*this, m_nSlots, nullptr);
}



//==============================================================================
// size
//------------------------------------------------------------------------------

template <typename E, nat t_p>
nat Map<E, t_p>::size() const {
	return m_size;
}



//==============================================================================
// empty
//------------------------------------------------------------------------------

template <typename E, nat t_p>
bool Map<E, t_p>::empty() const {
	return m_size == 0;
}



//==============================================================================
// nSlots
//------------------------------------------------------------------------------

template <typename E, nat t_p>
nat Map<E, t_p>::nSlots() const {
	return m_nSlots;
}



//==============================================================================
// seed
//------------------------------------------------------------------------------

template <typename E, nat t_p>
nat Map<E, t_p>::seed() const {
	return m_seed;
}

template <typename E, nat t_p>
void Map<E, t_p>::seed(nat seed) {
	m_seed = seed;
}



//==============================================================================
// fixed
//------------------------------------------------------------------------------

template <typename E, nat t_p>
bool Map<E, t_p>::fixed() const {
	return m_fixed;
}

template <typename E, nat t_p>
void Map<E, t_p>::fixed(bool fixed) {
	m_fixed = fixed;
}



//==============================================================================
// factor
//------------------------------------------------------------------------------

template <typename E, nat t_p>
double Map<E, t_p>::factor() const {
	return m_factor;
}

template <typename E, nat t_p>
void Map<E, t_p>::factor(double factor) {
	m_factor = factor;
}



//==============================================================================
// printContents
//------------------------------------------------------------------------------

template <typename E, nat t_p>
void Map<E, t_p>::printContents(std::ostream & os, bool value, bool hash, bool address) const {
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

template <typename E, nat t_p>
typename Map<E, t_p>::MapStats Map<E, t_p>::stats() const {
	nat min = m_slots[0].size;
	nat max = m_slots[0].size;
	nat median = m_slots[0].size;
	double mean = double(m_slots[0].size);
	double stddev = 0.0f;

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
	stddev = sqrt(stddev);

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

template <typename E, nat t_p>
void Map<E, t_p>::printHisto(const MapStats & stats, std::ostream & os) {
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

template <typename E, nat t_p>
template <typename I_E>
Map<E, t_p>::TIterator<I_E>::TIterator(const Map<E, t_p> & map) :
	m_map(&map),
	m_slot(0),
	m_node(nullptr)
{
	while (!m_map->m_slots[m_slot].first) ++m_slot;
	if (m_slot < m_map->m_nSlots) m_node = m_map->m_slots[m_slot].first;
}

template <typename E, nat t_p>
template <typename I_E>
Map<E, t_p>::TIterator<I_E>::TIterator(const Map<E, t_p> & map, nat slot, typename Map<E, t_p>::Node * node) :
	m_map(&map),
	m_slot(slot),
	m_node(node)
{}

template <typename E, nat t_p>
template <typename I_E>
template <typename I_E_o>
Map<E, t_p>::TIterator<I_E>::TIterator(typename const Map<E, t_p>::TIterator<I_E_o> & iterator) :
	m_map(iterator.m_map),
	m_slot(iterator.m_slot),
	m_node(iterator.m_node)
{}



//==============================================================================
// operator=
//------------------------------------------------------------------------------

template <typename E, nat t_p>
template <typename I_E>
template <typename I_E_o>
typename Map<E, t_p>::TIterator<I_E> & Map<E, t_p>::TIterator<I_E>::operator=(typename const Map<E, t_p>::TIterator<I_E_o> & iterator) {
	m_map = iterator.m_map;
	m_slot = iterator.m_slot;
	m_node = iterator.m_node;
	return *this;
}



//==============================================================================
// operator bool
//------------------------------------------------------------------------------

template <typename E, nat t_p>
template <typename I_E>
Map<E, t_p>::TIterator<I_E>::operator bool() const {
	return m_node != nullptr;
}



//==============================================================================
// operator++
//------------------------------------------------------------------------------

template <typename E, nat t_p>
template <typename I_E>
typename Map<E, t_p>::TIterator<I_E> & Map<E, t_p>::TIterator<I_E>::operator++() {
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

template <typename E, nat t_p>
template <typename I_E>
typename Map<E, t_p>::TIterator<I_E> Map<E, t_p>::TIterator<I_E>::operator++(int) {
	Map<E, t_p>::TIterator<I_E> temp(*this);
	operator++();
	return temp;
}



//==============================================================================
// operator==
//------------------------------------------------------------------------------

template <typename E, nat t_p>
template <typename I_E>
template <typename I_E_o>
bool Map<E, t_p>::TIterator<I_E>::operator==(typename const Map<E, t_p>::TIterator<I_E_o> & o) const {
	return m_node == o.m_node;
}



//==============================================================================
// operator!=
//------------------------------------------------------------------------------

template <typename E, nat t_p>
template <typename I_E>
template <typename I_E_o>
bool Map<E, t_p>::TIterator<I_E>::operator!=(typename const Map<E, t_p>::TIterator<I_E_o> & o) const {
	return m_node != o.m_node;
}



//==============================================================================
// operator*
//------------------------------------------------------------------------------

template <typename E, nat t_p>
template <typename I_E>
typename Map<E, t_p>::TIterator<I_E>::I_E_ref Map<E, t_p>::TIterator<I_E>::operator*() const {
	return m_node->element;
}



//==============================================================================
// operator->
//------------------------------------------------------------------------------

template <typename E, nat t_p>
template <typename I_E>
typename Map<E, t_p>::TIterator<I_E>::I_E_ptr Map<E, t_p>::TIterator<I_E>::operator->() const {
	return &m_node->element;
}



//==============================================================================
// hashKey
//------------------------------------------------------------------------------

template <typename E, nat t_p>
template <typename I_E>
typename const Map<E, t_p>::H & Map<E, t_p>::TIterator<I_E>::hashKey() const {
	return m_node->hashKey;
}



//==============================================================================
// element
//------------------------------------------------------------------------------

template <typename E, nat t_p>
template <typename I_E>
typename Map<E, t_p>::TIterator<I_E>::I_E_ref Map<E, t_p>::TIterator<I_E>::element() const {
	return m_node->element;
}



//======================================================================================================================
// TECH IMPLEMENTATION /////////////////////////////////////////////////////////////////////////////////////////////////
//======================================================================================================================

namespace tech {



//==============================================================================
// hashMod
//------------------------------------------------------------------------------

template <typename T>
constexpr nat hashMod(const T & h, nat v) {
	return h % v;
}

constexpr nat hashMod(const u128 & h, nat v) {
	return (h.u64_1 ^ h.u64_2) % v;
}



//==============================================================================
// hashEqual
//------------------------------------------------------------------------------

template <typename T>
constexpr bool hashEqual(const T & h1, const T & h2) {
	return h1 == h2;
}

constexpr bool hashEqual(const u128 & h1, const u128 & h2) {
	return h1.u64_1 == h2.u64_1 && h1.u64_2 == h2.u64_2;
}



//==============================================================================
// hashLess
//------------------------------------------------------------------------------

template <typename T>
constexpr bool hashLess(const T & h1, const T & h2) {
	return h1 < h2;
}

constexpr bool hashLess(const u128 & h1, const u128 & h2) {
	return (h1.u64_1 ^ h1.u64_2) < (h2.u64_1 ^ h2.u64_2);
}



//==============================================================================
// hashGreater
//------------------------------------------------------------------------------

template <typename T>
constexpr bool hashGreater(const T & h1, const T & h2) {
	return h1 > h2;
}

constexpr bool hashGreater(const u128 & h1, const u128 & h2) {
	return (h1.u64_1 ^ h1.u64_2) > (h2.u64_1 ^ h2.u64_2);
}



}



}



//==============================================================================