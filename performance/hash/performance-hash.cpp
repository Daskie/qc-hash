#include <iomanip>
#include <iostream>
#include <chrono>

#include <qc-core/random.hpp>

#include <qc-hash/fasthash.hpp>

using namespace qc::types;

static unsigned long long now()
{
    return std::chrono::nanoseconds(std::chrono::steady_clock::now().time_since_epoch()).count();
}

static void randomize(void * const data, const size_t size)
{
    static qc::Random random{size_t(now())};

    u8 * const dataStart{static_cast<u8 *>(data)};
    u8 * const dataEnd{dataStart + size};
    u8 * const chunksStart{reinterpret_cast<u8 *>((reinterpret_cast<const uintptr_t &>(data) + (sizeof(size_t) - 1u)) / sizeof(size_t) * sizeof(size_t))};
    const size_t chunkCount{size_t((dataEnd - chunksStart) / sizeof(size_t))};
    u8 * const chunksEnd{chunksStart + (chunkCount * sizeof(size_t))};

    for (u8 * pos{dataStart}; pos < chunksStart; ++pos) {
        *pos = random.next<u8>();
    }

    size_t * const chunks{reinterpret_cast<size_t *>(chunksStart)};
    for (size_t i{0u}; i < chunkCount; ++i) {
        chunks[i] = random.next<size_t>();
    }

    for (u8 * pos{chunksEnd}; pos < dataEnd; ++pos) {
        *pos = random.next<u8>();
    }
}

static void printFactor(double factor)
{
    if (factor <= 1.0) {
        std::cout << (1.0f / factor) << "x faster";
    }
    else {
        std::cout << factor << "x slower";
    }
}

template <typename T, template <typename> typename H1, template <typename> typename H2>
static double compareTypeHash(const size_t reps, const size_t sets)
{
    static volatile size_t v{0u};

    std::vector<T> vals(reps);
    const H1<T> hash1{};
    const H2<T> hash2{};
    u64 time1{0u};
    u64 time2{0u};

    for (size_t set{0u}; set < sets; ++set) {
        randomize(vals.data(), reps * sizeof(T));

        const u64 t0{now()};

        for (size_t rep{0u}; rep < reps; ++rep) {
            v = hash1(vals[rep]);
        }

        const u64 t1{now()};

        for (size_t i{0u}; i < reps; ++i) {
            v = hash2(vals[i]);
        }

        const u64 t2{now()};

        time1 += t1 - t0;
        time2 += t2 - t1;
    }

    return double(time2) / double(time1);
}

template <size_t size, template <typename> typename H1, template <typename> typename H2>
static double compareSizeHash(const size_t reps, const size_t sets)
{
    static volatile size_t v{0u};

    std::vector<std::string> strs(reps);
    for (std::string & str : strs) {
        str = std::string(size, '\0');
    }
    const H1<std::string> hash1{};
    const H2<std::string> hash2{};
    u64 time1{0u};
    u64 time2{0u};

    for (size_t set{0u}; set < sets; ++set) {
        for (std::string & str : strs) {
            randomize(str.data(), size);
        }

        const u64 t0{now()};

        for (size_t rep{0u}; rep < reps; ++rep) {
            v = hash1(strs[rep]);
        }

        const u64 t1{now()};

        for (size_t i{0u}; i < reps; ++i) {
            v = hash2(strs[i]);
        }

        const u64 t2{now()};

        time1 += t1 - t0;
        time2 += t2 - t1;
    }

    return double(time2) / double(time1);
}

template <typename T> using H1 = std::hash<T>;
template <typename T> using H2 = qc_hash::fasthash::Hash<T>;

int main()
{
    constexpr size_t reps{1000u};
    constexpr size_t sets{1000u};

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Hash performance, comparing qc_hash::Hash to std::hash..." << std::endl;

    std::cout << std::endl << "     1 bytes... ";
    printFactor(compareTypeHash< u8, H1, H2>(reps, sets));
    std::cout << std::endl << "     2 bytes... ";
    printFactor(compareTypeHash<u16, H1, H2>(reps, sets));
    std::cout << std::endl << "     4 bytes... ";
    printFactor(compareTypeHash<u32, H1, H2>(reps, sets));
    std::cout << std::endl << "     8 bytes... ";
#ifdef _WIN64
    printFactor(compareTypeHash<u64, H1, H2>(reps, sets));
#else
    printFactor(compareSizeHash<      8u, H1, H2>(reps, sets));
#endif
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
