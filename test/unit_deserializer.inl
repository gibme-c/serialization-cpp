// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Unit tests for Serialization::deserializer_t. Heavy focus on adversarial
// buffer states: empty, short-by-one, past-end reads, inflated varint counts
// that would drive podV / podVV to allocate based on untrusted input.

#ifndef SERIALIZATION_UNIT_DESERIALIZER_INL
#define SERIALIZATION_UNIT_DESERIALIZER_INL

#include <cstdint>
#include <deserializer_t.h>
#include <limits>
#include <serializable_pod.h>
#include <serializer_t.h>
#include <string>
#include <vector>

namespace unit_des
{
    // A tiny Serializable POD just for podV tests in this file. We reuse
    // SerializablePod<8> — smaller than the default 32 so crafted byte streams
    // can cover multiple items in a compact test.
    using Pod8 = SerializablePod<8>;
}

// ---------- ctors ----------
static void test_des_ctor_from_serializer()
{
    Serialization::serializer_t w;
    w.uint32(0xDEADBEEFu);
    Serialization::deserializer_t r(w);
    ASSERT_EQ(r.size(), static_cast<size_t>(4));
    ASSERT_EQ(r.uint32(), 0xDEADBEEFu);
}

static void test_des_ctor_from_vector()
{
    const std::vector<unsigned char> v = {0xEF, 0xBE, 0xAD, 0xDE};
    Serialization::deserializer_t r(v);
    ASSERT_EQ(r.uint32(), 0xDEADBEEFu);
}

static void test_des_ctor_from_initializer_list()
{
    Serialization::deserializer_t r({0x01, 0x02, 0x03});
    ASSERT_EQ(r.size(), static_cast<size_t>(3));
    ASSERT_EQ(r.uint8(), 0x01);
    ASSERT_EQ(r.uint8(), 0x02);
    ASSERT_EQ(r.uint8(), 0x03);
}

static void test_des_ctor_from_hex_string()
{
    Serialization::deserializer_t r(std::string("deadbeef"));
    ASSERT_EQ(r.size(), static_cast<size_t>(4));
    // Bytes {DE,AD,BE,EF} in little-endian decode as 0xEFBEADDE; read with
    // big_endian=true to recover the value the hex string appears to show.
    ASSERT_EQ(r.uint32(true, true), 0xDEADBEEFu);  // peek + BE
    const auto h = r.to_string();
    ASSERT_EQ(h, std::string("deadbeef"));
}

static void test_des_ctor_from_empty_hex_string()
{
    Serialization::deserializer_t r(std::string(""));
    ASSERT_EQ(r.size(), static_cast<size_t>(0));
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

static void test_des_ctor_from_odd_hex_throws()
{
    ASSERT_THROWS_TYPE(Serialization::deserializer_t(std::string("abc")), std::length_error);
}

static void test_des_ctor_from_invalid_hex_throws()
{
    ASSERT_THROWS_TYPE(Serialization::deserializer_t(std::string("zz")), std::invalid_argument);
}

// ---------- primitive reads ----------
static void test_des_boolean_reads_both_states()
{
    Serialization::deserializer_t r({0x00, 0x01, 0x02, 0xFF});
    ASSERT_FALSE(r.boolean());
    ASSERT_TRUE(r.boolean());
    // Any byte != 1 decodes as false.
    ASSERT_FALSE(r.boolean());
    ASSERT_FALSE(r.boolean());
}

static void test_des_uint8_reads_each_byte()
{
    Serialization::deserializer_t r({0x10, 0x20, 0x30});
    ASSERT_EQ(r.uint8(), 0x10);
    ASSERT_EQ(r.uint8(), 0x20);
    ASSERT_EQ(r.uint8(), 0x30);
}

static void test_des_uint16_le_and_be()
{
    Serialization::deserializer_t r1({0x34, 0x12});
    ASSERT_EQ(r1.uint16(), 0x1234);
    Serialization::deserializer_t r2({0x12, 0x34});
    ASSERT_EQ(r2.uint16(false, true), 0x1234);
}

static void test_des_uint32_le_and_be()
{
    Serialization::deserializer_t r1({0xEF, 0xBE, 0xAD, 0xDE});
    ASSERT_EQ(r1.uint32(), 0xDEADBEEFu);
    Serialization::deserializer_t r2({0xDE, 0xAD, 0xBE, 0xEF});
    ASSERT_EQ(r2.uint32(false, true), 0xDEADBEEFu);
}

static void test_des_uint64_le_and_be()
{
    Serialization::serializer_t w;
    w.uint64(0x0102030405060708ULL);
    Serialization::deserializer_t r(w);
    ASSERT_EQ(r.uint64(), 0x0102030405060708ULL);
}

// ---------- peek mode: cursor does not advance ----------
static void test_des_peek_does_not_advance()
{
    Serialization::deserializer_t r({0x12, 0x34, 0x56, 0x78});
    const auto v = r.uint32(true);
    ASSERT_EQ(v, 0x78563412u);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(4));
    const auto v2 = r.uint32();
    ASSERT_EQ(v2, 0x78563412u);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

static void test_des_peek_bytes_does_not_advance()
{
    Serialization::deserializer_t r({0xDE, 0xAD, 0xBE, 0xEF});
    const auto b = r.bytes(2, true);
    ASSERT_EQ(b.size(), static_cast<size_t>(2));
    ASSERT_EQ(b[0], 0xDE);
    ASSERT_EQ(b[1], 0xAD);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(4));
}

// ---------- bytes() and hex() ----------
static void test_des_bytes_zero_count()
{
    Serialization::deserializer_t r({0xAA, 0xBB});
    const auto b = r.bytes(0);
    ASSERT_EQ(b.size(), static_cast<size_t>(0));
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(2));
}

static void test_des_bytes_exact_full_buffer()
{
    Serialization::deserializer_t r({0xAA, 0xBB, 0xCC});
    const auto b = r.bytes(3);
    ASSERT_EQ(b.size(), static_cast<size_t>(3));
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

static void test_des_hex_read()
{
    Serialization::deserializer_t r({0xDE, 0xAD, 0xBE, 0xEF});
    ASSERT_EQ(r.hex(4), std::string("deadbeef"));
}

static void test_des_hex_peek()
{
    Serialization::deserializer_t r({0xDE, 0xAD});
    ASSERT_EQ(r.hex(2, true), std::string("dead"));
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(2));
}

// ---------- read past end ----------
static void test_des_uint8_past_end_throws()
{
    Serialization::deserializer_t r({});
    ASSERT_THROWS_TYPE(r.uint8(), std::range_error);
}

static void test_des_uint16_half_past_end_throws()
{
    Serialization::deserializer_t r({0xAA});
    ASSERT_THROWS_TYPE(r.uint16(), std::range_error);
}

static void test_des_uint32_partial_throws()
{
    Serialization::deserializer_t r({0x01, 0x02, 0x03});
    ASSERT_THROWS_TYPE(r.uint32(), std::range_error);
}

static void test_des_uint64_partial_throws()
{
    Serialization::deserializer_t r({0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
    ASSERT_THROWS_TYPE(r.uint64(), std::range_error);
}

static void test_des_uint128_partial_throws()
{
    std::vector<unsigned char> v(15, 0xFF);
    Serialization::deserializer_t r(v);
    ASSERT_THROWS_TYPE(r.uint128(), std::range_error);
}

static void test_des_uint256_partial_throws()
{
    std::vector<unsigned char> v(31, 0xFF);
    Serialization::deserializer_t r(v);
    ASSERT_THROWS_TYPE(r.uint256(), std::range_error);
}

static void test_des_bytes_count_past_end_throws()
{
    Serialization::deserializer_t r({0x01, 0x02});
    ASSERT_THROWS_TYPE(r.bytes(3), std::range_error);
}

static void test_des_hex_count_past_end_throws()
{
    Serialization::deserializer_t r({0x01, 0x02});
    ASSERT_THROWS_TYPE(r.hex(3), std::range_error);
}

static void test_des_read_after_full_consumed_throws()
{
    Serialization::deserializer_t r({0xAA});
    (void)r.uint8();
    ASSERT_THROWS_TYPE(r.uint8(), std::range_error);
}

// ---------- skip / reset / compact / unread_bytes / unread_data ----------
static void test_des_skip_ok()
{
    Serialization::deserializer_t r({0x01, 0x02, 0x03, 0x04});
    r.skip(2);
    ASSERT_EQ(r.uint8(), 0x03);
    ASSERT_EQ(r.uint8(), 0x04);
}

static void test_des_skip_past_end_throws()
{
    Serialization::deserializer_t r({0x01, 0x02});
    ASSERT_THROWS_TYPE(r.skip(3), std::range_error);
}

static void test_des_skip_exact_end_ok()
{
    Serialization::deserializer_t r({0x01, 0x02, 0x03});
    r.skip(3);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

static void test_des_skip_zero_ok()
{
    Serialization::deserializer_t r({0x01});
    r.skip(0);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(1));
}

static void test_des_reset_returns_to_start()
{
    Serialization::deserializer_t r({0x01, 0x02, 0x03});
    (void)r.uint8();
    (void)r.uint8();
    r.reset();
    ASSERT_EQ(r.uint8(), 0x01);
}

static void test_des_reset_to_mid_position()
{
    Serialization::deserializer_t r({0x01, 0x02, 0x03, 0x04});
    (void)r.uint8();
    (void)r.uint8();
    r.reset(1);
    ASSERT_EQ(r.uint8(), 0x02);
}

static void test_des_compact_drops_consumed_bytes()
{
    // After compact(), the buffer contains only the previously-unread bytes
    // and the cursor is reset to 0, so the next read returns the first
    // unread byte directly.
    Serialization::deserializer_t r({0x01, 0x02, 0x03, 0x04});
    (void)r.uint16();  // offset -> 2
    r.compact();
    ASSERT_EQ(r.size(), static_cast<size_t>(2));
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(2));
    ASSERT_EQ(r.uint8(), 0x03);
    ASSERT_EQ(r.uint8(), 0x04);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

// Compact from the very start (offset == 0) is a no-op and still leaves
// the cursor at 0.
static void test_des_compact_from_start_is_noop()
{
    Serialization::deserializer_t r({0xAA, 0xBB, 0xCC});
    r.compact();
    ASSERT_EQ(r.size(), static_cast<size_t>(3));
    ASSERT_EQ(r.uint8(), 0xAA);
}

// Compact when everything has been consumed leaves an empty buffer and a
// zero cursor.
static void test_des_compact_after_full_read()
{
    Serialization::deserializer_t r({0xAA, 0xBB});
    (void)r.uint16();
    r.compact();
    ASSERT_EQ(r.size(), static_cast<size_t>(0));
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
    ASSERT_THROWS_TYPE(r.uint8(), std::range_error);
}

static void test_des_unread_bytes_decreases_with_reads()
{
    Serialization::deserializer_t r({0x01, 0x02, 0x03, 0x04, 0x05});
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(5));
    (void)r.uint8();
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(4));
    (void)r.uint16();
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(2));
}

static void test_des_unread_data_copy()
{
    Serialization::deserializer_t r({0x01, 0x02, 0x03, 0x04});
    (void)r.uint8();
    const auto rem = r.unread_data();
    const std::vector<unsigned char> expected = {0x02, 0x03, 0x04};
    ASSERT_BYTES_EQ(rem, expected);
}

static void test_des_unread_data_empty_when_consumed()
{
    Serialization::deserializer_t r({0x01});
    (void)r.uint8();
    const auto rem = r.unread_data();
    ASSERT_EQ(rem.size(), static_cast<size_t>(0));
}

// ---------- varint reads ----------
static void test_des_varint_reads_and_advances()
{
    Serialization::deserializer_t r({0x80, 0x01, 0x01});
    ASSERT_EQ(r.varint<uint32_t>(), 128u);
    ASSERT_EQ(r.varint<uint32_t>(), 1u);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

static void test_des_varint_peek()
{
    Serialization::deserializer_t r({0x80, 0x01});
    const auto v = r.varint<uint32_t>(true);
    ASSERT_EQ(v, 128u);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(2));
}

static void test_des_varint_truncated_throws()
{
    Serialization::deserializer_t r({0x80});
    ASSERT_THROWS_TYPE(r.varint<uint32_t>(), std::range_error);
}

// ---------- podV: inflated varint count must not silently allocate ----------
// The library currently trusts the varint count and loops calling pod<Type>()
// which reads Type::size() bytes each time. When the declared count exceeds
// what the buffer can support, the inner bytes() read will throw range_error
// eventually. We pin that behavior.
static void test_des_podV_inflated_count_throws_before_oom()
{
    // Encode count = 0xFFFFFFFF (very large), then zero payload.
    Serialization::serializer_t w;
    w.varint<uint32_t>(0xFFFFFFFFu);
    // No payload bytes — first pod<> read should run out of buffer.
    Serialization::deserializer_t r(w);
    ASSERT_THROWS_TYPE(r.podV<unit_des::Pod8>(), std::range_error);
}

static void test_des_podV_count_zero_empty_result()
{
    Serialization::serializer_t w;
    w.varint<uint32_t>(0u);
    Serialization::deserializer_t r(w);
    const auto v = r.podV<unit_des::Pod8>();
    ASSERT_EQ(v.size(), static_cast<size_t>(0));
}

static void test_des_podV_small_roundtrip()
{
    Serialization::serializer_t w;
    std::vector<unit_des::Pod8> items(3);
    for (size_t i = 0; i < items.size(); ++i)
    {
        (*items[i])[0] = static_cast<unsigned char>(i + 1);
    }
    w.pod(items);
    Serialization::deserializer_t r(w);
    const auto decoded = r.podV<unit_des::Pod8>();
    ASSERT_EQ(decoded.size(), items.size());
    for (size_t i = 0; i < items.size(); ++i)
    {
        ASSERT_EQ(decoded[i][0], static_cast<unsigned char>(i + 1));
    }
}

static void test_des_podVV_inflated_outer_count_throws()
{
    Serialization::serializer_t w;
    w.varint<uint32_t>(0xFFFFFFFFu);  // outer count huge
    Serialization::deserializer_t r(w);
    ASSERT_THROWS_TYPE(r.podVV<unit_des::Pod8>(), std::range_error);
}

static void test_des_podVV_inflated_inner_count_throws()
{
    Serialization::serializer_t w;
    w.varint<uint32_t>(1u);           // outer count = 1
    w.varint<uint32_t>(0xFFFFFFFFu);  // inner count huge
    Serialization::deserializer_t r(w);
    ASSERT_THROWS_TYPE(r.podVV<unit_des::Pod8>(), std::range_error);
}

static void test_des_podVV_empty_outer_returns_empty()
{
    Serialization::serializer_t w;
    w.varint<uint32_t>(0u);
    Serialization::deserializer_t r(w);
    const auto rr = r.podVV<unit_des::Pod8>();
    ASSERT_EQ(rr.size(), static_cast<size_t>(0));
}

// ---------- varintV: same story ----------
static void test_des_varintV_count_zero_empty()
{
    Serialization::serializer_t w;
    w.varint<uint32_t>(0u);
    Serialization::deserializer_t r(w);
    const auto v = r.varintV<uint32_t>();
    ASSERT_EQ(v.size(), static_cast<size_t>(0));
}

static void test_des_varintV_small_roundtrip()
{
    Serialization::serializer_t w;
    const std::vector<uint32_t> in = {1, 128, 16384, 0xABCDu};
    w.varint(in);
    Serialization::deserializer_t r(w);
    const auto out = r.varintV<uint32_t>();
    ASSERT_EQ(out.size(), in.size());
    for (size_t i = 0; i < in.size(); ++i)
    {
        ASSERT_EQ(out[i], in[i]);
    }
}

static void test_des_varintV_inflated_count_throws()
{
    Serialization::serializer_t w;
    w.varint<uint32_t>(0xFFFFFFFFu);
    Serialization::deserializer_t r(w);
    ASSERT_THROWS_TYPE(r.varintV<uint32_t>(), std::range_error);
}

// ---------- data() and size() ----------
static void test_des_data_and_size()
{
    Serialization::deserializer_t r({0x11, 0x22, 0x33});
    ASSERT_EQ(r.size(), static_cast<size_t>(3));
    ASSERT_EQ(r.data()[0], 0x11);
    ASSERT_EQ(r.data()[1], 0x22);
    ASSERT_EQ(r.data()[2], 0x33);
}

// ---------- to_string() ----------
static void test_des_to_string()
{
    Serialization::deserializer_t r({0xDE, 0xAD, 0xBE, 0xEF});
    ASSERT_EQ(r.to_string(), std::string("deadbeef"));
}

// ---------- big-endian primitives round-trip via serializer ----------
static void test_des_uint16_be_roundtrip_via_serializer()
{
    Serialization::serializer_t w;
    w.uint16(0xABCD, true);
    Serialization::deserializer_t r(w);
    ASSERT_EQ(r.uint16(false, true), 0xABCD);
}

static void test_des_uint32_be_roundtrip_via_serializer()
{
    Serialization::serializer_t w;
    w.uint32(0xDEADBEEFu, true);
    Serialization::deserializer_t r(w);
    ASSERT_EQ(r.uint32(false, true), 0xDEADBEEFu);
}

static void test_des_uint64_be_roundtrip_via_serializer()
{
    Serialization::serializer_t w;
    w.uint64(0x0102030405060708ULL, true);
    Serialization::deserializer_t r(w);
    ASSERT_EQ(r.uint64(false, true), 0x0102030405060708ULL);
}

static void test_des_uint128_roundtrip()
{
    Serialization::serializer_t w;
    w.uint128(uint128_t(0x12345));
    Serialization::deserializer_t r(w);
    ASSERT_EQ(r.uint128(), uint128_t(0x12345));
}

static void test_des_uint256_roundtrip()
{
    Serialization::serializer_t w;
    w.uint256(uint256_t(0x0ABCDEF1u));
    Serialization::deserializer_t r(w);
    ASSERT_EQ(r.uint256(), uint256_t(0x0ABCDEF1u));
}

// ---------- reset() to past-end followed by read throws ----------
static void test_des_reset_to_end_read_throws()
{
    Serialization::deserializer_t r({0x01, 0x02, 0x03});
    r.reset(3);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
    ASSERT_THROWS_TYPE(r.uint8(), std::range_error);
}

// ---------- zero-size buffer reads throw for every primitive ----------
static void test_des_empty_buffer_reads_all_throw()
{
    {
        Serialization::deserializer_t r({});
        ASSERT_THROWS_TYPE(r.uint8(), std::range_error);
    }
    {
        Serialization::deserializer_t r({});
        ASSERT_THROWS_TYPE(r.uint16(), std::range_error);
    }
    {
        Serialization::deserializer_t r({});
        ASSERT_THROWS_TYPE(r.uint32(), std::range_error);
    }
    {
        Serialization::deserializer_t r({});
        ASSERT_THROWS_TYPE(r.uint64(), std::range_error);
    }
    {
        Serialization::deserializer_t r({});
        ASSERT_THROWS_TYPE(r.varint<uint32_t>(), std::range_error);
    }
}

// ---------- pod<T> single-element read works and respects peek ----------
static void test_des_pod_single_peek()
{
    Serialization::serializer_t w;
    unit_des::Pod8 item;
    for (size_t i = 0; i < 8; ++i)
    {
        (*item)[i] = static_cast<unsigned char>(0xA0 + i);
    }
    w.pod(item);
    Serialization::deserializer_t r(w);
    auto peeked = r.pod<unit_des::Pod8>(true);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(8));
    ASSERT_EQ(peeked[0], 0xA0);
    auto read = r.pod<unit_des::Pod8>();
    ASSERT_EQ(read[7], 0xA7);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

#endif
