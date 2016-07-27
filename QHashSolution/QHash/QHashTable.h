//Code written by Austin Quick.
//
//@updated 1/21/2015
//@since August 2015

#pragma once

#include "QHash.h"



namespace QHash {



constexpr uint32_t DEFAULT_SEED = 0;

constexpr int DEFAULT_NSLOTS = 128;



//HashTable/////////////////////////////////////////////////////////////////////

//Basic hash table implimentation using the murmur3 hashing algorithm.
//Setup as a vector of Slots, each a list (not std::list) of nodes.
//Each node contains a pointer to the item, the hashkey, and a pointer
//to the next node.
//Supports storing one type, but data can be accessed with any type of key or a
//pointer to an array of raw data.
//std::string comes implemented using c_str, and the \0 is ignored.
//Can have a minimum of 1 slot, but may have 0 size.
//
//P indicates the precision of the hash. A 32 bit P will use 32 bit hashing and
//hash codes, a 64 bit P will use 64, etc.
template <typename T, typename P = size_t>
class HashTable {



	//A linked-list bucket for the hashtable.
	struct Slot {

		//Stores a pointer to its item, that item's hashkey,
		//and a pointer to the next node in the slot.
		struct Node {

			Node(const T & item, P hashKey, Node * next = nullptr) :
				item_(item), hashKey_(hashKey), next_(next) {}

			T item_;
			P hashKey_;
			Node * next_;

		};



		//Default Constructor
		Slot();
		//Copy Constructor
		Slot(const Slot & other);

		//Destructor
		~Slot();

		//Assignment Operator Overload
		Slot & operator=(const Slot & other);

		//Creates a new node for item and stores it in ascending order by hashkey.
		//if an item already exists, does nothing and returns false
		bool push(const T & item, P hashKey);

		//Transverses node sequence and sets dest to item at hashkey.
		//returns false if no item is found
		bool peek(P hashKey, T ** dest) const;

		//Transverses node sequence until it finds node with hashkey.
		//"Removes" node by assigning its successor as the successor of its predecessor.
		//sets dest to item replaced and returns false if no item is found with hashkey
		bool pop(P hashKey, T * dest);

		//Transverses node sequence...
		//...if it finds a corresponding node, replaces that node with a new node
		//with item and hashkey, then sets dest to item that was replaced and
		//returns true, if it does not find a node with hashkey, it adds the
		//item and returns false
		bool set(const T & item, P hashKey, T * dest);

		//Returns if the slot contains the item, and sets *keyDest to the hashkey
		bool contains(const T & item, P * keyDest) const;

		//empties the slot. after, first_ = nullptr and size = 0
		void clear();

		//Returns whether the two slots are equivalent, with the same number of
		//elements, and the same objects stored
		bool equals(const Slot & other) const;

		//Will attempt to os << *item, hashkey, and address based on bool keys.
		void printContents(std::ostream & os, bool value, bool hash, bool address) const;



		//the first node in the sequence
		Node * first_;

		//the current number of nodes
		int size_;

	};



	//Basic iterator used to iterate forwards over the table.
	//iterates forward over the slot, then moves to the next slot.
	template <typename T_Ref>
	class Iterator {

		public:

		Iterator(const HashTable<T, P> & table);

		Iterator & operator=(const Iterator & o);

		operator bool() const;

		Iterator & operator++();
		Iterator operator++(int);

		bool operator==(const Iterator & o) const;
		bool operator!=(const Iterator & o) const;

		T_Ref operator*() const;



		private:

		const HashTable<T, P> & table_;
		int slot_;
		typename Slot::Node * node_;

	};



	public:



	typedef Iterator<T &> MIterator;
	typedef Iterator<const T &> CIterator;



	//Default Constructor
	HashTable();
	//Constructor
	explicit HashTable(int nSlots);
	//Copy Constructor
	HashTable(const HashTable & other);
	//Move Constructor
	HashTable(HashTable && other);
	//Variadic Constructor
	template <typename K, typename... TKs>
	HashTable(int size, const T & item, const K & key, const TKs &... tks);

	//Copy Assignment Operator
	HashTable & operator=(const HashTable & other);
	//Move Assignment Operator
	HashTable & operator=(HashTable && other);

	//Destructor
	~HashTable();



	//Hashes key and then forwards to addByHash.
	template <typename K>
	void add(const T & item, const K & key);
	template <typename K>
	void add(const T & item, const K * keyPtr, int nKeyElements);
	//string.c_str() is used as the key data, not including the \0.
	void add(const T & item, const std::string & key);
	//required to allow convenient use of literal strings
	void add(const T & item, const char * key);

	//Takes hashKey % nSlots_ to find appropriate slot, and then pushes item
	//to that slot.
	void addByHash(const T & item, P hashKey);


	//Hashes key and then forwards to getByHash.
	template <typename K>
	T & get(const K & key) const;
	template <typename K>
	T & get(const K * keyPtr, int nKeyElements) const;
	//string.c_str() is used as the key data, not including the \0.
	T & get(const std::string & key) const;
	//required to allow convenient use of literal strings
	T & get(const char * key) const;

	//Takes hashKey % nSlots_ to find appropriate slot, and then peeks with
	//hashKey for item.
	T & getByHash(P hashKey) const;



	//Hashes key and then forwards to setByHash.
	template <typename K>
	void set(const T & item, const K & key);
	template <typename K>
	void set(const T & item, const K * keyPtr, int nKeyElements);
	//string.c_str() is used as the key data, not including the \0.
	void set(const T & item, const std::string & key);
	//required to allow convenient use of literal strings
	void set(const T & item, const char * key);

	//Takes hashKey % nSlots_ to find appropriate slot, and then sets that slot
	//with item and hashKey. If node.set returns null, then there was no
	//pre-existing item with that hashkey, and it was added.
	void setByHash(const T & item, P hashKey);



	//Hashes key and then forwards to removeByHash.
	template <typename K>
	T remove(const K & key);
	template <typename K>
	T remove(const K * keyPtr, int nKeyElements);
	//string.c_str() is used as the key data, not including the \0.
	T remove(const std::string & key);
	//required to allow convenient use of literal strings
	T remove(const char * key);

	//Takes hashKey % nSlots_ to find appropriate slot, and then pops hashkey
	//in that slot.
	T removeByHash(P hashKey);



	//Hashes key and then forwards to hasByHash.
	template <typename K>
	bool has(const K & key) const;
	template <typename K>
	bool has(const K * keyPtr, int nKeyElements) const;
	//string.c_str() is used as the key data, not including the \0.
	bool has(const std::string & key) const;
	//required to allow convenient use of literal strings
	bool has(const char * key) const;

	//Takes hashKey % nSlots_ to find appropriate slot, and then peeks hashkey
	//in that slot.
	bool hasByHash(P hashKey) const;



	//Returns if the table contains the item, and sets keyDest to the hashkey
	bool contains(const T & item, P * keyDest = nullptr) const;

	//Resizes the table so that there are nSlots slots.
	//All items are re-organized.
	//Relatively expensive method.
	void resize(int nSlots);

	//clears the table. all slots are cleared. when finished, size_ = 0
	void clear();

	//Returns whether the two tables are equivalent in size and content
	bool equals(const HashTable<T, P> & other) const;

	//Creates an Iterator for the table.
	MIterator begin();
	//Creates a const iterator for the table.
	CIterator cbegin() const;

	//getters
	int size() const;
	int nSlots() const;
	uint32_t seed() const;

	//setters
	void setSeed(uint32_t seed);

	//Mainly used for cout << hashtable;
	//generates string with nSlots and size.
	//
	//*note: defined here because linking errors
	friend std::ostream & operator<<(std::ostream & os, const HashTable & hashTable) {
		return os << "nSlots:" << hashTable.nSlots_ << ", nItems:" << hashTable.size_;
	}

	//Calls slot.printContents for each slot, to in effect print the entire
	//contents of the table. NOT RECOMMENDED FOR LARGE TABLES
	void printContents(std::ostream & os, bool value, bool hash, bool address) const;

	//Prints a statistical analysis of the table including nSlots, size, and
	//in regards to the size of each slot, the mean, upper and lower 10% mean,
	//median, max, min, standard deviation, variance, and a histogram.
	struct HashTableStats {
		int min, max, median;
		float mean, stddev;
		std::shared_ptr<std::unique_ptr<int[]>> histo;
	};

	HashTableStats stats() const;

	static void printHisto(const HashTableStats & stats, std::ostream & os);



	private:

	//total number of elements
	int size_;
	//number of slots
	int nSlots_;
	//the vector of slots
	std::unique_ptr<Slot[]> slots_;
	//the seed to use for hashing operations
	uint32_t seed_ = DEFAULT_SEED;

};



//Cosmetic exception to be thrown when an item can not be found with given
//hash. Equivalent of index-out-of-bounds exception.
class ItemNotFoundException : public std::exception {};

//Cosmetic exception to be thrown when trying to add an item with a hashkey
//already in use
class PreexistingItemException : public std::exception {};

//Cosmetic exception to be thrown when two data keys generate the same
//hashKey. Should be an extremely rare scenario. Not currently implemented.
class HashKeyCollisionException : public std::exception {};



//Slot Implementation///////////////////////////////////////////////////////////

template <typename T, typename P>
HashTable<T, P>::Slot::Slot() :
	first_(nullptr),
	size_(0) {}

template <typename T, typename P>
HashTable<T, P>::Slot::Slot(const Slot & other) {
	if (&other == this) {
		return;
	}

	size_ = other.size_;
	if (other.first_) {
		first_ = new Node(*other.first_);

		Node * node = first_;
		while (node->next_) {
			node->next_ = new Node(*node->next_);
			node = node->next_;
		}
	}
	else {
		first_ = nullptr;
	}
}

template <typename T, typename P>
typename HashTable<T, P>::Slot & HashTable<T, P>::Slot::operator=(const Slot & other) {
	if (&other == this) {
		return *this;
	}

	size_ = other.size_;
	if (other.first_) {
		first_ = new Node(*other.first_);

		Node * node = first_;
		while (node->next_) {
			node->next_ = new Node(*node->next_);
			node = node->next_;
		}
	}
	else {
		first_ = nullptr;
	}

	return *this;
}

template <typename T, typename P>
HashTable<T, P>::Slot::~Slot() {
	if (first_) {
		Node * node = first_->next_;
		delete first_;
		while (node) {
			first_ = node;
			node = node->next_;
			delete first_;
		}
	}
}

//inserts new items in ascending order by hashKey
template <typename T, typename P>
bool HashTable<T, P>::Slot::push(const T & item, P hashKey) {
	if (!first_) {
		first_ = new Node(item, hashKey);
		++size_;
		return true;
	}

	if (hashKey < first_->hashKey_) {
		first_ = new Node(item, hashKey, first_);
		++size_;
		return true;
	}
	if (hashKey == first_->hashKey_) {
		return false;
	}

	Node * node = first_;
	while (node->next_ && node->next_->hashKey_ < hashKey) {
		node = node->next_;
	}

	if (node->next_) {
		if (node->next_->hashKey_ == hashKey) {
			return false;
		}
		node->next_ = new Node(item, hashKey, node->next_);
	}
	else {
		node->next_ = new Node(item, hashKey);
	}

	++size_;
	return true;

}

template <typename T, typename P>
bool HashTable<T, P>::Slot::peek(P hashKey, T ** dest) const {
	Node * node = first_;
	while (node && node->hashKey_ < hashKey) {
		node = node->next_;
	}
	if (node && node->hashKey_ == hashKey) {
		*dest = &node->item_;
		return true;
	}
	return false;
}

template <typename T, typename P>
bool HashTable<T, P>::Slot::pop(P hashKey, T * dest) {
	if (!first_) {
		return false;
	}

	if (first_->hashKey_ == hashKey) {
		*dest = first_->item_;
		Node * temp = first_;
		first_ = first_->next_;
		delete temp;
		size_--;
		return true;
	}

	Node * node = first_;
	while (node->next_ && node->next_->hashKey_ < hashKey) {
		node = node->next_;
	}
	if (node->next_ && node->next_->hashKey_ == hashKey) {
		*dest = node->next_->item_;
		Node * temp = node->next_;
		node->next_ = node->next_->next_;
		delete temp;
		size_--;
		return true;
	}

	return false;
}

template <typename T, typename P>
bool HashTable<T, P>::Slot::set(const T & item, P hashKey, T * dest) {
	if (!first_) {
		first_ = new Node(item, hashKey);
		++size_;
		return false;
	}

	if (first_->hashKey_ == hashKey) {
		*dest = first_->item_;
		Node * tempN = first_->next_;
		delete first_;
		first_ = new Node(item, hashKey, tempN);
		return true;
	}

	if (first_->hashKey_ > hashKey) {
		first_ = new Node(item, hashKey, first_);
		++size_;
		return false;
	}

	Node * node = first_;
	while (node->next_ && node->next_->hashKey_ < hashKey) {
		node = node->next_;
	}
	if (node->next_) {
		if (node->next_->hashKey_ == hashKey) {
			*dest = node->next_->item_;
			Node * tempN = node->next_->next_;
			delete node->next_;
			node->next_ = new Node(item, hashKey, tempN);
			return true;
		}
		node->next_ = new Node(item, hashKey, node->next_);
		++size_;
		return false;
	}
	node->next_ = new Node(item, hashKey);
	++size_;
	return false;
}

template <typename T, typename P>
bool HashTable<T, P>::Slot::contains(const T & item, P * keyDest) const {
	Node * node = first_;
	while (node) {
		if (node->item_ == item) {
			if (keyDest) {
				*keyDest = node->hashKey_;
			}
			return true;
		}
		node = node->next_;
	}
	return false;
}

template <typename T, typename P>
void HashTable<T, P>::Slot::clear() {
	if (!first_) {
		return;
	}

	Node * node = first_, *temp;
	while (node) {
		temp = node->next_;
		delete node;
		node = temp;
	}

	first_ = nullptr;
	size_ = 0;
}

template <typename T, typename P>
bool HashTable<T, P>::Slot::equals(const Slot & other) const {
	if (&other == this) {
		return true;
	}

	if (other.size_ != size_) {
		return false;
	}

	Node * next1 = first_;
	Node * next2 = other.first_;
	while (next1 && next2) {
		if (next1->item_ != next2->item_) {
			return false;
		}
		next1 = next1->next_;
		next2 = next2->next_;
	}
	if (next1 != next2) {
		return false;
	}
	return true;
}

template <typename T, typename P>
void HashTable<T, P>::Slot::printContents(std::ostream & os, bool value, bool hash, bool address) const {
	static const int PHRESHOLD = 10;

	Node * node = first_;

	os << "[N:" << size_ << "]";

	if (size_ > PHRESHOLD) {
		os << "(too large to print)";
		return;
	}

	while (node) {
		os << "(";
		if (value) {
			os << node->item_;
		}
		if (hash) {
			if (value) {
				os << ", ";
			}
			os << (P)node->hashKey_;
		}
		if (address) {
			if (value || hash) {
				os << ", ";
			}
			os << std::hex << &node->item_ << std::dec;
		}
		os << ")";

		node = node->next_;
	}
}



//HashTable Iterator Implementation/////////////////////////////////////////////

template <typename T, typename P>
template <typename T_Ref>
HashTable<T, P>::Iterator<T_Ref>::Iterator(const HashTable<T, P> & table) :
	table_(table),
	slot_(0),
	node_(table_.slots_[slot_].first_)
{}

template <typename T, typename P>
template <typename T_Ref>
typename HashTable<T, P>::Iterator<T_Ref> & HashTable<T, P>::Iterator<T_Ref>::operator=(typename const HashTable<T, P>::Iterator<T_Ref> & o) {
	table_ = o.table_;
	slot_ = o.slot_;
	node_ = o.node_;
}

template <typename T, typename P>
template <typename T_Ref>
HashTable<T, P>::Iterator<T_Ref>::operator bool() const {
	return node_ != nullptr;
}

template <typename T, typename P>
template <typename T_Ref>
typename HashTable<T, P>::Iterator<T_Ref> & HashTable<T, P>::Iterator<T_Ref>::operator++() {
	node_ = node_->next_;
	if (!node_) {
		while (++slot_ < table_.nSlots_) {
			if (table_.slots_[slot_].size_ > 0) {
				node_ = table_.slots_[slot_].first_;
				break;
			}
		}
	}
	return *this;
}

template <typename T, typename P>
template <typename T_Ref>
typename HashTable<T, P>::Iterator<T_Ref> HashTable<T, P>::Iterator<T_Ref>::operator++(int) {
	HashTable<T, P>::Iterator<T_Ref> temp(*this);
	operator++();
	return temp;
}

template <typename T, typename P>
template <typename T_Ref>
bool HashTable<T, P>::Iterator<T_Ref>::operator==(typename const HashTable<T, P>::Iterator<T_Ref> & o) const {
	return node_ == o.node_;
}

template <typename T, typename P>
template <typename T_Ref>
bool HashTable<T, P>::Iterator<T_Ref>::operator!=(typename const HashTable<T, P>::Iterator<T_Ref> & o) const {
	return node_ != o.node_;
}

template <typename T, typename P>
template <typename T_Ref>
T_Ref HashTable<T, P>::Iterator<T_Ref>::operator*() const {
	return node_->item_;
}



//HashTable Implementation//////////////////////////////////////////////////////

template <typename T, typename P>
HashTable<T, P>::HashTable() :
	HashTable(DEFAULT_NSLOTS)
{}

template <typename T, typename P>
HashTable<T, P>::HashTable(int nSlots) :
	size_(0),
	nSlots_(nSlots < 1 ? 1 : nSlots),
	slots_(new Slot[nSlots_])
{}

template <typename T, typename P>
HashTable<T, P>::HashTable(const HashTable & other) :
	size_(other.size_),
	nSlots_(other.nSlots_),
	slots_(new Slot[nSlots_])
{
	for (int i = 0; i < nSlots_; ++i) {
		slots_[i] = other.slots_[i];
	}
}

template <typename T, typename P>
HashTable<T, P>::HashTable(HashTable<T, P> && other) :
	size_(other.size_),
	nSlots_(other.nSlots_),
	slots_(std::move(other.slots_))
{}

//helpers for variadic constructor
template <typename T, typename P, typename K>
void setMany(HashTable<T, P> & ht, const T & item, const K & key) {
	ht.set(item, key);
}
template <typename T, typename P, typename K, typename... Pairs>
void setMany(HashTable<T, P> & ht, const T & item, const K & key, const Pairs &... pairs) {
	ht.set(item, key);
	setMany(ht, pairs...);
}

template <typename T, typename P>
template <typename K, typename... TKs>
HashTable<T, P>::HashTable(int size, const T & item, const K & key, const TKs &... tks) :
	size_(0),
	nSlots_(size),
	slots_(new Slot[nSlots_])
{
	setMany(*this, item, key, tks...);
}



template <typename T, typename P>
HashTable<T, P> & HashTable<T, P>::operator=(const HashTable<T, P> & other) {
	if (&other == this) {
		return *this;
	}

	size_ = other.size_;
	nSlots_ = other.nSlots_;
	slots_ = std::make_unique<Slot[]>(nSlots_);
	for (int i = 0; i < nSlots_; ++i) {
		slots_[i] = other.slots_[i];
	}

	return *this;
}

template <typename T, typename P>
HashTable<T, P> & HashTable<T, P>::operator=(HashTable<T, P> && other) {
	size_ = other.size_;
	nSlots_ = other.nSlots_;
	slots_ = std::move(other.slots_);

	return *this;
}

template <typename T, typename P>
HashTable<T, P>::~HashTable() {
	slots_.reset();
}

template <typename T, typename P> template <typename K>
void HashTable<T, P>::add(const T & item, const K & key) {
	addByHash(item, hash<K, P>(key, seed_));
}

template <typename T, typename P> template <typename K>
void HashTable<T, P>::add(const T & item, const K * keyPtr, int nKeyElements) {
	addByHash(item, hash<K, P>(keyPtr, nKeyElements, seed_));
}

template <typename T, typename P>
void HashTable<T, P>::add(const T & item, const std::string & key) {
	addByHash(item, hash<std::string, P>(key, seed_));
}

template <typename T, typename P>
void HashTable<T, P>::add(const T & item, const char * key) {
	addByHash(item, hash<const char *, P>(key, seed_));
}

template <typename T, typename P>
void HashTable<T, P>::addByHash(const T & item, P hashKey) {
	if (slots_[hashKey % nSlots_].push(item, hashKey)) {
		++size_;
	}
	else {
		throw PreexistingItemException();
	}
}

template <typename T, typename P> template <typename K>
T & HashTable<T, P>::get(const K & key) const {
	return getByHash(hash<K, P>(key, seed_));
}

template <typename T, typename P> template <typename K>
T & HashTable<T, P>::get(const K * keyPtr, int nKeyElements) const {
	return getByHash(hash<K, P>(keyPtr, nKeyElements, seed_));
}

template <typename T, typename P>
T & HashTable<T, P>::get(const std::string & key) const {
	return getByHash(hash<std::string, P>(key, seed_));
}

template <typename T, typename P>
T & HashTable<T, P>::get(const char * key) const {
	return getByHash(hash<const char *, P>(key, seed_));
}

template <typename T, typename P>
T & HashTable<T, P>::getByHash(P hashKey) const {
	T * item;
	if (!slots_[hashKey % nSlots_].peek(hashKey, &item)) {
		throw ItemNotFoundException();
	}
	return *item;
}

template <typename T, typename P> template <typename K>
void HashTable<T, P>::set(const T & item, const K & key) {
	setByHash(item, hash<K, P>(key, seed_));
}

template <typename T, typename P> template <typename K>
void HashTable<T, P>::set(const T & item, const K * keyPtr, int nKeyElements) {
	setByHash(item, hash<K, P>(keyPtr, nKeyElements, seed_));
}

template <typename T, typename P>
void HashTable<T, P>::set(const T & item, const std::string & key) {
	setByHash(item, hash<std::string, P>(key, seed_));
}

template <typename T, typename P>
void HashTable<T, P>::set(const T & item, const char * key) {
	setByHash(item, hash<const char *, P>(key, seed_));
}

template <typename T, typename P>
void HashTable<T, P>::setByHash(const T & item, P hashKey) {
	T replaced;
	if (!slots_[hashKey % nSlots_].set(item, hashKey, &replaced)) {
		++size_;
	}
}

template <typename T, typename P> template <typename K>
T HashTable<T, P>::remove(const K & key) {
	return removeByHash(hash<K, P>(key, seed_));
}

template <typename T, typename P> template <typename K>
T HashTable<T, P>::remove(const K * keyPtr, int nKeyElements) {
	return removeByHash(hash<K, P>(keyPtr, nKeyElements, seed_));
}

template <typename T, typename P>
T HashTable<T, P>::remove(const std::string & key) {
	return removeByHash(hash<std::string, P>(key, seed_));
}

template <typename T, typename P>
T HashTable<T, P>::remove(const char * key) {
	return removeByHash(hash<const char *, P>(key, seed_));
}

template <typename T, typename P>
T HashTable<T, P>::removeByHash(P hashKey) {
	T item;
	if (slots_[hashKey % nSlots_].pop(hashKey, &item)) {
		size_--;
	}
	else {
		throw ItemNotFoundException();
	}
	return item;
}

template <typename T, typename P> template <typename K>
bool HashTable<T, P>::has(const K & key) const {
	return hasByHash(hash<K, P>(key, seed_));
}

template <typename T, typename P> template <typename K>
bool HashTable<T, P>::has(const K * keyPtr, int nKeyElements) const {
	return hasByHash(hash<K, P>(keyPtr, nKeyElements, seed_));
}

template <typename T, typename P>
bool HashTable<T, P>::has(const std::string & key) const {
	return hasByHash(hash<std::string, P>(key, seed_));
}

template <typename T, typename P>
bool HashTable<T, P>::has(const char * key) const {
	return hasByHash(hash<const char *, P>(key, seed_));
}

template <typename T, typename P>
bool HashTable<T, P>::hasByHash(P hashKey) const {
	T * item;
	return slots_[hashKey % nSlots_].peek(hashKey, &item);
}

template <typename T, typename P>
bool HashTable<T, P>::contains(const T & item, P * keyDest) const {
	for (int i = 0; i < nSlots_; ++i) {
		if (slots_[i].contains(item, keyDest)) {
			return true;
		}
	}
	return false;
}

template <typename T, typename P>
void HashTable<T, P>::resize(int nSlots) {
	if (nSlots == nSlots_) {
		return;
	}
	if (nSlots < 1) {
		nSlots = 1;
	}

	HashTable<T, P> table(nSlots);

	const Slot::Node * node;
	for (int i = 0; i < nSlots_; ++i) {
		node = slots_[i].first_;
		while (node) {
			table.addByHash(node->item_, node->hashKey_);
			node = node->next_;
		}
	}

	*this = std::move(table);
}

template <typename T, typename P>
void HashTable<T, P>::clear() {
	if (size_ == 0) {
		return;
	}

	for (int i = 0; i < nSlots_; ++i) {
		slots_[i].clear();
	}

	size_ = 0;
}

template <typename T, typename P>
bool HashTable<T, P>::equals(const HashTable<T, P> & other) const {
	if (&other == this) {
		return true;
	}

	if (other.nSlots_ != nSlots_ || other.size_ != size_) {
		return false;
	}

	for (int i = 0; i < nSlots_; ++i) {
		if (!slots_[i].equals(other.slots_[i])) {
			return false;
		}
	}

	return true;
}

template <typename T, typename P>
typename HashTable<T, P>::MIterator HashTable<T, P>::begin() {
	return MIterator(*this);
}

template <typename T, typename P>
typename HashTable<T, P>::CIterator HashTable<T, P>::cbegin() const {
	return CIterator(*this);
}

template <typename T, typename P>
int HashTable<T, P>::size() const {
	return size_;
}

template <typename T, typename P>
int HashTable<T, P>::nSlots() const {
	return nSlots_;
}

template <typename T, typename P>
uint32_t HashTable<T, P>::seed() const {
	return seed_;
}

template <typename T, typename P>
void HashTable<T, P>::setSeed(uint32_t seed) {
	seed_ = seed;
}

template <typename T, typename P>
void HashTable<T, P>::printContents(std::ostream & os, bool value, bool hash, bool address) const {
	static const int NSLOTS_THRESHOLD = 50;

	if (nSlots_ > NSLOTS_THRESHOLD) {
		os << "[S:" << nSlots_ << "][N:" << size_ << "](too large to print)";
		return;
	}

	for (int s = 0; s < nSlots_; ++s) {
		os << "[" << s << "]";
		slots_[s].printContents(os, value, hash, address);
		os << std::endl;
	}
}

template <typename T, typename P>
typename HashTable<T, P>::HashTableStats HashTable<T, P>::stats() const {
	int min = slots_[0].size_;
	int max = slots_[0].size_;
	int median = slots_[0].size_;
	float mean = float(slots_[0].size_);
	float stddev = 0.0f;

	int total = 0;
	for (int i = 0; i < nSlots_; ++i) {
		if (slots_[i].size_ < min) {
			min = slots_[i].size_;
		}
		else if (slots_[i].size_ > max) {
			max = slots_[i].size_;
		}

		total += slots_[i].size_;
	}
	mean = (float)total / nSlots_;

	int * sizeCounts = new int[max - min + 1];
	memset(sizeCounts, 0, (max - min + 1) * sizeof(int));
	for (int i = 0; i < nSlots_; ++i) {
		++sizeCounts[slots_[i].size_ - min];

		stddev += (slots_[i].size_ - mean) * (slots_[i].size_ - mean);
	}
	stddev /= nSlots_;
	stddev = sqrt(stddev);

	median = min;
	for (int i = 1; i < max - min + 1; ++i) {
		if (sizeCounts[i] > sizeCounts[median - min]) {
			median = i + min;
		}
	}

	return{
		min, max, median,
		mean, stddev,
		std::make_shared<std::unique_ptr<int[]>>(sizeCounts)
	};
}

template <typename T, typename P>
void HashTable<T, P>::printHisto(const HashTableStats & stats, std::ostream & os) {
	int sizeDigits = stats.max ? (int)log10(stats.max) + 1 : 1;
	int maxCount = (*stats.histo)[stats.median - stats.min];
	int countDigits = maxCount ? (int)log10(maxCount) + 1 : 1;
	int maxLength = 80 - sizeDigits - countDigits - 5; // 5 is for "[][]" & \n
	int length;
	for (int i = stats.min; i < stats.max + 1; ++i) {
		os << "[";
		os.width(sizeDigits);
		os << i << "][";
		os.width(countDigits);
		os << (*stats.histo)[i - stats.min];
		os << "]";
		length = int((float)maxLength * (*stats.histo)[i - stats.min] / maxCount + 0.5f);
		for (int j = 0; j < length; ++j) {
			os << '-';
		}
		os << endl;
	}
}



}