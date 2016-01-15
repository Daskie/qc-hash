//Code written by Austin Quick.
//
//@updated 1/14/2015
//@since August 2015

#pragma once

#include <cstdint>
#include <string>

namespace QHashAlgorithms {


//Simple pair to accommodate the 128 bit hash results.
struct uint128 { uint64_t h1; uint64_t h2; };

//Interprets nKeyBytes worth of key data using murmur_x86_32 and returns the
//hash.
uint32_t hash32(const void * key, int nKeyBytes, uint32_t seed = 0);

//Interprets the key string as c_str using murmur_x86_128 and returns the hash.
uint32_t hash32(const std::string & key, uint32_t seed = 0);

//Interprets nKeyBytes worth of key data using murmur_x86_32 and returns the
//hash.
uint128 hash64(const void * key, int nKeyBytes, uint32_t seed = 0);

//Interprets the key string as c_str using murmur_x86_128 and returns the hash.
uint128 hash64(const std::string & key, uint32_t seed = 0);

namespace MurmurHash3 {

//Hashes len bytes of key and writes a 32 bit result to out.
void murmur_x86_32(const void * key, int len, uint32_t seed, void * out);

//Hashes len bytes of key and writes two 64 bit results to out.
void murmur_x86_128(const void * key, int len, uint32_t seed, void * out);

//Hashes len bytes of key and writes two 64 bit results to out.
void murmur_x64_128(const void * key, int len, uint32_t seed, void * out);

}

}