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
	int arr[100];
	for (int i = 0; i < 100; ++i) {
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
		ht1.add(arr + i, string(1, char(i)));
	}
	if (ht1.size() != 30) return false;

	cout << "nullptr..." << endl;
	for (int i = 30; i < 40; ++i) {
		ht1.add(nullptr, i);
	}
	if (ht1.size() != 40) return false;

	cout << "by hash..." << endl;
	for (int i = 40; i < 50; ++i) {
		ht1.addByHash(arr + i, i);
	}
	if (ht1.size() != 50) return false;

	cout << "null key..." << endl;
	bool exThrown = false;
	try {
		ht1.add(nullptr, nullptr, 1);
	}
	catch (std::invalid_argument ex) {
		exThrown = true;
	}
	if (!exThrown) return false;

	cout << "preexisting item..." << endl;
	exThrown = false;
	try {
		ht1.add(arr + 50, 50);
		ht1.add(arr + 55, 50);
	}
	catch (QHash::PreexistingItemException ex){
		exThrown = true;
	}
	if (!exThrown) return false;

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
	bool exThrown = false;
	try {
		ht1.get(nullptr, 1);
	}
	catch (std::invalid_argument ex) {
		exThrown = true;
	}
	if (!exThrown) return false;

	cout << "by hash..." << endl;
	ht1.addByHash(arr + 20, 12345);
	if (*ht1.getByHash(12345) != arr[20]) return false;

	cout << "nonexistent..." << endl;
	exThrown = false;
	try {
		ht1.get("trololol");
	}
	catch (QHash::ItemNotFoundException ex) {
		exThrown = true;
	}
	if (!exThrown) return false;

	return true;
}

bool testSet() {
	int arr[100];
	for (int i = 0; i < 100; ++i) {
		arr[i] = i;
	}
	HashTable<int> ht1(5);

	cout << "void *..." << endl;
	unsigned long long hash = QHash::hash32(arr + 10, 4);
	ht1.set(arr + 10, arr + 10, sizeof(int));
	if (*ht1.get(arr + 10, sizeof(int)) != arr[10]) return false;

	cout << "int..." << endl;
	ht1.set(arr + 20, arr[20]);
	if (*ht1.get(arr[20]) != arr[20]) return false;

	cout << "string..." << endl;
	ht1.set(arr + 30, "okay");
	if (*ht1.get("okay") != arr[30]) return false;

	cout << "nullptr..." << endl;
	ht1.set(nullptr, arr[40]);
	if (ht1.get(arr[40])) return false;

	cout << "null key..." << endl;
	bool exThrown = false;
	try {
		ht1.set(nullptr, nullptr, 1);
	}
	catch (std::invalid_argument ex) {
		exThrown = true;
	}
	if (!exThrown) return false;

	cout << "by hash..." << endl;
	ht1.setByHash(arr + 50, 777);
	if (*ht1.getByHash(777) != arr[50]) return false;

	return true;
}

bool testRemove() {
	int arr[100];
	for (int i = 0; i < 100; ++i) {
		arr[i] = i;
	}

	HashTable<int> ht1(10);
	for (int i = 0; i < 100; ++i) {
		ht1.add(arr + i, i);
	}

	cout << "void *..." << endl;
	int * ip;
	for (int i = 0; i < 10; ++i) {
		ip = ht1.remove(arr + i, sizeof(int));
		if (*ip != i) return false;
	}
	if (ht1.size() != 90) return false;

	cout << "int..." << endl;
	for (int i = 10; i < 20; ++i) {
		ip = ht1.remove(i);
		if (*ip != i) return false;
	}
	if (ht1.size() != 80) return false;

	cout << "string..." << endl;
	ht1.add(arr + 25, "okay");
	ip = ht1.remove("okay");
	if (*ip != arr[25]) return false;
	if (ht1.size() != 80) return false;

	cout << "by hash..." << endl;
	for (int i = 30; i < 40; ++i) {
		ip = ht1.removeByHash(QHash::hash32(i));
		if (*ip != i) return false;
	}
	if (ht1.size() != 70) return false;

	cout << "null key..." << endl;
	bool exThrown = false;
	try {
		ip = ht1.remove(nullptr, 1);
	}
	catch (std::invalid_argument ex) {
		exThrown = true;
	}
	if (!exThrown) return false;

	cout << "nonexistent..." << endl;
	exThrown = false;
	try {
		ip = ht1.remove(-1);
	}
	catch (QHash::ItemNotFoundException ex) {
		exThrown = true;
	}
	if (!exThrown) return false;

	return true;
}

bool testContains() {
	int arr[100];
	for (int i = 0; i < 100; ++i) {
		arr[i] = i;
	}

	HashTable<int> ht1(10);

	cout << "standard..." << endl;
	for (int i = 0; i < 10; ++i) {
		if (ht1.contains(arr + i)) return false;
		ht1.add(arr + i, i);
	}
	for (int i = 0; i < 10; ++i) {
		if (!ht1.contains(arr + i)) return false;
		ht1.remove(i);
	}
	for (int i = 0; i < 10; ++i) {
		if (ht1.contains(arr + i)) return false;
	}

	return true;
}

bool testResize() {
	int arr[100];
	for (int i = 0; i < 100; ++i) {
		arr[i] = i;
	}

	HashTable<int> ht1(10);
	for (int i = 0; i < 100; ++i) {
		ht1.addByHash(arr + i, i);
	}

	cout << "larger..." << endl;
	ht1.resize(50);
	if (ht1.nSlots() != 50 || ht1.size() != 100) return false;
	for (int i = 0; i < 100; ++i) {
		if (!ht1.contains(arr + i)) return false;
	}

	cout << "smaller..." << endl;
	ht1.resize(10);
	if (ht1.nSlots() != 10 || ht1.size() != 100) return false;
	for (int i = 0; i < 100; ++i) {
		if (!ht1.contains(arr + i)) return false;
	}

	return true;
}

bool testClear() {
	int arr[100];

	HashTable<int> ht1(20);
	for (int i = 0; i < 100; ++i) {
		ht1.addByHash(arr + i, i);
	}

	cout << "standard..." << endl;
	ht1.clear();
	if (ht1.size() != 0 || ht1.nSlots() != 20) return false;
	for (int i = 0; i < 100; ++i) {
		if (ht1.contains(arr + i)) return false;
	}

	cout << "empty..." << endl;
	ht1.clear();
	if (ht1.size() != 0 || ht1.nSlots() != 20) return false;

	return true;
}

bool testEquals() {
	int arr[100];
	for (int i = 0; i < 100; ++i) {
		arr[i] = i;
	}
	
	HashTable<int> ht1(20);
	for (int i = 0; i < 100; ++i) {
		ht1.addByHash(arr + i, i);
	}

	cout << "equality..." << endl;
	HashTable<int> ht2(ht1);
	if (!ht2.equals(ht1)) return false;

	cout << "inequality..." << endl;
	ht2.removeByHash(0);
	if (ht2.equals(ht1)) return false;

	return true;
}

bool testPrintContents() {
	int arr[100];
	for (int i = 0; i < 100; ++i) {
		arr[i] = i;
	}

	cout << "standard..." << endl;
	HashTable<int> ht1(10);
	for (int i = 0; i < 30; ++i) {
		ht1.addByHash(arr + i, i);
	}
	cout << "value..." << endl;
	ht1.printContents(cout, true, false, false);
	cout << "key..." << endl;
	ht1.printContents(cout, false, true, false);
	cout << "address..." << endl;
	ht1.printContents(cout, false, false, true);
	cout << "value & key..." << endl;
	ht1.printContents(cout, true, true, false);
	cout << "key & address..." << endl;
	ht1.printContents(cout, false, true, true);
	cout << "value & address..." << endl;
	ht1.printContents(cout, true, false, true);
	cout << "all..." << endl;
	ht1.printContents(cout, true, true, true);

	cout << "empty..." << endl;
	HashTable<int> ht2(0);
	ht2.printContents(cout, true, true, true);

	cout << "too many items..." << endl;
	HashTable<int> ht3(3);
	for (int i = 0; i < 100; ++i) {
		ht3.addByHash(arr + i, i);
	}
	ht3.printContents(cout, true, true, true);

	cout << "too many slots..." << endl;
	HashTable<int> ht4(100);
	for (int i = 0; i < 100; ++i) {
		ht4.addByHash(arr + i, i);
	}
	ht4.printContents(cout, true, true, true);

	return true;
}

bool testStats() {
	cout << "standard..." << endl;
	int arr[1000];
	for (int i = 0; i < 1000; ++i) {
		arr[i] = i;
	}
	HashTable<int> ht1(100);
	for (int i = 0; i < 1000; ++i) {
		ht1.add(arr + i, i);
	}
	QHash::HashTable<int>::HashTableStats stats1 = ht1.stats();
	cout << "min:" << stats1.min << ", ";
	cout << "max:" << stats1.max << ", ";
	cout << "median:" << stats1.median << ", ";
	cout << "mean:" << stats1.mean << ", ";
	cout << "stddev:" << stats1.stddev << endl;
	HashTable<int>::printHisto(stats1, cout);

	cout << "empty..." << endl;
	HashTable<int> ht2(1);
	QHash::HashTable<int>::HashTableStats stats2 = ht2.stats();
	cout << "min:" << stats2.min << ", ";
	cout << "max:" << stats2.max << ", ";
	cout << "median:" << stats2.median << ", ";
	cout << "mean:" << stats2.mean << ", ";
	cout << "stddev:" << stats2.stddev << endl;
	HashTable<int>::printHisto(stats2, cout);

	/*
	cout << "huge..." << endl;
	HashTable<char> ht3(1000000);
	for (int i = 0; i < 10000000; ++i) {
		ht3.add(nullptr, i);
	}
	QHash::HashTable<char>::HashTableStats stats3 = ht3.stats();
	cout << "min:" << stats3.min << ", ";
	cout << "max:" << stats3.max << ", ";
	cout << "median:" << stats3.median << ", ";
	cout << "mean:" << stats3.mean << ", ";
	cout << "stddev:" << stats3.stddev << endl;
	HashTable<char>::printHisto(stats3, cout);
	*/

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

	cout << "Testing Clear..." << endl << endl;
	if (!testClear()) {
		cout << "Clear Test Failed!" << endl;
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