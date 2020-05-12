#include <iostream>
#include <unordered_map>

#include "CppUnitTest.h"

#include <qc-core/core.hpp>

#include "qc-hash.hpp"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace qc::core::types;

TEST_CLASS(Hash) {

    public:

    TEST_METHOD(Uniqueness) {
        std::unordered_map<unat, u08> map;
        qc::hash::Hash<u08> hash;
        for (int i(0); i < 256; ++i) {
            auto [it, placed](map.try_emplace(hash(u08(i)), u08(i)));
            Assert::IsTrue(placed);
        }
    }

};

TEST_CLASS(IdentityHash) {

    public:

    TEST_METHOD(Arithmetic) {
        u64 v(0xFEDCBA9876543210);

        Assert::AreEqual(unat(u08(v)), qc::hash::IdentityHash<u08>()(reinterpret_cast<const u08 &>(v)));
        Assert::AreEqual(unat(u08(v)), qc::hash::IdentityHash<s08>()(reinterpret_cast<const s08 &>(v)));

        Assert::AreEqual(unat(u16(v)), qc::hash::IdentityHash<u16>()(reinterpret_cast<const u16 &>(v)));
        Assert::AreEqual(unat(u16(v)), qc::hash::IdentityHash<s16>()(reinterpret_cast<const s16 &>(v)));

        Assert::AreEqual(unat(u32(v)), qc::hash::IdentityHash<u32>()(reinterpret_cast<const u32 &>(v)));
        Assert::AreEqual(unat(u32(v)), qc::hash::IdentityHash<s32>()(reinterpret_cast<const s32 &>(v)));
        Assert::AreEqual(unat(u32(v)), qc::hash::IdentityHash<f32>()(reinterpret_cast<const f32 &>(v)));
#if _WIN64
        Assert::AreEqual(unat(u64(v)), qc::hash::IdentityHash<u64>()(reinterpret_cast<const u64 &>(v)));
        Assert::AreEqual(unat(u64(v)), qc::hash::IdentityHash<s64>()(reinterpret_cast<const s64 &>(v)));
        Assert::AreEqual(unat(u64(v)), qc::hash::IdentityHash<f64>()(reinterpret_cast<const f64 &>(v)));
#endif
    }

    TEST_METHOD(Sizes) {
        struct S1 { u08 _0; };
        struct S2 { u08 _0, _1; };
        struct S3 { u08 _0, _1, _2; };
        struct S4 { u08 _0, _1, _2, _3; };
        struct S5 { u08 _0, _1, _2, _3, _4; };
        struct S6 { u08 _0, _1, _2, _3, _4, _5; };
        struct S7 { u08 _0, _1, _2, _3, _4, _5, _6; };
        struct S8 { u08 _0, _1, _2, _3, _4, _5, _6, _7; };

        u64 v(0xFEDCBA9876543210);

        Assert::AreEqual(unat(0x0000000000000010), qc::hash::IdentityHash<S1>()(reinterpret_cast<const S1 &>(v)));
        Assert::AreEqual(unat(0x0000000000003210), qc::hash::IdentityHash<S2>()(reinterpret_cast<const S2 &>(v)));
        Assert::AreEqual(unat(0x0000000000543210), qc::hash::IdentityHash<S3>()(reinterpret_cast<const S3 &>(v)));
        Assert::AreEqual(unat(0x0000000076543210), qc::hash::IdentityHash<S4>()(reinterpret_cast<const S4 &>(v)));
#if _WIN64
        Assert::AreEqual(unat(0x0000009876543210), qc::hash::IdentityHash<S5>()(reinterpret_cast<const S5 &>(v)));
        Assert::AreEqual(unat(0x0000BA9876543210), qc::hash::IdentityHash<S6>()(reinterpret_cast<const S6 &>(v)));
        Assert::AreEqual(unat(0x00DCBA9876543210), qc::hash::IdentityHash<S7>()(reinterpret_cast<const S7 &>(v)));
        Assert::AreEqual(unat(0xFEDCBA9876543210), qc::hash::IdentityHash<S8>()(reinterpret_cast<const S8 &>(v)));
#endif
    }

    TEST_METHOD(PointerKeys) {
        Assert::AreEqual(unat(1u), qc::hash::IdentityHash<const u08 *>()(reinterpret_cast<const u08 *>(0b0001)));
        Assert::AreEqual(unat(1u), qc::hash::IdentityHash<const u16 *>()(reinterpret_cast<const u16 *>(0b0010)));
        Assert::AreEqual(unat(1u), qc::hash::IdentityHash<const u32 *>()(reinterpret_cast<const u32 *>(0b0100)));
        Assert::AreEqual(unat(1u), qc::hash::IdentityHash<const u64 *>()(reinterpret_cast<const u64 *>(0b1000)));

        Assert::AreEqual(unat(1u), qc::hash::IdentityHash<const void *>()(reinterpret_cast<const void *>(1u)));
    }

};
