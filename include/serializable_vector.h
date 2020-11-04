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

template<typename Type> struct SerializableVector : Serializable
{
  public:
    SerializableVector() = default;

    JSON_OBJECT_CONSTRUCTOR(SerializableVector, fromJSON);

    /**
     * Constructs the structure from the supplied hex encoded string
     *
     * @param value
     */
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

    /**
     * Alias of struct::container::push_back()
     *
     * @param value
     */
    void append(const Type &value)
    {
        container.push_back(value);
    }

    /**
     * Alias of struct:container::back()
     *
     * @return
     */
    [[nodiscard]] Type back() const
    {
        return container.back();
    }

    /**
     * Deserializes the structure from a deserializer_t
     *
     * @param reader
     */
    void deserialize(Serialization::deserializer_t &reader) override
    {
        container = reader.podV<Type>();
    }

    /**
     * Deserializes the structure from a std::vector<unsigned char>
     *
     * @param data
     */
    void deserialize(const std::vector<unsigned char> &data) override
    {
        auto reader = Serialization::deserializer_t(data);

        deserialize(reader);
    }

    /**
     * Appends the provided vector to the end of the underlying container
     *
     * @param values
     */
    void extend(const std::vector<Type> &values)
    {
        for (const auto &value : values)
        {
            container.push_back(value);
        }
    }

    /**
     * Appends the provided vector to the end of the underlying container
     *
     * @param value
     */
    void extend(const SerializableVector<Type> &value)
    {
        extend(value.container);
    }

    /**
     * Loads the structure from JSON
     *
     * @param j
     */
    JSON_FROM_FUNC(fromJSON) override
    {
        container.clear();

        for (const auto &elem : get_json_array(j))
        {
            auto temp = Type(elem);

            container.emplace_back(temp);
        }
    }

    /**
     * Loads the pod from a JSON value in the specified key of the JSON object
     *
     * @param val
     * @param key
     */
    JSON_FROM_KEY_FUNC(fromJSON) override
    {
        if (!has_member(val, std::string(key)))
        {
            throw std::invalid_argument(std::string(key) + " not found in JSON object");
        }

        const auto &j = get_json_value(val, key);

        fromJSON(j);
    }

    /**
     * Serializes the structure using the supplied serializer_t
     *
     * @param writer
     */
    void serialize(Serialization::serializer_t &writer) const override
    {
        writer.pod(container);
    }

    /**
     * Serializes the structure to a std::vector<unsigned char>
     *
     * @return
     */
    [[nodiscard]] std::vector<unsigned char> serialize() const override
    {
        auto writer = Serialization::serializer_t();

        serialize(writer);

        return writer.vector();
    }

    /**
     * Returns the size of the structure
     *
     * Note: unlike other structures, this returns the size of (ie. the number of elements in)
     * the underlying container
     *
     * @return
     */
    [[nodiscard]] size_t size() const override
    {
        return container.size();
    }

    /**
     * Serializes the structure to JSON
     *
     * @param writer
     */
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

    /**
     * Returns the structure as a string
     *
     * @return
     */
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
