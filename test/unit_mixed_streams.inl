// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Unit tests for mixed-type serialization streams. Exercises writing and
// reading multiple different types in a single serializer/deserializer
// session, including mixed endianness, large buffers, and stress scenarios.

#ifndef SERIALIZATION_UNIT_MIXED_STREAMS_INL
#define SERIALIZATION_UNIT_MIXED_STREAMS_INL

#include <cstdint>
#include <deserializer_t.h>
#include <serializable_pod.h>
#include <serializer_t.h>
#include <vector>

namespace unit_mix
{
    using Pod8 = SerializablePod<8>;
} // namespace unit_mix

// ---------- all types in one buffer ----------
static void test_mixed_all_types_roundtrip()
{
    Serialization::serializer_t w;

    w.boolean(true);
    w.uint8(0xAB);
    w.uint16(0x1234);
    w.uint32(0xDEADBEEFu, true); // BE
    w.uint64(0x0102030405060708ULL);
    w.varint<uint32_t>(300u);
    w.bytes(std::vector<unsigned char> {0xCA, 0xFE});
    w.hex("abcd");
    const uint128_t v128(0x12345ULL);
    w.uint128(v128);
    const uint256_t v256(0xABCDEF1u);
    w.uint256(v256);
    unit_mix::Pod8 pod;
    pod[0] = 0x42;
    w.pod(pod);

    Serialization::deserializer_t r(w);

    ASSERT_TRUE(r.boolean());
    ASSERT_EQ(r.uint8(), static_cast<unsigned char>(0xAB));
    ASSERT_EQ(r.uint16(), static_cast<uint16_t>(0x1234));
    ASSERT_EQ(r.uint32(false, true), 0xDEADBEEFu);
    ASSERT_EQ(r.uint64(), 0x0102030405060708ULL);
    ASSERT_EQ(r.varint<uint32_t>(), 300u);
    const auto b = r.bytes(2);
    ASSERT_EQ(b[0], 0xCA);
    ASSERT_EQ(b[1], 0xFE);
    ASSERT_EQ(r.hex(2), std::string("abcd"));
    ASSERT_TRUE(r.uint128() == v128);
    ASSERT_TRUE(r.uint256() == v256);
    const auto rpod = r.pod<unit_mix::Pod8>();
    ASSERT_EQ(rpod[0], 0x42);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

// ---------- interleaved endianness ----------
static void test_mixed_interleaved_endian()
{
    Serialization::serializer_t w;
    w.uint16(0x1234); // LE
    w.uint16(0x5678, true); // BE
    w.uint32(0xAABBCCDDu); // LE
    w.uint32(0x11223344u, true); // BE
    w.uint64(0xCAFEBABEDEADBEEFULL); // LE
    w.uint64(0x0807060504030201ULL, true); // BE

    Serialization::deserializer_t r(w);
    ASSERT_EQ(r.uint16(), static_cast<uint16_t>(0x1234));
    ASSERT_EQ(r.uint16(false, true), static_cast<uint16_t>(0x5678));
    ASSERT_EQ(r.uint32(), 0xAABBCCDDu);
    ASSERT_EQ(r.uint32(false, true), 0x11223344u);
    ASSERT_EQ(r.uint64(), 0xCAFEBABEDEADBEEFULL);
    ASSERT_EQ(r.uint64(false, true), 0x0807060504030201ULL);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

// ---------- varints interleaved with pods ----------
static void test_mixed_varints_and_pods()
{
    Serialization::serializer_t w;
    w.varint<uint32_t>(127u);
    w.varint<uint32_t>(128u);
    w.varint<uint32_t>(16384u);
    unit_mix::Pod8 p1;
    p1[0] = 0xAA;
    w.pod(p1);
    w.varint<uint32_t>(0u);
    unit_mix::Pod8 p2;
    p2[7] = 0xFF;
    w.pod(p2);

    Serialization::deserializer_t r(w);
    ASSERT_EQ(r.varint<uint32_t>(), 127u);
    ASSERT_EQ(r.varint<uint32_t>(), 128u);
    ASSERT_EQ(r.varint<uint32_t>(), 16384u);
    const auto rp1 = r.pod<unit_mix::Pod8>();
    ASSERT_EQ(rp1[0], 0xAA);
    ASSERT_EQ(r.varint<uint32_t>(), 0u);
    const auto rp2 = r.pod<unit_mix::Pod8>();
    ASSERT_EQ(rp2[7], 0xFF);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

// ---------- large mixed buffer stress ----------
static void test_mixed_large_stress_500()
{
    Serialization::serializer_t w;

    for (int i = 0; i < 500; ++i)
    {
        w.boolean(i % 2 == 0);
        w.uint8(static_cast<unsigned char>(i & 0xFF));
        w.uint16(static_cast<uint16_t>(i));
        w.uint32(static_cast<uint32_t>(i * 7));
        w.uint64(static_cast<uint64_t>(i) * 100000ULL);
        w.varint<uint32_t>(static_cast<uint32_t>(i));
        unit_mix::Pod8 p;
        p[0] = static_cast<unsigned char>(i & 0xFF);
        w.pod(p);
    }

    Serialization::deserializer_t r(w);

    for (int i = 0; i < 500; ++i)
    {
        ASSERT_EQ(r.boolean(), (i % 2 == 0));
        ASSERT_EQ(r.uint8(), static_cast<unsigned char>(i & 0xFF));
        ASSERT_EQ(r.uint16(), static_cast<uint16_t>(i));
        ASSERT_EQ(r.uint32(), static_cast<uint32_t>(i * 7));
        ASSERT_EQ(r.uint64(), static_cast<uint64_t>(i) * 100000ULL);
        ASSERT_EQ(r.varint<uint32_t>(), static_cast<uint32_t>(i));
        const auto p = r.pod<unit_mix::Pod8>();
        ASSERT_EQ(p[0], static_cast<unsigned char>(i & 0xFF));
    }

    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

// ---------- 1000 sequential varints ----------
static void test_mixed_1000_varints()
{
    Serialization::serializer_t w;
    for (uint32_t i = 0; i < 1000; ++i)
    {
        w.varint<uint32_t>(i);
    }

    Serialization::deserializer_t r(w);
    for (uint32_t i = 0; i < 1000; ++i)
    {
        ASSERT_EQ(r.varint<uint32_t>(), i);
    }

    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

#endif
