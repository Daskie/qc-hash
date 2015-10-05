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

	HashTable<char> table(3);
	
	char item = 'a';
	int key = 88;

	table.add<int>(item, key);

	cout << *table.get<int>(key, 4);

	cout << endl;
	system("pause");
	return 0;
}