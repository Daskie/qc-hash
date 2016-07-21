#include <string>
#include <iostream>

#include "QHash.h"
#include "QHashTable.h"

using std::string;
using std::cout;
using std::endl;

using QHash::HashTable;

HashTable<int> TABLE1(5);
HashTable<string> TABLE2(5);
HashTable<int *> TABLE3(5);
HashTable<char> TABLE4(100000);

void setupTables() {
	for (int i = 0; i < 10; ++i) {
		TABLE1.addByHash(i, i);
		TABLE2.addByHash("" + i, i);
		TABLE3.addByHash(new int(i), i);
	}
	for (int i = 0; i < 1000000; ++i) {
		TABLE4.addByHash(i % 256, i);
	}
}

bool testDefaultConstructor() {
	HashTable<int> ht;
	if (ht.nSlots() != QHash::DEFAULT_NSLOTS || ht.size() != 0) return false;	
	return true;
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

	cout << "int *..." << endl;
	HashTable<int *> ht3(TABLE3);
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

	cout << "int *..." << endl;
	HashTable<int *> ht3 = TABLE3;
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

	cout << "int *..." << endl;
	HashTable<int *> h3(TABLE3);
	HashTable<int *> ht3(std::move(h3));
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

	cout << "int *..." << endl;
	HashTable<int *> h3(TABLE3);
	HashTable<int *> ht3 = std::move(h3);
	if (&ht3 == &TABLE3 || !ht3.equals(TABLE3)) return false;

	cout << "huge..." << endl;
	HashTable<char> h4(TABLE4);
	HashTable<char> ht4 = std::move(h4);
	if (&ht4 == &TABLE4 || !ht4.equals(TABLE4)) return false;

	return true;
}

bool testVariadicConstructor() {
	cout << "uniform..." << endl;
	HashTable<int> ht1(
		6,
		0, 5,
		1, 4,
		2, 3,
		3, 2,
		4, 1,
		5, 0
	);
	for (int i = 0; i < 5; ++i) {
		if (ht1.get(i) != 5 - i) return false;
	}

	cout << "varying..." << endl;
	HashTable<int> ht2(
		6,
		0, 7,
		1, "abc",
		2, 'd',
		3, 9.9,
		4, 7.0f,
		5, std::string("five")
	);
	if (ht2.get(7) != 0 ||
		ht2.get("abc") != 1 ||
		ht2.get('d') != 2 ||
		ht2.get(9.9) != 3 ||
		ht2.get(7.0f) != 4 ||
		ht2.get(std::string("five")) != 5)
		return false;

	cout << "single pair..." << endl;
	HashTable<int> ht3(1, 77, 777);
	if (ht3.get(777) != 77) return false;

	return true;
}

bool testDestructor() {
	HashTable<long long> * ht1 = new HashTable<long long>(1000);
	for (int i = 0; i < 10000; ++i) {
		ht1->addByHash(i, i);
	}
	delete ht1;
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
		ht1.add(arr[i], arr + i, 1);
	}
	if (ht1.size() != 10) return false;

	cout << "int..." << endl;
	for (int i = 10; i < 20; ++i) {
		ht1.add(arr[i], arr[i]);
	}
	if (ht1.size() != 20) return false;

	cout << "string..." << endl;
	for (int i = 20; i < 30; ++i) {
		ht1.add(arr[i], string(1, char(i)));
	}
	if (ht1.size() != 30) return false;

	cout << "by hash..." << endl;
	for (int i = 30; i < 40; ++i) {
		ht1.addByHash(arr[i], i);
	}
	if (ht1.size() != 40) return false;

	return true;
}

bool testGet() {
	int arr[100];
	HashTable<int> ht1(5);
	for (int i = 0; i < 100; ++i) {
		arr[i] = i;
		ht1.add(arr[i], i);
	}

	cout << "void *..." << endl;
	for (int i = 0; i < 10; ++i) {
		if (ht1.get(arr + i, 1) != i) return false;
	}

	cout << "int..." << endl;
	for (int i = 10; i < 20; ++i) {
		if (ht1.get(i) != i) return false;
	}

	cout << "string..." << endl;
	int x = 777;
	ht1.add(x, "okay");
	if (ht1.get("okay") != 777) return false;

	cout << "by hash..." << endl;
	ht1.addByHash(arr[20], 12345);
	if (ht1.getByHash(12345) != arr[20]) return false;

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
	ht1.set(arr[10], arr + 10, 1);
	if (ht1.get(arr + 10, 1) != arr[10]) return false;

	cout << "int..." << endl;
	ht1.set(arr[20], arr[20]);
	if (ht1.get(arr[20]) != arr[20]) return false;

	cout << "string..." << endl;
	ht1.set(arr[30], "okay");
	if (ht1.get("okay") != arr[30]) return false;

	cout << "by hash..." << endl;
	ht1.setByHash(arr[50], 777);
	if (ht1.getByHash(777) != arr[50]) return false;

	return true;
}

bool testRemove() {
	int arr[100];
	for (int i = 0; i < 100; ++i) {
		arr[i] = i;
	}

	HashTable<int> ht1(10);
	for (int i = 0; i < 100; ++i) {
		ht1.add(arr[i], i);
	}

	cout << "void *..." << endl;
	for (int i = 0; i < 10; ++i) {
		if (ht1.remove(arr + i, 1) != i) return false;
	}
	if (ht1.size() != 90) return false;

	cout << "int..." << endl;
	for (int i = 10; i < 20; ++i) {
		if (ht1.remove(i) != i) return false;
	}
	if (ht1.size() != 80) return false;

	cout << "string..." << endl;
	ht1.add(arr[25], "okay");
	if (ht1.remove("okay") != arr[25]) return false;
	if (ht1.size() != 80) return false;

	cout << "by hash..." << endl;
	for (int i = 30; i < 40; ++i) {
		if (ht1.removeByHash(QHash::hash32(i, 0)) != i) return false;
	}
	if (ht1.size() != 70) return false;

	return true;
}

bool testHas() {
	int arr[100];
	HashTable<int> ht1(5);
	for (int i = 0; i < 100; ++i) {
		arr[i] = i;
		ht1.add(arr[i], i);
	}

	cout << "void *..." << endl;
	for (int i = 0; i < 10; ++i) {
		if (!ht1.has(arr + i, 1)) return false;
	}

	cout << "int..." << endl;
	for (int i = 10; i < 20; ++i) {
		if (!ht1.has(i)) return false;
	}

	cout << "string..." << endl;
	int x = 777;
	ht1.add(x, "okay");
	if (!ht1.has("okay")) return false;

	cout << "by hash..." << endl;
	ht1.addByHash(arr[20], 12345);
	if (!ht1.hasByHash(12345)) return false;

	return true;
}

bool testContains() {
	int arr[100];
	for (int i = 0; i < 100; ++i) {
		arr[i] = i;
	}

	HashTable<int> ht1(10);

	for (int i = 0; i < 10; ++i) {
		if (ht1.contains(arr[i])) return false;
		ht1.add(arr[i], i);
	}
	for (int i = 0; i < 10; ++i) {
		if (!ht1.contains(arr[i])) return false;
		ht1.remove(i);
	}
	for (int i = 0; i < 10; ++i) {
		if (ht1.contains(arr[i])) return false;
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
		ht1.addByHash(arr[i], i);
	}

	cout << "larger..." << endl;
	ht1.resize(50);
	if (ht1.nSlots() != 50 || ht1.size() != 100) return false;
	for (int i = 0; i < 100; ++i) {
		if (!ht1.contains(arr[i])) return false;
	}

	cout << "smaller..." << endl;
	ht1.resize(10);
	if (ht1.nSlots() != 10 || ht1.size() != 100) return false;
	for (int i = 0; i < 100; ++i) {
		if (!ht1.contains(arr[i])) return false;
	}

	return true;
}

bool testClear() {
	int arr[100];

	HashTable<int> ht1(20);
	for (int i = 0; i < 100; ++i) {
		ht1.addByHash(arr[i], i);
	}

	cout << "standard..." << endl;
	ht1.clear();
	if (ht1.size() != 0 || ht1.nSlots() != 20) return false;
	for (int i = 0; i < 100; ++i) {
		if (ht1.contains(arr[i])) return false;
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
		ht1.addByHash(arr[i], i);
	}

	cout << "equality..." << endl;
	HashTable<int> ht2(ht1);
	if (!ht2.equals(ht1)) return false;

	cout << "inequality..." << endl;
	ht2.removeByHash(0);
	if (ht2.equals(ht1)) return false;

	return true;
}

bool testAllPrimitiveTypes() {
	HashTable<int> ht(10);
	char c = 0;
	unsigned char uc = 1;
	short s = 2;
	unsigned short us = 3;
	int i = 4;
	unsigned int ui = 5;
	long l = 6;
	unsigned long ul = 7;
	long long ll = 8;
	unsigned long long ull = 9;
	float f = 10;
	double d = 11;

	cout << "add..." << endl;
	ht.add(0, c);
	ht.add(0, uc);
	ht.add(0, s);
	ht.add(0, us);
	ht.add(0, i);
	ht.add(0, ui);
	ht.add(0, l);
	ht.add(0, ul);
	ht.add(0, ll);
	ht.add(0, ull);
	ht.add(0, f);
	ht.add(0, d);

	cout << "get..." << endl;
	ht.get(c);
	ht.get(uc);
	ht.get(s);
	ht.get(us);
	ht.get(i);
	ht.get(ui);
	ht.get(l);
	ht.get(ul);
	ht.get(ll);
	ht.get(ull);
	ht.get(f);
	ht.get(d);

	cout << "set..." << endl;
	ht.set(0, c);
	ht.set(0, uc);
	ht.set(0, s);
	ht.set(0, us);
	ht.set(0, i);
	ht.set(0, ui);
	ht.set(0, l);
	ht.set(0, ul);
	ht.set(0, ll);
	ht.set(0, ull);
	ht.set(0, f);
	ht.set(0, d);

	cout << "remove..." << endl;
	ht.remove(c);
	ht.remove(uc);
	ht.remove(s);
	ht.remove(us);
	ht.remove(i);
	ht.remove(ui);
	ht.remove(l);
	ht.remove(ul);
	ht.remove(ll);
	ht.remove(ull);
	ht.remove(f);
	ht.remove(d);

	cout << "has..." << endl;
	ht.has(c);
	ht.has(uc);
	ht.has(s);
	ht.has(us);
	ht.has(i);
	ht.has(ui);
	ht.has(l);
	ht.has(ul);
	ht.has(ll);
	ht.has(ull);
	ht.has(f);
	ht.has(d);

	return true;
}

bool testReferenceNature() {
	HashTable<int> ht1(10);
	ht1.add(7, 777);
	ht1.get(777) = 8;
	if (ht1.get(777) != 8) {
		return false;
	}
	int * ip = &ht1.get(777);
	*ip = 9;
	if (ht1.get(777) != 9) {
		return false;
	}

	return true;
}

bool testErrorThrows() {
	HashTable<int> ht1(10);

	cout << "ItemNotFound..." << endl;
	try {
		ht1.get(0);
		return false;
	}
	catch (QHash::ItemNotFoundException ex) {}
	try {
		ht1.remove(0);
		return false;
	}
	catch (QHash::ItemNotFoundException ex) {}

	cout << "PreexistingItem..." << endl;
	try {
		ht1.add(0, 0);
		ht1.add(1, 0);
		return false;
	}
	catch (QHash::PreexistingItemException ex) {}

	return true;
}

bool testSeedNature() {
	HashTable<int> ht1(100);

	try {
		for (int i = 0; i < 100; ++i) {
			ht1.setSeed(i * i);
			ht1.add(i, 0);
		}
	}
	catch (QHash::PreexistingItemException ex) {
		return false;
	}
	try {
		for (int i = 0; i < 100; ++i) {
			ht1.setSeed(i * i);
			if (ht1.get(0) != i) {
				return false;
			}
		}
	}
	catch (QHash::ItemNotFoundException ex) {
		return false;
	}

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
		ht1.addByHash(arr[i], i);
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
		ht3.addByHash(arr[i], i);
	}
	ht3.printContents(cout, true, true, true);

	cout << "too many slots..." << endl;
	HashTable<int> ht4(100);
	for (unsigned int i = 0; i < 100; ++i) {
		ht4.addByHash(arr[i], i);
	}
	ht4.printContents(cout, true, true, true);

	return true;
}

bool testIterator() {
	cout << "standard..." << endl;
	HashTable<int> ht1(10);
	for (int i = 0; i < 100; ++i) {
		ht1.addByHash(i, i);
	}
	HashTable<int>::Iterator it1 = ht1.iterator();
	int i = 0;
	while (it1.hasNext()) {
		if (it1.next() != i % 10 * 10 + i / 10) {
			return false;
		}
		++i;
	}

	cout << "sorted list..." << endl;
	HashTable<int> ht2(1);
	for (int i = 0; i < 10; ++i) {
		for (int j = 9; j >= 0; --j) {
			ht2.addByHash(i * 10 + j, i * 10 + j);
		}
	}
	HashTable<int>::Iterator it2 = ht2.iterator();
	i = 0;
	while (it2.hasNext()) {
		if (it2.next() != i) {
			return false;
		}
		++i;
	}

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
		ht1.add(arr[i], i);
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

	cout << "Testing Default Constructor..." << endl << endl;
	if (!testDefaultConstructor()) {
		cout << "Default Constructor Test Failed!" << endl;
		return false;
	}
	cout << endl;

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

	cout << "Testing Variadic Constructor..." << endl << endl;
	if (!testVariadicConstructor()) {
		cout << "Variadic Constructor Test Failed!" << endl;
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

	cout << "Testing Has..." << endl << endl;
	if (!testHas()) {
		cout << "Has Test Failed!" << endl;
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

	cout << "Testing All Primitive Types..." << endl << endl;
	if (!testAllPrimitiveTypes()) {
		cout << "All Primitive Types Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing Reference Nature..." << endl << endl;
	if (!testReferenceNature()) {
		cout << "Reference Nature Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing Error Throws..." << endl << endl;
	if (!testErrorThrows()) {
		cout << "Error Throws Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing Seed Nature..." << endl << endl;
	if (!testSeedNature()) {
		cout << "Seed Nature Test Failed!" << endl;
		return false;
	}
	cout << endl;

	cout << "Testing Iterator..." << endl << endl;
	if (!testIterator()) {
		cout << "Iterator Test Failed!" << endl;
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

#include <tuple>
#include <initializer_list>


template <typename T>
class Blah {

	public:

	template <typename K>
	Blah(T t, K k) {
		cout << t << " " << k << " last" << endl;
		size = 1;
	}

	template <typename K, typename... TKs>
	Blah(T t, K k, TKs... tks) :		
		Blah(tks...)
	{
		cout << t << " " << k << " " << sizeof...(tks) << endl;
		size = (1 + sizeof...(tks) / 2 > size) ? (1 + sizeof...(tks) / 2) : size;
	}

	int size;
};


int main() {

	//Blah<int> b(0, 88, 1, 9.1, 2, "wow");
	//cout << b.size << endl;
	//std::cin.get();
	//return 0;

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