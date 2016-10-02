#include <string>
#include <iostream>

#include "QHash.h"
#include "QHashMap.h"

using std::string;
using std::cout;
using std::endl;

using namespace QHash;

HashMap<int> MAP1(5);
HashMap<string> MAP2(5);
HashMap<int *> MAP3(5);
HashMap<char> MAP4(100000);

void setupMaps() {
	for (int i = 0; i < 10; ++i) {
		MAP1.addByHash(i, i);
		MAP2.addByHash("" + i, i);
		MAP3.addByHash(new int(i), i);
	}
	for (int i = 0; i < 1000000; ++i) {
		MAP4.addByHash(i % 256, i);
	}
}

bool testDefaultConstructor() {
	HashMap<int> ht;
	if (ht.nSlots() != DEFAULT_NSLOTS || ht.size() != 0) return false;	
	return true;
}

bool testConstructor() {
	cout << "small..." << endl;
	HashMap<int> ht1(10);
	if (ht1.nSlots() != 10 || ht1.size() != 0) return false;

	cout << "huge..." << endl;
	HashMap<int> ht2(100000);
	if (ht2.nSlots() != 100000 || ht2.size() != 0) return false;

	cout << "zero..." << endl;
	HashMap<int> ht3(0);
	if (ht3.nSlots() != 1 || ht3.size() != 0) return false;

	return true;
}

bool testCopyConstructor() {
	cout << "int..." << endl;
	HashMap<int> ht1(MAP1);
	if (&ht1 == &MAP1 || !ht1.equals(MAP1)) return false;

	cout << "string..." << endl;
	HashMap<string> ht2(MAP2);
	if (&ht2 == &MAP2 || !ht2.equals(MAP2)) return false;

	cout << "int *..." << endl;
	HashMap<int *> ht3(MAP3);
	if (&ht3 == &MAP3 || !ht3.equals(MAP3)) return false;

	cout << "huge..." << endl;
	HashMap<char> ht4(MAP4);
	if (&ht4 == &MAP4 || !ht4.equals(MAP4)) return false;

	return true;
}

bool testCopyAssignment() {
	cout << "int..." << endl;
	HashMap<int> ht1 = MAP1;
	if (&ht1 == &MAP1 || !ht1.equals(MAP1)) return false;

	cout << "string..." << endl;
	HashMap<string> ht2 = MAP2;
	if (&ht2 == &MAP2 || !ht2.equals(MAP2)) return false;

	cout << "int *..." << endl;
	HashMap<int *> ht3 = MAP3;
	if (&ht3 == &MAP3 || !ht3.equals(MAP3)) return false;

	cout << "huge..." << endl;
	HashMap<char> ht4 = MAP4;
	if (&ht4 == &MAP4 || !ht4.equals(MAP4)) return false;

	return true;
}

bool testMoveConstructor() {
	cout << "int..." << endl;
	HashMap<int> h1(MAP1);
	HashMap<int> ht1(std::move(h1));
	if (&ht1 == &MAP1 || !ht1.equals(MAP1)) return false;

	cout << "string..." << endl;
	HashMap<string> h2(MAP2);
	HashMap<string> ht2(std::move(h2));
	if (&ht2 == &MAP2 || !ht2.equals(MAP2)) return false;

	cout << "int *..." << endl;
	HashMap<int *> h3(MAP3);
	HashMap<int *> ht3(std::move(h3));
	if (&ht3 == &MAP3 || !ht3.equals(MAP3)) return false;

	cout << "huge..." << endl;
	HashMap<char> h4(MAP4);
	HashMap<char> ht4(std::move(h4));
	if (&ht4 == &MAP4 || !ht4.equals(MAP4)) return false;

	return true;
}

bool testMoveAssignment() {
	cout << "int..." << endl;
	HashMap<int> h1(MAP1);
	HashMap<int> ht1 = std::move(h1);
	if (&ht1 == &MAP1 || !ht1.equals(MAP1)) return false;

	cout << "string..." << endl;
	HashMap<string> h2(MAP2);
	HashMap<string> ht2 = std::move(h2);
	if (&ht2 == &MAP2 || !ht2.equals(MAP2)) return false;

	cout << "int *..." << endl;
	HashMap<int *> h3(MAP3);
	HashMap<int *> ht3 = std::move(h3);
	if (&ht3 == &MAP3 || !ht3.equals(MAP3)) return false;

	cout << "huge..." << endl;
	HashMap<char> h4(MAP4);
	HashMap<char> ht4 = std::move(h4);
	if (&ht4 == &MAP4 || !ht4.equals(MAP4)) return false;

	return true;
}

bool testVariadicConstructor() {
	cout << "uniform..." << endl;
	HashMap<int> ht1(
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
	HashMap<int> ht2(
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
	HashMap<int> ht3(1, 77, 777);
	if (ht3.get(777) != 77) return false;

	return true;
}

bool testDestructor() {
	HashMap<long long> * ht1 = new HashMap<long long>(1000);
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

	HashMap<int> ht1(10);

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
	HashMap<int> ht1(5);
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
	HashMap<int> ht1(5);

	cout << "void *..." << endl;
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

	HashMap<int> ht1(10);
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
		if (ht1.removeByHash(hash(i, 0)) != i) return false;
	}
	if (ht1.size() != 70) return false;

	return true;
}

bool testHas() {
	int arr[100];
	HashMap<int> ht1(5);
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

	HashMap<int> ht1(10);

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

	HashMap<int> ht1(10);
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

	HashMap<int> ht1(20);
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
	
	HashMap<int> ht1(20);
	for (int i = 0; i < 100; ++i) {
		ht1.addByHash(arr[i], i);
	}

	cout << "equality..." << endl;
	HashMap<int> ht2(ht1);
	if (!ht2.equals(ht1)) return false;

	cout << "inequality..." << endl;
	ht2.removeByHash(0);
	if (ht2.equals(ht1)) return false;

	return true;
}

bool testAllPrimitiveTypes() {
	HashMap<int> ht(10);
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
	HashMap<int> ht1(10);
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
	HashMap<int> ht1(10);

	cout << "ItemNotFound..." << endl;
	try {
		ht1.get(0);
		return false;
	}
	catch (ItemNotFoundException ex) {}
	try {
		ht1.remove(0);
		return false;
	}
	catch (ItemNotFoundException ex) {}

	cout << "PreexistingItem..." << endl;
	try {
		ht1.add(0, 0);
		ht1.add(1, 0);
		return false;
	}
	catch (PreexistingItemException ex) {}

	return true;
}

bool testSeedNature() {
	HashMap<int> ht1(100);

	try {
		for (int i = 0; i < 100; ++i) {
			ht1.setSeed(i * i);
			ht1.add(i, 0);
		}
	}
	catch (PreexistingItemException ex) {
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
	catch (ItemNotFoundException ex) {
		return false;
	}

	return true;
}

bool testPrecisions() {
	cout << "x32..." << endl;
	HashMap<int, x32> ht1(10);
	ht1.add(7, 99);
	if (ht1.get(99) != 7) return false;
	if (ht1.hasByHash((x32)hash<int, x64>(99, DEFAULT_SEED))) return false;
	if (ht1.hasByHash((x32)(x64)hash<int, x128>(99, DEFAULT_SEED))) return false;

	cout << "x64..." << endl;
	HashMap<int, x64> ht2(10);
	ht2.add(7, 99);
	if (ht2.get(99) != 7) return false;
	if (ht2.hasByHash((x64)hash<int, x32>(99, DEFAULT_SEED))) return false;
	//if (ht2.hasByHash((x64)hash<int, x128>(99, DEFAULT_SEED))) return false; //identical implementation. would fail every time.

	cout << "x128..." << endl;
	HashMap<int, x128> ht3(10);
	ht3.add(7, 99);
	if (ht3.get(99) != 7) return false;
	if (ht3.hasByHash((x128)hash<int, x32>(99, DEFAULT_SEED))) return false;
	if (ht3.hasByHash((x128)hash<int, x64>(99, DEFAULT_SEED))) return false;

	return true;
}

bool testPrintContents() {
	int arr[100];
	for (int i = 0; i < 100; ++i) {
		arr[i] = i;
	}

	cout << "standard..." << endl;
	HashMap<int> ht1(10);
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
	HashMap<int> ht2(0);
	ht2.printContents(cout, true, true, true);

	cout << "too many items..." << endl;
	HashMap<int> ht3(3);
	for (int i = 0; i < 100; ++i) {
		ht3.addByHash(arr[i], i);
	}
	ht3.printContents(cout, true, true, true);

	cout << "too many slots..." << endl;
	HashMap<int> ht4(100);
	for (unsigned int i = 0; i < 100; ++i) {
		ht4.addByHash(arr[i], i);
	}
	ht4.printContents(cout, true, true, true);

	return true;
}

bool testIterator() {
	struct Test { int v; };
	cout << "standard..." << endl;
	HashMap<Test> ht1(10);
	for (int i = 0; i < 100; ++i) {
		ht1.addByHash({ i }, i);
	}
	int i = 0;
	for (auto it = ht1.begin(); it; ++it) {
		if (it->v != i % 10 * 10 + i / 10) return false;
		if ((*it).v != it->v) return false;
		(*it).v *= 2;
		++i;
	}

	cout << "const..." << endl;
	const HashMap<Test> * htp = &ht1;
	i = 0;
	for (auto it = htp->cbegin(); it; ++it) {
		if (it->v != 2 * (i % 10 * 10 + i / 10)) return false;
		if ((*it).v != it->v) return false;
		++i;
	}

	//cout << "conversion..." << endl;
	//HashMap<int>::MIterator mit = ht1.begin();
	//HashMap<int>::CIterator cit = ht1.cbegin();
	//cit = mit;

	return true;
}

bool testStats() {
	cout << "standard..." << endl;
	int arr[1000];
	for (int i = 0; i < 1000; ++i) {
		arr[i] = i;
	}
	HashMap<int> ht1(100);
	for (int i = 0; i < 1000; ++i) {
		ht1.add(arr[i], i);
	}
	HashMap<int>::HashMapStats stats1 = ht1.stats();
	cout << "min:" << stats1.min << ", ";
	cout << "max:" << stats1.max << ", ";
	cout << "median:" << stats1.median << ", ";
	cout << "mean:" << stats1.mean << ", ";
	cout << "stddev:" << stats1.stddev << endl;
	HashMap<int>::printHisto(stats1, cout);

	cout << "empty..." << endl;
	HashMap<int> ht2(1);
	HashMap<int>::HashMapStats stats2 = ht2.stats();
	cout << "min:" << stats2.min << ", ";
	cout << "max:" << stats2.max << ", ";
	cout << "median:" << stats2.median << ", ";
	cout << "mean:" << stats2.mean << ", ";
	cout << "stddev:" << stats2.stddev << endl;
	HashMap<int>::printHisto(stats2, cout);


	/*
	cout << "huge..." << endl;
	HashMap<char> ht3(1000000);
	for (int i = 0; i < 10000000; ++i) {
		ht3.add(nullptr, i);
	}
	HashMap<char>::HashMapStats stats3 = ht3.stats();
	cout << "min:" << stats3.min << ", ";
	cout << "max:" << stats3.max << ", ";
	cout << "median:" << stats3.median << ", ";
	cout << "mean:" << stats3.mean << ", ";
	cout << "stddev:" << stats3.stddev << endl;
	HashMap<char>::printHisto(stats3, cout);
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

	cout << "Testing Precisions..." << endl << endl;
	if (!testPrecisions()) {
		cout << "Precisions Test Failed!" << endl;
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

int main() {
	setupMaps();

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