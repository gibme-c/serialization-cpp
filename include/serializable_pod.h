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

#ifndef SERIALIZATION_SERIALIZABLE_POD_H
#define SERIALIZATION_SERIALIZABLE_POD_H

#include <ostream>
#include <serializable.h>

/**
 * A fixed-size byte array (default 32 bytes) that implements Serializable. Great for
 * things like hashes, keys, or any fixed-width binary blob. Displays as hex, securely
 * erases its memory on destruction, and supports comparison operators out of the box.
 * Override load_hook() if you need custom logic after deserialization.
 */
template<unsigned int SIZE = 32> struct SerializablePod : Serializable
{
  public:
    SerializablePod() = default;

    /** Constructs from a hex string (must decode to exactly SIZE bytes). */
    explicit SerializablePod(const std::string &value)
    {
        from_string(value);
    }

    ~SerializablePod()
    {
        serialization_secure_erase(bytes, sizeof(bytes));
    }

    // -- Operators: override these if your subclass has a different internal layout --

    virtual unsigned char *operator*()
    {
        return bytes;
    }

    virtual unsigned char &operator[](size_t i)
    {
        return bytes[i];
    }

    virtual unsigned char operator[](size_t i) const
    {
        return bytes[i];
    }

    virtual bool operator==(const SerializablePod<SIZE> &other) const
    {
        return std::equal(std::begin(bytes), std::end(bytes), std::begin(other.bytes));
    }

    virtual bool operator!=(const SerializablePod<SIZE> &other) const
    {
        return !(*this == other);
    }

    virtual bool operator<(const SerializablePod<SIZE> &other) const
    {
        for (size_t i = SIZE; i-- > 0;)
        {
            if (bytes[i] < other.bytes[i])
            {
                return true;
            }

            if (bytes[i] > other.bytes[i])
            {
                return false;
            }
        }

        return false;
    }

    virtual bool operator>(const SerializablePod<SIZE> &other) const
    {
        for (size_t i = SIZE; i-- > 0;)
        {
            if (bytes[i] > other.bytes[i])
            {
                return true;
            }

            if (bytes[i] < other.bytes[i])
            {
                return false;
            }
        }

        return false;
    }

    virtual bool operator<=(const SerializablePod<SIZE> &other) const
    {
        return (*this == other) || (*this < other);
    }

    virtual bool operator>=(const SerializablePod<SIZE> &other) const
    {
        return (*this == other) || (*this > other);
    }

    /** Direct pointer to the raw byte array. */
    [[nodiscard]] virtual const unsigned char *data() const
    {
        return bytes;
    }

    /** Loads from a byte vector (must be exactly SIZE bytes). Calls load_hook() afterward. */
    void deserialize(const std::vector<unsigned char> &data) override
    {
        if (data.size() != sizeof(bytes))
        {
            throw std::runtime_error("data is of the wrong size for this structure");
        }

        std::memcpy(&bytes, data.data(), data.size());

        load_hook();
    }

    /** Reads SIZE bytes from a deserializer_t and loads them. */
    void deserialize(Serialization::deserializer_t &reader) override
    {
        const auto data = reader.bytes(sizeof(bytes));

        deserialize(data);
    }

    /** True if all bytes are zero (i.e., never been set). */
    [[nodiscard]] virtual bool empty() const
    {
        return *this == SerializablePod<SIZE>();
    }

    /** Loads from a JSON string value (expects hex). */
    JSON_FROM_FUNC(fromJSON) override
    {
        JSON_STRING_OR_THROW()

        from_string(j.GetString());
    }

    /** Loads from a named key inside a JSON object. */
    JSON_FROM_KEY_FUNC(fromJSON) override
    {
        if (!has_member(val, std::string(key)))
        {
            throw std::invalid_argument(std::string(key) + " not found in JSON object");
        }

        const auto &j = get_json_value(val, key);

        fromJSON(j);
    }

    /** Writes the raw bytes into a serializer_t. */
    void serialize(Serialization::serializer_t &writer) const override
    {
        writer.bytes(bytes, sizeof(bytes));
    }

    /** Returns the raw bytes as a vector. */
    [[nodiscard]] std::vector<unsigned char> serialize() const override
    {
        return {std::begin(bytes), std::end(bytes)};
    }

    /** Always returns SIZE (the fixed byte count). */
    [[nodiscard]] size_t size() const override
    {
        return sizeof(bytes);
    }

    /** Writes as a hex string to the JSON writer. */
    JSON_TO_FUNC(toJSON) override
    {
        writer.String(to_string());
    }

    /** Returns the bytes as a hex string. */
    [[nodiscard]] std::string to_string() const override
    {
        const auto data = serialize();

        return Serialization::to_hex(data.data(), data.size());
    }

  protected:
    /** Decodes a hex string and loads it into the byte array. */
    void from_string(const std::string &str)
    {
        const auto input = Serialization::from_hex(str);

        if (input.size() != sizeof(bytes))
        {
            throw std::runtime_error("Value provided is of invalid size");
        }

        std::copy(input.begin(), input.end(), std::begin(bytes));

        load_hook();
    }

    /** Called after any load/deserialize. Override this for post-load validation or setup. */
    virtual void load_hook() {}

    unsigned char bytes[SIZE] = {0};
};

/** Lets you use SerializablePod with std::cout and friends. */
template<unsigned int SIZE>
inline std::ostream &operator<<(std::ostream &os, const SerializablePod<SIZE> &value)
{
    os << value.to_string();

    return os;
}

#endif
