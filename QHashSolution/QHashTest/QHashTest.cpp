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

HashTable<int> TABLE1;
HashTable<string> TABLE2;
HashTable<XXX> TABLE3;
HashTable<char> TABLE4;

void setupTables() {
	TABLE1 = HashTable<int>(5);
	TABLE2 = HashTable<string>(5);
	TABLE3 = HashTable<XXX>(5);
	for (int i = 0; i < 10; ++i) {
		TABLE1.addByHash(new int(i), i);
		TABLE2.addByHash(new string("" + i), i);
		TABLE3.addByHash(new XXX(), i);
	}
	TABLE4 = HashTable<char>(1000000);
	for (int i = 0; i < 10000000; ++i) {
		TABLE4.addByHash(new char(i % 256), i);
	}
}

bool testConstructor() {
	cout << "small..." << endl;
	HashTable<int> ht1(10);
	if (ht1.nSlots != 10 || ht1.size() != 0) return false;

	cout << "huge..." << endl;
	HashTable<int> ht2(100000);
	if (ht1.nSlots != 100000 || ht1.size() != 0) return false;

	cout << "zero..." << endl;
	HashTable<int> ht1(0);
	if (ht1.nSlots != 1 || ht1.size() != 0) return false;

	cout << "negative..." << endl;
	HashTable<int> ht1(-10);
	if (ht1.nSlots != 1 || ht1.size() != 0) return false;

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
	HashTable<long long> * ht1 = new HashTable<long long>(1000000);
	for (int i = 0; i < 10000000; ++i) {
		if (i % 100000 == 0) {
			int x = 0;
		}
		ht1->addByHash(new long long(i), i);
	}
	delete ht1;
	return true;
}

bool testAdd() {


	return true;
}

bool testGet() {


	return true;
}

bool testSet() {


	return true;
}

bool testRemove() {


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

	//while (true) {
	//	testDestructor();
	//}

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