// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Extended unit tests for Serialization::deserializer_t. Covers peek mode
// for all primitive types, mixed-endian interleaving, multi-compact chains,
// reset edge cases, error message pinning, and interleaved peek/skip/consume
// patterns.

#ifndef SERIALIZATION_UNIT_DESERIALIZER_EXTENDED_INL
#define SERIALIZATION_UNIT_DESERIALIZER_EXTENDED_INL

#include <cstdint>
#include <deserializer_t.h>
#include <serializable_pod.h>
#include <serializer_t.h>
#include <vector>

namespace unit_des_ext
{
    using Pod8 = SerializablePod<8>;
} // namespace unit_des_ext

// ========================================================================
// Peek mode for every primitive type
// ========================================================================

static void test_des_ext_peek_uint16()
{
    Serialization::serializer_t w;
    w.uint16(0x1234);
    Serialization::deserializer_t r(w);
    const auto peeked = r.uint16(true);
    ASSERT_EQ(peeked, static_cast<uint16_t>(0x1234));
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(2));
    const auto consumed = r.uint16();
    ASSERT_EQ(consumed, static_cast<uint16_t>(0x1234));
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

static void test_des_ext_peek_uint64()
{
    Serialization::serializer_t w;
    w.uint64(0x0102030405060708ULL);
    Serialization::deserializer_t r(w);
    const auto peeked = r.uint64(true);
    ASSERT_EQ(peeked, 0x0102030405060708ULL);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(8));
    const auto consumed = r.uint64();
    ASSERT_EQ(consumed, 0x0102030405060708ULL);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

static void test_des_ext_peek_uint128()
{
    Serialization::serializer_t w;
    const uint128_t val(0x0102030405060708ULL, 0x090A0B0C0D0E0F10ULL);
    w.uint128(val);
    Serialization::deserializer_t r(w);
    const auto peeked = r.uint128(true);
    ASSERT_TRUE(peeked == val);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(16));
    const auto consumed = r.uint128();
    ASSERT_TRUE(consumed == val);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

static void test_des_ext_peek_uint256()
{
    Serialization::serializer_t w;
    const uint256_t val(
        uint128_t(0x0102030405060708ULL, 0x090A0B0C0D0E0F10ULL),
        uint128_t(0x1112131415161718ULL, 0x191A1B1C1D1E1F20ULL));
    w.uint256(val);
    Serialization::deserializer_t r(w);
    const auto peeked = r.uint256(true);
    ASSERT_TRUE(peeked == val);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(32));
    const auto consumed = r.uint256();
    ASSERT_TRUE(consumed == val);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

static void test_des_ext_peek_boolean()
{
    Serialization::deserializer_t r({0x01});
    const auto peeked = r.boolean(true);
    ASSERT_TRUE(peeked);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(1));
    const auto consumed = r.boolean();
    ASSERT_TRUE(consumed);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

static void test_des_ext_peek_hex()
{
    Serialization::deserializer_t r({0xDE, 0xAD, 0xBE, 0xEF});
    const auto peeked = r.hex(4, true);
    ASSERT_EQ(peeked, std::string("deadbeef"));
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(4));
    const auto consumed = r.hex(4);
    ASSERT_EQ(consumed, std::string("deadbeef"));
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

static void test_des_ext_peek_varintV()
{
    Serialization::serializer_t w;
    const std::vector<uint32_t> vals = {1, 128, 256};
    w.varint(vals);
    Serialization::deserializer_t r(w);
    const auto full_size = r.unread_bytes();
    const auto peeked = r.varintV<uint32_t>(true);
    ASSERT_EQ(peeked.size(), static_cast<size_t>(3));
    ASSERT_EQ(peeked[0], 1u);
    ASSERT_EQ(peeked[1], 128u);
    ASSERT_EQ(peeked[2], 256u);
    ASSERT_EQ(r.unread_bytes(), full_size);
    const auto consumed = r.varintV<uint32_t>();
    ASSERT_EQ(consumed.size(), static_cast<size_t>(3));
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

static void test_des_ext_peek_podV()
{
    Serialization::serializer_t w;
    unit_des_ext::Pod8 a, b;
    a[0] = 0xAA;
    b[0] = 0xBB;
    std::vector<unit_des_ext::Pod8> items = {a, b};
    w.pod(items);
    Serialization::deserializer_t r(w);
    const auto full_size = r.unread_bytes();
    const auto peeked = r.podV<unit_des_ext::Pod8>(true);
    ASSERT_EQ(peeked.size(), static_cast<size_t>(2));
    ASSERT_EQ(peeked[0][0], 0xAA);
    ASSERT_EQ(peeked[1][0], 0xBB);
    ASSERT_EQ(r.unread_bytes(), full_size);
    const auto consumed = r.podV<unit_des_ext::Pod8>();
    ASSERT_EQ(consumed.size(), static_cast<size_t>(2));
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

static void test_des_ext_peek_podVV()
{
    Serialization::serializer_t w;
    unit_des_ext::Pod8 a, b, c;
    a[0] = 0x01;
    b[0] = 0x02;
    c[0] = 0x03;
    std::vector<std::vector<unit_des_ext::Pod8>> vv = {{a, b}, {c}};
    w.pod(vv);
    Serialization::deserializer_t r(w);
    const auto full_size = r.unread_bytes();
    const auto peeked = r.podVV<unit_des_ext::Pod8>(true);
    ASSERT_EQ(peeked.size(), static_cast<size_t>(2));
    ASSERT_EQ(peeked[0].size(), static_cast<size_t>(2));
    ASSERT_EQ(peeked[1].size(), static_cast<size_t>(1));
    ASSERT_EQ(r.unread_bytes(), full_size);
    const auto consumed = r.podVV<unit_des_ext::Pod8>();
    ASSERT_EQ(consumed.size(), static_cast<size_t>(2));
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

// ========================================================================
// Mixed-endian interleaving
// ========================================================================

static void test_des_ext_mixed_endian_interleaved()
{
    Serialization::serializer_t w;
    w.uint16(0x1234); // LE
    w.uint32(0xDEADBEEFu, true); // BE
    w.uint64(0x0102030405060708ULL); // LE
    w.uint32(0xCAFEBABEu, true); // BE

    Serialization::deserializer_t r(w);
    ASSERT_EQ(r.uint16(), static_cast<uint16_t>(0x1234));
    ASSERT_EQ(r.uint32(false, true), 0xDEADBEEFu);
    ASSERT_EQ(r.uint64(), 0x0102030405060708ULL);
    ASSERT_EQ(r.uint32(false, true), 0xCAFEBABEu);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

static void test_des_ext_mixed_endian_uint128_uint256()
{
    Serialization::serializer_t w;
    const uint128_t v128(0x0102030405060708ULL, 0x090A0B0C0D0E0F10ULL);
    const uint256_t v256(
        uint128_t(0x1112131415161718ULL, 0x191A1B1C1D1E1F20ULL),
        uint128_t(0x2122232425262728ULL, 0x292A2B2C2D2E2F30ULL));
    w.uint128(v128, true); // BE
    w.uint256(v256); // LE

    Serialization::deserializer_t r(w);
    ASSERT_TRUE(r.uint128(false, true) == v128);
    ASSERT_TRUE(r.uint256() == v256);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

// ========================================================================
// Multi-compact chains
// ========================================================================

static void test_des_ext_compact_chain()
{
    // Write 8 bytes: 0x01..0x08
    Serialization::deserializer_t r({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08});

    // Read 2, compact, read 2, compact, read 2, compact, read 2
    ASSERT_EQ(r.uint8(), 0x01);
    ASSERT_EQ(r.uint8(), 0x02);
    r.compact();
    ASSERT_EQ(r.size(), static_cast<size_t>(6));
    ASSERT_EQ(r.uint8(), 0x03);
    ASSERT_EQ(r.uint8(), 0x04);
    r.compact();
    ASSERT_EQ(r.size(), static_cast<size_t>(4));
    ASSERT_EQ(r.uint8(), 0x05);
    ASSERT_EQ(r.uint8(), 0x06);
    r.compact();
    ASSERT_EQ(r.size(), static_cast<size_t>(2));
    ASSERT_EQ(r.uint8(), 0x07);
    ASSERT_EQ(r.uint8(), 0x08);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

static void test_des_ext_compact_three_stages()
{
    Serialization::deserializer_t r({0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF});

    // Compact at offset 0 is a noop.
    r.compact();
    ASSERT_EQ(r.size(), static_cast<size_t>(6));
    ASSERT_EQ(r.uint8(), 0xAA);

    // Read one more, compact.
    ASSERT_EQ(r.uint8(), 0xBB);
    r.compact();
    ASSERT_EQ(r.size(), static_cast<size_t>(4));
    ASSERT_EQ(r.uint8(), 0xCC);
    ASSERT_EQ(r.uint8(), 0xDD);

    // Compact again.
    r.compact();
    ASSERT_EQ(r.size(), static_cast<size_t>(2));
    ASSERT_EQ(r.uint8(), 0xEE);
    ASSERT_EQ(r.uint8(), 0xFF);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

// ========================================================================
// Reset edge cases
// ========================================================================

static void test_des_ext_reset_past_size_read_throws()
{
    Serialization::deserializer_t r({0x01, 0x02, 0x03, 0x04});
    r.reset(100); // No bounds check — sets offset to 100
    ASSERT_THROWS_TYPE(r.uint8(), std::range_error);
}

static void test_des_ext_reset_past_size_skip_throws()
{
    Serialization::deserializer_t r({0x01, 0x02, 0x03, 0x04});
    r.reset(100);
    ASSERT_THROWS_TYPE(r.skip(1), std::range_error);
}

static void test_des_ext_reset_after_compact()
{
    // Read 2 of 4, compact (now 2 bytes, cursor=0), reset(1), read.
    Serialization::deserializer_t r({0x01, 0x02, 0x03, 0x04});
    (void)r.uint16(); // offset -> 2
    r.compact(); // buffer = {0x03, 0x04}, offset = 0
    ASSERT_EQ(r.size(), static_cast<size_t>(2));
    r.reset(1);
    ASSERT_EQ(r.uint8(), 0x04);
}

static void test_des_ext_compact_after_reset_to_zero()
{
    Serialization::deserializer_t r({0xAA, 0xBB, 0xCC, 0xDD});
    (void)r.uint16(); // offset -> 2
    (void)r.uint8(); // offset -> 3
    r.reset(0); // offset -> 0
    r.compact(); // at offset 0, compact is noop
    ASSERT_EQ(r.size(), static_cast<size_t>(4));
    ASSERT_EQ(r.uint8(), 0xAA);
}

// ========================================================================
// Error message pinning
// ========================================================================

static void test_des_ext_errmsg_read_past_end()
{
    Serialization::deserializer_t r({});
    ASSERT_THROWS_MSG(r.uint8(), "not enough data");
}

static void test_des_ext_errmsg_skip_past_end()
{
    Serialization::deserializer_t r({0x01, 0x02});
    ASSERT_THROWS_MSG(r.skip(3), "skip would exceed");
}

static void test_des_ext_errmsg_boolean_invalid()
{
    Serialization::deserializer_t r({0x02});
    ASSERT_THROWS_MSG(r.boolean(), "invalid boolean");
}

// ========================================================================
// Interleaved peek + skip + consume
// ========================================================================

static void test_des_ext_interleaved_peek_skip_consume()
{
    // Write: uint32(0xDEADBEEF) + uint16(0x1234) + uint8(0xFF)
    Serialization::serializer_t w;
    w.uint32(0xDEADBEEFu);
    w.uint16(0x1234);
    w.uint8(0xFF);
    Serialization::deserializer_t r(w);

    // Peek the uint32 (cursor stays at 0)
    ASSERT_EQ(r.uint32(true), 0xDEADBEEFu);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(7));

    // Skip past the uint32 (advance 4 bytes)
    r.skip(4);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(3));

    // Consume the uint16
    ASSERT_EQ(r.uint16(), static_cast<uint16_t>(0x1234));

    // Consume the uint8
    ASSERT_EQ(r.uint8(), static_cast<unsigned char>(0xFF));
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

#endif
