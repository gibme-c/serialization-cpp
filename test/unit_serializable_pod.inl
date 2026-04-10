// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Unit tests for SerializablePod<SIZE>. Exercises multiple sizes (8, 16, 32,
// 64, 128) along with error paths, JSON round-trips, and comparison operators.

#ifndef SERIALIZATION_UNIT_SERIALIZABLE_POD_INL
#define SERIALIZATION_UNIT_SERIALIZABLE_POD_INL

#include <cstring>
#include <deserializer_t.h>
#include <serializable_pod.h>
#include <serializer_t.h>
#include <sstream>
#include <string>

namespace unit_pod
{
    using P8 = SerializablePod<8>;
    using P16 = SerializablePod<16>;
    using P32 = SerializablePod<32>;
    using P64 = SerializablePod<64>;
    using P128 = SerializablePod<128>;
} // namespace unit_pod

// ---------- default ctor and empty() ----------
static void test_pod_default_is_empty_sz8()
{
    unit_pod::P8 p;
    ASSERT_TRUE(p.empty());
    for (size_t i = 0; i < 8; ++i)
    {
        ASSERT_EQ(p[i], 0);
    }
}

static void test_pod_default_is_empty_sz32()
{
    unit_pod::P32 p;
    ASSERT_TRUE(p.empty());
}

static void test_pod_default_is_empty_sz128()
{
    unit_pod::P128 p;
    ASSERT_TRUE(p.empty());
}

// ---------- size() returns SIZE ----------
static void test_pod_size_is_template_param()
{
    unit_pod::P8 p8;
    unit_pod::P16 p16;
    unit_pod::P32 p32;
    unit_pod::P64 p64;
    unit_pod::P128 p128;
    ASSERT_EQ(p8.size(), static_cast<size_t>(8));
    ASSERT_EQ(p16.size(), static_cast<size_t>(16));
    ASSERT_EQ(p32.size(), static_cast<size_t>(32));
    ASSERT_EQ(p64.size(), static_cast<size_t>(64));
    ASSERT_EQ(p128.size(), static_cast<size_t>(128));
}

// ---------- hex string ctor ----------
static void test_pod_hex_ctor_8()
{
    unit_pod::P8 p("0123456789abcdef");
    ASSERT_EQ(p[0], 0x01);
    ASSERT_EQ(p[7], 0xEF);
}

static void test_pod_hex_ctor_32()
{
    const std::string hex = "974506601a60dc465e6e9acddb563889e63471849ec4198656550354b8541fcb";
    unit_pod::P32 p(hex);
    ASSERT_EQ(p.to_string(), hex);
}

static void test_pod_hex_ctor_wrong_size_throws()
{
    ASSERT_THROWS_TYPE(unit_pod::P32("deadbeef"), std::length_error);
}

static void test_pod_hex_ctor_too_long_throws()
{
    ASSERT_THROWS_TYPE(unit_pod::P8("deadbeefdeadbeefdeadbeefdeadbeef"), std::length_error);
}

static void test_pod_hex_ctor_odd_length_throws()
{
    // from_hex throws length_error on odd input; the pod ctor propagates it
    // directly rather than wrapping it.
    ASSERT_THROWS_TYPE(unit_pod::P32(std::string(63, 'a')), std::length_error);
}

static void test_pod_hex_ctor_invalid_char_throws()
{
    ASSERT_THROWS_TYPE(unit_pod::P8("zzzzzzzzzzzzzzzz"), std::invalid_argument);
}

// ---------- serialize / deserialize via bytes vector ----------
static void test_pod_serialize_bytes_round_trip()
{
    unit_pod::P32 p("974506601a60dc465e6e9acddb563889e63471849ec4198656550354b8541fcb");
    const auto v = p.serialize();
    ASSERT_EQ(v.size(), static_cast<size_t>(32));
    unit_pod::P32 q;
    q.deserialize(v);
    ASSERT_TRUE(p == q);
}

static void test_pod_deserialize_wrong_size_throws()
{
    std::vector<unsigned char> wrong(31, 0);
    unit_pod::P32 p;
    ASSERT_THROWS_TYPE(p.deserialize(wrong), std::length_error);
}

static void test_pod_deserialize_empty_throws()
{
    std::vector<unsigned char> empty;
    unit_pod::P32 p;
    ASSERT_THROWS_TYPE(p.deserialize(empty), std::length_error);
}

static void test_pod_deserialize_too_long_throws()
{
    std::vector<unsigned char> too_long(40, 0xAA);
    unit_pod::P32 p;
    ASSERT_THROWS_TYPE(p.deserialize(too_long), std::length_error);
}

// ---------- serialize / deserialize via deserializer_t ----------
static void test_pod_serialize_via_reader_sized()
{
    Serialization::serializer_t w;
    unit_pod::P16 p;
    for (size_t i = 0; i < 16; ++i)
    {
        p[i] = static_cast<unsigned char>(i * 16 + i);
    }
    p.serialize(w);
    Serialization::deserializer_t r(w);
    unit_pod::P16 q;
    q.deserialize(r);
    ASSERT_TRUE(p == q);
}

static void test_pod_deserialize_via_reader_short_buffer_throws()
{
    Serialization::deserializer_t r({0x01, 0x02});
    unit_pod::P16 p;
    ASSERT_THROWS_TYPE(p.deserialize(r), std::range_error);
}

// ---------- comparison operators ----------
static void test_pod_equality_and_inequality()
{
    unit_pod::P8 a, b;
    ASSERT_TRUE(a == b);
    a[0] = 0x01;
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
}

static void test_pod_lt_gt_le_ge()
{
    unit_pod::P8 a, b;
    a[7] = 0x01; // highest-index byte is treated as most significant by the operator<
    b[7] = 0x02;
    ASSERT_TRUE(a < b);
    ASSERT_FALSE(b < a);
    ASSERT_TRUE(b > a);
    ASSERT_TRUE(a <= b);
    ASSERT_TRUE(b >= a);
    ASSERT_TRUE(a <= a);
    ASSERT_TRUE(a >= a);
    ASSERT_FALSE(a >= b);
    ASSERT_FALSE(b <= a);
}

static void test_pod_operators_equal_is_le_and_ge()
{
    unit_pod::P32 a("1111111111111111111111111111111111111111111111111111111111111111");
    unit_pod::P32 b(a);
    ASSERT_TRUE(a <= b);
    ASSERT_TRUE(a >= b);
    ASSERT_FALSE(a < b);
    ASSERT_FALSE(a > b);
}

// ---------- indexing operator ----------
static void test_pod_operator_index_read_write()
{
    unit_pod::P8 p;
    p[0] = 0xAB;
    p[7] = 0xCD;
    ASSERT_EQ(p[0], 0xAB);
    ASSERT_EQ(p[7], 0xCD);
}

static void test_pod_operator_dereference_returns_ptr()
{
    unit_pod::P8 p;
    unsigned char *ptr = *p;
    ptr[0] = 0xFE;
    ASSERT_EQ(p[0], 0xFE);
}

// ---------- data() points to the same bytes ----------
static void test_pod_data_ptr_matches_indexing()
{
    unit_pod::P16 p("00112233445566778899aabbccddeeff");
    for (size_t i = 0; i < 16; ++i)
    {
        ASSERT_EQ(p.data()[i], p[i]);
    }
}

// ---------- to_string() produces lowercase hex matching serialize() ----------
static void test_pod_to_string_matches_hex_encoding()
{
    const std::string hex = "0123456789abcdef0123456789abcdef";
    unit_pod::P16 p(hex);
    ASSERT_EQ(p.to_string(), hex);
}

// ---------- JSON round-trip via fromJSON / toJSON ----------
static void test_pod_fromJSON_string_round_trip()
{
    const std::string hex = "974506601a60dc465e6e9acddb563889e63471849ec4198656550354b8541fcb";
    unit_pod::P32 p(hex);
    JSON_INIT();
    p.toJSON(writer);
    JSON_DUMP(doc_str);
    ASSERT_EQ(doc_str, std::string("\"") + hex + "\"");

    STR_TO_JSON(doc_str, doc);
    unit_pod::P32 q;
    q.fromJSON(doc);
    ASSERT_TRUE(p == q);
}

static void test_pod_fromJSON_non_string_throws()
{
    rapidjson::Document doc;
    doc.Parse("123"); // number, not string
    ASSERT_FALSE(doc.HasParseError());
    unit_pod::P32 p;
    ASSERT_THROWS_TYPE(p.fromJSON(doc), std::invalid_argument);
}

static void test_pod_fromJSON_wrong_size_string_throws()
{
    rapidjson::Document doc;
    doc.Parse("\"deadbeef\"");
    ASSERT_FALSE(doc.HasParseError());
    unit_pod::P32 p;
    ASSERT_THROWS_TYPE(p.fromJSON(doc), std::length_error);
}

static void test_pod_fromJSON_invalid_hex_throws()
{
    rapidjson::Document doc;
    doc.Parse("\"zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz\"");
    ASSERT_FALSE(doc.HasParseError());
    unit_pod::P32 p;
    ASSERT_THROWS_TYPE(p.fromJSON(doc), std::invalid_argument);
}

// ---------- fromJSON(val, key) — load from object by key ----------
static void test_pod_fromJSON_by_key()
{
    const std::string hex = "974506601a60dc465e6e9acddb563889e63471849ec4198656550354b8541fcb";
    rapidjson::Document doc;
    doc.Parse((std::string("{\"hash\":\"") + hex + "\"}").c_str());
    ASSERT_FALSE(doc.HasParseError());
    unit_pod::P32 p;
    p.fromJSON(doc, "hash");
    ASSERT_EQ(p.to_string(), hex);
}

static void test_pod_fromJSON_missing_key_throws()
{
    rapidjson::Document doc;
    doc.Parse("{\"other\":\"aa\"}");
    ASSERT_FALSE(doc.HasParseError());
    unit_pod::P32 p;
    ASSERT_THROWS_TYPE(p.fromJSON(doc, "missing"), std::invalid_argument);
}

// ---------- ostream operator ----------
static void test_pod_ostream_is_hex()
{
    const std::string hex = "974506601a60dc465e6e9acddb563889e63471849ec4198656550354b8541fcb";
    unit_pod::P32 p(hex);
    std::ostringstream os;
    os << p;
    ASSERT_EQ(os.str(), hex);
}

// ---------- full byte-by-byte round trip via serializer/deserializer ----------
static void test_pod_full_pipeline_roundtrip_P64()
{
    unit_pod::P64 p;
    for (size_t i = 0; i < 64; ++i)
    {
        p[i] = static_cast<unsigned char>(0xA5 ^ i);
    }
    Serialization::serializer_t w;
    p.serialize(w);
    Serialization::deserializer_t r(w);
    unit_pod::P64 q;
    q.deserialize(r);
    ASSERT_TRUE(p == q);
}

static void test_pod_full_pipeline_roundtrip_P128()
{
    unit_pod::P128 p;
    for (size_t i = 0; i < 128; ++i)
    {
        p[i] = static_cast<unsigned char>(i * 3 + 7);
    }
    Serialization::serializer_t w;
    p.serialize(w);
    Serialization::deserializer_t r(w);
    unit_pod::P128 q;
    q.deserialize(r);
    ASSERT_TRUE(p == q);
}

// ---------- empty() flips when any byte is set ----------
static void test_pod_empty_flips_on_mutation()
{
    unit_pod::P32 p;
    ASSERT_TRUE(p.empty());
    p[15] = 0x01;
    ASSERT_FALSE(p.empty());
    p[15] = 0x00;
    ASSERT_TRUE(p.empty());
}

// ---------- P16 vs P32 cannot be compared (compile-time separation) ----------
// (No runtime test — just an instantiation sanity check.)
static void test_pod_distinct_sizes_instantiate_independently()
{
    unit_pod::P8 a;
    unit_pod::P16 b;
    unit_pod::P32 c;
    ASSERT_EQ(a.size(), static_cast<size_t>(8));
    ASSERT_EQ(b.size(), static_cast<size_t>(16));
    ASSERT_EQ(c.size(), static_cast<size_t>(32));
}

#endif
