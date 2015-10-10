#pragma once

#ifdef QHASH_EXPORTS
#define QHASH_API __declspec(dllexport) 
#else
#define QHASH_API __declspec(dllimport) 
#endif

#include <cstdint>
#include <string>

namespace QHashAlgorithms {

struct KeyBundle {
	KeyBundle(const void * key, int nBytes) : key_(key), nBytes_(nBytes) {}
	const void * key_;
	int nBytes_;
};

class KeyDecoder {
	public:
	virtual KeyBundle decode(KeyBundle kb) const {
		return kb;
	}
};

static const KeyDecoder DEFAULT_KEY_DECODER;

class StringKeyDecoder : public KeyDecoder {
	public:
	virtual KeyBundle decode(KeyBundle kb) const {
		std::string * s = (std::string *)kb.key_;
		kb.key_ = s->c_str();
		kb.nBytes_ = static_cast<int>(s->length()) - 1; //leave off the \0, static_cast because string.length() returns size_type
		return kb;
	}
};

static const StringKeyDecoder STRING_KEY_DECODER;

struct Hash128 { uint64_t h1; uint64_t h2; };

QHASH_API uint32_t hash32(const void * key, int nKeyBytes, uint32_t seed = 0);

QHASH_API uint32_t hash32(const std::string & key, uint32_t seed = 0);

QHASH_API uint32_t hash32(KeyBundle kb, uint32_t seed = 0);

QHASH_API Hash128 hash64(const void * key, int nKeyBytes, uint32_t seed = 0);

QHASH_API Hash128 hash64(const std::string & key, uint32_t seed = 0);

QHASH_API Hash128 hash64(KeyBundle kb, uint32_t seed = 0);

namespace MurmurHash3 {

QHASH_API void murmur_x86_32(const void * key, int len, uint32_t seed, void * out);

QHASH_API void murmur_x86_128(const void * key, int len, uint32_t seed, void * out);

QHASH_API void murmur_x64_128(const void * key, int len, uint32_t seed, void * out);

}

}