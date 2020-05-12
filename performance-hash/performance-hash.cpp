#include <iostream>
#include <iomanip>
#include <chrono>

#include "qc-hash.hpp"

static unsigned long long now() {
    return std::chrono::nanoseconds(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

static void randomize(void * data, size_t size) {
    uint32_t * intData(reinterpret_cast<uint32_t *>(data));
    for (size_t i(0u); i < size / 4u; ++i) {
        intData[i] = std::rand();
    }
    if (size % 4u) {
        uint8_t * byteData(reinterpret_cast<uint8_t *>(data));
        uint32_t extra(std::rand());
        uint8_t * extraByteData(reinterpret_cast<uint8_t *>(&extra));
        for (size_t i(size / 4u * 4u); i < size; ++i) {
            byteData[i] = extraByteData[i];
        }
    }
}

static void printFactor(double factor) {
    if (factor <= 1.0) {
        std::cout << (1.0f / factor) << "x faster";
    }
    else {
        std::cout << factor << "x slower";
    }
}

template <typename T>
static double compareTypeHash(size_t nElements, size_t nRounds) {
    std::unique_ptr<T[]> vals(new T[nElements]);
    randomize(vals.get(), nElements * sizeof(T));

    qc::hash::Hash<T> qHash;
    std::hash<T> stdHash;
    volatile size_t v(0u);
    double overallFactor(0.0);

    for (size_t round(0u); round < nRounds; ++round) {
        unsigned long long then(now());
        for (size_t i(0u); i < nElements; ++i) {
            v += qHash(vals[i]);
        }
        unsigned long long qTime = now() - then;

        then = now();
        for (size_t i(0u); i < nElements; ++i) {
            v += stdHash(vals[i]);
        }
        unsigned long long stdTime = now() - then;

        overallFactor += double(qTime) / double(stdTime);
    }

    return overallFactor / nRounds;
}

template <size_t t_size>
static double compareSizeHash(size_t nElements, size_t nRounds) {
    std::unique_ptr<uint8_t[]> data(new uint8_t[nElements * t_size]);
    randomize(data.get(), nElements * t_size);
    std::unique_ptr<std::string[]> strs(new std::string[nElements]);
    for (size_t i(0u); i < nElements; ++i) {
        strs[i] = std::string(reinterpret_cast<const char *>(data.get() + i * t_size), t_size);
    }

    qc::hash::Hash<std::string> qHash;
    std::hash<std::string> stdHash;
    volatile size_t v(0u);
    double overallFactor(0.0);

    for (size_t round(0u); round < nRounds; ++round) {
        unsigned long long then(now());
        for (size_t i(0u); i < nElements; ++i) {
            v += qHash(strs[i]);
        }
        unsigned long long qTime = now() - then;

        then = now();
        for (size_t i(0u); i < nElements; ++i) {
            v += stdHash(strs[i]);
        }
        unsigned long long stdTime = now() - then;

        overallFactor += double(qTime) / double(stdTime);
    }

    return overallFactor / nRounds;
}

int main() {
    constexpr size_t k_nBytes(8192u);
    constexpr size_t k_nRounds(10000u);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Hash performance, comparing qc::hash::Hash to std::hash..." << std::endl;

    std::cout << std::endl << "     1 bytes... ";
    printFactor(compareTypeHash< uint8_t>(k_nBytes /    1u, k_nRounds));
    std::cout << std::endl << "     2 bytes... ";
    printFactor(compareTypeHash<uint16_t>(k_nBytes /    2u, k_nRounds));
    std::cout << std::endl << "     4 bytes... ";
    printFactor(compareTypeHash<uint32_t>(k_nBytes /    4u, k_nRounds));
    std::cout << std::endl << "     8 bytes... ";
    printFactor(compareTypeHash<uint64_t>(k_nBytes /    8u, k_nRounds));
    std::cout << std::endl << "    16 bytes... ";
    printFactor(compareSizeHash<     16u>(k_nBytes /   16u, k_nRounds));
    std::cout << std::endl << "    32 bytes... ";
    printFactor(compareSizeHash<     32u>(k_nBytes /   32u, k_nRounds));
    std::cout << std::endl << "    64 bytes... ";
    printFactor(compareSizeHash<     64u>(k_nBytes /   64u, k_nRounds));
    std::cout << std::endl << "  1024 bytes... ";
    printFactor(compareSizeHash<   1024u>(k_nBytes / 1024u, k_nRounds));
    std::cout << std::endl << "     pointer... ";
    printFactor(compareTypeHash<size_t *>(k_nBytes / sizeof(size_t *), k_nRounds));
    std::cout << std::endl;

    std::cout << std::endl;
    std::cout << "Done" << std::endl;
    std::cin.get();
}
