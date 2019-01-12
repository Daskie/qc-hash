namespace qc {

    namespace detail {

        template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
        constexpr int log2Floor(T v) {
            static_assert(sizeof(T) <= 8);

            int log(0);
            if constexpr (sizeof(T) >= 8)
                if (v & 0xFFFFFFFF00000000ULL) { v >>= 32; log += 32; }
            if constexpr (sizeof(T) >= 4)
                if (v & 0x00000000FFFF0000ULL) { v >>= 16; log += 16; }
            if constexpr (sizeof(T) >= 2)
                if (v & 0x000000000000FF00ULL) { v >>=  8; log +=  8; }
            if (    v & 0x00000000000000F0ULL) { v >>=  4; log +=  4; }
            if (    v & 0x000000000000000CULL) { v >>=  2; log +=  2; }
            if (    v & 0x0000000000000002ULL) {           log +=  1; }
            return log;
        }

        template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
        constexpr int log2Ceil(T v) {
            return log2Floor(2 * v - 1);
        }

        template <typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
        constexpr T ceil2(T v) {
            return T(1) << log2Ceil(v);
        }

    }



    //==================================================================================================================
    // HASH IMPLEMENTATION /////////////////////////////////////////////////////////////////////////////////////////////
    //==================================================================================================================

    //==========================================================================
    // Hash::operator()
    //--------------------------------------------------------------------------

    template <typename K>
    size_t Hash<K>::operator()(const K & key) const {
        if constexpr (std::is_same_v<K, std::string>) {
            return hash(key.data(), key.size());
        }
        else if constexpr (std::is_same_v<K, std::string_view>) {
            return hash(key.data(), key.size());
        }
        //else if (std::is_pointer_v<std::remove_cv_t<std::remove_reference_t<K>>>) {
        //    return size_t(reinterpret_cast<const uintptr_t &>(key) >> detail::log2Floor(alignof(std::remove_pointer_t<std::remove_cv_t<std::remove_reference_t<K>>>)));
        //}
        else if constexpr (sizeof(K) == 1) {
            return size_t(murmur3::fmix32(uint32_t(reinterpret_cast<const uint8_t &>(key))));
        }
        else if constexpr (sizeof(K) == 2) {
            return size_t(murmur3::fmix32(uint32_t(reinterpret_cast<const uint16_t &>(key))));
        }
        else if constexpr (sizeof(K) == 4) {
            return size_t(murmur3::fmix32(reinterpret_cast<const uint32_t &>(key)));
        }
        else if constexpr (sizeof(K) == 8) {
            return size_t(murmur3::fmix64(reinterpret_cast<const uint64_t &>(key)));
        }
        else {
            return hash(&key, sizeof(K));
        }
    }

    //==========================================================================
    // NoHash::operator()
    //--------------------------------------------------------------------------

    template <typename K>
    size_t NoHash<K>::operator()(const K & key) const {
        using utype =
            std::conditional_t<sizeof(K) == 1,  uint8_t,
            std::conditional_t<sizeof(K) == 2, uint16_t,
            std::conditional_t<sizeof(K) == 4, uint32_t,
            uint64_t>>>;
        return size_t(reinterpret_cast<const utype &>(key));
    }

    //==========================================================================
    // hash
    //--------------------------------------------------------------------------

    size_t hash(const void * key, size_t n, size_t seed) {
        if constexpr (sizeof(size_t) == 4) {
            return murmur3::x86_32(key, uint32_t(n), uint32_t(seed));
        }
        else if constexpr (sizeof(size_t) == 8) {
            auto [h1, h2](murmur3::x64_128(key, uint64_t(n), uint64_t(seed)));
            return h1 ^ h2;
        }
    }



    //==================================================================================================================
    // MURMUR3 IMPLEMENTATION //////////////////////////////////////////////////////////////////////////////////////////
    //==================================================================================================================

    namespace murmur3 {

        constexpr uint32_t rotl32(uint32_t x, int r) {
            return (x << r) | (x >> (32 - r));
        }

        constexpr uint64_t rotl64(uint64_t x, int r) {
            return (x << r) | (x >> (64 - r));
        }

        //======================================================================
        // fmix32
        //----------------------------------------------------------------------

        constexpr uint32_t fmix32(uint32_t h) {
            h ^= h >> 16;
            h *= uint32_t(0x85ebca6b);
            h ^= h >> 13;
            h *= uint32_t(0xc2b2ae35);
            h ^= h >> 16;

            return h;
        }

        //======================================================================
        // fmix64
        //----------------------------------------------------------------------

        constexpr uint64_t fmix64(uint64_t h) {
            h ^= h >> 33;
            h *= uint64_t(0xff51afd7ed558ccdULL);
            h ^= h >> 33;
            h *= uint64_t(0xc4ceb9fe1a85ec53ULL);
            h ^= h >> 33;

            return h;
        }

        //======================================================================
        // x86_32
        //----------------------------------------------------------------------

        inline uint32_t x86_32(const void * key, uint32_t n, uint32_t seed) {
            const uint8_t * data(reinterpret_cast<const uint8_t *>(key));
            const std::int32_t nblocks(n / 4);

            uint32_t h1(seed);

            constexpr uint32_t c1(0xcc9e2d51);
            constexpr uint32_t c2(0x1b873593);

            const uint32_t * blocks(reinterpret_cast<const uint32_t *>(data + nblocks * 4));

            for (std::int32_t i(-nblocks); i < 0; ++i) {
                uint32_t k1(blocks[i]);

                k1 *= c1;
                k1  = rotl32(k1, 15);
                k1 *= c2;

                h1 ^= k1;
                h1  = rotl32(h1, 13);
                h1  = h1 * uint32_t(5) + uint32_t(0xe6546b64);
            }

            const uint8_t * tail(reinterpret_cast<const uint8_t *>(data + nblocks * 4));

            uint32_t k1(0);

            switch (n & 0b11) {
                case 0b11: k1 ^= uint32_t(tail[2]) << 16;
                case 0b10: k1 ^= uint32_t(tail[1]) << 8;
                case 0b01: k1 ^= uint32_t(tail[0]);
                    k1 *= c1;
                    k1  = rotl32(k1, 15);
                    k1 *= c2;
                    h1 ^= k1;
            };

            h1 ^= n;

            h1 = fmix32(h1);

            return h1;
        }

        //======================================================================
        // x86_128
        //----------------------------------------------------------------------

        inline std::tuple<uint32_t, uint32_t, uint32_t, uint32_t> x86_128(const void * key, uint32_t n, uint32_t seed) {
            const uint8_t * data(reinterpret_cast<const uint8_t *>(key));
            const std::int32_t nblocks(n / 16);

            uint32_t h1(seed);
            uint32_t h2(seed);
            uint32_t h3(seed);
            uint32_t h4(seed);

            constexpr uint32_t c1(0x239b961b);
            constexpr uint32_t c2(0xab0e9789);
            constexpr uint32_t c3(0x38b34ae5);
            constexpr uint32_t c4(0xa1e38b93);

            const uint32_t * blocks(reinterpret_cast<const uint32_t *>(data + nblocks * 16));

            for (std::int32_t i(-nblocks); i < 0; ++i) {
                uint32_t k1(blocks[i * 4 + 0]);
                uint32_t k2(blocks[i * 4 + 1]);
                uint32_t k3(blocks[i * 4 + 2]);
                uint32_t k4(blocks[i * 4 + 3]);

                k1 *= c1;
                k1  = rotl32(k1, 15);
                k1 *= c2;
                h1 ^= k1;

                h1  = rotl32(h1, 19);
                h1 += h2;
                h1  = h1 * uint32_t(5) + uint32_t(0x561ccd1b);

                k2 *= c2;
                k2  = rotl32(k2, 16);
                k2 *= c3;
                h2 ^= k2;

                h2  = rotl32(h2, 17);
                h2 += h3;
                h2  = h2 * uint32_t(5) + uint32_t(0x0bcaa747);

                k3 *= c3;
                k3  = rotl32(k3, 17);
                k3 *= c4;
                h3 ^= k3;

                h3  = rotl32(h3, 15);
                h3 += h4;
                h3  = h3 * uint32_t(5) + uint32_t(0x96cd1c35);

                k4 *= c4;
                k4  = rotl32(k4, 18);
                k4 *= c1;
                h4 ^= k4;

                h4  = rotl32(h4, 13);
                h4 += h1;
                h4  = h4 * uint32_t(5) + uint32_t(0x32ac3b17);
            }

            const uint8_t * tail(data + nblocks * 16);

            uint32_t k1(0);
            uint32_t k2(0);
            uint32_t k3(0);
            uint32_t k4(0);

            switch (n & 0b1111) {
                case 0b1111: k4 ^= uint32_t(tail[14]) << 16;
                case 0b1110: k4 ^= uint32_t(tail[13]) <<  8;
                case 0b1101: k4 ^= uint32_t(tail[12]) <<  0;
                    k4 *= c4;
                    k4  = rotl32(k4, 18);
                    k4 *= c1;
                    h4 ^= k4;

                case 0b1100: k3 ^= uint32_t(tail[11]) << 24;
                case 0b1011: k3 ^= uint32_t(tail[10]) << 16;
                case 0b1010: k3 ^= uint32_t(tail[ 9]) <<  8;
                case 0b1001: k3 ^= uint32_t(tail[ 8]) <<  0;
                    k3 *= c3;
                    k3  = rotl32(k3, 17);
                    k3 *= c4;
                    h3 ^= k3;

                case 0b1000: k2 ^= uint32_t(tail[7]) << 24;
                case 0b0111: k2 ^= uint32_t(tail[6]) << 16;
                case 0b0110: k2 ^= uint32_t(tail[5]) <<  8;
                case 0b0101: k2 ^= uint32_t(tail[4]) <<  0;
                    k2 *= c2;
                    k2  = rotl32(k2, 16);
                    k2 *= c3;
                    h2 ^= k2;

                case 0b0100: k1 ^= uint32_t(tail[3]) << 24;
                case 0b0011: k1 ^= uint32_t(tail[2]) << 16;
                case 0b0010: k1 ^= uint32_t(tail[1]) <<  8;
                case 0b0001: k1 ^= uint32_t(tail[0]) <<  0;
                    k1 *= c1;
                    k1  = rotl32(k1, 15);
                    k1 *= c2;
                    h1 ^= k1;
            };

            h1 ^= n;
            h2 ^= n;
            h3 ^= n;
            h4 ^= n;

            h1 += h2;
            h1 += h3;
            h1 += h4;
            h2 += h1;
            h3 += h1;
            h4 += h1;

            h1 = fmix32(h1);
            h2 = fmix32(h2);
            h3 = fmix32(h3);
            h4 = fmix32(h4);

            h1 += h2;
            h1 += h3;
            h1 += h4;
            h2 += h1;
            h3 += h1;
            h4 += h1;

            return std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>(h1, h2, h3, h4);
        }

        //======================================================================
        // x64_128
        //----------------------------------------------------------------------

        inline std::pair<uint64_t, uint64_t> x64_128(const void * key, uint64_t n, uint64_t seed) {
            const uint8_t * data(reinterpret_cast<const uint8_t *>(key));
            const uint64_t nblocks(n / 16);

            uint64_t h1(seed);
            uint64_t h2(seed);

            const uint64_t c1(0x87c37b91114253d5ULL);
            const uint64_t c2(0x4cf5ad432745937fULL);

            const uint64_t * blocks(reinterpret_cast<const uint64_t *>(data));

            for (uint64_t i(0); i < nblocks; ++i) {
                uint64_t k1(blocks[i * 2 + 0]);
                uint64_t k2(blocks[i * 2 + 1]);

                k1 *= c1;
                k1  = rotl64(k1, 31);
                k1 *= c2;
                h1 ^= k1;

                h1  = rotl64(h1, 27);
                h1 += h2;
                h1  = h1 * uint64_t(5) + uint64_t(0x52dce729);

                k2 *= c2;
                k2  = rotl64(k2, 33);
                k2 *= c1;
                h2 ^= k2;

                h2  = rotl64(h2, 31);
                h2 += h1;
                h2  = h2 * uint64_t(5) + uint64_t(0x38495ab5);
            }

            const uint8_t * tail(data + nblocks * 16);

            uint64_t k1(0);
            uint64_t k2(0);

            switch (n & 0b1111) {
                case 0b1111: k2 ^= uint64_t(tail[14]) << 48;
                case 0b1110: k2 ^= uint64_t(tail[13]) << 40;
                case 0b1101: k2 ^= uint64_t(tail[12]) << 32;
                case 0b1100: k2 ^= uint64_t(tail[11]) << 24;
                case 0b1011: k2 ^= uint64_t(tail[10]) << 16;
                case 0b1010: k2 ^= uint64_t(tail[ 9]) <<  8;
                case 0b1001: k2 ^= uint64_t(tail[ 8]) <<  0;
                    k2 *= c2;
                    k2  = rotl64(k2, 33);
                    k2 *= c1;
                    h2 ^= k2;

                case 0b1000: k1 ^= uint64_t(tail[7]) << 56;
                case 0b0111: k1 ^= uint64_t(tail[6]) << 48;
                case 0b0110: k1 ^= uint64_t(tail[5]) << 40;
                case 0b0101: k1 ^= uint64_t(tail[4]) << 32;
                case 0b0100: k1 ^= uint64_t(tail[3]) << 24;
                case 0b0011: k1 ^= uint64_t(tail[2]) << 16;
                case 0b0010: k1 ^= uint64_t(tail[1]) <<  8;
                case 0b0001: k1 ^= uint64_t(tail[0]) <<  0;
                    k1 *= c1;
                    k1  = rotl64(k1, 31);
                    k1 *= c2;
                    h1 ^= k1;
            };

            h1 ^= n;
            h2 ^= n;

            h1 += h2;
            h2 += h1;

            h1 = fmix64(h1);
            h2 = fmix64(h2);

            h1 += h2;
            h2 += h1;

            return std::pair<uint64_t, uint64_t>(h1, h2);
        }

    }

}