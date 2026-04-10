// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Extended unit tests for SerializablePod<SIZE>. Covers copy construction,
// copy assignment, SIZE=1 comparison operators, and comparison invariants
// (transitivity, antisymmetry).

#ifndef SERIALIZATION_UNIT_SERIALIZABLE_POD_EXTENDED_INL
#define SERIALIZATION_UNIT_SERIALIZABLE_POD_EXTENDED_INL

#include <serializable_pod.h>

namespace unit_pod_ext
{
    using P1 = SerializablePod<1>;
    using P8 = SerializablePod<8>;
    using P32 = SerializablePod<32>;
} // namespace unit_pod_ext

// ---------- copy construction ----------
static void test_pod_ext_copy_ctor()
{
    const std::string hex = "974506601a60dc465e6e9acddb563889e63471849ec4198656550354b8541fcb";
    unit_pod_ext::P32 a(hex);
    unit_pod_ext::P32 b(a);
    ASSERT_TRUE(a == b);
    ASSERT_EQ(a.to_string(), b.to_string());
}

static void test_pod_ext_copy_ctor_independent()
{
    const std::string hex = "974506601a60dc465e6e9acddb563889e63471849ec4198656550354b8541fcb";
    unit_pod_ext::P32 a(hex);
    unit_pod_ext::P32 b(a);
    a[0] = 0x00;
    ASSERT_NE(a[0], b[0]);
    ASSERT_EQ(b[0], 0x97);
}

// ---------- copy assignment ----------
static void test_pod_ext_copy_assignment()
{
    const std::string hex = "974506601a60dc465e6e9acddb563889e63471849ec4198656550354b8541fcb";
    unit_pod_ext::P32 a(hex);
    unit_pod_ext::P32 b;
    ASSERT_TRUE(b.empty());
    b = a;
    ASSERT_TRUE(a == b);
    ASSERT_EQ(b.to_string(), hex);
}

static void test_pod_ext_copy_assignment_self()
{
    const std::string hex = "974506601a60dc465e6e9acddb563889e63471849ec4198656550354b8541fcb";
    unit_pod_ext::P32 a(hex);
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
    a = a;
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    ASSERT_EQ(a.to_string(), hex);
}

static void test_pod_ext_copy_assignment_independent()
{
    const std::string hex = "974506601a60dc465e6e9acddb563889e63471849ec4198656550354b8541fcb";
    unit_pod_ext::P32 a(hex);
    unit_pod_ext::P32 b;
    b = a;
    b[0] = 0xFF;
    ASSERT_EQ(a[0], 0x97);
    ASSERT_EQ(b[0], 0xFF);
}

// ---------- SIZE=1 comparison operators ----------
static void test_pod_ext_cmp_size1_less()
{
    unit_pod_ext::P1 a;
    unit_pod_ext::P1 b;
    a[0] = 0x00;
    b[0] = 0x01;
    ASSERT_TRUE(a < b);
    ASSERT_TRUE(a <= b);
    ASSERT_TRUE(a != b);
    ASSERT_FALSE(a > b);
    ASSERT_FALSE(a >= b);
    ASSERT_FALSE(a == b);
}

static void test_pod_ext_cmp_size1_equal()
{
    unit_pod_ext::P1 a;
    unit_pod_ext::P1 b;
    a[0] = 0xAA;
    b[0] = 0xAA;
    ASSERT_TRUE(a == b);
    ASSERT_TRUE(a <= b);
    ASSERT_TRUE(a >= b);
    ASSERT_FALSE(a < b);
    ASSERT_FALSE(a > b);
    ASSERT_FALSE(a != b);
}

// ---------- all-0xFF equality ----------
static void test_pod_ext_cmp_all_ff_equal()
{
    unit_pod_ext::P8 a;
    unit_pod_ext::P8 b;
    for (size_t i = 0; i < 8; ++i)
    {
        a[i] = 0xFF;
        b[i] = 0xFF;
    }
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a != b);
    ASSERT_FALSE(a < b);
    ASSERT_FALSE(a > b);
}

// ---------- transitivity: a < b < c => a < c ----------
static void test_pod_ext_cmp_transitivity()
{
    unit_pod_ext::P8 a, b, c;
    // Comparison is big-endian (from highest index). Set byte[7] to differ.
    a[7] = 0x01;
    b[7] = 0x02;
    c[7] = 0x03;
    ASSERT_TRUE(a < b);
    ASSERT_TRUE(b < c);
    ASSERT_TRUE(a < c);
    ASSERT_TRUE(a <= c);
    ASSERT_TRUE(c > a);
    ASSERT_TRUE(c >= a);
}

// ---------- antisymmetry: a < b => !(b < a) ----------
static void test_pod_ext_cmp_antisymmetry()
{
    unit_pod_ext::P8 a, b;
    a[7] = 0x10;
    b[7] = 0x20;
    ASSERT_TRUE(a < b);
    ASSERT_FALSE(b < a);
    ASSERT_TRUE(b > a);
    ASSERT_FALSE(a > b);
}

#endif
