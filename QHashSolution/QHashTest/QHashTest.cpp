#include <string>
#include <iostream>

#include "QHash.h"

using std::string;
using std::cout;
using std::endl;

using QHash::HashTable;

class XXX {
	int x;
	string s;
};

HashTable<int> TABLE1(5);
HashTable<string> TABLE2(5);
HashTable<XXX> TABLE3(5);
HashTable<char> TABLE4(100000);

void setupTables() {
	for (int i = 0; i < 10; ++i) {
		TABLE1.addByHash(new int(i), i);
		TABLE2.addByHash(new string("" + i), i);
		TABLE3.addByHash(new XXX(), i);
	}
	for (int i = 0; i < 1000000; ++i) {
		TABLE4.addByHash(new char(i % 256), i);
	}
}

bool testConstructor() {
	cout << "small..." << endl;
	HashTable<int> ht1(10);
	if (ht1.nSlots() != 10 || ht1.size() != 0) return false;

	cout << "huge..." << endl;
	HashTable<int> ht2(100000);
	if (ht2.nSlots() != 100000 || ht2.size() != 0) return false;

	cout << "zero..." << endl;
	HashTable<int> ht3(0);
	if (ht3.nSlots() != 1 || ht3.size() != 0) return false;

	cout << "negative..." << endl;
	HashTable<int> ht4(-10);
	if (ht4.nSlots() != 1 || ht4.size() != 0) return false;

	return true;
}

bool testCopyConstructor() {
	cout << "int..." << endl;
	HashTable<int> ht1(TABLE1);
	if (&ht1 == &TABLE1 || !ht1.equals(TABLE1)) return false;

	cout << "string..." << endl;
	HashTable<string> ht2(TABLE2);
	if (&ht2 == &TABLE2 || !ht2.equals(TABLE2)) return false;

	cout << "XXX..." << endl;
	HashTable<XXX> ht3(TABLE3);
	if (&ht3 == &TABLE3 || !ht3.equals(TABLE3)) return false;

	cout << "huge..." << endl;
	HashTable<char> ht4(TABLE4);
	if (&ht4 == &TABLE4 || !ht4.equals(TABLE4)) return false;

	return true;
}

bool testCopyAssignment() {
	cout << "int..." << endl;
	HashTable<int> ht1 = TABLE1;
	if (&ht1 == &TABLE1 || !ht1.equals(TABLE1)) return false;

	cout << "string..." << endl;
	HashTable<string> ht2 = TABLE2;
	if (&ht2 == &TABLE2 || !ht2.equals(TABLE2)) return false;

	cout << "XXX..." << endl;
	HashTable<XXX> ht3 = TABLE3;
	if (&ht3 == &TABLE3 || !ht3.equals(TABLE3)) return false;

	cout << "huge..." << endl;
	HashTable<char> ht4 = TABLE4;
	if (&ht4 == &TABLE4 || !ht4.equals(TABLE4)) return false;

	return true;
}

bool testMoveConstructor() {
	cout << "int..." << endl;
	HashTable<int> h1(TABLE1);
	HashTable<int> ht1(std::move(h1));
	if (&ht1 == &TABLE1 || !ht1.equals(TABLE1)) return false;

	cout << "string..." << endl;
	HashTable<string> h2(TABLE2);
	HashTable<string> ht2(std::move(h2));
	if (&ht2 == &TABLE2 || !ht2.equals(TABLE2)) return false;

	cout << "XXX..." << endl;
	HashTable<XXX> h3(TABLE3);
	HashTable<XXX> ht3(std::move(h3));
	if (&ht3 == &TABLE3 || !ht3.equals(TABLE3)) return false;

	cout << "huge..." << endl;
	HashTable<char> h4(TABLE4);
	HashTable<char> ht4(std::move(h4));
	if (&ht4 == &TABLE4 || !ht4.equals(TABLE4)) return false;

	return true;
}

bool testMoveAssignment() {
	cout << "int..." << endl;
	HashTable<int> h1(TABLE1);
	HashTable<int> ht1 = std::move(h1);
	if (&ht1 == &TABLE1 || !ht1.equals(TABLE1)) return false;

	cout << "string..." << endl;
	HashTable<string> h2(TABLE2);
	HashTable<string> ht2 = std::move(h2);
	if (&ht2 == &TABLE2 || !ht2.equals(TABLE2)) return false;

	cout << "XXX..." << endl;
	HashTable<XXX> h3(TABLE3);
	HashTable<XXX> ht3 = std::move(h3);
	if (&ht3 == &TABLE3 || !ht3.equals(TABLE3)) return false;

	cout << "huge..." << endl;
	HashTable<char> h4(TABLE4);
	HashTable<char> ht4 = std::move(h4);
	if (&ht4 == &TABLE4 || !ht4.equals(TABLE4)) return false;

	return true;
}

bool testDestructor() {
	long long * arr = new long long[10000];
	HashTable<long long> * ht1 = new HashTable<long long>(1000);
	for (int i = 0; i < 10000; ++i) {
		ht1->addByHash(arr + i, i);
	}
	delete ht1;
	delete arr;
	return true;
}

bool testAdd() {
	int arr[1000];
	for (int i = 0; i < 1000; ++i) {
		arr[i] = i;
	}

	HashTable<int> ht1(10);

	cout << "void *..." << endl;
	for (int i = 0; i < 10; ++i) {
		ht1.add(arr + i, arr + i, sizeof(int));
	}
	if (ht1.size() != 10) return false;

	cout << "int..." << endl;
	for (int i = 10; i < 20; ++i) {
		ht1.add(arr + i, arr[i]);
	}
	if (ht1.size() != 20) return false;

	cout << "string..." << endl;
	for (int i = 20; i < 30; ++i) {
		ht1.add(arr + i, "" + i);
	}
	if (ht1.size() != 30) return false;

	cout << "nullptr..." << endl;
	for (int i = 30; i < 40; ++i) {
		ht1.add(nullptr, i);
	}
	if (ht1.size() != 40) return false;

	cout << "null key..." << endl;
	bool exThrown = false;
	try {
		ht1.add(nullptr, nullptr, 1);
	}
	catch (std::invalid_argument ex) {
		exThrown = true;
	}
	if (!exThrown) return false;

	cout << "******" << ht1.size() << endl;

	return true;
}

bool testGet() {
	int arr[100];
	HashTable<int> ht1(5);
	for (int i = 0; i < 100; ++i) {
		arr[i] = i;
		ht1.add(arr + i, i);
	}

	cout << "void *..." << endl;
	for (int i = 0; i < 10; ++i) {
		if (*ht1.get(arr + i, sizeof(int)) != i) return false;
	}

	cout << "int..." << endl;
	for (int i = 10; i < 20; ++i) {
		if (*ht1.get(i) != i) return false;
	}

	cout << "string..." << endl;
	int x = 777;
	ht1.add(&x, "okay");
	if (*ht1.get("okay") != 777) return false;

	cout << "null key..." << endl;


	return true;
}

bool testSet() {


	return true;
}

bool testRemove() {


	return true;
}

bool testContains() {


	return true;
}

bool testResize() {


	return true;
}

bool testEquals() {


	return true;
}

bool testPrintContents() {


	return true;
}

bool testStats() {


	return true;
}

bool runTests() {

	cout << "Testing Constructor..." << endl << endl;
	if (!testConstructor()) {
		cout << "Constructor Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing Copy Constructor..." << endl << endl;
	if (!testCopyConstructor()) {
		cout << "Copy Constructor Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing Copy Assignment..." << endl << endl;
	if (!testCopyAssignment()) {
		cout << "Copy Assignment Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing Move Constructor..." << endl << endl;
	if (!testMoveConstructor()) {
		cout << "Move Constructor Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing Move Assignment..." << endl << endl;
	if (!testMoveAssignment()) {
		cout << "Move Assignment Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing Destructor..." << endl << endl;
	if (!testDestructor()) {
		cout << "Destructor Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing Add..." << endl << endl;
	if (!testAdd()) {
		cout << "Add Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing Get..." << endl << endl;
	if (!testGet()) {
		cout << "Get Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing Set..." << endl << endl;
	if (!testSet()) {
		cout << "Set Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing Remove..." << endl << endl;
	if (!testRemove()) {
		cout << "Remove Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing Contains..." << endl << endl;
	if (!testContains()) {
		cout << "Contains Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing Resize..." << endl << endl;
	if (!testResize()) {
		cout << "Resize Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing Equals..." << endl << endl;
	if (!testEquals()) {
		cout << "Equals Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing PrintContents..." << endl << endl;
	if (!testPrintContents()) {
		cout << "PrintContents Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing Stats..." << endl << endl;
	if (!testStats()) {
		cout << "Stats Test Failed!" << endl;
		return false;
	}
	cout << endl;

	return true;
}

int main() {

	setupTables();

	if (runTests()) {
		cout << "All Tests Passed" << endl;
	}
	else {
		cout << "Testing Failed" << endl;
	}

	cout << endl;
	system("pause");
	return 0;
}