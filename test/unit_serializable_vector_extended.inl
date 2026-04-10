// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Extended unit tests for SerializableVector<Type>. Covers valid podVV
// round-trips (including empty inner vectors), empty nested vector
// serialization, and size() consistency at varint encoding boundaries.

#ifndef SERIALIZATION_UNIT_SERIALIZABLE_VECTOR_EXTENDED_INL
#define SERIALIZATION_UNIT_SERIALIZABLE_VECTOR_EXTENDED_INL

#include <deserializer_t.h>
#include <serializable_pod.h>
#include <serializable_vector.h>
#include <serializer_t.h>
#include <vector>

namespace unit_svec_ext
{
    // A small POD for compact nested-vector tests.
    using Pod8 = SerializablePod<8>;

    struct V8 : SerializablePod<8>
    {
        V8() = default;
        explicit V8(const std::string &s): SerializablePod<8>(s) {}
        explicit V8(const JSONValue &j)
        {
            JSON_STRING_OR_THROW()
            from_string(j.GetString());
        }
    };
} // namespace unit_svec_ext

// ---------- podVV valid round-trips ----------
static void test_svec_ext_podVV_valid_roundtrip()
{
    // Build [[a, b], [c]]
    unit_svec_ext::Pod8 a, b, c;
    a[0] = 0x01;
    b[0] = 0x02;
    c[0] = 0x03;
    std::vector<std::vector<unit_svec_ext::Pod8>> vv = {{a, b}, {c}};

    Serialization::serializer_t w;
    w.pod(vv);

    Serialization::deserializer_t r(w);
    const auto result = r.podVV<unit_svec_ext::Pod8>();

    ASSERT_EQ(result.size(), static_cast<size_t>(2));
    ASSERT_EQ(result[0].size(), static_cast<size_t>(2));
    ASSERT_EQ(result[1].size(), static_cast<size_t>(1));
    ASSERT_EQ(result[0][0][0], 0x01);
    ASSERT_EQ(result[0][1][0], 0x02);
    ASSERT_EQ(result[1][0][0], 0x03);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

static void test_svec_ext_podVV_inner_empty()
{
    // Build [[]] — one inner vector that is empty.
    std::vector<std::vector<unit_svec_ext::Pod8>> vv = {{}};

    Serialization::serializer_t w;
    w.pod(vv);

    Serialization::deserializer_t r(w);
    const auto result = r.podVV<unit_svec_ext::Pod8>();

    ASSERT_EQ(result.size(), static_cast<size_t>(1));
    ASSERT_EQ(result[0].size(), static_cast<size_t>(0));
}

static void test_svec_ext_podVV_mixed_sizes()
{
    // Build [[a, b, c], [], [d]]
    unit_svec_ext::Pod8 a, b, c, d;
    a[0] = 0xAA;
    b[0] = 0xBB;
    c[0] = 0xCC;
    d[0] = 0xDD;
    std::vector<std::vector<unit_svec_ext::Pod8>> vv = {{a, b, c}, {}, {d}};

    Serialization::serializer_t w;
    w.pod(vv);

    Serialization::deserializer_t r(w);
    const auto result = r.podVV<unit_svec_ext::Pod8>();

    ASSERT_EQ(result.size(), static_cast<size_t>(3));
    ASSERT_EQ(result[0].size(), static_cast<size_t>(3));
    ASSERT_EQ(result[1].size(), static_cast<size_t>(0));
    ASSERT_EQ(result[2].size(), static_cast<size_t>(1));
    ASSERT_EQ(result[0][0][0], 0xAA);
    ASSERT_EQ(result[0][1][0], 0xBB);
    ASSERT_EQ(result[0][2][0], 0xCC);
    ASSERT_EQ(result[2][0][0], 0xDD);
}

// ---------- empty outer nested vector ----------
static void test_svec_ext_pod_empty_nested()
{
    std::vector<std::vector<unit_svec_ext::Pod8>> vv; // empty outer

    Serialization::serializer_t w;
    w.pod(vv);

    Serialization::deserializer_t r(w);
    const auto result = r.podVV<unit_svec_ext::Pod8>();

    ASSERT_EQ(result.size(), static_cast<size_t>(0));
}

// ---------- size() at varint boundary crossings ----------
static void test_svec_ext_size_boundary_127()
{
    // 127 elements: varint(127) = 0x7F = 1 byte
    SerializableVector<unit_svec_ext::V8> v;
    for (int i = 0; i < 127; ++i)
    {
        unit_svec_ext::V8 e;
        e[0] = static_cast<unsigned char>(i);
        v.append(e);
    }
    // size() = 1 (varint for 127) + 127 * 8 (elements)
    ASSERT_EQ(v.size(), static_cast<size_t>(1 + 127 * 8));
    ASSERT_EQ(v.size(), v.serialize().size());
}

static void test_svec_ext_size_boundary_128()
{
    // 128 elements: varint(128) = {0x80, 0x01} = 2 bytes
    SerializableVector<unit_svec_ext::V8> v;
    for (int i = 0; i < 128; ++i)
    {
        unit_svec_ext::V8 e;
        e[0] = static_cast<unsigned char>(i);
        v.append(e);
    }
    // size() = 2 (varint for 128) + 128 * 8 (elements)
    ASSERT_EQ(v.size(), static_cast<size_t>(2 + 128 * 8));
    ASSERT_EQ(v.size(), v.serialize().size());
}

static void test_svec_ext_size_consistency_multiple()
{
    const size_t counts[] = {0, 1, 127, 128, 255, 256};
    for (auto count : counts)
    {
        SerializableVector<unit_svec_ext::V8> v;
        for (size_t i = 0; i < count; ++i)
        {
            unit_svec_ext::V8 e;
            e[0] = static_cast<unsigned char>(i & 0xFF);
            v.append(e);
        }
        ASSERT_EQ(v.size(), v.serialize().size());
    }
}

#endif
