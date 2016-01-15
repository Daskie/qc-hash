#include "QHashTable.h"

namespace QHashTable {

//Stores a pointer to its item, that item's hashkey,
//and a pointer to the next node in the slot.
struct Node {

	Node(const void * item, unsigned long long hashKey, Node * next = nullptr) :
		item_(item), hashKey_(hashKey), next_(next) {}

	const void * item_;
	unsigned long long hashKey_;
	Node * next_;

};

//Contains a list of nodes that store each item and hashkey.
//Also known as a bucket.
class Slot {

	public:

	//Default Constructor
	Slot();

	//Copy Constructor
	Slot(const Slot & other);

	//Assignment Operator Overload
	Slot & operator=(const Slot & other);

	//Destructor
	~Slot();

	//Creates a new node for item and stores it in ascending order by hashkey.
	void push(const void * item, unsigned long long hashKey);

	//Transverses node sequence and returns item pointer of associated hashkey.
	const void * peek(unsigned long long hashKey) const;

	//Transverses node sequence until it finds node with hashkey.
	//"Removes" node by assigning its successor as the successor of its predecessor.
	const void * pop(unsigned long long hashKey);

	//Transverses node sequence...
	//...if it finds a corresponding node, replaces that node with a new node
	//with item and hashkey, then returns pointer to item that was replaced.
	//...if it does not find a node with hashkey, it adds the item and returns null.
	const void * set(const void * item, unsigned long long hashKey);

	//getter
	const Node * first() const;

	//getter
	int size() const;

	//Will attempt to os << *item, hashkey, and address based on bool keys.
	void printContents(std::ostream & os) const;

	private:

	//the first node in the sequence
	Node * first_;

	//the current number of nodes
	int size_;

	//IMPLEMENTATION////////////////////////////////////////////////////////////

	Slot() :
		first_(nullptr),
		size_(0) {}

	Slot(const Slot & other) {
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

	Slot & operator=(const Slot & other) {
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

	~Slot() {
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
	void push(const void * item, unsigned long long hashKey) {
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

	const void * peek(unsigned long long hashKey) const {
		Node * node = first_;
		while (node && node->hashKey_ <= hashKey) {
			if (node->hashKey_ == hashKey) {
				return node->item_;
			}
			node = node->next_;
		}
		return nullptr;
	}

	const void * pop(unsigned long long hashKey) {
		if (!first_) {
			return nullptr;
		}

		if (first_->hashKey_ == hashKey) {
			const void * item = first_->item_;
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
			const void * item = node->next_->item_;
			node->next_ = node->next_->next_;
			size_--;
			return item;
		}

		return nullptr;
	}

	//returns what it replaced, otherwise null
	const void * set(const void * item, unsigned long long hashKey) {
		if (!first_) {
			first_ = new Node(item, hashKey);
			++size_;
			return nullptr;
		}

		if (first_->hashKey_ == hashKey) {
			const void * tempI = first_->item_;
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
				const void * tempI = node->next_->item_;
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

	int size() const {
		return size_;
	}

	void printContents(std::ostream & os) const {
		Node * node = first_;

		os << "(N:" << size_ << ") ";

		while (node) {
			os << "[";
			os << " (K:" << (unsigned long long)node->hashKey_ << ")";
			os << std::hex << " (A:" << node->item_ << ")" << std::dec;
			os << "], ";

			node = node->next_;
		}
	}

};

}