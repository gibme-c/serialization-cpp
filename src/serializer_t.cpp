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

#include <serializer_t.h>

namespace Serialization
{
    serializer_t::serializer_t(const serializer_t &writer)
    {
        buffer = writer.vector();
    }

    serializer_t::serializer_t(std::initializer_list<unsigned char> input)
    {
        buffer = std::vector<unsigned char>(input);
    }

    serializer_t::serializer_t(const std::vector<unsigned char> &input)
    {
        buffer = input;
    }

    unsigned char &serializer_t::operator[](size_t i)
    {
        return buffer[i];
    }

    unsigned char serializer_t::operator[](size_t i) const
    {
        return buffer[i];
    }

    void serializer_t::boolean(bool value)
    {
        if (value)
        {
            buffer.push_back(1);
        }
        else
        {
            buffer.push_back(0);
        }
    }

    void serializer_t::bytes(const void *data, size_t length)
    {
        if (data == nullptr && length > 0)
        {
            throw std::invalid_argument("cannot read bytes from null pointer");
        }

        auto const *raw = static_cast<unsigned char const *>(data);

        for (size_t i = 0; i < length; ++i)
        {
            buffer.push_back(raw[i]);
        }
    }

    void serializer_t::bytes(const std::vector<unsigned char> &value)
    {
        extend(value);
    }

    const unsigned char *serializer_t::data() const
    {
        return buffer.data();
    }

    void serializer_t::extend(const std::vector<unsigned char> &vector)
    {
        for (const auto &element : vector)
        {
            buffer.push_back(element);
        }
    }

    void serializer_t::hex(const std::string &value)
    {
        const auto bytes = from_hex(value);

        extend(bytes);
    }

    void serializer_t::reset()
    {
        buffer.clear();
    }

    size_t serializer_t::size() const
    {
        return buffer.size();
    }

    std::string serializer_t::to_string() const
    {
        return to_hex(buffer.data(), buffer.size());
    }

    void serializer_t::uint8(const unsigned char &value)
    {
        buffer.push_back(value);
    }

    void serializer_t::uint16(const uint16_t &value, bool big_endian)
    {
        const auto packed = pack(value, big_endian);

        extend(packed);
    }

    void serializer_t::uint32(const uint32_t &value, bool big_endian)
    {
        const auto packed = pack(value, big_endian);

        extend(packed);
    }

    void serializer_t::uint64(const uint64_t &value, bool big_endian)
    {
        const auto packed = pack(value, big_endian);

        extend(packed);
    }

    void serializer_t::uint128(const uint128_t &value, bool big_endian)
    {
        const auto packed = pack(value, big_endian);

        extend(packed);
    }

    void serializer_t::uint256(const uint256_t &value, bool big_endian)
    {
        const auto packed = pack(value, big_endian);

        extend(packed);
    }

    std::vector<unsigned char> serializer_t::vector() const
    {
        return buffer;
    }
} // namespace Serialization
