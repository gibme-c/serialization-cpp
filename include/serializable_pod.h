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

#include <serializable.h>

template<unsigned int SIZE = 32> struct SerializablePod : Serializable
{
  public:
    SerializablePod() = default;

    /**
     * Constructs the pod from the supplied hex encoded string
     *
     * @param value
     */
    explicit SerializablePod(const std::string &value)
    {
        from_string(value);
    }

    ~SerializablePod()
    {
        secure_erase(bytes, sizeof(bytes));
    }

    /**
     * Overloading of standard operators to make operations using this structure
     * to use a lot cleaner syntactic sugar in downstream code. These should generally
     * be overwritten if the internal structure is different than the default.
     */

    virtual unsigned char *operator*()
    {
        return bytes;
    }

    virtual unsigned char &operator[](int i)
    {
        return bytes[i];
    }

    virtual unsigned char operator[](int i) const
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

    /**
     * Returns a pointer to the underlying data structure
     *
     * @return
     */
    [[nodiscard]] virtual const unsigned char *data() const
    {
        return bytes;
    }

    /**
     * Deserializes the pod from the supplied vector of unsigned char (bytes)
     *
     * @param data
     */
    void deserialize(const std::vector<unsigned char> &data) override
    {
        if (data.size() != sizeof(bytes))
        {
            throw std::runtime_error("data is of the wrong size for this structure");
        }

        std::memcpy(&bytes, data.data(), data.size());

        load_hook();
    }

    /**
     * Deserializes the pod from the supplied reader
     *
     * @param reader
     */
    void deserialize(Serialization::deserializer_t &reader) override
    {
        const auto data = reader.bytes(sizeof(bytes));

        deserialize(data);
    }

    /**
     * Returns if the structure is empty (unset)
     *
     * @return
     */
    [[nodiscard]] virtual bool empty() const
    {
        return *this == SerializablePod<SIZE>();
    }

    /**
     * Loads the pod from a JSON value
     *
     * @param j
     */
    JSON_FROM_FUNC(fromJSON) override
    {
        JSON_STRING_OR_THROW()

        from_string(j.GetString());
    }

    /**
     * Loads the pod from a JSON value in the specified key of the JSON object
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
     * Serializes the pod into bytes using the supplied serializer_t instance
     *
     * @param writer
     */
    void serialize(Serialization::serializer_t &writer) const override
    {
        writer.bytes(bytes, sizeof(bytes));
    }

    /**
     * Returns the pod as a vector of unsigned chars (bytes)
     *
     * @return
     */
    [[nodiscard]] std::vector<unsigned char> serialize() const override
    {
        return {std::begin(bytes), std::end(bytes)};
    }

    /**
     * Returns the size of the data in the pod
     *
     * @return
     */
    [[nodiscard]] size_t size() const override
    {
        return sizeof(bytes);
    }

    /**
     * Writes the pod to the to the supplied json writer as a string
     *
     * @param writer
     */
    JSON_TO_FUNC(toJSON) override
    {
        writer.String(to_string());
    }

    /**
     * Returns the pod as a hex encoded string
     *
     * @return
     */
    [[nodiscard]] std::string to_string() const override
    {
        const auto data = serialize();

        return Serialization::to_hex(data.data(), data.size());
    }

  protected:
    /**
     * Loads the POD from a hex encoded string
     *
     * @param str
     */
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

    /**
     * Hook that's called after loading the pod from a string and other methods
     */
    virtual void load_hook() {}

    unsigned char bytes[SIZE] = {0};
};

/**
 * Providing overloads into the std namespace such that we can easily included
 * points, scalars, and signatures in output streams
 */
namespace std
{
    template<unsigned int SIZE> inline ostream &operator<<(ostream &os, const SerializablePod<SIZE> &value)
    {
        os << value.to_string();

        return os;
    }
} // namespace std

#endif
