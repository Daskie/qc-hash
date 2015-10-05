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
	
	char items[]{ 'a', 'b', 'c', 'd', 'e', 'f', 'g' };
	int keys[]{ 1, 2, 3, 4, 5, 6, 7 };

	for (int i = 0; i < 7; i++) {
		table.add<int>(items[i], keys[i]);
	}

	cout << table << endl;
	table.printContents(cout);

	cout << endl;
	system("pause");
	return 0;
}