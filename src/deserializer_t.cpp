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

#include <deserializer_t.h>

namespace Serialization
{
    deserializer_t::deserializer_t(const serializer_t &writer)
    {
        buffer = writer.vector();
    }

    deserializer_t::deserializer_t(std::initializer_list<unsigned char> input)
    {
        buffer = std::vector<unsigned char>(input.begin(), input.end());
    }

    deserializer_t::deserializer_t(const std::vector<unsigned char> &input)
    {
        buffer = input;
    }

    deserializer_t::deserializer_t(const std::string &input)
    {
        buffer = from_hex(input);
    }

    bool deserializer_t::boolean(bool peek)
    {
        return uint8(peek) == 1;
    }

    std::vector<unsigned char> deserializer_t::bytes(size_t count, bool peek)
    {
        const auto start = offset;

        if (!peek)
        {
            offset += count;
        }

        return {buffer.begin() + start, buffer.begin() + start + count};
    }

    void deserializer_t::compact()
    {
        buffer = std::vector<unsigned char>(buffer.begin() + offset, buffer.end());
    }

    const unsigned char *deserializer_t::data() const
    {
        return buffer.data();
    }

    std::string deserializer_t::hex(size_t length, bool peek)
    {
        const auto temp = bytes(length, peek);

        return to_hex(temp.data(), temp.size());
    }

    void deserializer_t::reset(size_t position)
    {
        offset = position;
    }

    size_t deserializer_t::size() const
    {
        return buffer.size();
    }

    void deserializer_t::skip(size_t count)
    {
        offset += count;
    }

    std::string deserializer_t::to_string() const
    {
        return to_hex(buffer.data(), buffer.size());
    }

    unsigned char deserializer_t::uint8(bool peek)
    {
        const auto start = offset;

        if (!peek)
        {
            offset += sizeof(unsigned char);
        }

        return unpack<unsigned char>(buffer, start);
    }

    uint16_t deserializer_t::uint16(bool peek, bool big_endian)
    {
        const auto start = offset;

        if (!peek)
        {
            offset += sizeof(uint16_t);
        }

        return unpack<uint16_t>(buffer, start, big_endian);
    }

    uint32_t deserializer_t::uint32(bool peek, bool big_endian)
    {
        const auto start = offset;

        if (!peek)
        {
            offset += sizeof(uint32_t);
        }

        return unpack<uint32_t>(buffer, start, big_endian);
    }

    uint64_t deserializer_t::uint64(bool peek, bool big_endian)
    {
        const auto start = offset;

        if (!peek)
        {
            offset += sizeof(uint64_t);
        }

        return unpack<uint64_t>(buffer, start, big_endian);
    }

    uint128_t deserializer_t::uint128(bool peek, bool big_endian)
    {
        const auto start = offset;

        if (!peek)
        {
            offset += sizeof(uint128_t);
        }

        return unpack<uint128_t>(buffer, start, big_endian);
    }

    uint256_t deserializer_t::uint256(bool peek, bool big_endian)
    {
        const auto start = offset;

        if (!peek)
        {
            offset += sizeof(uint256_t);
        }

        return unpack<uint256_t>(buffer, start, big_endian);
    }

    size_t deserializer_t::unread_bytes() const
    {
        const auto unread = buffer.size() - offset;

        return (unread >= 0) ? unread : 0;
    }

    std::vector<unsigned char> deserializer_t::unread_data() const
    {
        return {buffer.begin() + offset, buffer.end()};
    }
} // namespace Serialization
