//Code written by Austin Quick.
//
//@updated 1/14/2015
//@since August 2015

#pragma once

#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>
#include <cmath>
#include <algorithm>
#include <list>
#include <utility>

#include "QHashAlgorithms.h"

namespace QHashTable {

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

	//Contains a list of nodes that store each item and hashkey.
	//Also known as a bucket.
	class Slot {

		public:

		//Stores a pointer to its item, that item's hashkey,
		//and a pointer to the next node in the slot.
		struct Node {

			Node(const T * item, unsigned long long hashKey, Node * next = nullptr) :
				item_(item), hashKey_(hashKey), next_(next) {}

			const T * item_;
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
		void push(const T * item, unsigned long long hashKey);

		//Transverses node sequence and returns item pointer of associated hashkey.
		const T * peek(unsigned long long hashKey) const;

		//Transverses node sequence until it finds node with hashkey.
		//"Removes" node by assigning its successor as the successor of its predecessor.
		const T * pop(unsigned long long hashKey);

		//Transverses node sequence...
		//...if it finds a corresponding node, replaces that node with a new node
		//with item and hashkey, then returns pointer to item that was replaced.
		//...if it does not find a node with hashkey, it adds the item and returns null.
		const T * set(const T * item, unsigned long long hashKey);

		//getter
		const Node * first() const;

		//getter
		int size() const;

		//Will attempt to os << *item, hashkey, and address based on bool keys.
		void printContents(std::ostream & os, bool item = false, bool hashKey = false, bool address = false) const;

		private:

		//the first node in the sequence
		Node * first_;

		//the current number of nodes
		int size_;

	};

	public:

	//Mainly used for cout << hashtable;
	//generates string with nSlots and size.
	//
	//*note: defined here because linking errors
	friend std::ostream & operator<<(std::ostream & os, const HashTable & hashTable) {
		return os << "HashTable: num slots: " << hashTable.nSlots_ << ", num items: " << hashTable.size_;
	}

	//Default Constructor
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

	//Decodes key with keyDecoder, passes that to QHashAlgorithms to obtain hashKey,
	//and then forwards that to addByHash.
	template <typename K>
	void add(const T & item, const K & key, int nBytes = sizeof(K), int seed = 0);

	//Passes arguments to above add method with STRING_KEY_DECODER.
	//string.c_str() is used as the key data, not including the \0.
	void add(const T & item, const std::string & key, int seed = 0);

	//Takes hashKey % nSlots_ to find appropriate slot, and then pushes item
	//to that slot.
	void addByHash(const T & item, unsigned long long hashKey);

	//Decodes key with keyDecoder, passes that to QHashAlgorithms to obtain hashKey,
	//and then forwards that to getByHash.
	template <typename K>
	T * get(const K & key, int nBytes = sizeof(K), int seed = 0) const;

	//Passes arguments to above get method with STRING_KEY_DECODER.
	//string.c_str() is used as the key data, not including the \0
	T * get(const std::string & key, int seed = 0) const;

	//Takes hashKey % nSlots_ to find appropriate slot, and then peeks with
	//hashKey for item.
	T * getByHash(unsigned long long hashKey) const;

	//Decodes key with keyDecoder, passes that to QHashAlgorithms to obtain hashKey,
	//and then forwards that to setByHash.
	template <typename K>
	T * set(const T & item, const K & key, int nBytes = sizeof(K), int seed = );

	//Passes arguments to above set method with STRING_KEY_DECODER.
	//string.c_str() is used as the key data, not including the \0.
	T * set(const T & item, const std::string & key, int seed = 0);

	//Takes hashKey % nSlots_ to find appropriate slot, and then sets that slot
	//with item and hashKey. If node.set returns null, then there was no
	//pre-existing item with that hashkey, and it was added.
	T * setByHash(const T & item, unsigned long long hashKey);

	//Decodes key with keyDecoder, passes that to QHashAlgorithms to obtain hashKey,
	//and then forwards that to removeByHash.
	template <typename K>
	T * remove(const K & key, int nBytes = sizeof(K), int seed = 0);

	//Passes arguments to above remove method with STRING_KEY_DECODER.
	//string.c_str() is used as the key data, not including the \0.
	T * remove(const std::string & key, int seed = 0);

	//Takes hashKey % nSlots_ to find appropriate slot, and then pops hashkey
	//in that slot.
	T * removeByHash(unsigned long long hashKey);

	//Resizes the table so that there are nSlots slots.
	//All items are re-organized.
	//Relatively expensive method.
	void resize(int nSlots);

	//getter
	int nSlots() const;

	//getter
	int size() const;

	//Prints a statistical analysis of the table including nSlots, size, and
	//in regards to the size of each slot, the mean, upper and lower 10% mean,
	//median, max, min, standard deviation, variance, and a histogram.
	void stats(std::ostream & os) const;

	//Calls slot.printContents for each slot, to in effect print the entire
	//contents of the table. NOT RECOMMENDED FOR LARGE TABLES
	void printContents(std::ostream & os, bool item = false, bool hashKey = false, bool address = false) const;

	protected:

	private:

	//total number of elements
	int size_;

	//number of slots
	int nSlots_;

	//the vector of slots
	std::vector<Slot> slots_;
};

//Cosmetic exception to be thrown when an item can not be found with given
//hash. Equivalent of item-out-of-bounds exception.
class ItemNotFoundException : public std::exception {};

//Cosmetic exception to be thrown when two data keys generate the same
//hashKey. Should be an extremely rare scenario. Not currently implemented.
class HashKeyCollisionException : public std::exception {};

}

//IMPLEMENTATION////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////																			

namespace QHashTable {
//Slot--------------------------------------------------------------------------

template <typename T>
HashTable<T>::Slot::Slot() : first_(nullptr), size_(0) {}

template <typename T>
HashTable<T>::Slot::Slot(const Slot & other) {
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
typename HashTable<T>::Slot & HashTable<T>::Slot::operator=(const typename HashTable<T>::Slot & other) {
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
void HashTable<T>::Slot::push(const T * item, unsigned long long hashKey) {
	if (!first_) {
		first_ = new Node(item, hashKey);
		++size_;
		return;
	}

	if (hashKey < first_->hashKey_) {
		first_ = new Node(item, hashKey, first_);
		++size_;
		return;
	}

	Node * node = first_;
	while (node->next_ && node->next_->hashKey_ < hashKey) {
		node = node->next_;
	}

	if (node->next_) {
		node->next_ = new Node(item, hashKey, node->next_);
	}
	else {
		node->next_ = new Node(item, hashKey);
	}

	++size_;
}

template <typename T>
const T * HashTable<T>::Slot::peek(unsigned long long hashKey) const {
	Node * node = first_;
	while (node && node->hashKey_ <= hashKey) {
		if (node->hashKey_ == hashKey) {
			return node->item_;
		}
		node = node->next_;
	}
	return nullptr;
}

template <typename T>
const T * HashTable<T>::Slot::pop(unsigned long long hashKey) {
	if (!first_) {
		return nullptr;
	}

	if (first_->hashKey_ == hashKey) {
		const T * item = first_->item_;
		Node * temp = first_;
		first_ = first_->next_;
		delete temp;
		size_--;
		return item;
	}

	Node * node = first_;
	while (node->next_ && node->next_->hashKey_ < hashKey) {
		node = node->next_;
	}
	if (node->next_ && node->next_->hashKey_ == hashKey) {
		const T * item = node->next_->item_;
		node->next_ = node->next_->next_;
		size_--;
		return item;
	}

	return nullptr;
}

//returns what it replaced, otherwise null
template <typename T>
const T * HashTable<T>::Slot::set(const T * item, unsigned long long hashKey) {
	if (!first_) {
		first_ = new Node(item, hashKey);
		++size_;
		return nullptr;
	}

	if (first_->hashKey_ == hashKey) {
		const T * tempI = first_->item_;
		Node * tempN = first_->next_;
		delete first_;
		first_ = new Node(item, hashKey, tempN);
		return tempI;
	}

	if (first_->hashKey_ > hashKey) {
		first_ = new Node(item, hashKey, first_);
		++size_;
		return nullptr;
	}

	Node * node = first_;
	while (node->next_ && node->next_->hashKey_ < hashKey) {
		node = node->next_;
	}
	if (node->next_) {
		if (node->next_->hashKey_ == hashKey) {
			const T * tempI = node->next_->item_;
			Node * tempN = node->next_->next_;
			delete node->next_;
			node->next_ = new Node(item, hashKey, tempN);
			return tempI;
		}
		node->next_ = new Node(item, hashKey, node->next_);
		++size_;
		return nullptr;
	}
	node->next_ = new Node(item, hashKey);
	++size_;
	return nullptr;
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
void HashTable<T>::Slot::printContents(std::ostream & os, bool item, bool hashKey, bool address) const {
	Node * node = first_;
	os << "(N:" << size_ << ") ";
	while (node) {
		os << "[";
		if (item) {
			os << *node->item_;
		}
		if (hashKey) {
			os << " (K:" << (unsigned long long)node->hashKey_ << ")";
		}
		if (address) {
			os << std::hex << " (A:" << node->item_ << ")" << std::dec;
		}
		os << "], ";
		node = node->next_;
	}
}

//HashTable---------------------------------------------------------------------

template <typename T>
HashTable<T>::HashTable(int nSlots) {
	if (nSlots < 1) {
		nSlots = 1;
	}

	nSlots_ = nSlots;
	size_ = 0;
	slots_.resize(nSlots);
}

template <typename T>
HashTable<T>::HashTable(const HashTable & other) {
	size_ = other.size_;
	nSlots_ = other.nSlots_;
	slots_ = other.slots_;
}

template <typename T>
HashTable<T> & HashTable<T>::operator=(const HashTable<T> & other) {
	size_ = other.size_;
	nSlots_ = other.nSlots_;
	slots_ = other.slots_;

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
HashTable<T>::~HashTable() {}

template <typename T>
template <typename K>
void HashTable<T>::add(const T & item, const K & key, int nBytes, int seed) {
	addByHash(item, QHashAlgorithms::hash32(&key, nBytes, seed));
}

template <typename T>
void HashTable<T>::add(const T & item, const std::string & key, int seed) {
	addByHash(item, QHashAlgorithms::hash32(key, seed);
}

template <typename T>
void HashTable<T>::addByHash(const T & item, unsigned long long hashKey) {
	slots_[hashKey % nSlots_].push(&item, hashKey);
	++size_;
}

template <typename T>
template <typename K>
T * HashTable<T>::get(const K & key, int nBytes, int seed) const {
	return getByHash(QHashAlgorithms::hash32(&key, nBytes, seed));
}

template <typename T>
T * HashTable<T>::get(const std::string & key, int seed) const {
	return getByHash(key, 0, seed, QHashAlgorithms::STRING_KEY_DECODER);
}

template <typename T>
T * HashTable<T>::getByHash(unsigned long long hashKey) const {
	const T * item = slots_[hashKey % nSlots_].peek(hashKey);
	if (!item) {
		throw ItemNotFoundException();
	}
	return const_cast<T*>(item); //given as taken, as a non-const. only stored as const
}

template <typename T>
template <typename K>
T * HashTable<T>::set(const T & item, const K & key, int nBytes, int seed, const QHashAlgorithms::KeyDecoder & keyDecoder) {
	QHashAlgorithms::KeyBundle kb(&key, nBytes);
	kb = keyDecoder.decode(kb);
	return setByHash(item, QHashAlgorithms::hash32(kb, seed));
}

template <typename T>
T * HashTable<T>::set(const T & item, const std::string & key, int seed) {
	return set<std::string>(item, key, 0, seed, QHashAlgorithms::STRING_KEY_DECODER);
}

template <typename T>
T * HashTable<T>::setByHash(const T & item, unsigned long long hashKey) {
	const T * replaced = slots_[hashKey % nSlots_].set(&item, hashKey);
	if (!replaced) {
		++size_;
	}
	return const_cast<T*>(replaced); //given as taken, as a non-const. only stored as const
}

template <typename T>
template <typename K>
T * HashTable<T>::remove(const K & key, int nBytes, int seed, const QHashAlgorithms::KeyDecoder & keyDecoder) {
	QHashAlgorithms::KeyBundle kb(&key, nBytes);
	kb = keyDecoder.decode(kb);
	return removeByHash(QHashAlgorithms::hash32(kb, seed));
}

template <typename T>
T * HashTable<T>::remove(const std::string & key, int seed) {
	return remove<std::string>(key, 0, seed, QHashAlgorithms::STRING_KEY_DECODER);
}

template <typename T>
T * HashTable<T>::removeByHash(unsigned long long hashKey) {
	const T * item = slots_[hashKey % nSlots_].pop(hashKey);
	if (!item) {
		throw ItemNotFoundException();
	}
	else {
		size_--;
	}
	return const_cast<T*>(item); //given as taken, as a non-const. only stored as const
}

template <typename T>
void HashTable<T>::resize(int nSlots) {
	if (nSlots == nSlots_) {
		return;
	}

	HashTable<T> table(nSlots);

	const HashTable<T>::Slot::Node * node;
	for (const Slot & slot : slots_) {
		node = slot.first();
		while (node) {
			table.addByHash(*node->item_, node->hashKey_);
			node = node->next_;
		}
	}

	//delete old stuff??

	*this = std::move(table);
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
void HashTable<T>::stats(std::ostream & os) const {
	if (size_ < 1) {
		os << "HashTable is empty." << endl;
		return;
	}

	std::vector<int> sizes(nSlots_);
	for (int i = 0; i < nSlots_; ++i) {
		sizes[i] = slots_[i].size();
	}
	std::sort(sizes.begin(), sizes.end());

	int maxSize = sizes.back();
	int minSize = sizes.front();

	double mean = 0, meanLow10 = 0, meanHigh10 = 0;
	int low10i = (int)std::ceil(sizes.size() * 0.1);
	int high10i = (int)std::floor(sizes.size() * 0.9);
	for (int i = 0; i < low10i; ++i) {
		mean += sizes[i];
		meanLow10 += sizes[i];
	}
	for (int i = low10i; i < high10i; ++i) {
		mean += sizes[i];
	}
	for (int i = high10i; i < sizes.size(); ++i) {
		mean += sizes[i];
		meanHigh10 += sizes[i];
	}
	mean /= sizes.size();
	meanLow10 /= low10i + 1;
	meanHigh10 /= sizes.size() - high10i;

	std::vector<int> sizeCounts(maxSize + 1);
	double variance = 0, deviation;
	for (int size : sizes) {
		++sizeCounts[size];
		variance += (size - mean) * (size - mean);
	}
	variance /= sizes.size();
	deviation = std::sqrt(variance);

	int median = 0;
	int maxSizeCount = sizeCounts.front();
	int minSizeCount = sizeCounts.front();
	for (int i = 1; i < sizeCounts.size(); ++i) {
		if (sizeCounts[i] > maxSizeCount) {
			maxSizeCount = sizeCounts[i];
			median = i;
		}
		else if (sizeCounts[i] < minSizeCount) {
			minSizeCount = sizeCounts[i];
		}
	}

	int maxCharLength = 50, numChars;
	int digits = (int)std::floor(std::log10(sizeCounts.size() - 1));
	os << "Num Slots: " << nSlots_ << ", Num Items: " << size_ << std::endl;
	os << "Mean: " << mean << ", lower 10%: " << meanLow10 << ", upper 10%: " << meanHigh10 << std::endl;
	os << "Median: " << median << ", Min: " << minSize << ", Max: " << maxSize << std::endl;
	os << "Standard Deviation: " << deviation << ", Variance: " << variance << endl;
	for (int i = 0; i < sizeCounts.size(); ++i) {
		os << "[";
		for (int j = 0; j < digits - std::max((int)std::floor(std::log10(i)), 0); ++j) { //so the first column digits are aligned
			os << ' ';
		}
		os << i << "]: ";
		numChars = (int)std::round((float)sizeCounts[i] / maxSizeCount * maxCharLength);
		for (int j = 0; j < numChars; ++j) {
			os << '-';
		}
		os << " (" << sizeCounts[i] << ")" << endl;
	}

}

template <typename T>
void HashTable<T>::printContents(std::ostream & os, bool item, bool hashKey, bool address) const {
	for (int s = 0; s < nSlots_; ++s) {
		os << "[" << s << "]: ";
		slots_[s].printContents(os, item, hashKey, address);
		os << std::endl;
	}
}

}