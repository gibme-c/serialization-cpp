// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Unit tests for serialization_secure_erase. The compiler must not elide the
// zeroing operation; we verify by reading back through a volatile pointer so
// the optimizer can't assume the buffer is dead.

#ifndef SERIALIZATION_UNIT_SECURE_ERASE_INL
#define SERIALIZATION_UNIT_SECURE_ERASE_INL

#include <cstring>
#include <serialization_secure_erase.h>

static void test_secure_erase_one_byte()
{
    unsigned char buf[1];
    buf[0] = 0xCC;
    serialization_secure_erase(buf, sizeof(buf));
    volatile unsigned char *vp = buf;
    ASSERT_EQ(vp[0], 0);
}

static void test_secure_erase_small_buffer()
{
    unsigned char buf[16];
    std::memset(buf, 0xAA, sizeof(buf));
    serialization_secure_erase(buf, sizeof(buf));
    volatile unsigned char *vp = buf;
    for (size_t i = 0; i < sizeof(buf); ++i)
    {
        ASSERT_EQ(vp[i], 0);
    }
}

static void test_secure_erase_medium_buffer()
{
    unsigned char buf[1024];
    for (size_t i = 0; i < sizeof(buf); ++i)
    {
        buf[i] = static_cast<unsigned char>(i & 0xFF);
    }
    serialization_secure_erase(buf, sizeof(buf));
    volatile unsigned char *vp = buf;
    for (size_t i = 0; i < sizeof(buf); ++i)
    {
        ASSERT_EQ(vp[i], 0);
    }
}

static void test_secure_erase_large_buffer()
{
    std::vector<unsigned char> buf(64 * 1024, 0xFF);
    serialization_secure_erase(buf.data(), buf.size());
    volatile unsigned char *vp = buf.data();
    for (size_t i = 0; i < buf.size(); ++i)
    {
        ASSERT_EQ(vp[i], 0);
    }
}

static void test_secure_erase_zero_length_is_noop()
{
    unsigned char buf[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    serialization_secure_erase(buf, 0);
    // Content must be preserved — a zero-length erase must not touch memory.
    ASSERT_EQ(buf[0], 0xDE);
    ASSERT_EQ(buf[1], 0xAD);
    ASSERT_EQ(buf[2], 0xBE);
    ASSERT_EQ(buf[3], 0xEF);
}

static void test_secure_erase_unaligned_offset()
{
    unsigned char buf[32];
    std::memset(buf, 0xAA, sizeof(buf));
    // Erase starting from an odd offset within the buffer.
    serialization_secure_erase(buf + 3, 7);
    // First three bytes preserved.
    ASSERT_EQ(buf[0], 0xAA);
    ASSERT_EQ(buf[1], 0xAA);
    ASSERT_EQ(buf[2], 0xAA);
    // Next seven zeroed.
    volatile unsigned char *vp = buf;
    for (size_t i = 3; i < 10; ++i)
    {
        ASSERT_EQ(vp[i], 0);
    }
    // Remainder preserved.
    for (size_t i = 10; i < sizeof(buf); ++i)
    {
        ASSERT_EQ(vp[i], 0xAA);
    }
}

#endif
