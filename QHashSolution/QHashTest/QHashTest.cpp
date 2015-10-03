// QHashTest.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <string>

#include "QHashAlgorithms.h"
#include "QHashTable.h"

using std::cout;
using std::endl;
using std::string;
using namespace QHashTable;

int main() {
	
	HashTable<int> table(3);
	int item = 77;
	string key = "blargh";
	table.add(item, key);

	auto lambda = [](const int & key, int nBytes) { return &key; };

	cout << *table.get(key);

	cout << endl;
	system("pause");
	return 0;
}