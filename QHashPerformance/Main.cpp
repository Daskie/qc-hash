#include <iostream>
#include <string>
#include <chrono>
#include <unordered_map>
#include <iomanip>
#include <memory>
#include <algorithm>
#include <random>

#include "QHash/Map.hpp"
#include "QCore/Core.hpp"
#include "QCore/Time.hpp"



using namespace qc::types;



namespace {



void randomize(void * data, unat size) {
    u32 * intData(reinterpret_cast<u32 *>(data));
    for (unat i(0); i < size / 4; ++i) {
        intData[i] = std::rand();
    }
    if (size % 4 != 0) {
        u08 * byteData(reinterpret_cast<u08 *>(data));
        u32 extra(std::rand());
        u08 * extraByteData(reinterpret_cast<u08 *>(&extra));
        for (unat i(size / 4 * 4); i < size; ++i) {
            byteData[i] = extraByteData[i];
        }
    }
}

void printFactor(double factor) {
    if (factor <= 1.0) {
        std::cout << (1.0f / factor) << "x faster";
    }
    else {
        std::cout << factor << "x slower";
    }
}

template <typename T>
double compareTypeHash(unat nElements, unat nRounds) {
    std::unique_ptr<T[]> vals(new T[nElements]);
    randomize(vals.get(), nElements * sizeof(T));

    qc::Hash<T> qHash;
    std::hash<T> stdHash;
    volatile unat v(0);
    double overallFactor(0.0);

    for (unat round(0); round < nRounds; ++round) {
        u64 then(qc::now());
        for (unat i(0); i < nElements; ++i) {
            v += qHash(vals[i]);
        }
        u64 qTime = qc::now() - then;

        then = qc::now();
        for (unat i(0); i < nElements; ++i) {
            v += stdHash(vals[i]);
        }
        u64 stdTime = qc::now() - then;

        overallFactor += double(qTime) / double(stdTime);
    }

    return overallFactor / nRounds;
}

template <unat t_size>
double compareSizeHash(unat nElements, unat nRounds) {
    std::unique_ptr<u08[]> data(new u08[nElements * t_size]);
    randomize(data.get(), nElements * t_size);
    std::unique_ptr<std::string[]> strs(new std::string[nElements]);
    for (unat i(0); i < nElements; ++i) {
        strs[i] = std::string(reinterpret_cast<const char *>(data.get() + i * t_size), t_size);
    }

    qc::Hash<std::string> qHash;
    std::hash<std::string> stdHash;
    volatile unat v(0);
    double overallFactor(0.0);

    for (unat round(0); round < nRounds; ++round) {
        u64 then(qc::now());
        for (unat i(0); i < nElements; ++i) {
            v += qHash(strs[i]);
        }
        u64 qTime = qc::now() - then;

        then = qc::now();
        for (unat i(0); i < nElements; ++i) {
            v += stdHash(strs[i]);
        }
        u64 stdTime = qc::now() - then;

        overallFactor += double(qTime) / double(stdTime);
    }

    return overallFactor / nRounds;
}

void runHashComparison() {
    constexpr unat k_nBytes(8192);
    constexpr unat k_nRounds(10000);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Hash performance, comparing qc::Hash to std::hash..." << std::endl;

    std::cout << std::endl << "     1 bytes... ";
    printFactor(compareTypeHash<  u08>(k_nBytes /    1, k_nRounds));
    std::cout << std::endl << "     2 bytes... ";
    printFactor(compareTypeHash<  u16>(k_nBytes /    2, k_nRounds));
    std::cout << std::endl << "     4 bytes... ";
    printFactor(compareTypeHash<  u32>(k_nBytes /    4, k_nRounds));
    std::cout << std::endl << "     8 bytes... ";
    printFactor(compareTypeHash<  u64>(k_nBytes /    8, k_nRounds));
    std::cout << std::endl << "    16 bytes... ";
    printFactor(compareSizeHash<   16>(k_nBytes /   16, k_nRounds));
    std::cout << std::endl << "    32 bytes... ";
    printFactor(compareSizeHash<   32>(k_nBytes /   32, k_nRounds));
    std::cout << std::endl << "    64 bytes... ";
    printFactor(compareSizeHash<   64>(k_nBytes /   64, k_nRounds));
    std::cout << std::endl << "  1024 bytes... ";
    printFactor(compareSizeHash< 1024>(k_nBytes / 1024, k_nRounds));
    std::cout << std::endl << "     pointer... ";
    printFactor(compareTypeHash<nat *>(k_nBytes / sizeof(nat *), k_nRounds));

    std::cout << std::endl;
}

template <typename K, typename QH, typename StdH>
double compareMapInsertion(const std::vector<K> & keys, qc::Map<K, nat, QH> & qMap, std::unordered_map<K, nat, StdH> & stdMap) {
    unat then(qc::now());
    for (unat i(0); i < keys.size(); ++i) {
        qMap.emplace(keys[i], i);
    }
    unat qTime(qc::now() - then);

    then = qc::now();
    for (unat i(0); i < keys.size(); ++i) {
        stdMap.emplace(keys[i], i);
    }
    unat stdTime(qc::now() - then);
    
    return double(qTime) / double(stdTime);
}

template <typename K, typename QH, typename StdH>
double compareMapAccess(const std::vector<K> & keys, qc::Map<K, nat, QH> & qMap, std::unordered_map<K, nat, StdH> & stdMap) {
    volatile nat v(0);

    unat then(qc::now());
    for (unat i(0); i < keys.size(); ++i) {
        v += qMap.at(keys[i]);
    }
    unat qTime(qc::now() - then);

    then = qc::now();
    for (unat i(0); i < keys.size(); ++i) {
        v += stdMap.at(keys[i]);
    }
    unat stdTime(qc::now() - then);
    
    return double(qTime) / double(stdTime);
}

template <typename K, typename QH, typename StdH>
double compareMapErasure(const std::vector<K> & keys, qc::Map<K, nat, QH> & qMap, std::unordered_map<K, nat, StdH> & stdMap) {
    volatile nat v(0);

    unat then(qc::now());
    for (unat i(0); i < keys.size(); ++i) {
        v += qMap.erase(keys[i]);
    }
    unat qTime(qc::now() - then);

    then = qc::now();
    for (unat i(0); i < keys.size(); ++i) {
        v += stdMap.erase(keys[i]);
    }
    unat stdTime(qc::now() - then);
    
    return double(qTime) / double(stdTime);
}

void runMapComparison() {
    constexpr unat nElements(8192);
    constexpr unat nRounds(1000);
    
    std::vector<nat> keys(nElements);
    for (nat i(0); i < nElements; ++i) {
        keys[i] = i;
    }
    std::shuffle(keys.begin(), keys.end(), std::default_random_engine());

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Map performance, comparing qc::Map to std::unordered_map..." << std::endl;
    std::cout << std::endl;

    double overallInsertionFactor(0.0);
    double overallAccessFactor(0.0);
    double overallErasureFactor(0.0);

    for (unat round(0); round < nRounds; ++round) {
        qc::Map<nat, nat> qMap;
        std::unordered_map<nat, nat> stdMap;

        overallInsertionFactor += compareMapInsertion(keys, qMap, stdMap);

        overallAccessFactor += compareMapAccess(keys, qMap, stdMap);

        overallErasureFactor += compareMapErasure(keys, qMap, stdMap);
    }

    overallInsertionFactor /= nRounds;
    overallAccessFactor /= nRounds;
    overallErasureFactor /= nRounds;

    std::cout << std::endl;
    
    std::cout << "  Insertion: ";
    printFactor(overallInsertionFactor);
    std::cout << std::endl;
    std::cout << "  Access: ";
    printFactor(overallAccessFactor);
    std::cout << std::endl;
    std::cout << "  Erasure: ";
    printFactor(overallErasureFactor);
    std::cout << std::endl;
}



}



int main() {
    runHashComparison();

    std::cout << std::endl;

    runMapComparison();

    std::cin.get();
    return 0;
}