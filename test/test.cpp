// Copyright (c) 2020-2024, Brandon Lehmann
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "test_helpers.h"

#include <limits>
#include <serialization.h>
#include <sstream>

typedef SerializablePod<32> value_t;

struct vec_value_t : SerializablePod<32>
{
    vec_value_t() = default;

    explicit vec_value_t(const std::string &value) : SerializablePod<32>(value) {}

    explicit vec_value_t(const JSONValue &j)
    {
        JSON_STRING_OR_THROW()

        from_string(j.GetString());
    }
};

static const std::string test_hex = "974506601a60dc465e6e9acddb563889e63471849ec4198656550354b8541fcb";

// Expanded unit test modules. Each file is a header-only fragment of static
// test functions in its own topic area. Included after the shared helpers so
// they can use value_t / vec_value_t / test_hex if needed.
#include "unit_pack_unpack.inl"
#include "unit_varint.inl"
#include "unit_string_helper.inl"
#include "unit_serializer.inl"
#include "unit_deserializer.inl"
#include "unit_serializable_pod.inl"
#include "unit_serializable_vector.inl"
#include "unit_json_helper.inl"
#include "unit_secure_erase.inl"

// ============================================================================
// serializer_t + deserializer_t round-trips
// ============================================================================

void test_boolean()
{
    Serialization::serializer_t writer;

    writer.boolean(true);
    writer.boolean(false);

    auto reader = Serialization::deserializer_t(writer);

    ASSERT_TRUE(reader.boolean());
    ASSERT_FALSE(reader.boolean());
}

void test_uint8()
{
    Serialization::serializer_t writer;

    writer.uint8(0);
    writer.uint8(127);
    writer.uint8(255);

    auto reader = Serialization::deserializer_t(writer);

    ASSERT_EQ(reader.uint8(), 0);
    ASSERT_EQ(reader.uint8(), 127);
    ASSERT_EQ(reader.uint8(), 255);
}

void test_uint16_le()
{
    Serialization::serializer_t writer;

    writer.uint16(0);
    writer.uint16(12345);
    writer.uint16(65535);

    auto reader = Serialization::deserializer_t(writer);

    ASSERT_EQ(reader.uint16(), 0);
    ASSERT_EQ(reader.uint16(), 12345);
    ASSERT_EQ(reader.uint16(), 65535);
}

void test_uint32_le()
{
    Serialization::serializer_t writer;

    writer.uint32(0);
    writer.uint32(305419896);
    writer.uint32(std::numeric_limits<uint32_t>::max());

    auto reader = Serialization::deserializer_t(writer);

    ASSERT_EQ(reader.uint32(), static_cast<uint32_t>(0));
    ASSERT_EQ(reader.uint32(), static_cast<uint32_t>(305419896));
    ASSERT_EQ(reader.uint32(), std::numeric_limits<uint32_t>::max());
}

void test_uint64_le()
{
    Serialization::serializer_t writer;

    writer.uint64(0);
    writer.uint64(1311768467294899695ULL);
    writer.uint64(std::numeric_limits<uint64_t>::max());

    auto reader = Serialization::deserializer_t(writer);

    ASSERT_EQ(reader.uint64(), static_cast<uint64_t>(0));
    ASSERT_EQ(reader.uint64(), static_cast<uint64_t>(1311768467294899695ULL));
    ASSERT_EQ(reader.uint64(), std::numeric_limits<uint64_t>::max());
}

void test_uint16_be()
{
    Serialization::serializer_t writer;

    writer.uint16(0x1234, true);

    auto reader = Serialization::deserializer_t(writer);

    ASSERT_EQ(reader.uint16(false, true), static_cast<uint16_t>(0x1234));
}

void test_uint32_be()
{
    Serialization::serializer_t writer;

    writer.uint32(0x12345678, true);

    auto reader = Serialization::deserializer_t(writer);

    ASSERT_EQ(reader.uint32(false, true), static_cast<uint32_t>(0x12345678));
}

void test_uint64_be()
{
    Serialization::serializer_t writer;

    writer.uint64(0x123456789ABCDEF0ULL, true);

    auto reader = Serialization::deserializer_t(writer);

    ASSERT_EQ(reader.uint64(false, true), static_cast<uint64_t>(0x123456789ABCDEF0ULL));
}

void test_uint128_le()
{
    const uint128_t val(0x0102030405060708ULL, 0x090A0B0C0D0E0F10ULL);

    Serialization::serializer_t writer;

    writer.uint128(val);

    auto reader = Serialization::deserializer_t(writer);

    ASSERT_TRUE(reader.uint128() == val);
}

void test_uint128_be()
{
    const uint128_t val(0x0102030405060708ULL, 0x090A0B0C0D0E0F10ULL);

    Serialization::serializer_t writer;

    writer.uint128(val, true);

    auto reader = Serialization::deserializer_t(writer);

    ASSERT_TRUE(reader.uint128(false, true) == val);
}

void test_uint256_le()
{
    const uint128_t upper(0x0102030405060708ULL, 0x090A0B0C0D0E0F10ULL);
    const uint128_t lower(0x1112131415161718ULL, 0x191A1B1C1D1E1F20ULL);
    const uint256_t val(upper, lower);

    Serialization::serializer_t writer;

    writer.uint256(val);

    auto reader = Serialization::deserializer_t(writer);

    ASSERT_TRUE(reader.uint256() == val);
}

void test_uint256_be()
{
    const uint128_t upper(0x0102030405060708ULL, 0x090A0B0C0D0E0F10ULL);
    const uint128_t lower(0x1112131415161718ULL, 0x191A1B1C1D1E1F20ULL);
    const uint256_t val(upper, lower);

    Serialization::serializer_t writer;

    writer.uint256(val, true);

    auto reader = Serialization::deserializer_t(writer);

    ASSERT_TRUE(reader.uint256(false, true) == val);
}

void test_bytes_void_ptr()
{
    const unsigned char raw[] = {0xDE, 0xAD, 0xBE, 0xEF};

    Serialization::serializer_t writer;

    writer.bytes(raw, sizeof(raw));

    auto reader = Serialization::deserializer_t(writer);

    const auto result = reader.bytes(4);

    ASSERT_EQ(result.size(), static_cast<size_t>(4));
    ASSERT_EQ(result[0], 0xDE);
    ASSERT_EQ(result[1], 0xAD);
    ASSERT_EQ(result[2], 0xBE);
    ASSERT_EQ(result[3], 0xEF);
}

void test_bytes_vector()
{
    const std::vector<unsigned char> data = {0x01, 0x02, 0x03};

    Serialization::serializer_t writer;

    writer.bytes(data);

    auto reader = Serialization::deserializer_t(writer);

    const auto result = reader.bytes(3);

    ASSERT_EQ(result, data);
}

void test_hex_write_read()
{
    Serialization::serializer_t writer;

    writer.hex("deadbeef");

    auto reader = Serialization::deserializer_t(writer);

    ASSERT_EQ(reader.hex(4), std::string("deadbeef"));
}

// ============================================================================
// varint
// ============================================================================

template<typename T>
static void varint_roundtrip(T value)
{
    Serialization::serializer_t writer;

    writer.varint(value);

    auto reader = Serialization::deserializer_t(writer);

    ASSERT_EQ(reader.varint<T>(), value);
}

void test_varint_uint8()
{
    varint_roundtrip<uint8_t>(0);
    varint_roundtrip<uint8_t>(127);
    varint_roundtrip<uint8_t>(128);
    varint_roundtrip<uint8_t>(255);
}

void test_varint_uint16()
{
    varint_roundtrip<uint16_t>(0);
    varint_roundtrip<uint16_t>(127);
    varint_roundtrip<uint16_t>(128);
    varint_roundtrip<uint16_t>(255);
    varint_roundtrip<uint16_t>(16383);
    varint_roundtrip<uint16_t>(16384);
    varint_roundtrip<uint16_t>(std::numeric_limits<uint16_t>::max());
}

void test_varint_uint32()
{
    varint_roundtrip<uint32_t>(0);
    varint_roundtrip<uint32_t>(127);
    varint_roundtrip<uint32_t>(128);
    varint_roundtrip<uint32_t>(16383);
    varint_roundtrip<uint32_t>(16384);
    varint_roundtrip<uint32_t>(std::numeric_limits<uint32_t>::max());
}

void test_varint_uint64()
{
    varint_roundtrip<uint64_t>(0);
    varint_roundtrip<uint64_t>(127);
    varint_roundtrip<uint64_t>(128);
    varint_roundtrip<uint64_t>(16383);
    varint_roundtrip<uint64_t>(16384);
    varint_roundtrip<uint64_t>(std::numeric_limits<uint64_t>::max());
}

void test_varint_vector()
{
    const std::vector<uint32_t> values = {0, 1, 127, 128, 16384, 1000000};

    Serialization::serializer_t writer;

    writer.varint(values);

    auto reader = Serialization::deserializer_t(writer);

    const auto result = reader.varintV<uint32_t>();

    ASSERT_EQ(result.size(), values.size());

    for (size_t i = 0; i < values.size(); ++i)
    {
        ASSERT_EQ(result[i], values[i]);
    }
}

// ============================================================================
// pod single, pod vector, pod nested vector
// ============================================================================

void test_pod_single()
{
    const auto val = value_t(test_hex);

    Serialization::serializer_t writer;

    writer.pod(val);

    auto reader = Serialization::deserializer_t(writer);

    const auto result = reader.pod<value_t>();

    ASSERT_EQ(result.to_string(), test_hex);
}

void test_pod_vector()
{
    const auto a = value_t(test_hex);
    const auto b = value_t(std::string(64, '0'));
    const std::vector<value_t> values = {a, b};

    Serialization::serializer_t writer;

    writer.pod(values);

    auto reader = Serialization::deserializer_t(writer);

    const auto result = reader.podV<value_t>();

    ASSERT_EQ(result.size(), static_cast<size_t>(2));
    ASSERT_EQ(result[0].to_string(), test_hex);
    ASSERT_EQ(result[1].to_string(), std::string(64, '0'));
}

void test_pod_nested_vector()
{
    const auto a = value_t(test_hex);
    const auto b = value_t(std::string(64, '0'));
    const std::vector<std::vector<value_t>> values = {{a, b}, {b}};

    Serialization::serializer_t writer;

    writer.pod(values);

    auto reader = Serialization::deserializer_t(writer);

    const auto result = reader.podVV<value_t>();

    ASSERT_EQ(result.size(), static_cast<size_t>(2));
    ASSERT_EQ(result[0].size(), static_cast<size_t>(2));
    ASSERT_EQ(result[1].size(), static_cast<size_t>(1));
    ASSERT_EQ(result[0][0].to_string(), test_hex);
    ASSERT_EQ(result[1][0].to_string(), std::string(64, '0'));
}

// ============================================================================
// peek mode
// ============================================================================

void test_peek_mode()
{
    Serialization::serializer_t writer;

    writer.uint32(42);
    writer.uint32(99);

    auto reader = Serialization::deserializer_t(writer);

    ASSERT_EQ(reader.uint32(true), static_cast<uint32_t>(42));
    ASSERT_EQ(reader.uint32(true), static_cast<uint32_t>(42));
    ASSERT_EQ(reader.uint32(), static_cast<uint32_t>(42));
    ASSERT_EQ(reader.uint32(), static_cast<uint32_t>(99));
}

// ============================================================================
// deserializer_t utilities
// ============================================================================

void test_deserializer_reset()
{
    Serialization::serializer_t writer;

    writer.uint8(0xAA);
    writer.uint8(0xBB);

    auto reader = Serialization::deserializer_t(writer);

    ASSERT_EQ(reader.uint8(), static_cast<unsigned char>(0xAA));

    reader.reset();

    ASSERT_EQ(reader.uint8(), static_cast<unsigned char>(0xAA));
}

void test_deserializer_skip()
{
    Serialization::serializer_t writer;

    writer.uint8(0x01);
    writer.uint8(0x02);
    writer.uint8(0x03);

    auto reader = Serialization::deserializer_t(writer);

    reader.skip(2);

    ASSERT_EQ(reader.uint8(), static_cast<unsigned char>(0x03));
}

void test_deserializer_compact()
{
    Serialization::serializer_t writer;

    writer.uint8(0x01);
    writer.uint8(0x02);
    writer.uint8(0x03);

    auto reader = Serialization::deserializer_t(writer);

    reader.uint8();

    reader.compact();

    // After compact(), the buffer contains only the previously-unread bytes
    // and the cursor is reset to 0, so the next read returns the first
    // remaining byte (0x02).
    ASSERT_EQ(reader.size(), static_cast<size_t>(2));
    ASSERT_EQ(reader.uint8(), static_cast<unsigned char>(0x02));
    ASSERT_EQ(reader.uint8(), static_cast<unsigned char>(0x03));
}

void test_deserializer_unread()
{
    Serialization::serializer_t writer;

    writer.uint8(0x01);
    writer.uint8(0x02);
    writer.uint8(0x03);

    auto reader = Serialization::deserializer_t(writer);

    reader.uint8();

    ASSERT_EQ(reader.unread_bytes(), static_cast<size_t>(2));

    const auto remaining = reader.unread_data();

    ASSERT_EQ(remaining.size(), static_cast<size_t>(2));
    ASSERT_EQ(remaining[0], 0x02);
    ASSERT_EQ(remaining[1], 0x03);
}

// ============================================================================
// serializer_t utilities
// ============================================================================

void test_serializer_size_and_data()
{
    Serialization::serializer_t writer;

    ASSERT_EQ(writer.size(), static_cast<size_t>(0));

    writer.uint8(0xFF);

    ASSERT_EQ(writer.size(), static_cast<size_t>(1));
    ASSERT_EQ(writer.data()[0], 0xFF);
}

void test_serializer_to_string()
{
    Serialization::serializer_t writer;

    writer.uint8(0xAB);
    writer.uint8(0xCD);

    ASSERT_EQ(writer.to_string(), std::string("abcd"));
}

void test_serializer_operator_index()
{
    Serialization::serializer_t writer;

    writer.uint8(0x01);
    writer.uint8(0x02);

    ASSERT_EQ(writer[0], 0x01);
    ASSERT_EQ(writer[1], 0x02);

    writer[0] = 0xFF;

    ASSERT_EQ(writer[0], 0xFF);
}

void test_serializer_reset()
{
    Serialization::serializer_t writer;

    writer.uint8(0x01);

    writer.reset();

    ASSERT_EQ(writer.size(), static_cast<size_t>(0));
}

void test_serializer_copy_ctor()
{
    Serialization::serializer_t a;

    a.uint8(0xAA);

    Serialization::serializer_t b(a);

    ASSERT_EQ(b.size(), static_cast<size_t>(1));
    ASSERT_EQ(b[0], 0xAA);
}

void test_serializer_initializer_list_ctor()
{
    Serialization::serializer_t writer({0x01, 0x02, 0x03});

    ASSERT_EQ(writer.size(), static_cast<size_t>(3));
    ASSERT_EQ(writer[0], 0x01);
    ASSERT_EQ(writer[2], 0x03);
}

void test_serializer_vector_ctor()
{
    const std::vector<unsigned char> data = {0xDE, 0xAD};

    Serialization::serializer_t writer(data);

    ASSERT_EQ(writer.size(), static_cast<size_t>(2));
    ASSERT_EQ(writer.to_string(), std::string("dead"));
}

// ============================================================================
// deserializer_t constructors
// ============================================================================

void test_deserializer_string_ctor()
{
    auto reader = Serialization::deserializer_t(std::string("abcd"));

    ASSERT_EQ(reader.uint8(), static_cast<unsigned char>(0xAB));
    ASSERT_EQ(reader.uint8(), static_cast<unsigned char>(0xCD));
}

void test_deserializer_initializer_list_ctor()
{
    auto reader = Serialization::deserializer_t({0x01, 0x02});

    ASSERT_EQ(reader.uint8(), static_cast<unsigned char>(0x01));
    ASSERT_EQ(reader.uint8(), static_cast<unsigned char>(0x02));
}

// ============================================================================
// string_helper
// ============================================================================

void test_hex_roundtrip()
{
    const std::string hex_str = "deadbeef01020304";

    const auto bytes = Serialization::from_hex(hex_str);

    const auto result = Serialization::to_hex(bytes.data(), bytes.size());

    ASSERT_EQ(result, hex_str);
}

void test_from_hex_empty()
{
    const auto bytes = Serialization::from_hex("");

    ASSERT_EQ(bytes.size(), static_cast<size_t>(0));
}

void test_from_hex_odd_length()
{
    ASSERT_THROWS(Serialization::from_hex("abc"));
}

void test_from_hex_invalid_char()
{
    ASSERT_THROWS(Serialization::from_hex("zz"));
}

void test_str_split_join()
{
    const std::string input = "hello world foo";

    const auto parts = Serialization::str_split(input);

    ASSERT_EQ(parts.size(), static_cast<size_t>(3));
    ASSERT_EQ(parts[0], std::string("hello"));
    ASSERT_EQ(parts[1], std::string("world"));
    ASSERT_EQ(parts[2], std::string("foo"));

    const auto joined = Serialization::str_join(parts);

    ASSERT_EQ(joined, input);
}

void test_str_pad()
{
    const auto result = Serialization::str_pad("hi", 5);

    ASSERT_EQ(result.size(), static_cast<size_t>(5));
    ASSERT_EQ(result, std::string("hi   "));
}

void test_str_trim()
{
    std::string s = "\t\nHello World\r\n";

    Serialization::str_trim(s);

    ASSERT_EQ(s, std::string("Hello World"));
}

void test_str_trim_lowercase()
{
    std::string s = "\tHello World\n";

    Serialization::str_trim(s, true);

    ASSERT_EQ(s, std::string("hello world"));
}

// ============================================================================
// SerializablePod
// ============================================================================

void test_pod_hex_construction()
{
    const auto val = value_t(test_hex);

    ASSERT_EQ(val.to_string(), test_hex);
}

void test_pod_serialize_deserialize()
{
    const auto original = value_t(test_hex);

    const auto bytes = original.serialize();

    auto copy = value_t();

    copy.deserialize(bytes);

    ASSERT_EQ(copy.to_string(), test_hex);
}

void test_pod_serialize_via_reader()
{
    const auto original = value_t(test_hex);

    Serialization::serializer_t writer;

    original.serialize(writer);

    auto reader = Serialization::deserializer_t(writer);

    auto copy = value_t();

    copy.deserialize(reader);

    ASSERT_EQ(copy.to_string(), test_hex);
}

void test_pod_json_roundtrip()
{
    const auto original = value_t(test_hex);

    JSON_INIT_BUFFER(buffer, writer);

    original.toJSON(writer);

    JSON_DUMP_BUFFER(buffer, json_str);

    JSON_PARSE(json_str);

    auto copy = value_t();

    copy.fromJSON(body);

    ASSERT_EQ(copy.to_string(), test_hex);
}

void test_pod_json_key_roundtrip()
{
    const auto original = value_t(test_hex);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    writer.StartObject();
    writer.Key("value");
    original.toJSON(writer);
    writer.EndObject();

    const std::string json_str = buffer.GetString();

    rapidjson::Document doc;
    doc.Parse(json_str.c_str());

    auto copy = value_t();

    copy.fromJSON(doc, "value");

    ASSERT_EQ(copy.to_string(), test_hex);
}

void test_pod_comparison_operators()
{
    const auto a = value_t(test_hex);
    const auto b = value_t(test_hex);
    const auto zero = value_t();

    ASSERT_TRUE(a == b);
    ASSERT_FALSE(a != b);
    ASSERT_TRUE(a != zero);
    ASSERT_TRUE(zero < a);
    ASSERT_TRUE(a > zero);
    ASSERT_TRUE(zero <= a);
    ASSERT_TRUE(a >= zero);
    ASSERT_TRUE(a <= a);
    ASSERT_TRUE(a >= a);
}

void test_pod_empty()
{
    const auto zero = value_t();
    const auto loaded = value_t(test_hex);

    ASSERT_TRUE(zero.empty());
    ASSERT_FALSE(loaded.empty());
}

void test_pod_wrong_size()
{
    const std::vector<unsigned char> too_short = {0x01, 0x02};

    auto val = value_t();

    ASSERT_THROWS(val.deserialize(too_short));
}

void test_pod_ostream()
{
    const auto val = value_t(test_hex);

    std::ostringstream oss;

    oss << val;

    ASSERT_EQ(oss.str(), test_hex);
}

// ============================================================================
// SerializableVector
// ============================================================================

void test_svec_append_back()
{
    SerializableVector<vec_value_t> vec;

    const auto a = vec_value_t(test_hex);

    vec.append(a);

    ASSERT_EQ(vec.size(), static_cast<size_t>(1));
    ASSERT_EQ(vec.back().to_string(), test_hex);
}

void test_svec_extend_vector()
{
    SerializableVector<vec_value_t> vec;

    const auto a = vec_value_t(test_hex);
    const auto b = vec_value_t(std::string(64, '0'));
    const std::vector<vec_value_t> items = {a, b};

    vec.extend(items);

    ASSERT_EQ(vec.size(), static_cast<size_t>(2));
    ASSERT_EQ(vec[0].to_string(), test_hex);
    ASSERT_EQ(vec[1].to_string(), std::string(64, '0'));
}

void test_svec_extend_svec()
{
    SerializableVector<vec_value_t> vec1;

    vec1.append(vec_value_t(test_hex));

    SerializableVector<vec_value_t> vec2;

    vec2.append(vec_value_t(std::string(64, '0')));

    vec1.extend(vec2);

    ASSERT_EQ(vec1.size(), static_cast<size_t>(2));
}

void test_svec_serialize_deserialize()
{
    SerializableVector<vec_value_t> original;

    original.append(vec_value_t(test_hex));
    original.append(vec_value_t(std::string(64, '0')));

    const auto bytes = original.serialize();

    SerializableVector<vec_value_t> copy;

    copy.deserialize(bytes);

    ASSERT_EQ(copy.size(), static_cast<size_t>(2));
    ASSERT_EQ(copy[0].to_string(), test_hex);
    ASSERT_EQ(copy[1].to_string(), std::string(64, '0'));
}

void test_svec_operators()
{
    SerializableVector<vec_value_t> a;

    a.append(vec_value_t(test_hex));

    SerializableVector<vec_value_t> b;

    b.append(vec_value_t(test_hex));

    SerializableVector<vec_value_t> c;

    c.append(vec_value_t(std::string(64, '0')));

    ASSERT_TRUE(a == b);
    ASSERT_TRUE(a != c);
    ASSERT_EQ(a[0].to_string(), test_hex);
}

void test_svec_json_roundtrip()
{
    SerializableVector<vec_value_t> original;

    original.append(vec_value_t(test_hex));
    original.append(vec_value_t(std::string(64, '0')));

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

    writer.StartObject();
    writer.Key("items");
    original.toJSON(writer);
    writer.EndObject();

    const std::string json_str = buffer.GetString();

    rapidjson::Document doc;
    doc.Parse(json_str.c_str());

    ASSERT_FALSE(doc.HasParseError());

    const auto &arr_val = get_json_value(doc, "items");

    SerializableVector<vec_value_t> copy;

    copy.fromJSON(arr_val);

    ASSERT_EQ(copy.size(), static_cast<size_t>(2));
    ASSERT_EQ(copy[0].to_string(), test_hex);
    ASSERT_EQ(copy[1].to_string(), std::string(64, '0'));
}

// ============================================================================
// secure_erase
// ============================================================================

void test_secure_erase()
{
    unsigned char buf[16];

    std::memset(buf, 0xAA, sizeof(buf));

    serialization_secure_erase(buf, sizeof(buf));

    for (size_t i = 0; i < sizeof(buf); ++i)
    {
        ASSERT_EQ(buf[i], static_cast<unsigned char>(0));
    }
}

// ============================================================================
// main
// ============================================================================

int main()
{
    std::cout << "serializer_t + deserializer_t round-trips:" << std::endl;
    RUN_TEST(test_boolean);
    RUN_TEST(test_uint8);
    RUN_TEST(test_uint16_le);
    RUN_TEST(test_uint32_le);
    RUN_TEST(test_uint64_le);
    RUN_TEST(test_uint16_be);
    RUN_TEST(test_uint32_be);
    RUN_TEST(test_uint64_be);
    RUN_TEST(test_uint128_le);
    RUN_TEST(test_uint128_be);
    RUN_TEST(test_uint256_le);
    RUN_TEST(test_uint256_be);
    RUN_TEST(test_bytes_void_ptr);
    RUN_TEST(test_bytes_vector);
    RUN_TEST(test_hex_write_read);

    std::cout << std::endl << "varint:" << std::endl;
    RUN_TEST(test_varint_uint8);
    RUN_TEST(test_varint_uint16);
    RUN_TEST(test_varint_uint32);
    RUN_TEST(test_varint_uint64);
    RUN_TEST(test_varint_vector);

    std::cout << std::endl << "pod:" << std::endl;
    RUN_TEST(test_pod_single);
    RUN_TEST(test_pod_vector);
    RUN_TEST(test_pod_nested_vector);

    std::cout << std::endl << "peek:" << std::endl;
    RUN_TEST(test_peek_mode);

    std::cout << std::endl << "deserializer_t utilities:" << std::endl;
    RUN_TEST(test_deserializer_reset);
    RUN_TEST(test_deserializer_skip);
    RUN_TEST(test_deserializer_compact);
    RUN_TEST(test_deserializer_unread);

    std::cout << std::endl << "serializer_t utilities:" << std::endl;
    RUN_TEST(test_serializer_size_and_data);
    RUN_TEST(test_serializer_to_string);
    RUN_TEST(test_serializer_operator_index);
    RUN_TEST(test_serializer_reset);
    RUN_TEST(test_serializer_copy_ctor);
    RUN_TEST(test_serializer_initializer_list_ctor);
    RUN_TEST(test_serializer_vector_ctor);

    std::cout << std::endl << "deserializer_t constructors:" << std::endl;
    RUN_TEST(test_deserializer_string_ctor);
    RUN_TEST(test_deserializer_initializer_list_ctor);

    std::cout << std::endl << "string_helper:" << std::endl;
    RUN_TEST(test_hex_roundtrip);
    RUN_TEST(test_from_hex_empty);
    RUN_TEST(test_from_hex_odd_length);
    RUN_TEST(test_from_hex_invalid_char);
    RUN_TEST(test_str_split_join);
    RUN_TEST(test_str_pad);
    RUN_TEST(test_str_trim);
    RUN_TEST(test_str_trim_lowercase);

    std::cout << std::endl << "SerializablePod:" << std::endl;
    RUN_TEST(test_pod_hex_construction);
    RUN_TEST(test_pod_serialize_deserialize);
    RUN_TEST(test_pod_serialize_via_reader);
    RUN_TEST(test_pod_json_roundtrip);
    RUN_TEST(test_pod_json_key_roundtrip);
    RUN_TEST(test_pod_comparison_operators);
    RUN_TEST(test_pod_empty);
    RUN_TEST(test_pod_wrong_size);
    RUN_TEST(test_pod_ostream);

    std::cout << std::endl << "SerializableVector:" << std::endl;
    RUN_TEST(test_svec_append_back);
    RUN_TEST(test_svec_extend_vector);
    RUN_TEST(test_svec_extend_svec);
    RUN_TEST(test_svec_serialize_deserialize);
    RUN_TEST(test_svec_operators);
    RUN_TEST(test_svec_json_roundtrip);

    std::cout << std::endl << "secure_erase:" << std::endl;
    RUN_TEST(test_secure_erase);

    // ========================================================================
    // Expanded coverage from unit_*.inl modules
    // ========================================================================

    SECTION("pack/unpack");
    RUN_TEST(test_pack_size_uint8);
    RUN_TEST(test_pack_size_uint16);
    RUN_TEST(test_pack_size_uint32);
    RUN_TEST(test_pack_size_uint64);
    RUN_TEST(test_pack_size_int8);
    RUN_TEST(test_pack_size_int16);
    RUN_TEST(test_pack_size_int32);
    RUN_TEST(test_pack_size_int64);
    RUN_TEST(test_pack_unpack_roundtrip_uint8_all);
    RUN_TEST(test_pack_unpack_roundtrip_uint16_grid);
    RUN_TEST(test_pack_unpack_roundtrip_uint32_grid);
    RUN_TEST(test_pack_unpack_roundtrip_uint64_grid);
    RUN_TEST(test_pack_unpack_roundtrip_int8_grid);
    RUN_TEST(test_pack_unpack_roundtrip_int16_grid);
    RUN_TEST(test_pack_unpack_roundtrip_int32_grid);
    RUN_TEST(test_pack_unpack_roundtrip_int64_grid);
    RUN_TEST(test_pack_unpack_uint16_be_roundtrip);
    RUN_TEST(test_pack_unpack_uint32_be_roundtrip);
    RUN_TEST(test_pack_unpack_uint64_be_roundtrip);
    RUN_TEST(test_pack_uint16_le_layout);
    RUN_TEST(test_pack_uint32_le_layout);
    RUN_TEST(test_pack_uint64_le_layout);
    RUN_TEST(test_unpack_uint32_at_offset);
    RUN_TEST(test_unpack_uint64_at_offset_be);
    RUN_TEST(test_unpack_empty_buffer_throws);
    RUN_TEST(test_unpack_too_small_buffer_throws);
    RUN_TEST(test_unpack_offset_exact_end_throws);
    RUN_TEST(test_unpack_offset_just_fits);
    RUN_TEST(test_unpack_offset_huge_throws);
    RUN_TEST(test_unpack_zero_offset_empty_type_should_fit);
    RUN_TEST(test_pack_unpack_mass_roundtrip_uint64);
    RUN_TEST(test_pack_unpack_mass_roundtrip_int32);
    RUN_TEST(test_pack_endianness_symmetry_uint32);
    RUN_TEST(test_pack_unpack_uint16_max_le_be);
    RUN_TEST(test_pack_unpack_uint32_max_le_be);
    RUN_TEST(test_pack_unpack_uint64_max_le_be);
    RUN_TEST(test_pack_int32_negative_bytes_match_cast);
    RUN_TEST(test_pack_int64_min_roundtrip);
    RUN_TEST(test_pack_unsigned_char_roundtrip);

    SECTION("varint");
    RUN_TEST(test_encode_varint_zero_uint8);
    RUN_TEST(test_encode_varint_0x7f_uint8);
    RUN_TEST(test_encode_varint_0x80_uint8);
    RUN_TEST(test_encode_varint_0xff_uint8);
    RUN_TEST(test_encode_varint_0x3fff_uint16);
    RUN_TEST(test_encode_varint_0x4000_uint16);
    RUN_TEST(test_encode_varint_0xffff_uint16);
    RUN_TEST(test_varint_roundtrip_uint8_exhaustive);
    RUN_TEST(test_varint_roundtrip_uint16_exhaustive);
    RUN_TEST(test_varint_roundtrip_uint32_grid);
    RUN_TEST(test_varint_roundtrip_uint64_grid);
    RUN_TEST(test_decode_varint_empty_buffer_throws);
    RUN_TEST(test_decode_varint_truncated_continuation_throws);
    RUN_TEST(test_decode_varint_offset_past_end_throws);
    RUN_TEST(test_decode_varint_offset_equals_size_throws);
    RUN_TEST(test_decode_varint_shift_overflow_uint8);
    RUN_TEST(test_decode_varint_shift_overflow_uint16);
    RUN_TEST(test_decode_varint_shift_overflow_uint32);
    RUN_TEST(test_decode_varint_shift_overflow_uint64);
    RUN_TEST(test_decode_varint_consumed_at_offset);
    RUN_TEST(test_decode_varint_stops_at_terminator);
    RUN_TEST(test_decode_varint_single_byte_all_widths);
    RUN_TEST(test_decode_varint_two_byte_128);
    RUN_TEST(test_varint_stream_multiple_values);
    RUN_TEST(test_decode_varint_corrupt_top_bit_rejects_when_overflow);
    RUN_TEST(test_decode_varint_uint8_multi_byte_rejected);
    RUN_TEST(test_decode_varint_uint8_overflow_two_byte);
    RUN_TEST(test_decode_varint_uint8_overflow_top_bit_of_second);
    RUN_TEST(test_decode_varint_uint16_overflow_three_byte);
    RUN_TEST(test_decode_varint_uint32_overflow_five_byte);
    RUN_TEST(test_decode_varint_uint8_max_decodes_ok);
    RUN_TEST(test_decode_varint_uint16_max_decodes_ok);
    RUN_TEST(test_decode_varint_uint32_max_decodes_ok);
    RUN_TEST(test_decode_varint_uint64_max_decodes_ok);
    RUN_TEST(test_decode_varint_uint64_final_byte_bit1_rejected);
    RUN_TEST(test_decode_varint_uint64_final_byte_high_bits_rejected);
    RUN_TEST(test_varint_mass_roundtrip_uint32);
    RUN_TEST(test_varint_mass_roundtrip_uint64);
    RUN_TEST(test_encode_varint_length_monotonic_uint32);
    RUN_TEST(test_decode_varint_consumed_equals_buffer_size_tight);

    SECTION("string_helper");
    RUN_TEST(test_from_hex_valid_lowercase);
    RUN_TEST(test_from_hex_valid_uppercase);
    RUN_TEST(test_from_hex_mixed_case);
    RUN_TEST(test_from_hex_all_digits);
    RUN_TEST(test_from_hex_empty_string_returns_empty);
    RUN_TEST(test_from_hex_odd_length_throws_length_error);
    RUN_TEST(test_from_hex_odd_length_single_char_throws);
    RUN_TEST(test_from_hex_invalid_char_throws_invalid_argument);
    RUN_TEST(test_from_hex_invalid_char_mid_string_throws);
    RUN_TEST(test_from_hex_space_char_throws);
    RUN_TEST(test_from_hex_null_byte_throws);
    RUN_TEST(test_from_hex_high_ascii_throws);
    RUN_TEST(test_from_hex_idempotent_via_to_hex);
    RUN_TEST(test_to_hex_empty);
    RUN_TEST(test_to_hex_single_byte);
    RUN_TEST(test_to_hex_produces_lowercase);
    RUN_TEST(test_to_hex_length_is_twice_input);
    RUN_TEST(test_str_split_basic);
    RUN_TEST(test_str_split_single_token_no_delim);
    RUN_TEST(test_str_split_trailing_delim);
    RUN_TEST(test_str_split_leading_delim);
    RUN_TEST(test_str_split_consecutive_delims);
    RUN_TEST(test_str_join_basic);
    RUN_TEST(test_str_join_empty_vector);
    RUN_TEST(test_str_join_single_element);
    RUN_TEST(test_str_join_empty_strings_keep_delims);
    RUN_TEST(test_str_join_default_delim_space);
    RUN_TEST(test_str_split_join_roundtrip_no_empty_parts);
    RUN_TEST(test_str_pad_shorter_input);
    RUN_TEST(test_str_pad_equal_length);
    RUN_TEST(test_str_pad_longer_input);
    RUN_TEST(test_str_pad_zero_length);
    RUN_TEST(test_str_pad_empty_input);
    RUN_TEST(test_str_trim_basic);
    RUN_TEST(test_str_trim_only_tabs_and_newlines);
    RUN_TEST(test_str_trim_formfeed_vtab);
    RUN_TEST(test_str_trim_all_whitespace);
    RUN_TEST(test_str_trim_empty);
    RUN_TEST(test_str_trim_no_whitespace);
    RUN_TEST(test_str_trim_lowercase_path);
    RUN_TEST(test_str_trim_lowercase_only_letters_changed);
    RUN_TEST(test_str_trim_preserves_internal_tabs_no);

    SECTION("serializer_t (expanded)");
    RUN_TEST(test_ser_default_empty);
    RUN_TEST(test_ser_boolean_true_is_0x01);
    RUN_TEST(test_ser_boolean_false_is_0x00);
    RUN_TEST(test_ser_uint8_byte_count);
    RUN_TEST(test_ser_uint16_byte_count_is_2);
    RUN_TEST(test_ser_uint32_byte_count_is_4);
    RUN_TEST(test_ser_uint64_byte_count_is_8);
    RUN_TEST(test_ser_uint128_byte_count_is_16);
    RUN_TEST(test_ser_uint256_byte_count_is_32);
    RUN_TEST(test_ser_bytes_ptr_len);
    RUN_TEST(test_ser_bytes_nullptr_zero_length_ok);
    RUN_TEST(test_ser_bytes_nullptr_nonzero_throws);
    RUN_TEST(test_ser_bytes_vector_overload);
    RUN_TEST(test_ser_bytes_vector_empty_is_noop);
    RUN_TEST(test_ser_hex_appends_bytes);
    RUN_TEST(test_ser_hex_empty_is_noop);
    RUN_TEST(test_ser_hex_odd_length_throws);
    RUN_TEST(test_ser_hex_invalid_char_throws);
    RUN_TEST(test_ser_to_string_matches_hex);
    RUN_TEST(test_ser_operator_index_read);
    RUN_TEST(test_ser_operator_index_write_mutation);
    RUN_TEST(test_ser_reset_clears);
    RUN_TEST(test_ser_copy_ctor);
    RUN_TEST(test_ser_initializer_list_ctor);
    RUN_TEST(test_ser_vector_ctor);
    RUN_TEST(test_ser_data_pointer_content);
    RUN_TEST(test_ser_varint_scalar);
    RUN_TEST(test_ser_varint_vector_prefix_count_then_values);
    RUN_TEST(test_ser_mixed_write_chain);
    RUN_TEST(test_ser_final_data_reflects_last_bytes);
    RUN_TEST(test_ser_vector_returns_copy);

    SECTION("deserializer_t (expanded)");
    RUN_TEST(test_des_ctor_from_serializer);
    RUN_TEST(test_des_ctor_from_vector);
    RUN_TEST(test_des_ctor_from_initializer_list);
    RUN_TEST(test_des_ctor_from_hex_string);
    RUN_TEST(test_des_ctor_from_empty_hex_string);
    RUN_TEST(test_des_ctor_from_odd_hex_throws);
    RUN_TEST(test_des_ctor_from_invalid_hex_throws);
    RUN_TEST(test_des_boolean_reads_both_states);
    RUN_TEST(test_des_uint8_reads_each_byte);
    RUN_TEST(test_des_uint16_le_and_be);
    RUN_TEST(test_des_uint32_le_and_be);
    RUN_TEST(test_des_uint64_le_and_be);
    RUN_TEST(test_des_peek_does_not_advance);
    RUN_TEST(test_des_peek_bytes_does_not_advance);
    RUN_TEST(test_des_bytes_zero_count);
    RUN_TEST(test_des_bytes_exact_full_buffer);
    RUN_TEST(test_des_hex_read);
    RUN_TEST(test_des_hex_peek);
    RUN_TEST(test_des_uint8_past_end_throws);
    RUN_TEST(test_des_uint16_half_past_end_throws);
    RUN_TEST(test_des_uint32_partial_throws);
    RUN_TEST(test_des_uint64_partial_throws);
    RUN_TEST(test_des_uint128_partial_throws);
    RUN_TEST(test_des_uint256_partial_throws);
    RUN_TEST(test_des_bytes_count_past_end_throws);
    RUN_TEST(test_des_hex_count_past_end_throws);
    RUN_TEST(test_des_read_after_full_consumed_throws);
    RUN_TEST(test_des_skip_ok);
    RUN_TEST(test_des_skip_past_end_throws);
    RUN_TEST(test_des_skip_exact_end_ok);
    RUN_TEST(test_des_skip_zero_ok);
    RUN_TEST(test_des_reset_returns_to_start);
    RUN_TEST(test_des_reset_to_mid_position);
    RUN_TEST(test_des_compact_drops_consumed_bytes);
    RUN_TEST(test_des_compact_from_start_is_noop);
    RUN_TEST(test_des_compact_after_full_read);
    RUN_TEST(test_des_unread_bytes_decreases_with_reads);
    RUN_TEST(test_des_unread_data_copy);
    RUN_TEST(test_des_unread_data_empty_when_consumed);
    RUN_TEST(test_des_varint_reads_and_advances);
    RUN_TEST(test_des_varint_peek);
    RUN_TEST(test_des_varint_truncated_throws);
    RUN_TEST(test_des_podV_inflated_count_throws_before_oom);
    RUN_TEST(test_des_podV_count_zero_empty_result);
    RUN_TEST(test_des_podV_small_roundtrip);
    RUN_TEST(test_des_podVV_inflated_outer_count_throws);
    RUN_TEST(test_des_podVV_inflated_inner_count_throws);
    RUN_TEST(test_des_podVV_empty_outer_returns_empty);
    RUN_TEST(test_des_varintV_count_zero_empty);
    RUN_TEST(test_des_varintV_small_roundtrip);
    RUN_TEST(test_des_varintV_inflated_count_throws);
    RUN_TEST(test_des_data_and_size);
    RUN_TEST(test_des_to_string);
    RUN_TEST(test_des_uint16_be_roundtrip_via_serializer);
    RUN_TEST(test_des_uint32_be_roundtrip_via_serializer);
    RUN_TEST(test_des_uint64_be_roundtrip_via_serializer);
    RUN_TEST(test_des_uint128_roundtrip);
    RUN_TEST(test_des_uint256_roundtrip);
    RUN_TEST(test_des_reset_to_end_read_throws);
    RUN_TEST(test_des_empty_buffer_reads_all_throw);
    RUN_TEST(test_des_pod_single_peek);

    SECTION("SerializablePod (expanded)");
    RUN_TEST(test_pod_default_is_empty_sz8);
    RUN_TEST(test_pod_default_is_empty_sz32);
    RUN_TEST(test_pod_default_is_empty_sz128);
    RUN_TEST(test_pod_size_is_template_param);
    RUN_TEST(test_pod_hex_ctor_8);
    RUN_TEST(test_pod_hex_ctor_32);
    RUN_TEST(test_pod_hex_ctor_wrong_size_throws);
    RUN_TEST(test_pod_hex_ctor_too_long_throws);
    RUN_TEST(test_pod_hex_ctor_odd_length_throws);
    RUN_TEST(test_pod_hex_ctor_invalid_char_throws);
    RUN_TEST(test_pod_serialize_bytes_round_trip);
    RUN_TEST(test_pod_deserialize_wrong_size_throws);
    RUN_TEST(test_pod_deserialize_empty_throws);
    RUN_TEST(test_pod_deserialize_too_long_throws);
    RUN_TEST(test_pod_serialize_via_reader_sized);
    RUN_TEST(test_pod_deserialize_via_reader_short_buffer_throws);
    RUN_TEST(test_pod_equality_and_inequality);
    RUN_TEST(test_pod_lt_gt_le_ge);
    RUN_TEST(test_pod_operators_equal_is_le_and_ge);
    RUN_TEST(test_pod_operator_index_read_write);
    RUN_TEST(test_pod_operator_dereference_returns_ptr);
    RUN_TEST(test_pod_data_ptr_matches_indexing);
    RUN_TEST(test_pod_to_string_matches_hex_encoding);
    RUN_TEST(test_pod_fromJSON_string_round_trip);
    RUN_TEST(test_pod_fromJSON_non_string_throws);
    RUN_TEST(test_pod_fromJSON_wrong_size_string_throws);
    RUN_TEST(test_pod_fromJSON_invalid_hex_throws);
    RUN_TEST(test_pod_fromJSON_by_key);
    RUN_TEST(test_pod_fromJSON_missing_key_throws);
    RUN_TEST(test_pod_ostream_is_hex);
    RUN_TEST(test_pod_full_pipeline_roundtrip_P64);
    RUN_TEST(test_pod_full_pipeline_roundtrip_P128);
    RUN_TEST(test_pod_empty_flips_on_mutation);
    RUN_TEST(test_pod_distinct_sizes_instantiate_independently);

    SECTION("SerializableVector (expanded)");
    RUN_TEST(test_svec_default_empty);
    RUN_TEST(test_svec_append_and_back);
    RUN_TEST(test_svec_extend_vector_expanded);
    RUN_TEST(test_svec_extend_svec_expanded);
    RUN_TEST(test_svec_equality_same_length);
    RUN_TEST(test_svec_equality_differs_at_index);
    RUN_TEST(test_svec_equality_length_mismatch_not_equal);
    RUN_TEST(test_svec_operator_index);
    RUN_TEST(test_svec_serialize_roundtrip);
    RUN_TEST(test_svec_serialize_empty);
    RUN_TEST(test_svec_deserialize_via_reader);
    RUN_TEST(test_svec_ctor_from_hex_string_roundtrip);
    RUN_TEST(test_svec_ctor_from_empty_hex_string);
    RUN_TEST(test_svec_ctor_from_invalid_hex_throws);
    RUN_TEST(test_svec_ctor_from_json_array_value);
    RUN_TEST(test_svec_ctor_from_json_object_value_throws);
    RUN_TEST(test_svec_ctor_from_json_by_key_array);
    RUN_TEST(test_svec_deserialize_inflated_count_throws);
    RUN_TEST(test_svec_deserialize_count_one_empty_payload_throws);
    RUN_TEST(test_svec_deserialize_count_two_one_payload_throws);
    RUN_TEST(test_svec_toJSON_fromJSON_array_roundtrip);
    RUN_TEST(test_svec_fromJSON_non_array_throws);
    RUN_TEST(test_svec_fromJSON_empty_array);
    RUN_TEST(test_svec_fromJSON_by_key);
    RUN_TEST(test_svec_fromJSON_missing_key_throws);
    RUN_TEST(test_svec_distinct_instances_equal_when_content_matches);
    RUN_TEST(test_svec_to_string_roundtrip_matches);
    RUN_TEST(test_svec_many_elements_roundtrip);

    SECTION("json_helper");
    RUN_TEST(test_json_has_member_true);
    RUN_TEST(test_json_has_member_false);
    RUN_TEST(test_json_get_value_present);
    RUN_TEST(test_json_get_value_missing_throws);
    RUN_TEST(test_json_get_bool_true);
    RUN_TEST(test_json_get_bool_false);
    RUN_TEST(test_json_get_bool_type_mismatch_throws);
    RUN_TEST(test_json_get_bool_missing_key_throws);
    RUN_TEST(test_json_get_int64);
    RUN_TEST(test_json_get_int64_type_mismatch_throws);
    RUN_TEST(test_json_get_uint64);
    RUN_TEST(test_json_get_uint64_type_mismatch_throws);
    RUN_TEST(test_json_get_uint32);
    RUN_TEST(test_json_get_uint32_type_mismatch_throws);
    RUN_TEST(test_json_get_double);
    RUN_TEST(test_json_get_double_type_mismatch_throws);
    RUN_TEST(test_json_get_string);
    RUN_TEST(test_json_get_string_type_mismatch_throws);
    RUN_TEST(test_json_get_string_empty);
    RUN_TEST(test_json_get_array_ok);
    RUN_TEST(test_json_get_array_type_mismatch_throws);
    RUN_TEST(test_json_get_array_empty);
    RUN_TEST(test_json_get_object_ok);
    RUN_TEST(test_json_get_object_root);
    RUN_TEST(test_json_get_object_type_mismatch_throws);
    RUN_TEST(test_json_get_object_missing_key_throws);
    RUN_TEST(test_json_parse_macro_malformed_throws);
    RUN_TEST(test_json_parse_macro_ok);
    RUN_TEST(test_json_parse_empty_string_throws);
    RUN_TEST(test_json_str_to_json_ok);
    RUN_TEST(test_json_str_to_json_malformed_throws);
    RUN_TEST(test_json_load_string_macro);
    RUN_TEST(test_json_load_bool_macro);
    RUN_TEST(test_json_load_u32_macro);
    RUN_TEST(test_json_load_u64_macro);
    RUN_TEST(test_json_load_string_missing_key_throws);
    RUN_TEST(test_json_if_member_present);
    RUN_TEST(test_json_if_member_absent);
    RUN_TEST(test_json_init_dump_roundtrip);
    RUN_TEST(test_json_load_keyv_macro);
    RUN_TEST(test_json_load_keyvv_macro);
    RUN_TEST(test_json_member_or_throw_hits_when_missing);

    SECTION("secure_erase (expanded)");
    RUN_TEST(test_secure_erase_one_byte);
    RUN_TEST(test_secure_erase_small_buffer);
    RUN_TEST(test_secure_erase_medium_buffer);
    RUN_TEST(test_secure_erase_large_buffer);
    RUN_TEST(test_secure_erase_zero_length_is_noop);
    RUN_TEST(test_secure_erase_unaligned_offset);

    return test_summary();
}
