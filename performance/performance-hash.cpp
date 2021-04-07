#include <iomanip>
#include <iostream>
#include <chrono>

#include <qc-core/random.hpp>

#include <qc-hash/qc-hash.hpp>

static unsigned long long now() {
    return std::chrono::nanoseconds(std::chrono::steady_clock::now().time_since_epoch()).count();
}

static void randomize(void * const data, const size_t size) {
    static qc::Random random{now()};

    uint8_t * const dataStart{static_cast<uint8_t *>(data)};
    uint8_t * const dataEnd{dataStart + size};
    uint8_t * const chunksStart{reinterpret_cast<uint8_t *>((reinterpret_cast<const uintptr_t &>(data) + 7u) & ~uintptr_t(0b111u))};
    const size_t chunkCount{size_t((dataEnd - chunksStart) >> 3)};
    uint8_t * const chunksEnd{chunksStart + (chunkCount << 3)};

    for (uint8_t * pos{dataStart}; pos < chunksStart; ++pos) {
        *pos = random.next<uint8_t>();
    }

    uint64_t * const chunks{reinterpret_cast<uint64_t *>(chunksStart)};
    for (size_t i{0u}; i < chunkCount; ++i) {
        chunks[i] = random.next<uint64_t>();
    }

    for (uint8_t * pos{chunksEnd}; pos < dataEnd; ++pos) {
        *pos = random.next<uint8_t>();
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

template <typename T, template <typename> typename H1, template <typename> typename H2>
static double compareTypeHash(const size_t reps, const size_t sets) {
    std::vector<T> vals(reps);
    const H1<T> hash1{};
    const H2<T> hash2{};
    uint64_t time1{0u};
    uint64_t time2{0u};

    volatile size_t v{0u};

    for (size_t set{0u}; set < sets; ++set) {
        randomize(vals.data(), reps * sizeof(T));

        const uint64_t t0{now()};

        for (size_t rep{0u}; rep < reps; ++rep) {
            v = hash1(vals[rep]);
        }

        const uint64_t t1{now()};

        for (size_t i{0u}; i < reps; ++i) {
            v = hash2(vals[i]);
        }

        const uint64_t t2{now()};

        time1 += t1 - t0;
        time2 += t2 - t1;
    }

    return double(time2) / double(time1);
}

template <size_t size, template <typename> typename H1, template <typename> typename H2>
static double compareSizeHash(const size_t reps, const size_t sets) {
    std::vector<std::string> strs(reps);
    for (std::string & str : strs) {
        str = std::string(size, '\0');
    }
    const H1<std::string> hash1{};
    const H2<std::string> hash2{};
    uint64_t time1{0u};
    uint64_t time2{0u};

    volatile size_t v{0u};

    for (size_t set{0u}; set < sets; ++set) {
        for (std::string & str : strs) {
            randomize(str.data(), size);
        }

        const uint64_t t0{now()};

        for (size_t rep{0u}; rep < reps; ++rep) {
            v = hash1(strs[rep]);
        }

        const uint64_t t1{now()};

        for (size_t i{0u}; i < reps; ++i) {
            v = hash2(strs[i]);
        }

        const uint64_t t2{now()};

        time1 += t1 - t0;
        time2 += t2 - t1;
    }

    return double(time2) / double(time1);
}

template <typename T> using H1 = std::hash<T>;
template <typename T> using H2 = qc_hash::Hash<T>;

int main() {
    constexpr size_t reps{1000u};
    constexpr size_t sets{1000u};

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Hash performance, comparing qc_hash::Hash to std::hash..." << std::endl;

    std::cout << std::endl << "     1 bytes... ";
    printFactor(compareTypeHash< uint8_t, H1, H2>(reps, sets));
    std::cout << std::endl << "     2 bytes... ";
    printFactor(compareTypeHash<uint16_t, H1, H2>(reps, sets));
    std::cout << std::endl << "     4 bytes... ";
    printFactor(compareTypeHash<uint32_t, H1, H2>(reps, sets));
    std::cout << std::endl << "     8 bytes... ";
    printFactor(compareTypeHash<uint64_t, H1, H2>(reps, sets));
    std::cout << std::endl << "    16 bytes... ";
    printFactor(compareSizeHash<     16u, H1, H2>(reps, sets));
    std::cout << std::endl << "    32 bytes... ";
    printFactor(compareSizeHash<     32u, H1, H2>(reps, sets));
    std::cout << std::endl << "    64 bytes... ";
    printFactor(compareSizeHash<     64u, H1, H2>(reps, sets));
    std::cout << std::endl << "  1024 bytes... ";
    printFactor(compareSizeHash<   1024u, H1, H2>(reps, sets));
    std::cout << std::endl << "     pointer... ";
    printFactor(compareTypeHash<size_t *, H1, H2>(reps, sets));
    std::cout << std::endl;

    std::cout << std::endl;
    std::cout << "Done" << std::endl;
}
