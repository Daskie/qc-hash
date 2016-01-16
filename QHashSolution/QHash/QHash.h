//Code written by Austin Quick.
//
//@updated 1/14/2015
//@since August 2015

#pragma once

#include <iostream>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <memory>

namespace {
const int DEFAULT_SEED = 0;
}

//Hashing Algorithms////////////////////////////////////////////////////////////
namespace QHash {

namespace MurmurHash3 {

inline uint32_t rotl32(uint32_t x, int8_t r) {
	return (x << r) | (x >> (32 - r));
}

inline uint64_t rotl64(uint64_t x, int8_t r) {
	return (x << r) | (x >> (64 - r));
}

inline uint32_t fmix32(uint32_t h) {
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h;
}

inline uint64_t fmix64(uint64_t k) {
	k ^= k >> 33;
	k *= 0xff51afd7ed558ccdULL;
	k ^= k >> 33;
	k *= 0xc4ceb9fe1a85ec53ULL;
	k ^= k >> 33;

	return k;
}

//Hashes len bytes of key and writes a 32 bit result to out.
void murmur_x86_32(const void * key, int len, uint32_t seed, void * out) {
	const uint8_t * data = (const uint8_t *)key;
	const int nblocks = len / 4;
	int i;

	uint32_t h1 = seed;

	uint32_t c1 = 0xcc9e2d51;
	uint32_t c2 = 0x1b873593;

	//----------
	// body

	const uint32_t * blocks = (const uint32_t *)(data + nblocks * 4);

	for (i = -nblocks; i; ++i) {
		uint32_t k1 = blocks[i];

		k1 *= c1;
		k1 = rotl32(k1, 15);
		k1 *= c2;

		h1 ^= k1;
		h1 = rotl32(h1, 13);
		h1 = h1 * 5 + 0xe6546b64;
	}

	//----------
	// tail

	const uint8_t * tail = (const uint8_t *)(data + nblocks * 4);

	uint32_t k1 = 0;

	switch (len & 3) {
		case 3: k1 ^= tail[2] << 16;
		case 2: k1 ^= tail[1] << 8;
		case 1: k1 ^= tail[0];
			k1 *= c1; k1 = rotl32(k1, 15); k1 *= c2; h1 ^= k1;
	};

	//----------
	// finalization

	h1 ^= len;

	h1 = fmix32(h1);

	*(uint32_t *)out = h1;
}

//Hashes len bytes of key and writes two 64 bit results to out.
void murmur_x86_128(const void * key, int len, uint32_t seed, void * out) {
	const uint8_t * data = (const uint8_t *)key;
	const int nblocks = len / 16;
	int i;

	uint32_t h1 = seed;
	uint32_t h2 = seed;
	uint32_t h3 = seed;
	uint32_t h4 = seed;

	uint32_t c1 = 0x239b961b;
	uint32_t c2 = 0xab0e9789;
	uint32_t c3 = 0x38b34ae5;
	uint32_t c4 = 0xa1e38b93;

	//----------
	// body

	const uint32_t * blocks = (const uint32_t *)(data + nblocks * 16);

	for (i = -nblocks; i; ++i) {
		uint32_t k1 = blocks[i * 4 + 0];
		uint32_t k2 = blocks[i * 4 + 1];
		uint32_t k3 = blocks[i * 4 + 2];
		uint32_t k4 = blocks[i * 4 + 3];

		k1 *= c1; k1 = rotl32(k1, 15); k1 *= c2; h1 ^= k1;

		h1 = rotl32(h1, 19); h1 += h2; h1 = h1 * 5 + 0x561ccd1b;

		k2 *= c2; k2 = rotl32(k2, 16); k2 *= c3; h2 ^= k2;

		h2 = rotl32(h2, 17); h2 += h3; h2 = h2 * 5 + 0x0bcaa747;

		k3 *= c3; k3 = rotl32(k3, 17); k3 *= c4; h3 ^= k3;

		h3 = rotl32(h3, 15); h3 += h4; h3 = h3 * 5 + 0x96cd1c35;

		k4 *= c4; k4 = rotl32(k4, 18); k4 *= c1; h4 ^= k4;

		h4 = rotl32(h4, 13); h4 += h1; h4 = h4 * 5 + 0x32ac3b17;
	}

	//----------
	// tail

	const uint8_t * tail = (const uint8_t*)(data + nblocks * 16);

	uint32_t k1 = 0;
	uint32_t k2 = 0;
	uint32_t k3 = 0;
	uint32_t k4 = 0;

	switch (len & 15) {
		case 15: k4 ^= tail[14] << 16;
		case 14: k4 ^= tail[13] << 8;
		case 13: k4 ^= tail[12] << 0;
			k4 *= c4; k4 = rotl32(k4, 18); k4 *= c1; h4 ^= k4;

		case 12: k3 ^= tail[11] << 24;
		case 11: k3 ^= tail[10] << 16;
		case 10: k3 ^= tail[9] << 8;
		case  9: k3 ^= tail[8] << 0;
			k3 *= c3; k3 = rotl32(k3, 17); k3 *= c4; h3 ^= k3;

		case  8: k2 ^= tail[7] << 24;
		case  7: k2 ^= tail[6] << 16;
		case  6: k2 ^= tail[5] << 8;
		case  5: k2 ^= tail[4] << 0;
			k2 *= c2; k2 = rotl32(k2, 16); k2 *= c3; h2 ^= k2;

		case  4: k1 ^= tail[3] << 24;
		case  3: k1 ^= tail[2] << 16;
		case  2: k1 ^= tail[1] << 8;
		case  1: k1 ^= tail[0] << 0;
			k1 *= c1; k1 = rotl32(k1, 15); k1 *= c2; h1 ^= k1;
	};

	//----------
	// finalization

	h1 ^= len; h2 ^= len; h3 ^= len; h4 ^= len;

	h1 += h2; h1 += h3; h1 += h4;
	h2 += h1; h3 += h1; h4 += h1;

	h1 = fmix32(h1);
	h2 = fmix32(h2);
	h3 = fmix32(h3);
	h4 = fmix32(h4);

	h1 += h2; h1 += h3; h1 += h4;
	h2 += h1; h3 += h1; h4 += h1;

	((uint32_t *)out)[0] = h1;
	((uint32_t *)out)[1] = h2;
	((uint32_t *)out)[2] = h3;
	((uint32_t *)out)[3] = h4;
}

//Hashes len bytes of key and writes two 64 bit results to out.
void murmur_x64_128(const void * key, int len, uint32_t seed, void * out) {
	const uint8_t * data = (const uint8_t *)key;
	const int nblocks = len / 16;
	int i;

	uint64_t h1 = seed;
	uint64_t h2 = seed;

	uint64_t c1 = 0x87c37b91114253d5ULL;
	uint64_t c2 = 0x4cf5ad432745937fULL;

	//----------
	// body

	const uint64_t * blocks = (const uint64_t *)(data);

	for (i = 0; i < nblocks; ++i) {
		uint64_t k1 = blocks[i * 2 + 0];
		uint64_t k2 = blocks[i * 2 + 1];

		k1 *= c1; k1 = rotl64(k1, 31); k1 *= c2; h1 ^= k1;

		h1 = rotl64(h1, 27); h1 += h2; h1 = h1 * 5 + 0x52dce729;

		k2 *= c2; k2 = rotl64(k2, 33); k2 *= c1; h2 ^= k2;

		h2 = rotl64(h2, 31); h2 += h1; h2 = h2 * 5 + 0x38495ab5;
	}

	//----------
	// tail

	const uint8_t * tail = (const uint8_t *)(data + nblocks * 16);

	uint64_t k1 = 0;
	uint64_t k2 = 0;

	switch (len & 15) {
		case 15: k2 ^= (uint64_t)(tail[14]) << 48;
		case 14: k2 ^= (uint64_t)(tail[13]) << 40;
		case 13: k2 ^= (uint64_t)(tail[12]) << 32;
		case 12: k2 ^= (uint64_t)(tail[11]) << 24;
		case 11: k2 ^= (uint64_t)(tail[10]) << 16;
		case 10: k2 ^= (uint64_t)(tail[9]) << 8;
		case  9: k2 ^= (uint64_t)(tail[8]) << 0;
			k2 *= c2; k2 = rotl64(k2, 33); k2 *= c1; h2 ^= k2;

		case  8: k1 ^= (uint64_t)(tail[7]) << 56;
		case  7: k1 ^= (uint64_t)(tail[6]) << 48;
		case  6: k1 ^= (uint64_t)(tail[5]) << 40;
		case  5: k1 ^= (uint64_t)(tail[4]) << 32;
		case  4: k1 ^= (uint64_t)(tail[3]) << 24;
		case  3: k1 ^= (uint64_t)(tail[2]) << 16;
		case  2: k1 ^= (uint64_t)(tail[1]) << 8;
		case  1: k1 ^= (uint64_t)(tail[0]) << 0;
			k1 *= c1; k1 = rotl64(k1, 31); k1 *= c2; h1 ^= k1;
	};

	//----------
	// finalization

	h1 ^= len; h2 ^= len;

	h1 += h2;
	h2 += h1;

	h1 = fmix64(h1);
	h2 = fmix64(h2);

	h1 += h2;
	h2 += h1;

	((uint64_t *)out)[0] = h1;
	((uint64_t *)out)[1] = h2;
}

}

//Simple pair to accommodate the 128 bit hash results.
struct uint128 { uint64_t h1; uint64_t h2; };

//Interprets nKeyBytes worth of key data using murmur_x86_32 and returns the
//hash.
inline uint32_t hash32(const void * key, int nKeyBytes, uint32_t seed = DEFAULT_SEED) {
	if (!key) {
		throw std::invalid_argument("key cannot be null");
	}

	uint32_t hash;
	MurmurHash3::murmur_x86_32(key, nKeyBytes, seed, &hash);
	return hash;
}

//Interprets key using murmur_x86_32 and returns the hash.
inline uint32_t hash32(uint32_t key, uint32_t seed = DEFAULT_SEED) {

	uint32_t hash;
	MurmurHash3::murmur_x86_32(&key, sizeof(uint32_t), seed, &hash);
	return hash;
}

//Interprets the key string as c_str using murmur_x86_128 and returns the hash.
inline uint32_t hash32(const std::string & key, uint32_t seed = DEFAULT_SEED) {
	uint32_t hash;
	MurmurHash3::murmur_x86_32(key.c_str(), static_cast<int>(key.length()), seed, &hash); //leave off the \0
	return hash;
}

//Interprets nKeyBytes worth of key data using murmur_x86_32 and returns the
//hash.
inline uint128 hash64(const void * key, int nKeyBytes, uint32_t seed = DEFAULT_SEED) {
	if (!key) {
		throw std::invalid_argument("key cannot be null");
	}

	uint64_t hash[2]{};
	MurmurHash3::murmur_x64_128(key, nKeyBytes, seed, &hash);
	return uint128{ hash[0], hash[1] };
}

//Interprets key using murmur_x86_32 and returns the hash
inline uint128 hash64(uint32_t key, uint32_t seed = DEFAULT_SEED) {
	uint64_t hash[2]{};
	MurmurHash3::murmur_x64_128(&key, sizeof(uint32_t), seed, &hash);
	return uint128{ hash[0], hash[1] };
}

//Interprets the key string as c_str using murmur_x86_128 and returns the hash.
inline uint128 hash64(const std::string & key, uint32_t seed = DEFAULT_SEED) {
	uint64_t hash[2]{};
	MurmurHash3::murmur_x64_128(key.c_str(), static_cast<int>(key.length()), seed, &hash); //leave off the \0
	return uint128{ hash[0], hash[1] };
}

}

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
		//if an item already exists, does nothing and returns false
		bool push(const T * item, unsigned long long hashKey);

		//Transverses node sequence and sets dest to item at hashkey.
		//returns false if no item is found
		bool peek(unsigned long long hashKey, const T ** dest) const;

		//Transverses node sequence until it finds node with hashkey.
		//"Removes" node by assigning its successor as the successor of its predecessor.
		//sets dest to item replaced and returns false if no item is found with hashkey
		bool pop(unsigned long long hashKey, const T ** dest);

		//Transverses node sequence...
		//...if it finds a corresponding node, replaces that node with a new node
		//with item and hashkey, then sets dest to item that was replaced and
		//returns true, if it does not find a node with hashkey, it adds the
		//item and returns false
		bool set(const T * item, unsigned long long hashKey, const T ** dest);

		//Returns if the slot contains the item, and sets *keyDest to the hashkey
		bool contains(const T * item, unsigned long long * keyDest) const;

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
	void add(const T * item, const void * key, int nBytes, int seed = DEFAULT_SEED);
	void add(const T * item, int key, int seed = DEFAULT_SEED);
	//string.c_str() is used as the key data, not including the \0.
	void add(const T * item, const std::string & key, int seed = DEFAULT_SEED);

	//Takes hashKey % nSlots_ to find appropriate slot, and then pushes item
	//to that slot.
	void addByHash(const T * item, unsigned long long hashKey);

	//Hashes key and then forwards to getByHash.
	T * get(const void * key, int nBytes, int seed = DEFAULT_SEED) const;
	T * get(int key, int seed = DEFAULT_SEED) const;
	//string.c_str() is used as the key data, not including the \0
	T * get(const std::string & key, int seed = DEFAULT_SEED) const;

	//Takes hashKey % nSlots_ to find appropriate slot, and then peeks with
	//hashKey for item.
	T * getByHash(unsigned long long hashKey) const;

	//Hashes key and then forwards to setByHash.
	T * set(const T * item, const void * key, int nBytes, int seed = DEFAULT_SEED);
	T * set(const T * item, int key, int seed = DEFAULT_SEED);
	//string.c_str() is used as the key data, not including the \0.
	T * set(const T * item, const std::string & key, int seed = DEFAULT_SEED);

	//Takes hashKey % nSlots_ to find appropriate slot, and then sets that slot
	//with item and hashKey. If node.set returns null, then there was no
	//pre-existing item with that hashkey, and it was added.
	T * setByHash(const T * item, unsigned long long hashKey);

	//Hashes key and then forwards to removeByHash.
	T * remove(const void * key, int nBytes, int seed = DEFAULT_SEED);
	T * remove(int key, int seed = DEFAULT_SEED);
	//string.c_str() is used as the key data, not including the \0.
	T * remove(const std::string & key, int seed = DEFAULT_SEED);

	//Returns if the table contains the item, and sets keyDest to the hashkey
	bool contains(const T * item, unsigned long long * keyDest = nullptr) const;

	//Takes hashKey % nSlots_ to find appropriate slot, and then pops hashkey
	//in that slot.
	T * removeByHash(unsigned long long hashKey);

	//Resizes the table so that there are nSlots slots.
	//All items are re-organized.
	//Relatively expensive method.
	void resize(int nSlots);

	//Returns whether the two tables are equivalent in size and content
	bool equals(const HashTable<T> & other) const;

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
	struct HashTableStats { int min, max, median; float mean, stddev; };
	HashTableStats stats(std::ostream & os) const;

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
	size_(0)
{}

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
bool HashTable<T>::Slot::push(const T * item, unsigned long long hashKey) {
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
bool HashTable<T>::Slot::peek(unsigned long long hashKey, const T ** dest) const {
	Node * node = first_;
	while (node && node->hashKey_ < hashKey) {
		node = node->next_;
	}
	if (node && node->hashKey_ == hashKey) {
		*dest = node->item_;
		return true;
	}
	return false;
}

template <typename T>
bool HashTable<T>::Slot::pop(unsigned long long hashKey, const T ** dest) {
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
bool HashTable<T>::Slot::set(const T * item, unsigned long long hashKey, const T ** dest) {
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
bool HashTable<T>::Slot::contains(const T * item, unsigned long long * keyDest) const {
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
	Node * node = first_;

	os << "N:" << size_;

	while (node) {
		os << "(";
		if (value) {
			os << *node->item_;
		}
		if (hash) {
			if (value) {
				os << ", ";
			}
			os << (unsigned long long)node->hashKey_ << ", ";
		}
		if (address) {
			if (value || hash) {
				os << ", ";
			}
			os << std::hex << (void *)(node->item_) << std::dec;
		}
		os << ")";

		node = node->next_;
	}
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
	nSlots_(other.nSlots_)
{}

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
void HashTable<T>::add(const T * item, const void * key, int nBytes, int seed) {
	addByHash(item, QHash::hash32(key, nBytes, seed));
}

template <typename T>
void HashTable<T>::add(const T * item, int key, int seed) {
	addByHash(item, QHash::hash32(key, seed));
}

template <typename T>
void HashTable<T>::add(const T * item, const std::string & key, int seed) {
	addByHash(item, QHash::hash32(key, seed));
}

template <typename T>
void HashTable<T>::addByHash(const T * item, unsigned long long hashKey) {
	if (slots_[hashKey % nSlots_].push(item, hashKey)) {
		++size_;
	}
	else {
		throw PreexistingItemException();
	}
}

template <typename T>
T * HashTable<T>::get(const void * key, int nBytes, int seed) const {
	return getByHash(QHash::hash32(key, nBytes, seed));
}

template <typename T>
T * HashTable<T>::get(int key, int seed) const {
	return getByHash(QHash::hash32(key, seed));
}

template <typename T>
T * HashTable<T>::get(const std::string & key, int seed) const {
	return getByHash(QHash::hash32(key, seed));
}

template <typename T>
T * HashTable<T>::getByHash(unsigned long long hashKey) const {
	const T * item;
	if (!slots_[hashKey % nSlots_].peek(hashKey, &item)) {
		throw ItemNotFoundException();
	}
	return const_cast<T*>(item); //given as taken, as a non-const. only stored as const
}

template <typename T>
T * HashTable<T>::set(const T * item, const void * key, int nBytes, int seed) {
	return setByHash(item, QHash::hash32(key, nBytes, seed));
}

template <typename T>
T * HashTable<T>::set(const T * item, int key, int seed) {
	return setByHash(item, QHash::hash32(key, seed));
}

template <typename T>
T * HashTable<T>::set(const T * item, const std::string & key, int seed) {
	return setByHash(item, QHash::hash32(key, seed));
}

template <typename T>
T * HashTable<T>::setByHash(const T * item, unsigned long long hashKey) {
	const T * replaced;
	if (!slots_[hashKey % nSlots_].set(item, hashKey, &replaced) {
		++size_;
	}
	return const_cast<T*>(replaced); //given as taken, as a non-const. only stored as const
}

template <typename T>
T * HashTable<T>::remove(const void * key, int nBytes, int seed) {
	return removeByHash(QHash::hash32(key, nBytes, seed));
}

template <typename T>
T * HashTable<T>::remove(int key, int seed) {
	return removeByHash(QHash::hash32(key, seed));
}

template <typename T>
T * HashTable<T>::remove(const std::string & key, int seed) {
	return removeByHash(QHash::hash32(key, seed));
}

template <typename T>
T * HashTable<T>::removeByHash(unsigned long long hashKey) {
	const T * item;
	if (slots_[hashKey % nSlots_].pop(hashKey, &item)) {
		size_--;
	}
	else {
		throw ItemNotFoundException();
	}
	return const_cast<T*>(item); //given as taken, as a non-const. only stored as const
}

template <typename T>
bool HashTable<T>::contains(const T * item, unsigned long long * keyDest) const {
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

	HashTable<T> table(nSlots);

	const HashTable<T>::Slot::Node * node;
	for (int i = 0; i < nSlots_; ++i) {
		node = slots_[i].first();
		while (node) {
			table.addByHash(*node->item_, node->hashKey_);
			node = node->next_;
		}
	}

	//delete old stuff??

	*this = std::move(table);
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
int HashTable<T>::nSlots() const {
	return nSlots_;
}

template <typename T>
int HashTable<T>::size() const {
	return size_;
}

template <typename T>
void HashTable<T>::printContents(std::ostream & os, bool value, bool hash, bool address) const {
	for (int s = 0; s < nSlots_; ++s) {
		os << "[" << s << "]: ";
		slots_[s].printContents(os, value, hash, address);
		os << std::endl;
	}
}

template <typename T>
typename HashTable<T>::HashTableStats HashTable<T>::stats(std::ostream & os) const {
	int min = slots_[0].size();
	int max = slots_[0].size();
	int median = slots_[0].size();
	float mean = slots_[0].size();
	float stddev = 0;

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

	int * sizeCounts = new int[max - min];
	memset(sizeCounts, 0, (max - min) * sizeof(int));
	for (int i = 0; i < nSlots_; ++i) {
		++sizeCounts[slots_[i].size() + min];

		stddev += (slots_[i].size() - mean) * (slots_[i].size() - mean);
	}
	stddev /= nSlots_;
	stddev = sqrt(stddev);

	median = min;
	for (int i = 1; i < max - min; ++i) {
		if (sizeCounts[i] > sizeCounts[median - min]) {
			median = i + min;
		}
	}

	delete[] sizeCounts;

	return { min, max, median, mean, stddev };
}

}