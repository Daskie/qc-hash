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

	template <typename T>
	class HashTable {

		class Slot {

		private:

			struct Node {

				T * item_;
				unsigned long long hashKey_;
				Node * next_;

				Node(T * item, unsigned long long hashKey, Node * next = nullptr) :
					item_(item), hashKey_(hashKey), next_(next) {}

			};

			Node * first_;
			int size_;

		public:

			Slot();

			Slot(const Slot & other);

			Slot & operator=(const Slot & other);

			~Slot();

			void push(T * item, unsigned long long hashKey);

			T * peek(unsigned long long hashKey) const;

			T * pop(unsigned long long hashKey);

			T * set(T * item, unsigned long long hashKey);

			int size() const;

			void printContents(std::ostream & os, bool item = false, bool hashKey = false, bool address = false) const;

		};

	public:

		//Static Members

		/**
		**
		**/
		friend std::ostream & operator<<(std::ostream & os, const HashTable & hashTable) {
			return os << "[ HashTable: num slots: " << hashTable.nSlots_ << ", num items: " << hashTable.size_ << " ]";
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

		template <typename K>
		void add(T & item, const K & key, int nBytes = sizeof(K), int seed = 0, const QHashAlgorithms::KeyDecoder & keyDecoder = QHashAlgorithms::DEFAULT_KEY_DECODER);

		void add(T & item, const std::string & key, int seed = 0);

		void addByHash(T & item, unsigned int hashkey);

		template <typename K>
		T * get(const K & key, int nBytes = sizeof(K), int seed = 0, const QHashAlgorithms::KeyDecoder & keyDecoder = QHashAlgorithms::DEFAULT_KEY_DECODER) const;

		T * get(const std::string & key, int seed = 0) const;

		T * getByHash(unsigned int hashKey) const;

		template <typename K>
		T * set(T & item, const K & key, int nBytes = sizeof(K), int seed = 0, const QHashAlgorithms::KeyDecoder & keyDecoder = QHashAlgorithms::DEFAULT_KEY_DECODER);

		T * set(T & item, const std::string & key, int seed = 0);

		T * setByHash(T & item, unsigned int hashKey);

		template <typename K>
		T * remove(const K & key, int nBytes = sizeof(K), int seed = 0, const QHashAlgorithms::KeyDecoder & keyDecoder = QHashAlgorithms::DEFAULT_KEY_DECODER);

		T * remove(const std::string & key, int seed = 0);

		T * removeByHash(unsigned int hashKey);

		int size() const;

		void stats(std::ostream & os) const;

		void printContents(std::ostream & os, bool item = false, bool hashKey = false, bool address = false) const;

	protected:

	private:

		int size_; //total number of elements
		int nSlots_;
		std::vector<Slot> slots_;
	};

	class ItemNotFoundException : public std::exception {};

	class HashKeyCollisionException : public std::exception {};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////																			

	//Slot----------------------------------------------------------------------

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
	void HashTable<T>::Slot::push(T * item, unsigned long long hashKey) {
		if (!first_) {
			first_ = new Node(item, hashKey);
			size_++;
			return;
		}

		if (hashKey < first_->hashKey_) {
			first_ = new Node(item, hashKey, first_);
			size_++;
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

		size_++;
	}

	template <typename T>
	T * HashTable<T>::Slot::peek(unsigned long long hashKey) const {
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
	T * HashTable<T>::Slot::pop(unsigned long long hashKey) {
		if (!first_) {
			return nullptr;
		}

		if (first_->hashKey_ == hashKey) {
			T * item = first_->item_;
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
			T * item = node->next_->item_;
			node->next_ = node->next_->next_;
			size_--;
			return item;
		}

		return nullptr;
	}

	template <typename T>
	T * HashTable<T>::Slot::set(T * item, unsigned long long hashKey) { //return what it replaced, otherwise null
		if (!first_) {
			first_ = new Node(item, hashKey);
			size_++;
			return nullptr;
		}

		if (first_->hashKey_ == hashKey) {
			T * tempI = first_->item_;
			Node * tempN = first_->next_;
			delete first_;
			first_ = new Node(item, hashKey, tempN);
			return tempI;
		}

		if (first_->hashKey_ > hashKey) {
			first_ = new Node(item, hashKey, first_);
			size_++;
			return nullptr;
		}

		Node * node = first_;
		while (node->next_ && node->next_->hashKey_ < hashKey) {
			node = node->next_;
		}
		if (node->next_) {
			if (node->next_->hashKey_ == hashKey) {
				T * tempI = node->next_->item_;
				Node * tempN = node->next_->next_;
				delete node->next_;
				node->next_ = new Node(item, hashKey, tempN);
				return tempI;
			}
			node->next_ = new Node(item, hashKey, node->next_);
			size_++;
			return nullptr;
		}
		node->next_ = new Node(item, hashKey);
		size_++;
		return nullptr;
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
				os << std::hex << " (A:" << (unsigned int)node->item_ << ")" << std::dec;
			}
			os << "], ";
			node = node->next_;
		}
	}

	//HashTable-----------------------------------------------------------------

	template <typename T>
	HashTable<T>::HashTable(int nSlots) : nSlots_(nSlots), size_(0) {
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
	HashTable<T>::~HashTable() {}

	template <typename T>
	template <typename K>
	void HashTable<T>::add(T & item, const K & key, int nBytes, int seed, const QHashAlgorithms::KeyDecoder & keyDecoder) {
		QHashAlgorithms::KeyBundle kb(&key, nBytes);
		kb = keyDecoder.decode(kb);
		addByHash(item, QHashAlgorithms::hash32(kb, seed));
	}

	template <typename T>
	void HashTable<T>::add(T & item, const std::string & key, int seed) {
		add(item, key, 0, seed, QHashAlgorithms::STRING_KEY_DECODER);
	}

	template <typename T>
	void HashTable<T>::addByHash(T & item, unsigned int hashKey) {
		slots_[hashKey % nSlots_].push(&item, hashKey);
		size_++;
	}

	template <typename T>
	template <typename K>
	T * HashTable<T>::get(const K & key, int nBytes, int seed, const QHashAlgorithms::KeyDecoder & keyDecoder) const {
		QHashAlgorithms::KeyBundle kb(&key, nBytes);
		kb = keyDecoder.decode(kb);
		return getByHash(QHashAlgorithms::hash32(kb, seed));
	}

	template <typename T>
	T * HashTable<T>::get(const std::string & key, int seed) const {
		return get<std::string>(key, 0, seed, QHashAlgorithms::STRING_KEY_DECODER);
	}

	template <typename T>
	T * HashTable<T>::getByHash(unsigned int hashKey) const {
		T * item = slots_[hashKey % nSlots_].peek(hashKey);
		if (!item) {
			throw ItemNotFoundException();
		}
		return item;
	}

	template <typename T>
	template <typename K>
	T * HashTable<T>::set(T & item, const K & key, int nBytes, int seed, const QHashAlgorithms::KeyDecoder & keyDecoder) {
		QHashAlgorithms::KeyBundle kb(&key, nBytes);
		kb = keyDecoder.decode(kb);
		return setByHash(item, QHashAlgorithms::hash32(kb, seed));
	}

	template <typename T>
	T * HashTable<T>::set(T & item, const std::string & key, int seed) {
		return set<std::string>(item, key, 0, seed, QHashAlgorithms::STRING_KEY_DECODER);
	}

	template <typename T>
	T * HashTable<T>::setByHash(T & item, unsigned int hashKey) {
		T * replaced = slots_[hashKey % nSlots_].set(&item, hashKey);
		if (!replaced) {
			size_++;
		}
		return replaced;
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
	T * HashTable<T>::removeByHash(unsigned int hashKey) {
		T * item = slots_[hashKey % nSlots_].pop(hashKey);
		if (!item) {
			throw ItemNotFoundException();
		}
		else {
			size_--;
		}
		return item;
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
		for (int i = 0; i < nSlots_; i++) {
			sizes[i] = slots_[i].size();
		}
		std::sort(sizes.begin(), sizes.end());

		int maxSize = sizes.back();
		int minSize = sizes.front();

		double mean = 0, meanLow10 = 0, meanHigh10 = 0;
		int low10i = std::ceil(sizes.size() * 0.1);
		int high10i = std::floor(sizes.size() * 0.9);
		for (int i = 0; i < low10i; i++) {
			mean += sizes[i];
			meanLow10 += sizes[i];
		}
		for (int i = low10i; i < high10i; i++) {
			mean += sizes[i];
		}
		for (int i = high10i; i < sizes.size(); i++) {
			mean += sizes[i];
			meanHigh10 += sizes[i];
		}
		mean /= sizes.size();
		meanLow10 /= low10i + 1;
		meanHigh10 /= sizes.size() - high10i;

		std::vector<int> sizeCounts(maxSize + 1);
		double variance = 0, deviation;
		for (int size : sizes) {
			sizeCounts[size]++;
			variance += (size - mean) * (size - mean);
		}
		variance /= sizes.size();
		deviation = std::sqrt(variance);

		int median = 0;
		int maxSizeCount = sizeCounts.front();
		int minSizeCount = sizeCounts.front();
		for (int i = 1; i < sizeCounts.size(); i++) {
			if (sizeCounts[i] > maxSizeCount) {
				maxSizeCount = sizeCounts[i];
				median = i;
			}
			else if (sizeCounts[i] < minSizeCount) {
				minSizeCount = sizeCounts[i];
			}
		}

		int maxCharLength = 50, numChars;
		int digits = std::floor(std::log10(sizeCounts.size() - 1));
		os << "Num Slots: " << nSlots_ << ", Num Items: " << size_ << std::endl;
		os << "Mean: " << mean << ", lower 10%: " << meanLow10 << ", upper 10%: " << meanHigh10 << std::endl;
		os << "Median: " << median << ", Min: " << minSize << ", Max: " << maxSize << std::endl;
		os << "Standard Deviation: " << deviation << ", Variance: " << variance << endl;
		for (int i = 0; i < sizeCounts.size(); i++) {
			os << "[";
			for (int j = 0; j < digits - std::max((int)std::floor(std::log10(i)), 0); j++) { //so the first column digits are aligned
				os << ' ';
			}
			os << i << "]: ";
			numChars = std::round((float)sizeCounts[i] / maxSizeCount * maxCharLength);
			for (int j = 0; j < numChars; j++) {
				os << '-';
			}
			os << " (" << sizeCounts[i] << ")" << endl;
		}

	}

	template <typename T>
	void HashTable<T>::printContents(std::ostream & os, bool item, bool hashKey, bool address) const {
		for (int s = 0; s < nSlots_; s++) {
			os << "[" << s << "]: ";
			slots_[s].printContents(os, item, hashKey, address);
			os << std::endl;
		}
	}

}