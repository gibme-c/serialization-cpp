// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Unit tests for SerializableVector<Type>. Includes coverage of nested
// vectors, JSON round-trips (via fromJSON, not the ctor — see note in
// test_svec_ctor_from_json_value_throws), and adversarial varint counts.

#ifndef SERIALIZATION_UNIT_SERIALIZABLE_VECTOR_INL
#define SERIALIZATION_UNIT_SERIALIZABLE_VECTOR_INL

#include <serializable_pod.h>
#include <serializable_vector.h>
#include <string>
#include <vector>

namespace unit_svec
{
    // A SerializablePod<32> subtype that can be constructed from a JSONValue
    // (expected to be a string), so it satisfies SerializableVector<Type>'s
    // fromJSON requirement that elements be constructible from JSONValue.
    struct V : SerializablePod<32>
    {
        V() = default;
        explicit V(const std::string &s): SerializablePod<32>(s) {}
        explicit V(const JSONValue &j)
        {
            JSON_STRING_OR_THROW()
            from_string(j.GetString());
        }
    };
} // namespace unit_svec

static const std::string k_svec_hex_a = "974506601a60dc465e6e9acddb563889e63471849ec4198656550354b8541fcb";
static const std::string k_svec_hex_b = "0101010101010101010101010101010101010101010101010101010101010101";
static const std::string k_svec_hex_z = std::string(64, '0');

// ---------- default ctor empty ----------
static void test_svec_default_empty()
{
    SerializableVector<unit_svec::V> v;
    ASSERT_EQ(v.count(), static_cast<size_t>(0));
    ASSERT_EQ(v.size(), static_cast<size_t>(1));
}

// ---------- append / back ----------
static void test_svec_append_and_back()
{
    SerializableVector<unit_svec::V> v;
    unit_svec::V a(k_svec_hex_a);
    unit_svec::V b(k_svec_hex_b);
    v.append(a);
    v.append(b);
    ASSERT_EQ(v.count(), static_cast<size_t>(2));
    ASSERT_TRUE(v.back() == b);
    ASSERT_TRUE(v[0] == a);
}

// ---------- extend(vector) ----------
static void test_svec_extend_vector_expanded()
{
    SerializableVector<unit_svec::V> v;
    std::vector<unit_svec::V> add = {unit_svec::V(k_svec_hex_a), unit_svec::V(k_svec_hex_b)};
    v.extend(add);
    ASSERT_EQ(v.count(), static_cast<size_t>(2));
    ASSERT_TRUE(v[0] == add[0]);
    ASSERT_TRUE(v[1] == add[1]);
}

// ---------- extend(SerializableVector) ----------
static void test_svec_extend_svec_expanded()
{
    SerializableVector<unit_svec::V> a;
    a.append(unit_svec::V(k_svec_hex_a));
    SerializableVector<unit_svec::V> b;
    b.append(unit_svec::V(k_svec_hex_b));
    a.extend(b);
    ASSERT_EQ(a.count(), static_cast<size_t>(2));
    ASSERT_TRUE(a[1] == b[0]);
}

// ---------- equality / inequality ----------
// Historical note: operator== previously used the two-iterator form of
// std::equal which silently ignored length mismatches. Fixed to use the
// four-iterator overload. The _length_mismatch test below was flipped
// from pinning-broken-behavior to pinning-correct-behavior as part of
// that fix.
static void test_svec_equality_same_length()
{
    SerializableVector<unit_svec::V> a;
    a.append(unit_svec::V(k_svec_hex_a));
    SerializableVector<unit_svec::V> b;
    b.append(unit_svec::V(k_svec_hex_a));
    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a != b);
}

static void test_svec_equality_differs_at_index()
{
    SerializableVector<unit_svec::V> a;
    a.append(unit_svec::V(k_svec_hex_a));
    SerializableVector<unit_svec::V> b;
    b.append(unit_svec::V(k_svec_hex_b));
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
}

// Regression for the previously-broken length-mismatch comparison. A vector
// with fewer elements must NOT compare equal to a longer vector whose prefix
// matches — both must have the same size AND the same contents.
static void test_svec_equality_length_mismatch_not_equal()
{
    SerializableVector<unit_svec::V> a;
    a.append(unit_svec::V(k_svec_hex_a));
    SerializableVector<unit_svec::V> b;
    b.append(unit_svec::V(k_svec_hex_a));
    b.append(unit_svec::V(k_svec_hex_b));
    ASSERT_FALSE(a == b);
    ASSERT_TRUE(a != b);
    // And the reverse direction (longer on the left).
    ASSERT_FALSE(b == a);
    ASSERT_TRUE(b != a);
}

// ---------- operator[] read ----------
static void test_svec_operator_index()
{
    SerializableVector<unit_svec::V> v;
    v.append(unit_svec::V(k_svec_hex_a));
    v.append(unit_svec::V(k_svec_hex_b));
    ASSERT_EQ(v[0].to_string(), k_svec_hex_a);
    ASSERT_EQ(v[1].to_string(), k_svec_hex_b);
}

// ---------- serialize / deserialize binary round-trip ----------
static void test_svec_serialize_roundtrip()
{
    SerializableVector<unit_svec::V> v;
    v.append(unit_svec::V(k_svec_hex_a));
    v.append(unit_svec::V(k_svec_hex_b));
    v.append(unit_svec::V(k_svec_hex_z));
    const auto bytes = v.serialize();
    SerializableVector<unit_svec::V> copy;
    copy.deserialize(bytes);
    ASSERT_TRUE(v == copy);
}

static void test_svec_serialize_empty()
{
    SerializableVector<unit_svec::V> v;
    const auto bytes = v.serialize();
    // Expected: a single zero byte (the varint count = 0).
    ASSERT_EQ(bytes.size(), static_cast<size_t>(1));
    ASSERT_EQ(bytes[0], 0x00);
    SerializableVector<unit_svec::V> copy;
    copy.deserialize(bytes);
    ASSERT_EQ(copy.count(), static_cast<size_t>(0));
}

// ---------- deserialize via reader ----------
static void test_svec_deserialize_via_reader()
{
    SerializableVector<unit_svec::V> v;
    v.append(unit_svec::V(k_svec_hex_a));
    v.append(unit_svec::V(k_svec_hex_b));
    const auto bytes = v.serialize();
    Serialization::deserializer_t r(bytes);
    SerializableVector<unit_svec::V> copy;
    copy.deserialize(r);
    ASSERT_TRUE(v == copy);
    ASSERT_EQ(r.unread_bytes(), static_cast<size_t>(0));
}

// ---------- hex-string ctor ----------
static void test_svec_ctor_from_hex_string_roundtrip()
{
    SerializableVector<unit_svec::V> v;
    v.append(unit_svec::V(k_svec_hex_a));
    v.append(unit_svec::V(k_svec_hex_b));
    const auto hex = v.to_string();
    SerializableVector<unit_svec::V> copy(hex);
    ASSERT_TRUE(v == copy);
}

static void test_svec_ctor_from_empty_hex_string()
{
    SerializableVector<unit_svec::V> copy(std::string("00"));
    ASSERT_EQ(copy.count(), static_cast<size_t>(0));
}

static void test_svec_ctor_from_invalid_hex_throws()
{
    ASSERT_THROWS_TYPE(SerializableVector<unit_svec::V>(std::string("zz")), std::invalid_argument);
}

// ---------- JSON array constructor (post-fix) ----------
static void test_svec_ctor_from_json_array_value()
{
    const std::string js = std::string("[\"") + k_svec_hex_a + "\",\"" + k_svec_hex_b + "\"]";
    rapidjson::Document doc;
    doc.Parse(js.c_str());
    ASSERT_FALSE(doc.HasParseError());
    SerializableVector<unit_svec::V> v(doc);
    ASSERT_EQ(v.count(), static_cast<size_t>(2));
    ASSERT_EQ(v[0].to_string(), k_svec_hex_a);
    ASSERT_EQ(v[1].to_string(), k_svec_hex_b);
}

static void test_svec_ctor_from_json_object_value_throws()
{
    // An object is not a valid direct input for the constructor — only a
    // JSON array is accepted.
    rapidjson::Document doc;
    doc.Parse("{\"k\":1}");
    ASSERT_FALSE(doc.HasParseError());
    // Lambda wrapper avoids the most-vexing-parse ambiguity that would
    // otherwise treat `SerializableVector<V>(doc)` as a variable declaration
    // shadowing `doc`.
    ASSERT_THROWS_TYPE(
        [&]()
        {
            SerializableVector<unit_svec::V> v(doc);
            (void)v;
        }(),
        std::invalid_argument);
}

static void test_svec_ctor_from_json_by_key_array()
{
    const std::string js = std::string("{\"items\":[\"") + k_svec_hex_a + "\"]}";
    rapidjson::Document doc;
    doc.Parse(js.c_str());
    ASSERT_FALSE(doc.HasParseError());
    SerializableVector<unit_svec::V> v(doc, "items");
    ASSERT_EQ(v.count(), static_cast<size_t>(1));
    ASSERT_EQ(v[0].to_string(), k_svec_hex_a);
}

// ---------- adversarial deserialization: inflated count ----------
static void test_svec_deserialize_inflated_count_throws()
{
    // Manually write a varint count of 0xFFFFFFFFFFFFFF (close to max safe) and
    // nothing else. The first inner pod<> read should throw range_error when
    // the buffer runs out.
    Serialization::serializer_t w;
    w.varint<uint64_t>(0x00FFFFFFFFFFFFFFULL);
    SerializableVector<unit_svec::V> v;
    ASSERT_THROWS_TYPE(v.deserialize(w.vector()), std::range_error);
}

static void test_svec_deserialize_count_one_empty_payload_throws()
{
    // count=1 but zero payload bytes — first element read must throw.
    Serialization::serializer_t w;
    w.varint<uint64_t>(1ULL);
    SerializableVector<unit_svec::V> v;
    ASSERT_THROWS_TYPE(v.deserialize(w.vector()), std::range_error);
}

static void test_svec_deserialize_count_two_one_payload_throws()
{
    // count=2 but only one payload element (32 bytes) — second read must throw.
    Serialization::serializer_t w;
    w.varint<uint64_t>(2ULL);
    unit_svec::V only(k_svec_hex_a);
    w.pod(only);
    SerializableVector<unit_svec::V> v;
    ASSERT_THROWS_TYPE(v.deserialize(w.vector()), std::range_error);
}

// ---------- toJSON + fromJSON array round-trip ----------
static void test_svec_toJSON_fromJSON_array_roundtrip()
{
    SerializableVector<unit_svec::V> v;
    v.append(unit_svec::V(k_svec_hex_a));
    v.append(unit_svec::V(k_svec_hex_b));

    JSON_INIT();
    v.toJSON(writer);
    JSON_DUMP(s);

    STR_TO_JSON(s, doc);
    ASSERT_TRUE(doc.IsArray());

    SerializableVector<unit_svec::V> copy;
    copy.fromJSON(doc);
    ASSERT_TRUE(v == copy);
}

static void test_svec_fromJSON_non_array_throws()
{
    rapidjson::Document doc;
    doc.Parse("\"not an array\"");
    ASSERT_FALSE(doc.HasParseError());
    SerializableVector<unit_svec::V> v;
    ASSERT_THROWS_TYPE(v.fromJSON(doc), std::invalid_argument);
}

static void test_svec_fromJSON_empty_array()
{
    rapidjson::Document doc;
    doc.Parse("[]");
    ASSERT_FALSE(doc.HasParseError());
    SerializableVector<unit_svec::V> v;
    v.fromJSON(doc);
    ASSERT_EQ(v.count(), static_cast<size_t>(0));
}

static void test_svec_fromJSON_by_key()
{
    const std::string js = std::string("{\"items\":[\"") + k_svec_hex_a + "\",\"" + k_svec_hex_b + "\"]}";
    rapidjson::Document doc;
    doc.Parse(js.c_str());
    ASSERT_FALSE(doc.HasParseError());
    SerializableVector<unit_svec::V> v;
    v.fromJSON(doc, "items");
    ASSERT_EQ(v.count(), static_cast<size_t>(2));
    ASSERT_EQ(v[0].to_string(), k_svec_hex_a);
    ASSERT_EQ(v[1].to_string(), k_svec_hex_b);
}

static void test_svec_fromJSON_missing_key_throws()
{
    rapidjson::Document doc;
    doc.Parse("{\"other\":[]}");
    ASSERT_FALSE(doc.HasParseError());
    SerializableVector<unit_svec::V> v;
    ASSERT_THROWS_TYPE(v.fromJSON(doc, "missing"), std::invalid_argument);
}

// ---------- operator== and != are value-based, not identity ----------
static void test_svec_distinct_instances_equal_when_content_matches()
{
    SerializableVector<unit_svec::V> a;
    SerializableVector<unit_svec::V> b;
    a.append(unit_svec::V(k_svec_hex_a));
    b.append(unit_svec::V(k_svec_hex_a));
    ASSERT_TRUE(a == b);
}

// ---------- container backs to_string which round-trips via ctor ----------
static void test_svec_to_string_roundtrip_matches()
{
    SerializableVector<unit_svec::V> v;
    v.append(unit_svec::V(k_svec_hex_a));
    v.append(unit_svec::V(k_svec_hex_z));
    SerializableVector<unit_svec::V> copy(v.to_string());
    ASSERT_TRUE(v == copy);
}

// ---------- stress: many elements round-trip ----------
static void test_svec_many_elements_roundtrip()
{
    SerializableVector<unit_svec::V> v;
    for (int i = 0; i < 256; ++i)
    {
        unit_svec::V e;
        e[0] = static_cast<unsigned char>(i);
        v.append(e);
    }
    const auto bytes = v.serialize();
    SerializableVector<unit_svec::V> copy;
    copy.deserialize(bytes);
    ASSERT_TRUE(v == copy);
    ASSERT_EQ(copy.count(), static_cast<size_t>(256));
}

// ---------- size() returns serialized byte count ----------
static void test_svec_size_equals_serialize_size()
{
    // Empty vector
    {
        SerializableVector<unit_svec::V> v;
        ASSERT_EQ(v.size(), v.serialize().size());
    }
    // Single element
    {
        SerializableVector<unit_svec::V> v;
        v.append(unit_svec::V(k_svec_hex_a));
        ASSERT_EQ(v.size(), v.serialize().size());
    }
    // Multiple elements
    {
        SerializableVector<unit_svec::V> v;
        v.append(unit_svec::V(k_svec_hex_a));
        v.append(unit_svec::V(k_svec_hex_b));
        v.append(unit_svec::V(k_svec_hex_z));
        ASSERT_EQ(v.size(), v.serialize().size());
    }
}

#endif
