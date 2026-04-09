// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Unit tests for Serialization::encode_varint / Serialization::decode_varint.

#ifndef SERIALIZATION_UNIT_VARINT_INL
#define SERIALIZATION_UNIT_VARINT_INL

#include <cstdint>
#include <initializer_list>
#include <limits>
#include <serialization_helper.h>
#include <tuple>
#include <vector>

namespace unit_vi
{
    template<typename T>
    static void check_encode(T value, std::initializer_list<unsigned char> expected)
    {
        const auto e = Serialization::encode_varint<T>(value);
        ASSERT_EQ(e.size(), expected.size());
        size_t i = 0;
        for (auto b : expected)
        {
            ASSERT_EQ(e[i++], b);
        }
    }

    template<typename T> static void check_roundtrip(std::initializer_list<T> samples)
    {
        for (auto v : samples)
        {
            const auto e = Serialization::encode_varint<T>(v);
            const auto [decoded, consumed] = Serialization::decode_varint<T>(e);
            ASSERT_EQ(decoded, v);
            ASSERT_EQ(consumed, e.size());
        }
    }

    template<typename T> static void check_single_byte_decode(
        const std::vector<unsigned char> &buf, T expected_value, size_t expected_consumed)
    {
        const auto [r, c] = Serialization::decode_varint<T>(buf);
        ASSERT_EQ(r, expected_value);
        ASSERT_EQ(c, expected_consumed);
    }
}  // namespace unit_vi

// ---------- encode spot layouts ----------
static void test_encode_varint_zero_uint8() { unit_vi::check_encode<uint8_t>(0, {0x00}); }
static void test_encode_varint_0x7f_uint8() { unit_vi::check_encode<uint8_t>(0x7F, {0x7F}); }
static void test_encode_varint_0x80_uint8() { unit_vi::check_encode<uint8_t>(0x80, {0x80, 0x01}); }
static void test_encode_varint_0xff_uint8() { unit_vi::check_encode<uint8_t>(0xFF, {0xFF, 0x01}); }
static void test_encode_varint_0x3fff_uint16() { unit_vi::check_encode<uint16_t>(0x3FFF, {0xFF, 0x7F}); }
static void test_encode_varint_0x4000_uint16() { unit_vi::check_encode<uint16_t>(0x4000, {0x80, 0x80, 0x01}); }
static void test_encode_varint_0xffff_uint16() { unit_vi::check_encode<uint16_t>(0xFFFF, {0xFF, 0xFF, 0x03}); }

// ---------- exhaustive round trips for narrow widths ----------
static void test_varint_roundtrip_uint8_exhaustive()
{
    for (int v = 0; v < 256; ++v)
    {
        const auto e = Serialization::encode_varint<uint8_t>(static_cast<uint8_t>(v));
        const auto [decoded, consumed] = Serialization::decode_varint<uint8_t>(e);
        ASSERT_EQ(decoded, static_cast<uint8_t>(v));
        ASSERT_EQ(consumed, e.size());
    }
}

static void test_varint_roundtrip_uint16_exhaustive()
{
    for (int v = 0; v <= 0xFFFF; ++v)
    {
        const auto e = Serialization::encode_varint<uint16_t>(static_cast<uint16_t>(v));
        const auto [decoded, consumed] = Serialization::decode_varint<uint16_t>(e);
        ASSERT_EQ(decoded, static_cast<uint16_t>(v));
        ASSERT_EQ(consumed, e.size());
    }
}

// ---------- sampled grids ----------
static void test_varint_roundtrip_uint32_grid()
{
    unit_vi::check_roundtrip<uint32_t>(
        {0u, 1u, 0x7Fu, 0x80u, 0x3FFFu, 0x4000u, 0x1FFFFFu, 0x200000u, 0xFFFFFFFu, 0x10000000u,
         0x7FFFFFFFu, 0x80000000u, 0xFFFFFFFEu, 0xFFFFFFFFu});
}

static void test_varint_roundtrip_uint64_grid()
{
    unit_vi::check_roundtrip<uint64_t>(
        {0ULL, 1ULL, 0x7FULL, 0x80ULL, 0x3FFFULL, 0x4000ULL, 0x1FFFFFULL, 0xFFFFFFFFULL,
         0x100000000ULL, 0xFFFFFFFFFFULL, 0x100000000000ULL, 0xFFFFFFFFFFFFFFULL,
         0x0100000000000000ULL, 0x7FFFFFFFFFFFFFFFULL, 0x8000000000000000ULL,
         0xFFFFFFFFFFFFFFFEULL, std::numeric_limits<uint64_t>::max()});
}

// ---------- decode error paths ----------
static void test_decode_varint_empty_buffer_throws()
{
    std::vector<unsigned char> empty;
    ASSERT_THROWS_TYPE(Serialization::decode_varint<uint32_t>(empty), std::range_error);
}

static void test_decode_varint_truncated_continuation_throws()
{
    for (size_t n = 1; n <= 4; ++n)
    {
        std::vector<unsigned char> buf(n, 0x80);
        ASSERT_THROWS_TYPE(Serialization::decode_varint<uint32_t>(buf), std::range_error);
    }
}

static void test_decode_varint_offset_past_end_throws()
{
    std::vector<unsigned char> buf = {0x01, 0x02, 0x03};
    ASSERT_THROWS_TYPE(Serialization::decode_varint<uint32_t>(buf, 10), std::range_error);
}

static void test_decode_varint_offset_equals_size_throws()
{
    std::vector<unsigned char> buf = {0x01, 0x02, 0x03};
    ASSERT_THROWS_TYPE(Serialization::decode_varint<uint32_t>(buf, 3), std::range_error);
}

static void test_decode_varint_shift_overflow_uint8()
{
    std::vector<unsigned char> buf = {0x80, 0x80, 0x01};
    ASSERT_THROWS_TYPE(Serialization::decode_varint<uint8_t>(buf), std::range_error);
}

static void test_decode_varint_shift_overflow_uint16()
{
    std::vector<unsigned char> buf = {0x80, 0x80, 0x80, 0x01};
    ASSERT_THROWS_TYPE(Serialization::decode_varint<uint16_t>(buf), std::range_error);
}

static void test_decode_varint_shift_overflow_uint32()
{
    std::vector<unsigned char> buf = {0x80, 0x80, 0x80, 0x80, 0x80, 0x01};
    ASSERT_THROWS_TYPE(Serialization::decode_varint<uint32_t>(buf), std::range_error);
}

static void test_decode_varint_shift_overflow_uint64()
{
    std::vector<unsigned char> buf = {
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01};
    ASSERT_THROWS_TYPE(Serialization::decode_varint<uint64_t>(buf), std::range_error);
}

// ---------- consumed-bytes count and offset chaining ----------
static void test_decode_varint_consumed_at_offset()
{
    std::vector<unsigned char> buf = {0xFF, 0xFF};
    const auto e = Serialization::encode_varint<uint32_t>(123456u);
    buf.insert(buf.end(), e.begin(), e.end());
    buf.push_back(0xAA);
    const auto [decoded, consumed] = Serialization::decode_varint<uint32_t>(buf, 2);
    ASSERT_EQ(decoded, 123456u);
    ASSERT_EQ(consumed, e.size());
}

static void test_decode_varint_stops_at_terminator()
{
    std::vector<unsigned char> buf = {0x01, 0xFF, 0xDE, 0xAD};
    const auto [decoded, consumed] = Serialization::decode_varint<uint32_t>(buf);
    ASSERT_EQ(decoded, 1u);
    ASSERT_EQ(consumed, static_cast<size_t>(1));
}

static void test_decode_varint_single_byte_all_widths()
{
    std::vector<unsigned char> buf = {0x42};
    unit_vi::check_single_byte_decode<uint8_t>(buf, 0x42, 1);
    unit_vi::check_single_byte_decode<uint16_t>(buf, 0x42, 1);
    unit_vi::check_single_byte_decode<uint32_t>(buf, 0x42u, 1);
    unit_vi::check_single_byte_decode<uint64_t>(buf, 0x42ULL, 1);
}

static void test_decode_varint_two_byte_128()
{
    std::vector<unsigned char> buf = {0x80, 0x01};
    unit_vi::check_single_byte_decode<uint16_t>(buf, 128u, 2);
    unit_vi::check_single_byte_decode<uint32_t>(buf, 128u, 2);
}

static void test_varint_stream_multiple_values()
{
    std::vector<unsigned char> buf;
    auto append = [&](const std::vector<unsigned char> &v) { buf.insert(buf.end(), v.begin(), v.end()); };
    const uint32_t values[] = {0u, 1u, 127u, 128u, 0xDEADBEEFu};
    for (auto v : values)
    {
        append(Serialization::encode_varint<uint32_t>(v));
    }

    size_t offset = 0;
    for (auto expected : values)
    {
        const auto [r, c] = Serialization::decode_varint<uint32_t>(buf, offset);
        ASSERT_EQ(r, expected);
        offset += c;
    }
    ASSERT_EQ(offset, buf.size());
}

// Encode a value then flip its terminator's continuation bit; the decoder
// must reject rather than crash or return garbage.
static void test_decode_varint_corrupt_top_bit_rejects_when_overflow()
{
    auto e = Serialization::encode_varint<uint32_t>(0xFFFFFFFFu);
    e.back() |= 0x80;
    ASSERT_THROWS_TYPE(Serialization::decode_varint<uint32_t>(e), std::range_error);
}

static void test_decode_varint_uint8_multi_byte_rejected()
{
    std::vector<unsigned char> buf = {0x80, 0x80, 0x01};
    ASSERT_THROWS_TYPE(Serialization::decode_varint<uint8_t>(buf), std::range_error);
}

// ---------- narrowing overflow into narrow types ----------
static void test_decode_varint_uint8_overflow_two_byte()
{
    std::vector<unsigned char> buf = {0x80, 0x02};  // encodes 256
    ASSERT_THROWS_TYPE(Serialization::decode_varint<uint8_t>(buf), std::range_error);
}

static void test_decode_varint_uint8_overflow_top_bit_of_second()
{
    std::vector<unsigned char> buf = {0xFF, 0x02};  // encodes 0x17F
    ASSERT_THROWS_TYPE(Serialization::decode_varint<uint8_t>(buf), std::range_error);
}

static void test_decode_varint_uint16_overflow_three_byte()
{
    std::vector<unsigned char> buf = {0x80, 0x80, 0x04};  // encodes 0x10000
    ASSERT_THROWS_TYPE(Serialization::decode_varint<uint16_t>(buf), std::range_error);
}

static void test_decode_varint_uint32_overflow_five_byte()
{
    std::vector<unsigned char> buf = {0x80, 0x80, 0x80, 0x80, 0x10};  // encodes 0x100000000
    ASSERT_THROWS_TYPE(Serialization::decode_varint<uint32_t>(buf), std::range_error);
}

static void test_decode_varint_uint8_max_decodes_ok()
{
    std::vector<unsigned char> buf = {0xFF, 0x01};  // 255
    unit_vi::check_single_byte_decode<uint8_t>(buf, static_cast<uint8_t>(0xFF), 2);
}

static void test_decode_varint_uint16_max_decodes_ok()
{
    std::vector<unsigned char> buf = {0xFF, 0xFF, 0x03};  // 65535
    unit_vi::check_single_byte_decode<uint16_t>(buf, static_cast<uint16_t>(0xFFFF), 3);
}

static void test_decode_varint_uint32_max_decodes_ok()
{
    std::vector<unsigned char> buf = {0xFF, 0xFF, 0xFF, 0xFF, 0x0F};
    unit_vi::check_single_byte_decode<uint32_t>(buf, 0xFFFFFFFFu, 5);
}

static void test_decode_varint_uint64_max_decodes_ok()
{
    // uint64 max = 9 * 0xFF + 0x01 (bit 0 of the 10th byte maps to bit 63).
    std::vector<unsigned char> buf = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01};
    unit_vi::check_single_byte_decode<uint64_t>(buf, 0xFFFFFFFFFFFFFFFFULL, 10);
}

// Any bit beyond bit 0 of the 10th byte represents bit 64+ of the value
// and must be rejected rather than silently dropped.
static void test_decode_varint_uint64_final_byte_bit1_rejected()
{
    std::vector<unsigned char> buf = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02};
    ASSERT_THROWS_TYPE(Serialization::decode_varint<uint64_t>(buf), std::range_error);
}

static void test_decode_varint_uint64_final_byte_high_bits_rejected()
{
    for (unsigned char bad : {0x02u, 0x04u, 0x08u, 0x10u, 0x20u, 0x40u})
    {
        std::vector<unsigned char> buf = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
                                          static_cast<unsigned char>(bad)};
        ASSERT_THROWS_TYPE(Serialization::decode_varint<uint64_t>(buf), std::range_error);
    }
}

// ---------- seeded mass round trips ----------
static void test_varint_mass_roundtrip_uint32()
{
    uint32_t state = 0xC0FFEEu;
    for (int i = 0; i < 4096; ++i)
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        const auto e = Serialization::encode_varint<uint32_t>(state);
        const auto [r, c] = Serialization::decode_varint<uint32_t>(e);
        ASSERT_EQ(r, state);
        ASSERT_EQ(c, e.size());
    }
}

static void test_varint_mass_roundtrip_uint64()
{
    uint64_t state = 0xDEADBEEFCAFEULL;
    for (int i = 0; i < 4096; ++i)
    {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        const auto e = Serialization::encode_varint<uint64_t>(state);
        const auto [r, c] = Serialization::decode_varint<uint64_t>(e);
        ASSERT_EQ(r, state);
        ASSERT_EQ(c, e.size());
    }
}

// ---------- encode length monotonicity ----------
static void test_encode_varint_length_monotonic_uint32()
{
    ASSERT_EQ(Serialization::encode_varint<uint32_t>(0x7Fu).size(), 1u);
    ASSERT_EQ(Serialization::encode_varint<uint32_t>(0x80u).size(), 2u);
    ASSERT_EQ(Serialization::encode_varint<uint32_t>(0x3FFFu).size(), 2u);
    ASSERT_EQ(Serialization::encode_varint<uint32_t>(0x4000u).size(), 3u);
    ASSERT_EQ(Serialization::encode_varint<uint32_t>(0x1FFFFFu).size(), 3u);
    ASSERT_EQ(Serialization::encode_varint<uint32_t>(0x200000u).size(), 4u);
    ASSERT_EQ(Serialization::encode_varint<uint32_t>(0xFFFFFFFu).size(), 4u);
    ASSERT_EQ(Serialization::encode_varint<uint32_t>(0x10000000u).size(), 5u);
    ASSERT_EQ(Serialization::encode_varint<uint32_t>(0xFFFFFFFFu).size(), 5u);
}

static void test_decode_varint_consumed_equals_buffer_size_tight()
{
    const auto e = Serialization::encode_varint<uint32_t>(0xABCDEFu);
    const auto [r, c] = Serialization::decode_varint<uint32_t>(e);
    ASSERT_EQ(r, 0xABCDEFu);
    ASSERT_EQ(c, e.size());
}

#endif
