#pragma once

#ifdef QHASH_EXPORTS
#define QHASH_API __declspec(dllexport) 
#else
#define QHASH_API __declspec(dllimport) 
#endif

namespace QHashTable {

	QHASH_API int get();

}