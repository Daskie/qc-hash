#pragma once

#include "QHashAlgorithms.h"

#include <iostream>
#include <string>
#include <stdexcept>

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

			T * peek(unsigned long long hashKey);

			T * pop(unsigned long long hashKey);

			int size();

		};

	public:

		//Static Members

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

		void remove();

		template <typename K>
		T * get(const K & key, int nBytes, int seed = 0, const QHashAlgorithms::KeyDecoder & keyDecoder = DEFAULT_DECODER);

		T * get(const std::string & key, int seed = 0);

		T * get(unsigned int keyHash);

		void set();

		int size();

	protected:

	private:

		int size_; //total number of elements
		int nSlots_;
		Slot * slots_;
	};

	class ItemNotFoundException : public std::exception {};

	class HashKeyCollisionException : public std::exception {};

////////////////////////////////////////////////////////////////////////////////

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
			while (first_->next_ && first_->next_->hashKey_ < hashKey) {
				node = first_->next_;
			}
			if (node->next_->hashKey_ == hashKey) {
				throw HashKeyCollisionException();
			}
			node->next_ = new Node(item, hashKey, node->next_->next_);
		}

		size_++;
	}

	template <typename T>
	T * HashTable<T>::Slot::peek(unsigned long long hashKey) {
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
		return nullptr;
	}

	template <typename T>
	int HashTable<T>::Slot::size() {
		return size_;
	}

	//HashTable-----------------------------------------------------------------

	//Default Constructor
	template <typename T>
	HashTable<T>::HashTable(int nSlots) : nSlots_(nSlots), size_(0) {
		slots_ = new Slot[nSlots];

	}

	//Copy Constructor
	template <typename T>
	HashTable<T>::HashTable(const HashTable & other) {
		size_ = other.size_;
		nSlots_ = other.nSlots_;
		slots_ = new Slot[nSlots_];
		for (int i = 0; i < nSlots_; i++) {
			slots_[i] = other.slots_[i];
		}
	}

	//Assignment Overload
	template <typename T>
	HashTable<T> & HashTable<T>::operator=(const HashTable<T> & other) {
		size_ = other.size_;
		nSlots_ = other.nSlots_;
		slots_ = new Slot[nSlots_];
		for (int i = 0; i < nSlots_; i++) {
			slots_[i] = other.slots_[i];
		}

		return *this;
	}

	//Destructor
	template <typename T>
	HashTable<T>::~HashTable() {
		delete[] slots_;
	}

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
	void HashTable<T>::remove() {

	}

	template <typename T>
	template <typename K>
	T * HashTable<T>::get(const K & key, int nBytes, int seed, const QHashAlgorithms::KeyDecoder & keyDecoder) {
		QHashAlgorithms::KeyBundle kb(&key, nBytes);
		kb = keyDecoder.decode(kb);
		return get(QHashAlgorithms::hash32(kb, seed));
	}

	template <typename T>
	T * HashTable<T>::get(const std::string & key, int seed) {
		return get(key, 0, seed, QHashAlgorithms::STRING_KEY_DECODER);
	}

	template <typename T>
	T * HashTable<T>::get(unsigned int hashKey) {
		T * item = slots_[hashKey % nSlots_].peek(hashKey);
		if (!item) {
			throw ItemNotFoundException();
		}
		return item;
	}

	template <typename T>
	void HashTable<T>::set() {

	}

	template <typename T>
	int HashTable<T>::size() {
		return size_;
	}

}