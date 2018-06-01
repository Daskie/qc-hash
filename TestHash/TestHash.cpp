#include <SDKDDKVer.h>

#include "CppUnitTest.h"

#include "QHash/Hash.hpp"



using namespace Microsoft::VisualStudio::CppUnitTestFramework;



TEST_CLASS(Hash) {

    public:
        
    TEST_METHOD(IntegerKeys) {
        Assert::AreEqual(size_t(0), qc::Hash<  int8_t>()(0));
        Assert::AreEqual(size_t(0), qc::Hash< uint8_t>()(0));
        Assert::AreEqual(size_t(0), qc::Hash< int16_t>()(0));
        Assert::AreEqual(size_t(0), qc::Hash<uint16_t>()(0));
        Assert::AreEqual(size_t(0), qc::Hash< int32_t>()(0));
        Assert::AreEqual(size_t(0), qc::Hash<uint32_t>()(0));
        Assert::AreEqual(size_t(0), qc::Hash< int64_t>()(0));
        Assert::AreEqual(size_t(0), qc::Hash<uint64_t>()(0));
    }

    TEST_METHOD(PointerKeys) {
        Assert::AreEqual(size_t(1), qc::Hash< int8_t *>()(reinterpret_cast< int8_t *>(0b0001)));
        Assert::AreEqual(size_t(1), qc::Hash<int16_t *>()(reinterpret_cast<int16_t *>(0b0010)));
        Assert::AreEqual(size_t(1), qc::Hash<int32_t *>()(reinterpret_cast<int32_t *>(0b0100)));
        Assert::AreEqual(size_t(1), qc::Hash<int64_t *>()(reinterpret_cast<int64_t *>(0b1000)));
    }

    TEST_METHOD(All8BitKeys) {
        qc::Hash<uint8_t> hash;
        for (int i(0); i < 256; ++i) {
            Assert::AreEqual(size_t(i), hash(uint8_t(i)));
        }
    }

};