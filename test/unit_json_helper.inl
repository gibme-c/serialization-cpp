// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Unit tests for json_helper.h — typed getters, has_member, the JSON_PARSE
// macro error path, and the LOAD_*_FROM_JSON loaders.

#ifndef SERIALIZATION_UNIT_JSON_HELPER_INL
#define SERIALIZATION_UNIT_JSON_HELPER_INL

#include <json_helper.h>
#include <rapidjson/document.h>
#include <string>
#include <vector>

namespace unit_jh
{
    // Parse a literal JSON string; the test fails if parsing does.
    static rapidjson::Document parse(const char *s)
    {
        rapidjson::Document doc;
        if (doc.Parse(s).HasParseError())
        {
            throw std::runtime_error(std::string("test parse failed for: ") + s);
        }
        return doc;
    }
} // namespace unit_jh

// ---------- has_member ----------
static void test_json_has_member_true()
{
    auto doc = unit_jh::parse("{\"a\":1,\"b\":\"x\"}");
    ASSERT_TRUE(has_member(doc, std::string("a")));
    ASSERT_TRUE(has_member(doc, std::string("b")));
}

static void test_json_has_member_false()
{
    auto doc = unit_jh::parse("{\"a\":1}");
    ASSERT_FALSE(has_member(doc, std::string("missing")));
}

// ---------- get_json_value ----------
static void test_json_get_value_present()
{
    auto doc = unit_jh::parse("{\"a\":42}");
    const auto &val = get_json_value(doc, std::string("a"));
    ASSERT_TRUE(val.IsInt());
    ASSERT_EQ(val.GetInt(), 42);
}

static void test_json_get_value_missing_throws()
{
    auto doc = unit_jh::parse("{}");
    ASSERT_THROWS_TYPE(get_json_value(doc, std::string("missing")), std::invalid_argument);
}

// ---------- get_json_bool ----------
static void test_json_get_bool_true()
{
    auto doc = unit_jh::parse("{\"flag\":true}");
    ASSERT_TRUE(get_json_bool(doc, std::string("flag")));
}

static void test_json_get_bool_false()
{
    auto doc = unit_jh::parse("{\"flag\":false}");
    ASSERT_FALSE(get_json_bool(doc, std::string("flag")));
}

static void test_json_get_bool_type_mismatch_throws()
{
    auto doc = unit_jh::parse("{\"flag\":1}");
    ASSERT_THROWS_TYPE(get_json_bool(doc, std::string("flag")), std::invalid_argument);
}

static void test_json_get_bool_missing_key_throws()
{
    auto doc = unit_jh::parse("{}");
    ASSERT_THROWS_TYPE(get_json_bool(doc, std::string("missing")), std::invalid_argument);
}

// ---------- get_json_int64_t ----------
static void test_json_get_int64()
{
    auto doc = unit_jh::parse("{\"v\":-9223372036854775807}");
    ASSERT_EQ(get_json_int64_t(doc, std::string("v")), -9223372036854775807LL);
}

static void test_json_get_int64_type_mismatch_throws()
{
    auto doc = unit_jh::parse("{\"v\":\"abc\"}");
    ASSERT_THROWS_TYPE(get_json_int64_t(doc, std::string("v")), std::invalid_argument);
}

// ---------- get_json_uint64_t ----------
static void test_json_get_uint64()
{
    auto doc = unit_jh::parse("{\"v\":18446744073709551615}");
    ASSERT_EQ(get_json_uint64_t(doc, std::string("v")), 18446744073709551615ULL);
}

static void test_json_get_uint64_type_mismatch_throws()
{
    auto doc = unit_jh::parse("{\"v\":true}");
    ASSERT_THROWS_TYPE(get_json_uint64_t(doc, std::string("v")), std::invalid_argument);
}

// ---------- get_json_uint32_t ----------
static void test_json_get_uint32()
{
    auto doc = unit_jh::parse("{\"v\":4294967295}");
    ASSERT_EQ(get_json_uint32_t(doc, std::string("v")), 4294967295u);
}

static void test_json_get_uint32_type_mismatch_throws()
{
    auto doc = unit_jh::parse("{\"v\":\"str\"}");
    ASSERT_THROWS_TYPE(get_json_uint32_t(doc, std::string("v")), std::invalid_argument);
}

// ---------- get_json_double ----------
static void test_json_get_double()
{
    auto doc = unit_jh::parse("{\"v\":3.14}");
    ASSERT_NEAR(get_json_double(doc, std::string("v")), 3.14, 1e-9);
}

static void test_json_get_double_type_mismatch_throws()
{
    auto doc = unit_jh::parse("{\"v\":\"pi\"}");
    ASSERT_THROWS_TYPE(get_json_double(doc, std::string("v")), std::invalid_argument);
}

// ---------- get_json_string ----------
static void test_json_get_string()
{
    auto doc = unit_jh::parse("{\"s\":\"hello\"}");
    ASSERT_EQ(get_json_string(doc, std::string("s")), std::string("hello"));
}

static void test_json_get_string_type_mismatch_throws()
{
    auto doc = unit_jh::parse("{\"s\":42}");
    ASSERT_THROWS_TYPE(get_json_string(doc, std::string("s")), std::invalid_argument);
}

static void test_json_get_string_empty()
{
    auto doc = unit_jh::parse("{\"s\":\"\"}");
    ASSERT_EQ(get_json_string(doc, std::string("s")), std::string(""));
}

// ---------- get_json_array ----------
static void test_json_get_array_ok()
{
    auto doc = unit_jh::parse("{\"a\":[1,2,3]}");
    auto arr = get_json_array(doc, std::string("a"));
    ASSERT_EQ(arr.Size(), 3u);
    ASSERT_EQ(arr[0].GetInt(), 1);
    ASSERT_EQ(arr[2].GetInt(), 3);
}

static void test_json_get_array_type_mismatch_throws()
{
    auto doc = unit_jh::parse("{\"a\":\"notarr\"}");
    ASSERT_THROWS_TYPE(get_json_array(doc, std::string("a")), std::invalid_argument);
}

static void test_json_get_array_empty()
{
    auto doc = unit_jh::parse("{\"a\":[]}");
    auto arr = get_json_array(doc, std::string("a"));
    ASSERT_EQ(arr.Size(), 0u);
}

// ---------- get_json_object ----------
static void test_json_get_object_ok()
{
    auto doc = unit_jh::parse("{\"inner\":{\"k\":42}}");
    auto obj = get_json_object(doc, std::string("inner"));
    ASSERT_TRUE(obj.HasMember("k"));
}

static void test_json_get_object_root()
{
    auto doc = unit_jh::parse("{\"a\":1,\"b\":2}");
    auto obj = get_json_object(doc);
    size_t count = 0;
    for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it)
    {
        ++count;
    }
    ASSERT_EQ(count, static_cast<size_t>(2));
}

static void test_json_get_object_type_mismatch_throws()
{
    auto doc = unit_jh::parse("{\"a\":[1,2,3]}");
    ASSERT_THROWS_TYPE(get_json_object(doc, std::string("a")), std::invalid_argument);
}

static void test_json_get_object_missing_key_throws()
{
    auto doc = unit_jh::parse("{}");
    ASSERT_THROWS_TYPE(get_json_object(doc, std::string("missing")), std::invalid_argument);
}

// ---------- JSON_PARSE macro error path ----------
static void test_json_parse_macro_malformed_throws()
{
    const std::string bad = "{not valid json";
    try
    {
        JSON_PARSE(bad);
        (void)body;
        throw std::runtime_error("expected JSON_PARSE to throw");
    }
    catch (const std::invalid_argument &)
    {
    }
}

static void test_json_parse_macro_ok()
{
    const std::string good = "{\"v\":42}";
    JSON_PARSE(good);
    ASSERT_TRUE(body.IsObject());
    ASSERT_EQ(get_json_int64_t(body, std::string("v")), 42);
}

static void test_json_parse_empty_string_throws()
{
    const std::string empty = "";
    try
    {
        JSON_PARSE(empty);
        (void)body;
        throw std::runtime_error("expected JSON_PARSE to throw");
    }
    catch (const std::invalid_argument &)
    {
    }
}

// ---------- STR_TO_JSON macro ----------
static void test_json_str_to_json_ok()
{
    const std::string good = "{\"k\":\"v\"}";
    STR_TO_JSON(good, mybody);
    ASSERT_TRUE(mybody.IsObject());
    ASSERT_EQ(get_json_string(mybody, std::string("k")), std::string("v"));
}

static void test_json_str_to_json_malformed_throws()
{
    const std::string bad = "[not valid";
    try
    {
        STR_TO_JSON(bad, mybody);
        (void)mybody;
        throw std::runtime_error("expected STR_TO_JSON to throw");
    }
    catch (const std::invalid_argument &)
    {
    }
}

// ---------- LOAD_*_FROM_JSON macros ----------
static void test_json_load_string_macro()
{
    auto doc = unit_jh::parse("{\"name\":\"brandon\"}");
    const auto &j = doc;
    std::string name;
    LOAD_STRING_FROM_JSON(name);
    ASSERT_EQ(name, std::string("brandon"));
}

static void test_json_load_bool_macro()
{
    auto doc = unit_jh::parse("{\"enabled\":true}");
    const auto &j = doc;
    bool enabled = false;
    LOAD_BOOL_FROM_JSON(enabled);
    ASSERT_TRUE(enabled);
}

static void test_json_load_u32_macro()
{
    auto doc = unit_jh::parse("{\"count\":12345}");
    const auto &j = doc;
    uint32_t count = 0;
    LOAD_U32_FROM_JSON(count);
    ASSERT_EQ(count, 12345u);
}

static void test_json_load_u64_macro()
{
    auto doc = unit_jh::parse("{\"total\":9999999999}");
    const auto &j = doc;
    uint64_t total = 0;
    LOAD_U64_FROM_JSON(total);
    ASSERT_EQ(total, 9999999999ULL);
}

static void test_json_load_string_missing_key_throws()
{
    auto doc = unit_jh::parse("{}");
    const auto &j = doc;
    try
    {
        std::string name;
        LOAD_STRING_FROM_JSON(name);
        throw std::runtime_error("expected LOAD_STRING_FROM_JSON to throw");
    }
    catch (const std::invalid_argument &)
    {
    }
}

// ---------- JSON_IF_MEMBER macro ----------
static void test_json_if_member_present()
{
    auto doc = unit_jh::parse("{\"field\":7}");
    const auto &j = doc;
    int seen = 0;
    JSON_IF_MEMBER(field)
    {
        seen = 1;
    }
    ASSERT_EQ(seen, 1);
}

static void test_json_if_member_absent()
{
    auto doc = unit_jh::parse("{}");
    const auto &j = doc;
    int seen = 0;
    JSON_IF_MEMBER(field)
    {
        seen = 1;
    }
    ASSERT_EQ(seen, 0);
}

// ---------- JSON_INIT / JSON_DUMP round-trip ----------
static void test_json_init_dump_roundtrip()
{
    JSON_INIT();
    writer.StartObject();
    writer.Key("a");
    writer.Int(42);
    writer.EndObject();
    JSON_DUMP(str);
    ASSERT_EQ(str, std::string("{\"a\":42}"));
}

// ---------- LOAD_KEYV_FROM_JSON / LOAD_KEYVV_FROM_JSON ----------
namespace unit_json
{
    struct Tag
    {
        std::string value;
        explicit Tag(const JSONValue &j)
        {
            JSON_STRING_OR_THROW()
            value = j.GetString();
        }
    };
} // namespace unit_json

static void test_json_load_keyv_macro()
{
    auto doc = unit_jh::parse("{\"tags\":[\"a\",\"b\",\"c\"]}");
    const auto &j = doc;
    std::vector<unit_json::Tag> tags;
    LOAD_KEYV_FROM_JSON(tags, unit_json::Tag);
    ASSERT_EQ(tags.size(), static_cast<size_t>(3));
    ASSERT_EQ(tags[0].value, std::string("a"));
    ASSERT_EQ(tags[1].value, std::string("b"));
    ASSERT_EQ(tags[2].value, std::string("c"));
}

static void test_json_load_keyvv_macro()
{
    auto doc = unit_jh::parse("{\"groups\":[[\"x\",\"y\"],[\"z\"]]}");
    const auto &j = doc;
    std::vector<std::vector<unit_json::Tag>> groups;
    LOAD_KEYVV_FROM_JSON(groups, unit_json::Tag);
    ASSERT_EQ(groups.size(), static_cast<size_t>(2));
    ASSERT_EQ(groups[0].size(), static_cast<size_t>(2));
    ASSERT_EQ(groups[0][0].value, std::string("x"));
    ASSERT_EQ(groups[0][1].value, std::string("y"));
    ASSERT_EQ(groups[1].size(), static_cast<size_t>(1));
    ASSERT_EQ(groups[1][0].value, std::string("z"));
}

// ---------- JSON_MEMBER_OR_THROW indirect exercise ----------
static void test_json_member_or_throw_hits_when_missing()
{
    auto doc = unit_jh::parse("{}");
    const auto &j = doc;
    try
    {
        JSON_MEMBER_OR_THROW("absent")
        throw std::runtime_error("expected JSON_MEMBER_OR_THROW to fire");
    }
    catch (const std::invalid_argument &)
    {
    }
}

#endif
