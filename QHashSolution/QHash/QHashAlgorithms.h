#pragma once

#ifdef QHASH_EXPORTS
#define QHASH_API __declspec(dllexport) 
#else
#define QHASH_API __declspec(dllimport) 
#endif

namespace QHashAlgorithms {

	QHASH_API int hash();

}