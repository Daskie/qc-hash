// QHashTest.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <string>
#include <utility>

#include "QHashAlgorithms.h"
#include "QHashTable.h"

using std::cout;
using std::endl;
using std::string;
using namespace QHashTable;

int main() {

	cout << "Starting HashTable testing..." << endl;

	cout << endl;

	//Constructors

	cout << "Testing Constructors..." << endl << endl;

	//default
	HashTable<int> ht1(7);
	cout << ht1 << endl;
	//copy
	HashTable<int> ht2(ht1);
	cout << ht2 << endl;
	//assign
	HashTable<int> ht3(7); ht3 = ht2;
	cout << ht3 << endl;
	//move copy
	HashTable<int> ht4(std::move(ht3));
	cout << ht4 << endl;
	//move assign
	HashTable<int> ht5(7); ht5 = std::move(ht4);
	cout << ht5 << endl;

	cout << endl;

	//Add

	cout << "Testing Add..." << endl << endl;

	HashTable<char> cTable(26);
	HashTable<string> sTable(5);

	char chars[26];
	for (char c = 'a'; c <= 'z'; c++) {
		chars[c - 'a'] = c;
	}

	string strings[]{ "one", "two", "three", "four", "five" };

	for (char & c : chars) {
		cTable.add<char>(c, c);
	}
	for (string & s : strings) {
		sTable.add(s, s);
	}
	cout << cTable << endl;
	cTable.printContents(cout, true, true);
	cout << endl;
	cout << sTable << endl;
	sTable.printContents(cout, true, true);
	cout << endl;

	cout << endl;

	//Get

	cout << "Testing Get..." << endl << endl;

	for (char & c : chars) {
		cout << *cTable.get<char>(c) << ", ";
	}
	cout << endl;
	for (string & s : strings) {
		cout << *sTable.get(s) << ", ";
	}
	cout << endl;

	cout << endl;

	//= Overload

	cout << "Testing = Overload..." << endl << endl;

	HashTable<string> sTable2 = sTable;
	cout << "original..." << endl;
	cout << sTable << endl;
	sTable.printContents(cout, true, true, true);
	cout << "copy..." << endl;
	cout << sTable2 << endl;
	sTable2.printContents(cout, true, true, true);

	cout << endl;

	//Set

	cout << "Testing Set..." << endl << endl;

	for (int i = 0; i < 26; i++) {
		cTable.set(chars[26 - i - 1], chars[i]);
	}
	for (int i = 0; i < 5; i++) {
		sTable.set(strings[5 - i - 1], strings[i]);
	}
	cout << cTable << endl;
	cTable.printContents(cout, true, true);
	for (char & c : chars) {
		cout << *cTable.get<char>(c) << ", ";
	}
	cout << endl << endl;
	cout << sTable << endl;
	sTable.printContents(cout, true, true);
	for (string & s : strings) {
		cout << *sTable.get(s) << ", ";
	}
	cout << endl << endl;

	cout << endl;

	//Remove

	cout << "Testing Remove..." << endl << endl;

	for (char & c : chars) {
		cTable.remove<char>(c);
	}
	for (string & s : strings) {
		sTable.remove(s);
	}
	cout << cTable << endl;
	cTable.printContents(cout, true, true);
	cout << endl;
	cout << sTable << endl;
	sTable.printContents(cout, true, true);
	cout << endl;

	cout << endl;

	//Large Data

	cout << "Testing Large Data..." << endl << endl;

	HashTable<int> iTable(10000);
	for (int i = 0; i < 100000; i++) {
		iTable.add<int>(i, i);
	}
	cout << iTable << endl;
	iTable.stats(cout);

	cout << endl;
	system("pause");
	return 0;
}