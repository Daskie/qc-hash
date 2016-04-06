//Code written by Austin Quick.
//
//@updated 1/21/2015
//@since August 2015

#pragma once

#include "QHash.h"

//HashTable/////////////////////////////////////////////////////////////////////
namespace QHash {

//Basic hash table implimentation using the murmur3 hashing algorithm.
//Setup as a vector of Slots, each a list (not std::list) of nodes.
//Each node contains a pointer to the item, the hashkey, and a pointer
//to the next node.
//Supports storing one type, but data can be accessed with any type of key.
//A custom QHashAlgorithms::KeyDecoder can be provided for interpreting
//keys of unusual data storage.
//std::string comes implemented using c_str, and the \0 is dropped.
//Can have a minimum of 1 slot, but may have 0 size.
//
//Currently only using the 32 bit hash, which should be sufficient for all but
//the largest tables. TODO: Implement 64 bit hash option.
template <typename T>
class HashTable {

	//A linked-list bucket for the hashtable.
	class Slot {

		public:

		//Stores a pointer to its item, that item's hashkey,
		//and a pointer to the next node in the slot.
		struct Node {

			Node(const T & item, unsigned long long hashKey, Node * next = nullptr) :
				item_(item), hashKey_(hashKey), next_(next) {}

			T item_;
			unsigned long long hashKey_;
			Node * next_;

		};

		//Default Constructor
		Slot();

		//Copy Constructor
		Slot(const Slot & other);

		//Assignment Operator Overload
		Slot & operator=(const Slot & other);

		//Destructor
		~Slot();

		//Creates a new node for item and stores it in ascending order by hashkey.
		//if an item already exists, does nothing and returns false
		bool push(const T & item, unsigned long long hashKey);

		//Transverses node sequence and sets dest to item at hashkey.
		//returns false if no item is found
		bool peek(unsigned long long hashKey, T ** dest) const;

		//Transverses node sequence until it finds node with hashkey.
		//"Removes" node by assigning its successor as the successor of its predecessor.
		//sets dest to item replaced and returns false if no item is found with hashkey
		bool pop(unsigned long long hashKey, T * dest);

		//Transverses node sequence...
		//...if it finds a corresponding node, replaces that node with a new node
		//with item and hashkey, then sets dest to item that was replaced and
		//returns true, if it does not find a node with hashkey, it adds the
		//item and returns false
		bool set(const T & item, unsigned long long hashKey, T * dest);

		//Returns if the slot contains the item, and sets *keyDest to the hashkey
		bool contains(const T & item, unsigned long long * keyDest) const;

		//empties the slot. after, first_ = nullptr and size = 0
		void clear();

		//Returns whether the two slots are equivalent, with the same number of
		//elements, and the same objects stored
		bool equals(const Slot & other) const;

		//getter
		const Node * first() const;

		//getter
		int size() const;

		//Will attempt to os << *item, hashkey, and address based on bool keys.
		void printContents(std::ostream & os, bool value, bool hash, bool address) const;

		private:

		//the first node in the sequence
		Node * first_;

		//the current number of nodes
		int size_;
	};

	public:

	//Basic iterator used to iterate forwards over the table.
	//iterates forward over the slot, then moves to the next slot.
	class Iterator {

		public:

		Iterator(const HashTable<T> & table);

		bool hasNext() const;

		const T & next();

		private:

		const HashTable<T> & table_;
		int currentSlot_;
		const typename Slot::Node * currentNode_;

	};

	//Mainly used for cout << hashtable;
	//generates string with nSlots and size.
	//
	//*note: defined here because linking errors
	friend std::ostream & operator<<(std::ostream & os, const HashTable & hashTable) {
		return os << "nSlots:" << hashTable.nSlots_ << ", nItems:" << hashTable.size_;
	}

	//Constructor
	explicit HashTable(int nSlots);

	//Copy Constructor
	HashTable(const HashTable & other);

	//Copy Assignment Operator
	HashTable & operator=(const HashTable & other);

	//Move Constructor
	HashTable(HashTable && other);

	//Move Assignment Operator
	HashTable & operator=(HashTable && other);

	//Destructor
	~HashTable();

	//Hashes key and then forwards to addByHash.
	void add(const T & item, const void * key, int nBytes, int seed = DEFAULT_SEED);
	//string.c_str() is used as the key data, not including the \0.
	void add(const T & item, const std::string & key, int seed = DEFAULT_SEED);
	void add(const T & item, char key, int seed = DEFAULT_SEED);
	void add(const T & item, unsigned char key, int seed = DEFAULT_SEED);
	void add(const T & item, short key, int seed = DEFAULT_SEED);
	void add(const T & item, unsigned short key, int seed = DEFAULT_SEED);
	void add(const T & item, int key, int seed = DEFAULT_SEED);
	void add(const T & item, unsigned int key, int seed = DEFAULT_SEED);
	void add(const T & item, long key, int seed = DEFAULT_SEED);
	void add(const T & item, unsigned long key, int seed = DEFAULT_SEED);
	void add(const T & item, long long key, int seed = DEFAULT_SEED);
	void add(const T & item, unsigned long long key, int seed = DEFAULT_SEED);
	void add(const T & item, float key, int seed = DEFAULT_SEED);
	void add(const T & item, double key, int seed = DEFAULT_SEED);


	//Takes hashKey % nSlots_ to find appropriate slot, and then pushes item
	//to that slot.
	void addByHash(const T & item, unsigned long long hashKey);

	//Hashes key and then forwards to getByHash.
	T & get(const void * key, int nBytes, int seed = DEFAULT_SEED) const;
	//string.c_str() is used as the key data, not including the \0
	T & get(const std::string & key, int seed = DEFAULT_SEED) const;
	T & get(char key, int seed = DEFAULT_SEED) const;
	T & get(unsigned char key, int seed = DEFAULT_SEED) const;
	T & get(short key, int seed = DEFAULT_SEED) const;
	T & get(unsigned short key, int seed = DEFAULT_SEED) const;
	T & get(int key, int seed = DEFAULT_SEED) const;
	T & get(unsigned int key, int seed = DEFAULT_SEED) const;
	T & get(long key, int seed = DEFAULT_SEED) const;
	T & get(unsigned long key, int seed = DEFAULT_SEED) const;
	T & get(long long key, int seed = DEFAULT_SEED) const;
	T & get(unsigned long long key, int seed = DEFAULT_SEED) const;
	T & get(float key, int seed = DEFAULT_SEED) const;
	T & get(double key, int seed = DEFAULT_SEED) const;

	//Takes hashKey % nSlots_ to find appropriate slot, and then peeks with
	//hashKey for item.
	T & getByHash(unsigned long long hashKey) const;

	//Hashes key and then forwards to setByHash.
	void set(const T & item, const void * key, int nBytes, int seed = DEFAULT_SEED);
	//string.c_str() is used as the key data, not including the \0.
	void set(const T & item, const std::string & key, int seed = DEFAULT_SEED);
	void set(const T & item, char key, int seed = DEFAULT_SEED);
	void set(const T & item, unsigned char key, int seed = DEFAULT_SEED);
	void set(const T & item, short key, int seed = DEFAULT_SEED);
	void set(const T & item, unsigned short key, int seed = DEFAULT_SEED);
	void set(const T & item, int key, int seed = DEFAULT_SEED);
	void set(const T & item, unsigned int key, int seed = DEFAULT_SEED);
	void set(const T & item, long key, int seed = DEFAULT_SEED);
	void set(const T & item, unsigned long key, int seed = DEFAULT_SEED);
	void set(const T & item, long long key, int seed = DEFAULT_SEED);
	void set(const T & item, unsigned long long key, int seed = DEFAULT_SEED);
	void set(const T & item, float key, int seed = DEFAULT_SEED);
	void set(const T & item, double key, int seed = DEFAULT_SEED);

	//Takes hashKey % nSlots_ to find appropriate slot, and then sets that slot
	//with item and hashKey. If node.set returns null, then there was no
	//pre-existing item with that hashkey, and it was added.
	void setByHash(const T & item, unsigned long long hashKey);

	//Hashes key and then forwards to removeByHash.
	T remove(const void * key, int nBytes, int seed = DEFAULT_SEED);
	//string.c_str() is used as the key data, not including the \0.
	T remove(const std::string & key, int seed = DEFAULT_SEED);
	T remove(char key, int seed = DEFAULT_SEED);
	T remove(unsigned char key, int seed = DEFAULT_SEED);
	T remove(short key, int seed = DEFAULT_SEED);
	T remove(unsigned short key, int seed = DEFAULT_SEED);
	T remove(int key, int seed = DEFAULT_SEED);
	T remove(unsigned int key, int seed = DEFAULT_SEED);
	T remove(long key, int seed = DEFAULT_SEED);
	T remove(unsigned long key, int seed = DEFAULT_SEED);
	T remove(long long key, int seed = DEFAULT_SEED);
	T remove(unsigned long long key, int seed = DEFAULT_SEED);
	T remove(float key, int seed = DEFAULT_SEED);
	T remove(double key, int seed = DEFAULT_SEED);

	//Takes hashKey % nSlots_ to find appropriate slot, and then pops hashkey
	//in that slot.
	T removeByHash(unsigned long long hashKey);

	//Hashes key and then forwards to hasByHash.
	bool has(const void * key, int nBytes, int seed = DEFAULT_SEED);
	//string.c_str() is used as the key data, no including the \0
	bool has(const std::string & key, int seed = DEFAULT_SEED);
	bool has(char key, int seed = DEFAULT_SEED);
	bool has(unsigned char key, int seed = DEFAULT_SEED);
	bool has(short key, int seed = DEFAULT_SEED);
	bool has(unsigned short key, int seed = DEFAULT_SEED);
	bool has(int key, int seed = DEFAULT_SEED);
	bool has(unsigned int key, int seed = DEFAULT_SEED);
	bool has(long key, int seed = DEFAULT_SEED);
	bool has(unsigned long key, int seed = DEFAULT_SEED);
	bool has(long long key, int seed = DEFAULT_SEED);
	bool has(unsigned long long key, int seed = DEFAULT_SEED);
	bool has(float key, int seed = DEFAULT_SEED);
	bool has(double key, int seed = DEFAULT_SEED);

	//Takes hashKey % nSlots_ to find appropriate slot, and then peeks hashkey
	//in that slot.
	bool hasByHash(unsigned long long hashKey);

	//Returns if the table contains the item, and sets keyDest to the hashkey
	bool contains(const T & item, unsigned long long * keyDest = nullptr) const;

	//Resizes the table so that there are nSlots slots.
	//All items are re-organized.
	//Relatively expensive method.
	void resize(int nSlots);

	//clears the table. all slots are cleared. when finished, size_ = 0
	void clear();

	//Returns whether the two tables are equivalent in size and content
	bool equals(const HashTable<T> & other) const;

	//Creates an Iterator for the table.
	Iterator iterator() const;

	//getter
	int nSlots() const;

	//getter
	int size() const;

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

	protected:

	private:

	//total number of elements
	int size_;

	//number of slots
	int nSlots_;

	//the vector of slots
	std::unique_ptr<Slot[]> slots_;
};

//Cosmetic exception to be thrown when an item can not be found with given
//hash. Equivalent of item-out-of-bounds exception.
class ItemNotFoundException : public std::exception {};

//Cosmetic exception to be thrown when trying to add an item with a hashkey
//already in use
class PreexistingItemException : public std::exception {};

//Cosmetic exception to be thrown when two data keys generate the same
//hashKey. Should be an extremely rare scenario. Not currently implemented.
class HashKeyCollisionException : public std::exception {};

}

//Slot Implementation///////////////////////////////////////////////////////////
namespace QHash {

template <typename T>
HashTable<T>::Slot::Slot() :
	first_(nullptr),
	size_(0) {}

template <typename T>
HashTable<T>::Slot::Slot(const Slot & other) {
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

template <typename T>
typename HashTable<T>::Slot & HashTable<T>::Slot::operator=(const Slot & other) {
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


template <typename T>
HashTable<T>::Slot::~Slot() {
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
template <typename T>
bool HashTable<T>::Slot::push(const T & item, unsigned long long hashKey) {
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

template <typename T>
bool HashTable<T>::Slot::peek(unsigned long long hashKey, T ** dest) const {
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

template <typename T>
bool HashTable<T>::Slot::pop(unsigned long long hashKey, T * dest) {
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

template <typename T>
bool HashTable<T>::Slot::set(const T & item, unsigned long long hashKey, T * dest) {
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

template <typename T>
bool HashTable<T>::Slot::contains(const T & item, unsigned long long * keyDest) const {
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

template <typename T>
void HashTable<T>::Slot::clear() {
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

template <typename T>
bool HashTable<T>::Slot::equals(const Slot & other) const {
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

template <typename T>
const typename HashTable<T>::Slot::Node * HashTable<T>::Slot::first() const {
	return first_;
}

template <typename T>
int HashTable<T>::Slot::size() const {
	return size_;
}

template <typename T>
void HashTable<T>::Slot::printContents(std::ostream & os, bool value, bool hash, bool address) const {
	static const int SIZE_THRESHOLD = 10;

	Node * node = first_;

	os << "[N:" << size_ << "]";

	if (size_ > SIZE_THRESHOLD) {
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
			os << (unsigned long long)node->hashKey_;
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

}

//HashTable Iterator Implementation/////////////////////////////////////////////
namespace QHash {

template <typename T>
HashTable<T>::Iterator::Iterator(const HashTable<T> & table) :
	table_(table)
{
	currentSlot_ = 0;
	currentNode_ = table_.slots_[0].first();
}

template <typename T>
bool HashTable<T>::Iterator::hasNext() const {
	return currentNode_ != nullptr;
}

template <typename T>
const T & HashTable<T>::Iterator::next() {
	const T & current = currentNode_->item_;
	currentNode_ = currentNode_->next_;
	if (!currentNode_) {
		while (++currentSlot_ < table_.nSlots_) {
			if (table_.slots_[currentSlot_].size() > 0) {
				currentNode_ = table_.slots_[currentSlot_].first();
				break;
			}
		}
	}
	return current;
}

}

//HashTable Implementation//////////////////////////////////////////////////////
namespace QHash {

template <typename T>
HashTable<T>::HashTable(int nSlots) {
	if (nSlots < 1) {
		nSlots = 1;
	}

	nSlots_ = nSlots;
	size_ = 0;
	slots_ = std::make_unique<Slot[]>(nSlots);
}

template <typename T>
HashTable<T>::HashTable(const HashTable & other) {
	if (&other == this) {
		return;
	}

	size_ = other.size_;
	nSlots_ = other.nSlots_;
	slots_ = std::make_unique<Slot[]>(nSlots_);
	for (int i = 0; i < nSlots_; ++i) {
		slots_[i] = other.slots_[i];
	}
}

template <typename T>
HashTable<T> & HashTable<T>::operator=(const HashTable<T> & other) {
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

template <typename T>
HashTable<T>::HashTable(HashTable<T> && other) :
	slots_(std::move(other.slots_)),
	size_(other.size_),
	nSlots_(other.nSlots_) {}

template <typename T>
HashTable<T> & HashTable<T>::operator=(HashTable<T> && other) {
	slots_ = std::move(other.slots_);
	size_ = other.size_;
	nSlots_ = other.nSlots_;

	return *this;
}

template <typename T>
HashTable<T>::~HashTable() {
	slots_.reset();
}

template <typename T>
void HashTable<T>::add(const T & item, const void * key, int nBytes, int seed) {
	addByHash(item, QHash::hash32(key, nBytes, seed));
}

template <typename T>
void HashTable<T>::add(const T & item, const std::string & key, int seed) {
	addByHash(item, QHash::hash32(key, seed));
}

template <typename T>
void HashTable<T>::add(const T & item, char key, int seed) {
	addByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::add(const T & item, unsigned char key, int seed) {
	addByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::add(const T & item, short key, int seed) {
	addByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::add(const T & item, unsigned short key, int seed) {
	addByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::add(const T & item, int key, int seed) {
	addByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::add(const T & item, unsigned int key, int seed) {
	addByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::add(const T & item, long key, int seed) {
	addByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::add(const T & item, unsigned long key, int seed) {
	addByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::add(const T & item, long long key, int seed) {
	addByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::add(const T & item, unsigned long long key, int seed) {
	addByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::add(const T & item, float key, int seed) {
	addByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::add(const T & item, double key, int seed) {
	addByHash(item, QHash::hash32(key, seed));
}

template <typename T>
void HashTable<T>::addByHash(const T & item, unsigned long long hashKey) {
	if (slots_[hashKey % nSlots_].push(item, hashKey)) {
		++size_;
	}
	else {
		throw PreexistingItemException();
	}
}

template <typename T>
T & HashTable<T>::get(const void * key, int nBytes, int seed) const {
	return getByHash(QHash::hash32(key, nBytes, seed));
}

template <typename T>
T & HashTable<T>::get(const std::string & key, int seed) const {
	return getByHash(QHash::hash32(key, seed));
}

template <typename T>
T & HashTable<T>::get(char key, int seed) const {
	return getByHash(QHash::hash32(key, seed));
}
template <typename T>
T & HashTable<T>::get(unsigned char key, int seed) const {
	return getByHash(QHash::hash32(key, seed));
}
template <typename T>
T & HashTable<T>::get(short key, int seed) const {
	return getByHash(QHash::hash32(key, seed));
}
template <typename T>
T & HashTable<T>::get(unsigned short key, int seed) const {
	return getByHash(QHash::hash32(key, seed));
}
template <typename T>
T & HashTable<T>::get(int key, int seed) const {
	return getByHash(QHash::hash32(key, seed));
}
template <typename T>
T & HashTable<T>::get(unsigned int key, int seed) const {
	return getByHash(QHash::hash32(key, seed));
}
template <typename T>
T & HashTable<T>::get(long key, int seed) const {
	return getByHash(QHash::hash32(key, seed));
}
template <typename T>
T & HashTable<T>::get(unsigned long key, int seed) const {
	return getByHash(QHash::hash32(key, seed));
}
template <typename T>
T & HashTable<T>::get(long long key, int seed) const {
	return getByHash(QHash::hash32(key, seed));
}
template <typename T>
T & HashTable<T>::get(unsigned long long key, int seed) const {
	return getByHash(QHash::hash32(key, seed));
}
template <typename T>
T & HashTable<T>::get(float key, int seed) const {
	return getByHash(QHash::hash32(key, seed));
}
template <typename T>
T & HashTable<T>::get(double key, int seed) const {
	return getByHash(QHash::hash32(key, seed));
}

template <typename T>
T & HashTable<T>::getByHash(unsigned long long hashKey) const {
	T * item;
	if (!slots_[hashKey % nSlots_].peek(hashKey, &item)) {
		throw ItemNotFoundException();
	}
	return *item;
}

template <typename T>
void HashTable<T>::set(const T & item, const void * key, int nBytes, int seed) {
	setByHash(item, QHash::hash32(key, nBytes, seed));
}

template <typename T>
void HashTable<T>::set(const T & item, const std::string & key, int seed) {
	setByHash(item, QHash::hash32(key, seed));
}

template <typename T>
void HashTable<T>::set(const T & item, char key, int seed) {
	setByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::set(const T & item, unsigned char key, int seed) {
	setByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::set(const T & item, short key, int seed) {
	setByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::set(const T & item, unsigned short key, int seed) {
	setByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::set(const T & item, int key, int seed) {
	setByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::set(const T & item, unsigned int key, int seed) {
	setByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::set(const T & item, long key, int seed) {
	setByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::set(const T & item, unsigned long key, int seed) {
	setByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::set(const T & item, long long key, int seed) {
	setByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::set(const T & item, unsigned long long key, int seed) {
	setByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::set(const T & item, float key, int seed) {
	setByHash(item, QHash::hash32(key, seed));
}
template <typename T>
void HashTable<T>::set(const T & item, double key, int seed) {
	setByHash(item, QHash::hash32(key, seed));
}

template <typename T>
void HashTable<T>::setByHash(const T & item, unsigned long long hashKey) {
	T replaced;
	if (!slots_[hashKey % nSlots_].set(item, hashKey, &replaced)) {
		++size_;
	}
}

template <typename T>
T HashTable<T>::remove(const void * key, int nBytes, int seed) {
	return removeByHash(QHash::hash32(key, nBytes, seed));
}

template <typename T>
T HashTable<T>::remove(const std::string & key, int seed) {
	return removeByHash(QHash::hash32(key, seed));
}

template <typename T>
T HashTable<T>::remove(char key, int seed) {
	return removeByHash(QHash::hash32(key, seed));
}
template <typename T>
T HashTable<T>::remove(unsigned char key, int seed) {
	return removeByHash(QHash::hash32(key, seed));
}
template <typename T>
T HashTable<T>::remove(short key, int seed) {
	return removeByHash(QHash::hash32(key, seed));
}
template <typename T>
T HashTable<T>::remove(unsigned short key, int seed) {
	return removeByHash(QHash::hash32(key, seed));
}
template <typename T>
T HashTable<T>::remove(int key, int seed) {
	return removeByHash(QHash::hash32(key, seed));
}
template <typename T>
T HashTable<T>::remove(unsigned int key, int seed) {
	return removeByHash(QHash::hash32(key, seed));
}
template <typename T>
T HashTable<T>::remove(long key, int seed) {
	return removeByHash(QHash::hash32(key, seed));
}
template <typename T>
T HashTable<T>::remove(unsigned long key, int seed) {
	return removeByHash(QHash::hash32(key, seed));
}
template <typename T>
T HashTable<T>::remove(long long key, int seed) {
	return removeByHash(QHash::hash32(key, seed));
}
template <typename T>
T HashTable<T>::remove(unsigned long long key, int seed) {
	return removeByHash(QHash::hash32(key, seed));
}
template <typename T>
T HashTable<T>::remove(float key, int seed) {
	return removeByHash(QHash::hash32(key, seed));
}
template <typename T>
T HashTable<T>::remove(double key, int seed) {
	return removeByHash(QHash::hash32(key, seed));
}

template <typename T>
T HashTable<T>::removeByHash(unsigned long long hashKey) {
	T item;
	if (slots_[hashKey % nSlots_].pop(hashKey, &item)) {
		size_--;
	}
	else {
		throw ItemNotFoundException();
	}
	return item;
}

template <typename T>
bool HashTable<T>::has(const void * key, int nBytes, int seed) {
	return hasByHash(QHash::hash32(key, nBytes, seed));
}

template <typename T>
bool HashTable<T>::has(const std::string & key, int seed) {
	return hasByHash(QHash::hash32(key, seed));
}

template <typename T>
bool HashTable<T>::has(char key, int seed) {
	return hasByHash(QHash::hash32(key, seed));
}
template <typename T>
bool HashTable<T>::has(unsigned char key, int seed) {
	return hasByHash(QHash::hash32(key, seed));
}
template <typename T>
bool HashTable<T>::has(short key, int seed) {
	return hasByHash(QHash::hash32(key, seed));
}
template <typename T>
bool HashTable<T>::has(unsigned short key, int seed) {
	return hasByHash(QHash::hash32(key, seed));
}
template <typename T>
bool HashTable<T>::has(int key, int seed) {
	return hasByHash(QHash::hash32(key, seed));
}
template <typename T>
bool HashTable<T>::has(unsigned int key, int seed) {
	return hasByHash(QHash::hash32(key, seed));
}
template <typename T>
bool HashTable<T>::has(long key, int seed) {
	return hasByHash(QHash::hash32(key, seed));
}
template <typename T>
bool HashTable<T>::has(unsigned long key, int seed) {
	return hasByHash(QHash::hash32(key, seed));
}
template <typename T>
bool HashTable<T>::has(long long key, int seed) {
	return hasByHash(QHash::hash32(key, seed));
}
template <typename T>
bool HashTable<T>::has(unsigned long long key, int seed) {
	return hasByHash(QHash::hash32(key, seed));
}
template <typename T>
bool HashTable<T>::has(float key, int seed) {
	return hasByHash(QHash::hash32(key, seed));
}
template <typename T>
bool HashTable<T>::has(double key, int seed) {
	return hasByHash(QHash::hash32(key, seed));
}

template <typename T>
bool HashTable<T>::hasByHash(unsigned long long hashKey) {
	T * item;
	return slots_[hashKey % nSlots_].peek(hashKey, &item);
	//return *item;
}

template <typename T>
bool HashTable<T>::contains(const T & item, unsigned long long * keyDest) const {
	for (int i = 0; i < nSlots_; ++i) {
		if (slots_[i].contains(item, keyDest)) {
			return true;
		}
	}
	return false;
}

template <typename T>
void HashTable<T>::resize(int nSlots) {
	if (nSlots == nSlots_) {
		return;
	}
	if (nSlots < 1) {
		nSlots = 1;
	}

	HashTable<T> table(nSlots);

	const Slot::Node * node;
	for (int i = 0; i < nSlots_; ++i) {
		node = slots_[i].first();
		while (node) {
			table.addByHash(node->item_, node->hashKey_);
			node = node->next_;
		}
	}

	//delete old stuff??
	//no, gets deleted automagically. not sure how, but it does.

	*this = std::move(table);
}

template <typename T>
void HashTable<T>::clear() {
	if (size_ == 0) {
		return;
	}

	for (int i = 0; i < nSlots_; ++i) {
		slots_[i].clear();
	}

	size_ = 0;
}

template <typename T>
bool HashTable<T>::equals(const HashTable<T> & other) const {
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

template <typename T>
typename HashTable<T>::Iterator HashTable<T>::iterator() const {
	return Iterator(*this);
}

template <typename T>
int HashTable<T>::nSlots() const {
	return nSlots_;
}

template <typename T>
int HashTable<T>::size() const {
	return size_;
}

template <typename T>
void HashTable<T>::printContents(std::ostream & os, bool value, bool hash, bool address) const {
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

template <typename T>
typename HashTable<T>::HashTableStats HashTable<T>::stats() const {
	int min = slots_[0].size();
	int max = slots_[0].size();
	int median = slots_[0].size();
	float mean = float(slots_[0].size());
	float stddev = 0.0f;

	int total = 0;
	for (int i = 0; i < nSlots_; ++i) {
		if (slots_[i].size() < min) {
			min = slots_[i].size();
		}
		else if (slots_[i].size() > max) {
			max = slots_[i].size();
		}

		total += slots_[i].size();
	}
	mean = (float)total / nSlots_;

	int * sizeCounts = new int[max - min + 1];
	memset(sizeCounts, 0, (max - min + 1) * sizeof(int));
	for (int i = 0; i < nSlots_; ++i) {
		++sizeCounts[slots_[i].size() - min];

		stddev += (slots_[i].size() - mean) * (slots_[i].size() - mean);
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

template <typename T>
void HashTable<T>::printHisto(const HashTableStats & stats, std::ostream & os) {
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