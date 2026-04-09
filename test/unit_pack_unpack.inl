// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Unit tests for Serialization::pack / Serialization::unpack.

#ifndef SERIALIZATION_UNIT_PACK_UNPACK_INL
#define SERIALIZATION_UNIT_PACK_UNPACK_INL

#include <cstdint>
#include <initializer_list>
#include <limits>
#include <serialization_helper.h>
#include <vector>

namespace unit_pu
{
    // Generic size assertion: pack<T>(v).size() == sizeof(T).
    template<typename T> static void check_pack_size()
    {
        const auto out = Serialization::pack<T>(static_cast<T>(1));
        ASSERT_EQ(out.size(), sizeof(T));
    }

    // Generic round-trip grid helper: for each sample value, verify that
    // unpack(pack(v)) == v in both little- and big-endian layouts.
    template<typename T> static void check_roundtrip_grid(std::initializer_list<T> samples)
    {
        for (auto v : samples)
        {
            ASSERT_EQ(Serialization::unpack<T>(Serialization::pack<T>(v)), v);
            ASSERT_EQ(Serialization::unpack<T>(Serialization::pack<T>(v, true), 0, true), v);
        }
    }

    // Mass round-trip with a seeded xorshift; acts as an in-suite mini-fuzz.
    template<typename T, typename State>
    static void check_mass_roundtrip(State seed, int iters)
    {
        State state = seed;
        for (int i = 0; i < iters; ++i)
        {
            state ^= state << 13;
            state ^= state >> 7;
            state ^= state << 17;
            const auto v = static_cast<T>(state);
            ASSERT_EQ(Serialization::unpack<T>(Serialization::pack<T>(v)), v);
            ASSERT_EQ(Serialization::unpack<T>(Serialization::pack<T>(v, true), 0, true), v);
        }
    }
}  // namespace unit_pu

// ---------- pack size across widths ----------
static void test_pack_size_uint8() { unit_pu::check_pack_size<uint8_t>(); }
static void test_pack_size_uint16() { unit_pu::check_pack_size<uint16_t>(); }
static void test_pack_size_uint32() { unit_pu::check_pack_size<uint32_t>(); }
static void test_pack_size_uint64() { unit_pu::check_pack_size<uint64_t>(); }
static void test_pack_size_int8() { unit_pu::check_pack_size<int8_t>(); }
static void test_pack_size_int16() { unit_pu::check_pack_size<int16_t>(); }
static void test_pack_size_int32() { unit_pu::check_pack_size<int32_t>(); }
static void test_pack_size_int64() { unit_pu::check_pack_size<int64_t>(); }

// ---------- exhaustive round-trip for uint8 ----------
static void test_pack_unpack_roundtrip_uint8_all()
{
    for (int v = 0; v < 256; ++v)
    {
        const auto p = Serialization::pack<uint8_t>(static_cast<uint8_t>(v));
        ASSERT_EQ(Serialization::unpack<uint8_t>(p), static_cast<uint8_t>(v));
    }
}

// ---------- sampled round-trip across widths ----------
static void test_pack_unpack_roundtrip_uint16_grid()
{
    unit_pu::check_roundtrip_grid<uint16_t>(
        {0, 1, 0x7F, 0x80, 0xFF, 0x100, 0x1234, 0x7FFF, 0x8000, 0xFFFE, 0xFFFF});
}

static void test_pack_unpack_roundtrip_uint32_grid()
{
    unit_pu::check_roundtrip_grid<uint32_t>(
        {0u, 1u, 0xFFu, 0x100u, 0xFFFFu, 0x10000u, 0xDEADBEEFu, 0x7FFFFFFFu, 0x80000000u,
         0xFFFFFFFEu, 0xFFFFFFFFu});
}

static void test_pack_unpack_roundtrip_uint64_grid()
{
    unit_pu::check_roundtrip_grid<uint64_t>(
        {0ULL, 1ULL, 0xFFULL, 0x100ULL, 0xFFFFULL, 0x10000ULL, 0xFFFFFFFFULL, 0x100000000ULL,
         0xDEADBEEFCAFEBABEULL, 0x7FFFFFFFFFFFFFFFULL, 0x8000000000000000ULL,
         0xFFFFFFFFFFFFFFFEULL, std::numeric_limits<uint64_t>::max()});
}

static void test_pack_unpack_roundtrip_int8_grid()
{
    unit_pu::check_roundtrip_grid<int8_t>(
        {std::numeric_limits<int8_t>::min(), int8_t{-1}, int8_t{0}, int8_t{1},
         std::numeric_limits<int8_t>::max()});
}

static void test_pack_unpack_roundtrip_int16_grid()
{
    unit_pu::check_roundtrip_grid<int16_t>(
        {std::numeric_limits<int16_t>::min(), int16_t{-1234}, int16_t{-1}, int16_t{0},
         int16_t{1}, int16_t{1234}, std::numeric_limits<int16_t>::max()});
}

static void test_pack_unpack_roundtrip_int32_grid()
{
    unit_pu::check_roundtrip_grid<int32_t>(
        {std::numeric_limits<int32_t>::min(), -1, 0, 1, std::numeric_limits<int32_t>::max()});
}

static void test_pack_unpack_roundtrip_int64_grid()
{
    unit_pu::check_roundtrip_grid<int64_t>(
        {std::numeric_limits<int64_t>::min(), -1LL, 0LL, 1LL, std::numeric_limits<int64_t>::max()});
}

// ---------- big-endian byte order verification ----------
static void test_pack_unpack_uint16_be_roundtrip()
{
    const uint16_t v = 0x1234;
    const auto p = Serialization::pack<uint16_t>(v, true);
    ASSERT_EQ(p[0], 0x12);
    ASSERT_EQ(p[1], 0x34);
    ASSERT_EQ(Serialization::unpack<uint16_t>(p, 0, true), v);
}

static void test_pack_unpack_uint32_be_roundtrip()
{
    const uint32_t v = 0xDEADBEEF;
    const auto p = Serialization::pack<uint32_t>(v, true);
    ASSERT_EQ(p[0], 0xDE);
    ASSERT_EQ(p[1], 0xAD);
    ASSERT_EQ(p[2], 0xBE);
    ASSERT_EQ(p[3], 0xEF);
    ASSERT_EQ(Serialization::unpack<uint32_t>(p, 0, true), v);
}

static void test_pack_unpack_uint64_be_roundtrip()
{
    const uint64_t v = 0x0102030405060708ULL;
    const auto p = Serialization::pack<uint64_t>(v, true);
    for (size_t i = 0; i < 8; ++i)
    {
        ASSERT_EQ(p[i], static_cast<unsigned char>(i + 1));
    }
    ASSERT_EQ(Serialization::unpack<uint64_t>(p, 0, true), v);
}

// ---------- little-endian byte layout verification ----------
static void test_pack_uint16_le_layout()
{
    const auto p = Serialization::pack<uint16_t>(0x1234);
    ASSERT_EQ(p[0], 0x34);
    ASSERT_EQ(p[1], 0x12);
}

static void test_pack_uint32_le_layout()
{
    const auto p = Serialization::pack<uint32_t>(0xDEADBEEF);
    ASSERT_EQ(p[0], 0xEF);
    ASSERT_EQ(p[1], 0xBE);
    ASSERT_EQ(p[2], 0xAD);
    ASSERT_EQ(p[3], 0xDE);
}

static void test_pack_uint64_le_layout()
{
    const auto p = Serialization::pack<uint64_t>(0x0102030405060708ULL);
    for (size_t i = 0; i < 8; ++i)
    {
        ASSERT_EQ(p[i], static_cast<unsigned char>(8 - i));
    }
}

// ---------- unpack at non-zero offsets ----------
static void test_unpack_uint32_at_offset()
{
    std::vector<unsigned char> buf = {0xAA, 0xBB};
    const auto enc = Serialization::pack<uint32_t>(0xDEADBEEFu);
    buf.insert(buf.end(), enc.begin(), enc.end());
    buf.push_back(0xCC);
    ASSERT_EQ(Serialization::unpack<uint32_t>(buf, 2), 0xDEADBEEFu);
}

static void test_unpack_uint64_at_offset_be()
{
    std::vector<unsigned char> buf = {0x00, 0x00, 0x00};
    const auto enc = Serialization::pack<uint64_t>(0x0102030405060708ULL, true);
    buf.insert(buf.end(), enc.begin(), enc.end());
    ASSERT_EQ(Serialization::unpack<uint64_t>(buf, 3, true), 0x0102030405060708ULL);
}

// ---------- unpack error paths ----------
static void test_unpack_empty_buffer_throws()
{
    std::vector<unsigned char> empty;
    ASSERT_THROWS_TYPE(Serialization::unpack<uint32_t>(empty), std::range_error);
}

static void test_unpack_too_small_buffer_throws()
{
    std::vector<unsigned char> buf = {0x01, 0x02, 0x03};
    ASSERT_THROWS_TYPE(Serialization::unpack<uint32_t>(buf), std::range_error);
}

static void test_unpack_offset_exact_end_throws()
{
    std::vector<unsigned char> buf = {0x01, 0x02, 0x03, 0x04};
    ASSERT_THROWS_TYPE(Serialization::unpack<uint32_t>(buf, 1), std::range_error);
}

static void test_unpack_offset_just_fits()
{
    // LE bytes {EE,FF,00,11} -> 0xEE | (0xFF<<8) | (0x00<<16) | (0x11<<24) = 0x1100FFEE.
    std::vector<unsigned char> buf = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11};
    ASSERT_EQ(Serialization::unpack<uint32_t>(buf, 4), static_cast<uint32_t>(0x1100FFEE));
}

static void test_unpack_offset_huge_throws()
{
    std::vector<unsigned char> buf(8, 0);
    ASSERT_THROWS_TYPE(Serialization::unpack<uint32_t>(buf, 1000000), std::range_error);
}

static void test_unpack_zero_offset_empty_type_should_fit()
{
    std::vector<unsigned char> buf = {0x42};
    ASSERT_EQ(Serialization::unpack<uint8_t>(buf, 0), 0x42);
}

// ---------- mass round-trips (seeded) ----------
static void test_pack_unpack_mass_roundtrip_uint64()
{
    unit_pu::check_mass_roundtrip<uint64_t>(uint64_t{0xC0FFEE123456789AULL}, 2048);
}

static void test_pack_unpack_mass_roundtrip_int32()
{
    unit_pu::check_mass_roundtrip<int32_t>(uint32_t{0xABCDEF01u}, 2048);
}

// ---------- endianness symmetry ----------
static void test_pack_endianness_symmetry_uint32()
{
    const uint32_t v = 0x11223344;
    const auto le = Serialization::pack<uint32_t>(v);
    const auto be = Serialization::pack<uint32_t>(v, true);
    ASSERT_EQ(le.size(), be.size());
    for (size_t i = 0; i < le.size(); ++i)
    {
        ASSERT_EQ(le[i], be[le.size() - 1 - i]);
    }
}

// ---------- max value round-trip via the grid helper ----------
static void test_pack_unpack_uint16_max_le_be()
{
    unit_pu::check_roundtrip_grid<uint16_t>({std::numeric_limits<uint16_t>::max()});
}

static void test_pack_unpack_uint32_max_le_be()
{
    unit_pu::check_roundtrip_grid<uint32_t>({std::numeric_limits<uint32_t>::max()});
}

static void test_pack_unpack_uint64_max_le_be()
{
    unit_pu::check_roundtrip_grid<uint64_t>({std::numeric_limits<uint64_t>::max()});
}

// ---------- signed negative layout ----------
static void test_pack_int32_negative_bytes_match_cast()
{
    const auto p = Serialization::pack<int32_t>(-1);
    for (size_t i = 0; i < p.size(); ++i)
    {
        ASSERT_EQ(p[i], 0xFF);
    }
}

static void test_pack_int64_min_roundtrip()
{
    unit_pu::check_roundtrip_grid<int64_t>({std::numeric_limits<int64_t>::min()});
}

static void test_pack_unsigned_char_roundtrip()
{
    unit_pu::check_roundtrip_grid<unsigned char>({static_cast<unsigned char>(0x5A)});
}

#endif
