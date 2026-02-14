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
//
// Inspired by the work of Zpalmtree

#ifndef SERIALIZATION_JSON_HELPER_H
#define SERIALIZATION_JSON_HELPER_H

#include <rapidjson/document.h>
#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <stdexcept>

// -- Validation macros: throw with a helpful message if the JSON value is the wrong type --

#define JSON_TYPE_NAME kTypeNames[j.GetType()]
#define JSON_STRING_OR_THROW()                                                             \
    if (!j.IsString())                                                                     \
    {                                                                                      \
        throw std::invalid_argument("JSON value is of the wrong type: " + JSON_TYPE_NAME); \
    }

#define JSON_OBJECT_OR_THROW()                                                             \
    if (!j.IsObject())                                                                     \
    {                                                                                      \
        throw std::invalid_argument("JSON value is of the wrong type: " + JSON_TYPE_NAME); \
    }

#define JSON_MEMBER_OR_THROW(value)                                                    \
    if (!has_member(j, std::string(value)))                                            \
    {                                                                                  \
        throw std::invalid_argument(std::string(value) + " not found in JSON object"); \
    }

/** Check if a field exists before trying to load it (use inside fromJSON). */
#define JSON_IF_MEMBER(field) if (has_member(j, #field))

/** Generates constructors that build the object from a JSON object or a named key within one. */
#define JSON_OBJECT_CONSTRUCTOR(objtype, funccall)        \
    explicit objtype(const JSONValue &j)                  \
    {                                                     \
        JSON_OBJECT_OR_THROW()                            \
        funccall(j);                                      \
    }                                                     \
                                                          \
    objtype(const JSONValue &val, const std::string &key) \
    {                                                     \
        const auto &j = get_json_value(val, key);         \
        JSON_OBJECT_OR_THROW()                            \
        funccall(j);                                      \
    }

/** Same as JSON_OBJECT_CONSTRUCTOR but expects a JSON string value instead. */
#define JSON_STRING_CONSTRUCTOR(objtype, funccall)        \
    explicit objtype(const JSONValue &j)                  \
    {                                                     \
        JSON_STRING_OR_THROW()                            \
        funccall(j);                                      \
    }                                                     \
                                                          \
    objtype(const JSONValue &val, const std::string &key) \
    {                                                     \
        const auto &j = get_json_value(val, key);         \
        JSON_STRING_OR_THROW()                            \
        funccall(j);                                      \
    }

// -- Method signature macros: use these to declare your fromJSON / toJSON methods --

/** Declares: void name(const JSONValue &j) */
#define JSON_FROM_FUNC(name) void name(const JSONValue &j)
/** Declares: void name(const JSONValue &val, const std::string &key) */
#define JSON_FROM_KEY_FUNC(name) void name(const JSONValue &val, const std::string &key)
/** Declares: void name(Writer &writer) const */
#define JSON_TO_FUNC(name) void name(rapidjson::Writer<rapidjson::StringBuffer> &writer) const

// -- Writer setup / teardown macros --

/** Creates a StringBuffer + Writer pair (named `buffer` and `writer`). */
#define JSON_INIT()                 \
    rapidjson::StringBuffer buffer; \
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer)

/** Same as JSON_INIT but lets you pick the variable names. */
#define JSON_INIT_BUFFER(buffer, writer) \
    rapidjson::StringBuffer buffer;      \
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer)

/** Grabs the finished JSON string from the buffer (named `str`). */
#define JSON_DUMP(str) const std::string str = buffer.GetString()

/** Same as JSON_DUMP but with custom buffer name. */
#define JSON_DUMP_BUFFER(buffer, str) const std::string str = (buffer).GetString()

// -- Parsing macros --

/** Parses a JSON string into a Document called `body`. Throws on invalid JSON. */
#define JSON_PARSE(json)                                     \
    rapidjson::Document body;                                \
    if (body.Parse((json).c_str()).HasParseError())          \
    {                                                        \
        throw std::invalid_argument("Could not parse JSON"); \
    }

/** Same as JSON_PARSE but lets you pick the variable names. */
#define STR_TO_JSON(str, body)                               \
    rapidjson::Document body;                                \
    if ((body).Parse((str).c_str()).HasParseError())         \
    {                                                        \
        throw std::invalid_argument("Could not parse JSON"); \
    }

// -- Field loading macros: use inside fromJSON() to pull fields by name --
// The JSON key must match the C++ field name exactly.

/** Loads a Serializable field -- calls field.fromJSON(j, "field"). */
#define LOAD_KEY_FROM_JSON(field)    \
    {                                \
        JSON_MEMBER_OR_THROW(#field) \
                                     \
        (field).fromJSON(j, #field); \
    }

/** Loads a vector of Serializable objects from a JSON array field. */
#define LOAD_KEYV_FROM_JSON(field, type)                   \
    {                                                      \
        JSON_MEMBER_OR_THROW(#field)                       \
        (field).clear();                                   \
        for (const auto &elem : get_json_array(j, #field)) \
        {                                                  \
            auto temp = type(elem);                        \
            (field).emplace_back(temp);                    \
        }                                                  \
    }

/** Loads a nested (2D) vector of Serializable objects from a JSON array-of-arrays field. */
#define LOAD_KEYVV_FROM_JSON(field, type)                    \
    {                                                        \
        JSON_MEMBER_OR_THROW(#field)                         \
        (field).clear();                                     \
        for (const auto &level1 : get_json_array(j, #field)) \
        {                                                    \
            (field).resize((field).size() + 1);              \
            auto &inner = (field).back();                    \
            for (const auto &elem : get_json_array(level1))  \
            {                                                \
                auto temp = type(elem);                      \
                inner.emplace_back(temp);                    \
            }                                                \
        }                                                    \
    }

/** Loads a std::string field from JSON. */
#define LOAD_STRING_FROM_JSON(field)          \
    {                                         \
        JSON_MEMBER_OR_THROW(#field);         \
        (field) = get_json_string(j, #field); \
    }

/** Loads a bool field from JSON. */
#define LOAD_BOOL_FROM_JSON(field)          \
    {                                       \
        JSON_MEMBER_OR_THROW(#field);       \
        (field) = get_json_bool(j, #field); \
    }

/** Loads a uint64_t field from JSON. */
#define LOAD_U64_FROM_JSON(field)               \
    {                                           \
        JSON_MEMBER_OR_THROW(#field);           \
        (field) = get_json_uint64_t(j, #field); \
    }

/** Loads a uint32_t field from JSON. */
#define LOAD_U32_FROM_JSON(field)               \
    {                                           \
        JSON_MEMBER_OR_THROW(#field);           \
        (field) = get_json_uint32_t(j, #field); \
    }

// -- Field writing macros: use inside toJSON() to write fields by name --

/** Writes a Serializable field as "field": <value>. */
#define KEY_TO_JSON(field)      \
    {                           \
        writer.Key(#field);     \
        (field).toJSON(writer); \
    }

/** Writes a vector of Serializable objects as a JSON array. */
#define KEYV_TO_JSON(field)                 \
    {                                       \
        writer.Key(#field);                 \
        writer.StartArray();                \
        {                                   \
            for (const auto &val : (field)) \
            {                               \
                val.toJSON(writer);         \
            }                               \
        }                                   \
        writer.EndArray();                  \
    }

/** Writes a nested (2D) vector as a JSON array of arrays. */
#define KEYVV_TO_JSON(field)                       \
    {                                              \
        writer.Key(#field);                        \
        writer.StartArray();                       \
        {                                          \
            for (const auto &level1 : (field))     \
            {                                      \
                writer.StartArray();               \
                {                                  \
                    for (const auto &val : level1) \
                    {                              \
                        val.toJSON(writer);        \
                    }                              \
                }                                  \
                writer.EndArray();                 \
            }                                      \
        }                                          \
        writer.EndArray();                         \
    }

/** Writes a uint64_t field. */
#define U64_TO_JSON(field)    \
    {                         \
        writer.Key(#field);   \
        writer.Uint64(field); \
    }

/** Writes a uint32_t field. */
#define U32_TO_JSON(field)  \
    {                       \
        writer.Key(#field); \
        writer.Uint(field); \
    }

/** Writes a std::string field. */
#define STRING_TO_JSON(field) \
    {                         \
        writer.Key(#field);   \
        writer.String(field); \
    }

/** Writes a bool field. */
#define BOOL_TO_JSON(field) \
    {                       \
        writer.Key(#field); \
        writer.Bool(field); \
    }

typedef rapidjson::GenericObject<
    true,
    rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>>>
    JSONObject;

typedef rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>>
    JSONValue;

static const std::string kTypeNames[] = {"Null", "False", "True", "Object", "Array", "String", "Number", "Double"};

// -- Typed getter functions for pulling values out of RapidJSON documents --

/** Returns true if the JSON object has a key with the given name. */
template<typename T> bool has_member(const T &j, const std::string &key)
{
    const auto val = j.FindMember(key);

    return val != j.MemberEnd();
}

/** Gets the raw JSON value at the given key. Throws if the key is missing. */
template<typename T> const rapidjson::Value &get_json_value(const T &j, const std::string &key)
{
    const auto val = j.FindMember(key);

    if (val == j.MemberEnd())
    {
        throw std::invalid_argument("Missing JSON parameter: '" + key + "'");
    }

    return val->value;
}

/** Gets a bool from a JSON value. Throws if it's not a bool. */
template<typename T> bool get_json_bool(const T &j)
{
    if (!j.IsBool())
    {
        throw std::invalid_argument("JSON parameter is wrong type. Expected bool, got " + kTypeNames[j.GetType()]);
    }

    return j.GetBool();
}

/** Gets a bool from a named key in a JSON object. */
template<typename T> bool get_json_bool(const T &j, const std::string &key)
{
    const auto &val = get_json_value(j, key);

    return get_json_bool(val);
}

/** Gets an int64_t from a JSON value. Throws on type mismatch. */
template<typename T> int64_t get_json_int64_t(const T &j)
{
    if (!j.IsInt64())
    {
        throw std::invalid_argument("JSON parameter is wrong type. Expected int64_t, got " + kTypeNames[j.GetType()]);
    }

    return j.GetInt64();
}

/** Gets an int64_t from a named key in a JSON object. */
template<typename T> int64_t get_json_int64_t(const T &j, const std::string &key)
{
    const auto &val = get_json_value(j, key);

    return get_json_int64_t(val);
}

/** Gets a uint64_t from a JSON value. Throws on type mismatch. */
template<typename T> uint64_t get_json_uint64_t(const T &j)
{
    if (!j.IsUint64())
    {
        throw std::invalid_argument("JSON parameter is wrong type. Expected uint64_t, got " + kTypeNames[j.GetType()]);
    }

    return j.GetUint64();
}

/** Gets a uint64_t from a named key in a JSON object. */
template<typename T> uint64_t get_json_uint64_t(const T &j, const std::string &key)
{
    const auto &val = get_json_value(j, key);

    return get_json_uint64_t(val);
}

/** Gets a uint32_t from a JSON value. Throws on type mismatch. */
template<typename T> uint32_t get_json_uint32_t(const T &j)
{
    if (!j.IsUint())
    {
        throw std::invalid_argument("JSON parameter is wrong type. Expected uint32_t, got " + kTypeNames[j.GetType()]);
    }

    return j.GetUint();
}

/** Gets a uint32_t from a named key in a JSON object. */
template<typename T> uint32_t get_json_uint32_t(const T &j, const std::string &key)
{
    const auto &val = get_json_value(j, key);

    return get_json_uint32_t(val);
}

/** Gets a double from a JSON value. Throws on type mismatch. */
template<typename T> double get_json_double(const T &j)
{
    if (!j.IsDouble())
    {
        throw std::invalid_argument("JSON parameter is wrong type. Expected double, got " + kTypeNames[j.GetType()]);
    }

    return j.GetDouble();
}

/** Gets a double from a named key in a JSON object. */
template<typename T> double get_json_double(const T &j, const std::string &key)
{
    const auto &val = get_json_value(j, key);

    return get_json_double(val);
}

/** Gets a string from a JSON value. Throws on type mismatch. */
template<typename T> std::string get_json_string(const T &j)
{
    if (!j.IsString())
    {
        throw std::invalid_argument(
            "JSON parameter is wrong type. Expected std::string, got " + kTypeNames[j.GetType()]);
    }

    return j.GetString();
}

/** Gets a string from a named key in a JSON object. */
template<typename T> std::string get_json_string(const T &j, const std::string &key)
{
    const auto &val = get_json_value(j, key);

    return get_json_string(val);
}

/** Gets a JSON array. Throws if the value isn't an array. */
template<typename T> auto get_json_array(const T &j)
{
    if (!j.IsArray())
    {
        throw std::invalid_argument("JSON parameter is wrong type. Expected Array, got " + kTypeNames[j.GetType()]);
    }

    return j.GetArray();
}

/** Gets a JSON array from a named key in a JSON object. */
template<typename T> auto get_json_array(const T &j, const std::string &key)
{
    const auto &val = get_json_value(j, key);

    return get_json_array(val);
}

/** Gets a JSON object. Throws if the value isn't an object. */
template<typename T> JSONObject get_json_object(const T &j)
{
    if (!j.IsObject())
    {
        throw std::invalid_argument("JSON parameter is wrong type. Expected Object, got " + kTypeNames[j.GetType()]);
    }

    return j.Get_Object();
}

/** Gets a JSON object from a named key in a JSON object. */
template<typename T> JSONObject get_json_object(const T &j, const std::string &key)
{
    const auto &val = get_json_value(j, key);

    return get_json_object(val);
}

#endif // SERIALIZATION_JSON_HELPER_H
