// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Unit tests for Serialization::serializer_t. Covers every public method,
// constructor overload, boundary conditions, and error paths.

#ifndef SERIALIZATION_UNIT_SERIALIZER_INL
#define SERIALIZATION_UNIT_SERIALIZER_INL

#include <cstdint>
#include <limits>
#include <serializer_t.h>
#include <string_helper.h>
#include <vector>

// ---------- default-constructed serializer is empty ----------
static void test_ser_default_empty()
{
    Serialization::serializer_t w;
    ASSERT_EQ(w.size(), static_cast<size_t>(0));
    ASSERT_EQ(w.to_string(), std::string(""));
    ASSERT_EQ(w.vector().size(), static_cast<size_t>(0));
}

// ---------- boolean writes exactly one byte (0 or 1) ----------
static void test_ser_boolean_true_is_0x01()
{
    Serialization::serializer_t w;
    w.boolean(true);
    ASSERT_EQ(w.size(), static_cast<size_t>(1));
    ASSERT_EQ(w.vector()[0], 0x01);
}

static void test_ser_boolean_false_is_0x00()
{
    Serialization::serializer_t w;
    w.boolean(false);
    ASSERT_EQ(w.size(), static_cast<size_t>(1));
    ASSERT_EQ(w.vector()[0], 0x00);
}

// ---------- uint8/16/32/64 byte-count increments ----------
static void test_ser_uint8_byte_count()
{
    Serialization::serializer_t w;
    w.uint8(0xAB);
    ASSERT_EQ(w.size(), static_cast<size_t>(1));
}

static void test_ser_uint16_byte_count_is_2()
{
    Serialization::serializer_t w;
    w.uint16(0xABCD);
    ASSERT_EQ(w.size(), static_cast<size_t>(2));
}

static void test_ser_uint32_byte_count_is_4()
{
    Serialization::serializer_t w;
    w.uint32(0xDEADBEEFu);
    ASSERT_EQ(w.size(), static_cast<size_t>(4));
}

static void test_ser_uint64_byte_count_is_8()
{
    Serialization::serializer_t w;
    w.uint64(0xCAFEBABEDEADBEEFULL);
    ASSERT_EQ(w.size(), static_cast<size_t>(8));
}

static void test_ser_uint128_byte_count_is_16()
{
    Serialization::serializer_t w;
    w.uint128(uint128_t(12345));
    ASSERT_EQ(w.size(), static_cast<size_t>(16));
}

static void test_ser_uint256_byte_count_is_32()
{
    Serialization::serializer_t w;
    w.uint256(uint256_t(54321));
    ASSERT_EQ(w.size(), static_cast<size_t>(32));
}

// ---------- bytes(ptr, len) writes the expected content ----------
static void test_ser_bytes_ptr_len()
{
    Serialization::serializer_t w;
    const unsigned char data[] = {0x01, 0x02, 0x03, 0x04};
    w.bytes(data, sizeof(data));
    ASSERT_EQ(w.size(), sizeof(data));
    const auto v = w.vector();
    for (size_t i = 0; i < sizeof(data); ++i)
    {
        ASSERT_EQ(v[i], data[i]);
    }
}

static void test_ser_bytes_nullptr_zero_length_ok()
{
    Serialization::serializer_t w;
    w.bytes(nullptr, 0);  // must NOT throw
    ASSERT_EQ(w.size(), static_cast<size_t>(0));
}

static void test_ser_bytes_nullptr_nonzero_throws()
{
    Serialization::serializer_t w;
    ASSERT_THROWS_TYPE(w.bytes(nullptr, 5), std::invalid_argument);
}

// ---------- bytes(vector) appends in order ----------
static void test_ser_bytes_vector_overload()
{
    Serialization::serializer_t w;
    const std::vector<unsigned char> v = {0xAA, 0xBB, 0xCC};
    w.bytes(v);
    ASSERT_BYTES_EQ(w.vector(), v);
}

static void test_ser_bytes_vector_empty_is_noop()
{
    Serialization::serializer_t w;
    const std::vector<unsigned char> v;
    w.bytes(v);
    ASSERT_EQ(w.size(), static_cast<size_t>(0));
}

// ---------- hex() decodes and appends ----------
static void test_ser_hex_appends_bytes()
{
    Serialization::serializer_t w;
    w.hex("deadbeef");
    const std::vector<unsigned char> expected = {0xDE, 0xAD, 0xBE, 0xEF};
    ASSERT_BYTES_EQ(w.vector(), expected);
}

static void test_ser_hex_empty_is_noop()
{
    Serialization::serializer_t w;
    w.hex("");
    ASSERT_EQ(w.size(), static_cast<size_t>(0));
}

static void test_ser_hex_odd_length_throws()
{
    Serialization::serializer_t w;
    ASSERT_THROWS_TYPE(w.hex("abc"), std::length_error);
}

static void test_ser_hex_invalid_char_throws()
{
    Serialization::serializer_t w;
    ASSERT_THROWS_TYPE(w.hex("zz"), std::invalid_argument);
}

// ---------- to_string() is lowercase hex of buffer ----------
static void test_ser_to_string_matches_hex()
{
    Serialization::serializer_t w;
    w.bytes(std::vector<unsigned char>{0xDE, 0xAD, 0xBE, 0xEF});
    ASSERT_EQ(w.to_string(), std::string("deadbeef"));
}

// ---------- operator[] read and write ----------
static void test_ser_operator_index_read()
{
    Serialization::serializer_t w;
    w.bytes(std::vector<unsigned char>{0x11, 0x22, 0x33});
    ASSERT_EQ(w[0], 0x11);
    ASSERT_EQ(w[1], 0x22);
    ASSERT_EQ(w[2], 0x33);
}

static void test_ser_operator_index_write_mutation()
{
    Serialization::serializer_t w;
    w.bytes(std::vector<unsigned char>{0x11, 0x22, 0x33});
    w[1] = 0xFF;
    ASSERT_EQ(w[1], 0xFF);
    ASSERT_EQ(w.vector()[1], 0xFF);
}

// ---------- reset() clears buffer ----------
static void test_ser_reset_clears()
{
    Serialization::serializer_t w;
    w.uint32(0xDEADBEEF);
    ASSERT_EQ(w.size(), static_cast<size_t>(4));
    w.reset();
    ASSERT_EQ(w.size(), static_cast<size_t>(0));
    ASSERT_EQ(w.vector().size(), static_cast<size_t>(0));
}

// ---------- ctors: copy, initializer_list, vector ----------
static void test_ser_copy_ctor()
{
    Serialization::serializer_t a;
    a.uint32(0xCAFEBABE);
    Serialization::serializer_t b(a);
    ASSERT_BYTES_EQ(b.vector(), a.vector());
    // Copies are independent — modifying b must not affect a.
    b.uint8(0xFF);
    ASSERT_EQ(a.size(), static_cast<size_t>(4));
    ASSERT_EQ(b.size(), static_cast<size_t>(5));
}

static void test_ser_initializer_list_ctor()
{
    Serialization::serializer_t w({0x01, 0x02, 0x03});
    ASSERT_EQ(w.size(), static_cast<size_t>(3));
    ASSERT_EQ(w[0], 0x01);
    ASSERT_EQ(w[1], 0x02);
    ASSERT_EQ(w[2], 0x03);
}

static void test_ser_vector_ctor()
{
    const std::vector<unsigned char> v = {0xAA, 0xBB, 0xCC, 0xDD};
    Serialization::serializer_t w(v);
    ASSERT_BYTES_EQ(w.vector(), v);
}

// ---------- data() pointer content ----------
static void test_ser_data_pointer_content()
{
    Serialization::serializer_t w;
    w.bytes(std::vector<unsigned char>{0x10, 0x20, 0x30});
    const auto *p = w.data();
    ASSERT_EQ(p[0], 0x10);
    ASSERT_EQ(p[1], 0x20);
    ASSERT_EQ(p[2], 0x30);
}

// ---------- varint<T> and varint<vector<T>> ----------
static void test_ser_varint_scalar()
{
    Serialization::serializer_t w;
    w.varint<uint32_t>(128u);
    const std::vector<unsigned char> expected = {0x80, 0x01};
    ASSERT_BYTES_EQ(w.vector(), expected);
}

static void test_ser_varint_vector_prefix_count_then_values()
{
    Serialization::serializer_t w;
    const std::vector<uint32_t> vals = {1, 2, 3, 128};
    w.varint(vals);
    // Expected: varint(count=4)=04, varint(1)=01, varint(2)=02, varint(3)=03,
    // varint(128)=80 01
    const std::vector<unsigned char> expected = {0x04, 0x01, 0x02, 0x03, 0x80, 0x01};
    ASSERT_BYTES_EQ(w.vector(), expected);
}

// ---------- writes chain consistently: many mixed operations ----------
static void test_ser_mixed_write_chain()
{
    Serialization::serializer_t w;
    w.boolean(true);                                     // 1 byte
    w.uint8(0xFF);                                       // 1 byte
    w.uint16(0x1234);                                    // 2 bytes
    w.uint32(0xDEADBEEFu);                               // 4 bytes
    w.uint64(0x0123456789ABCDEFULL);                     // 8 bytes
    w.bytes(std::vector<unsigned char>{0xAA, 0xBB, 0xCC}); // 3 bytes
    ASSERT_EQ(w.size(), static_cast<size_t>(19));
}

// ---------- many writes do not invalidate data() pointer across small growth ----------
// (not a reallocation test — just that the final pointer reflects final bytes)
static void test_ser_final_data_reflects_last_bytes()
{
    Serialization::serializer_t w;
    for (int i = 0; i < 16; ++i)
    {
        w.uint8(static_cast<unsigned char>(i));
    }
    for (size_t i = 0; i < 16; ++i)
    {
        ASSERT_EQ(w.data()[i], static_cast<unsigned char>(i));
    }
}

// ---------- vector() returns a copy ----------
static void test_ser_vector_returns_copy()
{
    Serialization::serializer_t w;
    w.uint32(0xCAFEBABE);
    auto v = w.vector();
    v[0] = 0x00;  // mutating the returned copy must not affect w
    ASSERT_NE(w.vector()[0], v[0]);
}

#endif
