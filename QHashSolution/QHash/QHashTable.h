#pragma once

#include "QHashAlgorithms.h"

#include <iostream>
#include <string>
#include <stdexcept>
#include <vector>

namespace QHashTable {

	template <typename T>
	class HashTable {

		struct Slot {

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

			int size() const;

			void printContents(std::ostream & os, bool item = false, bool hashKey = false, bool address = false) const;

		};

	public:

		//Static Members
		friend std::ostream & operator<<(std::ostream & os, const HashTable & hashTable) {
			return os << "[ HashTable: num slots: " << hashTable.nSlots_ << ", num items: " << hashTable.size_ << " ]";
		}

		//Default Constructor
		HashTable(int nSlots);

		//Copy Constructor
		HashTable(const HashTable & other);

		//Assignment Overload
		HashTable & operator=(const HashTable & other);

		//Destructor
		~HashTable();

		template <typename K>
		void add(T & item, const K & key, int nBytes = sizeof(K), int seed = 0);

		void add(T & item, const std::string & key, int seed = 0);

		T * removeByHash(unsigned int hashKey);

		template <typename K>
		T * remove(const K & key, int nBytes = sizeof(K), int seed = 0, const QHashAlgorithms::KeyDecoder & keyDecoder = QHashAlgorithms::DEFAULT_KEY_DECODER);

		T * remove(const std::string & key, int seed = 0);

		T * getByHash(unsigned int hashKey);

		template <typename K>
		T * get(const K & key, int nBytes, int seed = 0, const QHashAlgorithms::KeyDecoder & keyDecoder = QHashAlgorithms::DEFAULT_DECODER);

		T * get(const std::string & key, int seed = 0);

		void set();

		int size();

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
		if (size_ == 0) {
			first_ = new Node(item, hashKey);
		}

		else if (hashKey < first_->hashKey_) {
			first_ = new Node(item, hashKey, first_);
		}

		else {
			Node * node = first_;
			while (node->next_ && node->next_->hashKey_ < hashKey) {
				node = node->next_;
			}

			if (node->next_) {
				if (node->next_->hashKey_ == hashKey) {
					throw HashKeyCollisionException();
				}
				node->next_ = new Node(item, hashKey, node->next_);
			}

			else {
				node->next_ = new Node(item, hashKey);
			}
		}

		size_++;
	}

	template <typename T>
	T * HashTable<T>::Slot::peek(unsigned long long hashKey) const {
		Node * node = first_;
		while (node) {
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
		while (node->next_) {
			if (node->next_->hashKey_ == hashKey) {
				T * item = node->next_->item_;
				node->next_ = node->next_->next_;
				size_--;
				return item;
			}
			node = node->next_;
		}

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
		os << std::hex;
		while (node) {
			os << "[";
			if (item) {
				os << *node->item_;
			}
			if (hashKey) {
				os << " (K:" << (unsigned long long)node->hashKey_ << ")";
			}
			if (address) {
				os << " (A:" << (unsigned int)node->item_ << ")";
			}
			os << "], ";
			node = node->next_;
		}
		os << std::dec;
	}

	//HashTable-----------------------------------------------------------------

	//Default Constructor
	template <typename T>
	HashTable<T>::HashTable(int nSlots) : nSlots_(nSlots), size_(0) {
		slots_.resize(nSlots);
	}

	//Copy Constructor
	template <typename T>
	HashTable<T>::HashTable(const HashTable & other) {
		size_ = other.size_;
		nSlots_ = other.nSlots_;
		slots_(other.slots);
	}

	//Assignment Overload
	template <typename T>
	HashTable<T> & HashTable<T>::operator=(const HashTable<T> & other) {
		size_ = other.size_;
		nSlots_ = other.nSlots_;
		slots_ = other.slots_;

		return *this;
	}

	//Destructor
	template <typename T>
	HashTable<T>::~HashTable() {}

	template <typename T>
	template <typename K>
	void HashTable<T>::add(T & item, const K & key, int nBytes, int seed) {
		unsigned int hashKey = QHashAlgorithms::hash32(&key, nBytes, seed);
		slots_[hashKey % nSlots_].push(&item, hashKey);
		size_++;
	}

	template <typename T>
	void HashTable<T>::add(T & item, const std::string & key, int seed) {
		unsigned int hashKey = QHashAlgorithms::hash32(key, seed);
		slots_[hashKey % nSlots_].push(&item, hashKey);
		size_++;
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
	template <typename K>
	T * HashTable<T>::remove(const K & key, int nBytes, int seed, const QHashAlgorithms::KeyDecoder & keyDecoder) {
		QHashAlgorithms::KeyBundle kb(&key, nBytes);
		kb = keyDecoder.decode(kb);
		return removeByHash(QHashAlgorithms::hash32(kb, seed));
	}

	template <typename T>
	T * HashTable<T>::remove(const std::string & key, int seed) {
		return removeByHash(key, 0, seed, QHashAlgorithms::STRING_KEY_DECODER);
	}

	template <typename T>
	T * HashTable<T>::getByHash(unsigned int hashKey) {
		T * item = slots_[hashKey % nSlots_].peek(hashKey);
		if (!item) {
			throw ItemNotFoundException();
		}
		return item;
	}

	template <typename T>
	template <typename K>
	T * HashTable<T>::get(const K & key, int nBytes, int seed, const QHashAlgorithms::KeyDecoder & keyDecoder) {
		QHashAlgorithms::KeyBundle kb(&key, nBytes);
		kb = keyDecoder.decode(kb);
		return getByHash(QHashAlgorithms::hash32(kb, seed));
	}

	template <typename T>
	T * HashTable<T>::get(const std::string & key, int seed) {
		return getByHash(key, 0, seed, QHashAlgorithms::STRING_KEY_DECODER);
	}

	template <typename T>
	void HashTable<T>::set() {

	}

	template <typename T>
	int HashTable<T>::size() {
		return size_;
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