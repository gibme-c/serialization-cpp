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

    ASSERT_EQ(reader.size(), static_cast<size_t>(2));
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

    return test_summary();
}
