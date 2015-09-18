// QHashTest.cpp : Defines the entry point for the console application.
//

#include <iostream>

#include "QHashAlgorithms.h"
#include "QHashTable.h"

using std::cout;
using std::endl;

int main() {
	cout << QHashTable::get() << endl;

	cout << endl;
	system("pause");
	return 0;
}