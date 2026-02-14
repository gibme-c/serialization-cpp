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

#ifndef SERIALIZATION_SERIALIZABLE_VECTOR_H
#define SERIALIZATION_SERIALIZABLE_VECTOR_H

#include <serializable.h>

/**
 * A serializable wrapper around std::vector<Type> where Type itself must be Serializable.
 * Handles binary serialization, JSON array conversion, and hex string round-tripping.
 */
template<typename Type> struct SerializableVector : Serializable
{
  public:
    SerializableVector() = default;

    JSON_OBJECT_CONSTRUCTOR(SerializableVector, fromJSON);

    /** Constructs from a hex string by deserializing the decoded bytes. */
    explicit SerializableVector(const std::string &value)
    {
        from_string(value);
    }

    Type &operator[](size_t i)
    {
        return container[i];
    }

    Type operator[](size_t i) const
    {
        return container[i];
    }

    bool operator==(const SerializableVector<Type> &other) const
    {
        return std::equal(container.begin(), container.end(), other.container.begin());
    }

    bool operator!=(const SerializableVector<Type> &other) const
    {
        return !(*this == other);
    }

    /** Adds an element to the end. */
    void append(const Type &value)
    {
        container.push_back(value);
    }

    /** Returns the last element. */
    [[nodiscard]] Type back() const
    {
        return container.back();
    }

    /** Reads a varint-prefixed list of elements from a deserializer_t. */
    void deserialize(Serialization::deserializer_t &reader) override
    {
        container = reader.podV<Type>();
    }

    /** Reads from a raw byte vector. */
    void deserialize(const std::vector<unsigned char> &data) override
    {
        auto reader = Serialization::deserializer_t(data);

        deserialize(reader);
    }

    /** Appends all elements from a vector to the end. */
    void extend(const std::vector<Type> &values)
    {
        for (const auto &value : values)
        {
            container.push_back(value);
        }
    }

    /** Appends all elements from another SerializableVector to the end. */
    void extend(const SerializableVector<Type> &value)
    {
        extend(value.container);
    }

    /** Populates from a JSON array -- each element is constructed as Type(elem). */
    JSON_FROM_FUNC(fromJSON) override
    {
        container.clear();

        for (const auto &elem : get_json_array(j))
        {
            auto temp = Type(elem);

            container.emplace_back(temp);
        }
    }

    /** Populates from a named key inside a JSON object. */
    JSON_FROM_KEY_FUNC(fromJSON) override
    {
        if (!has_member(val, std::string(key)))
        {
            throw std::invalid_argument(std::string(key) + " not found in JSON object");
        }

        const auto &j = get_json_value(val, key);

        fromJSON(j);
    }

    /** Writes a varint count followed by each element into the serializer. */
    void serialize(Serialization::serializer_t &writer) const override
    {
        writer.pod(container);
    }

    /** Returns the serialized bytes. */
    [[nodiscard]] std::vector<unsigned char> serialize() const override
    {
        auto writer = Serialization::serializer_t();

        serialize(writer);

        return writer.vector();
    }

    /** Returns the element count (not byte size). */
    [[nodiscard]] size_t size() const override
    {
        return container.size();
    }

    /** Writes as a JSON array with each element calling its own toJSON(). */
    void toJSON(rapidjson::Writer<rapidjson::StringBuffer> &writer) const override
    {
        writer.StartArray();
        {
            for (const auto &val : container)
            {
                val.toJSON(writer);
            }
        }
        writer.EndArray();
    }

    /** Returns the serialized bytes as a hex string. */
    [[nodiscard]] std::string to_string() const override
    {
        const auto data = serialize();

        return Serialization::to_hex(data.data(), data.size());
    }

    std::vector<Type> container = std::vector<Type>();

  protected:
    void from_string(const std::string &str)
    {
        auto reader = Serialization::deserializer_t(str);

        deserialize(reader);
    }
};

#endif
