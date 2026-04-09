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

#ifndef SERIALIZATION_HELPER_H
#define SERIALIZATION_HELPER_H

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace Serialization
{
    /** Converts any fixed-size type into a byte vector (optionally big-endian). */
    template<typename Type> std::vector<unsigned char> pack(const Type &value, bool big_endian = false)
    {
        static_assert(sizeof(Type) <= 64, "Type exceeds pack buffer size");

        unsigned char bytes[64] = {0};

        std::memcpy(&bytes, &value, sizeof(Type));

        auto result = std::vector<unsigned char>(bytes, bytes + sizeof(Type));

        if (big_endian)
        {
            std::reverse(result.begin(), result.end());
        }

        return result;
    }

    /** Reads a fixed-size type back out of a byte vector at the given offset. */
    template<typename Type>
    Type unpack(const std::vector<unsigned char> &packed, size_t offset = 0, bool big_endian = false)
    {
        static_assert(sizeof(Type) <= 64, "Type exceeds unpack buffer size");

        const auto size = sizeof(Type);

        if (size > packed.size() || offset > packed.size() - size)
        {
            throw std::range_error("not enough data to complete request");
        }

        unsigned char bytes[64] = {0};

        std::memcpy(bytes, packed.data() + offset, size);

        if (big_endian)
        {
            std::reverse(bytes, bytes + size);
        }

        Type value = 0;
        std::memcpy(&value, bytes, size);

        return value;
    }

    /** Encodes a value using variable-length encoding -- smaller values use fewer bytes. */
    template<typename Type> std::vector<unsigned char> encode_varint(const Type &value)
    {
        // LEB128: ceil(width/7) bytes max.
        constexpr size_t max_length = (sizeof(Type) * 8 + 6) / 7;

        std::vector<unsigned char> output;

        Type val = value;

        while (val >= 0x80)
        {
            output.push_back(static_cast<unsigned char>((val & 0x7f) | 0x80));
            val >>= 7;
        }

        output.push_back(static_cast<unsigned char>(val));

        if (output.size() > max_length)
        {
            throw std::range_error("varint encoding exceeds max length for type");
        }

        return output;
    }

    /** Decodes a varint from the byte vector at the given offset. Returns {value, bytes_consumed}. */
    template<typename Type>
    std::tuple<Type, size_t> decode_varint(const std::vector<unsigned char> &packed, const size_t offset = 0)
    {
        if (offset > packed.size())
        {
            throw std::range_error("offset exceeds sizes of vector");
        }

        auto counter = offset;

        size_t shift = 0;

        // Accumulate in uint64_t so the narrowing check below fires when
        // Type is narrower (e.g. decoding 256 into uint8_t).
        uint64_t temp_result = 0;

        unsigned char b;

        do
        {
            if (counter >= packed.size())
            {
                throw std::range_error("could not decode varint");
            }

            b = packed[counter++];

            if (shift >= sizeof(Type) * 8)
            {
                throw std::range_error("varint encoding exceeds type size");
            }

            // On the final byte, the payload bits must fit in what remains
            // of Type; otherwise bits silently land beyond the value range.
            const size_t remaining_bits = sizeof(Type) * 8 - shift;
            if (remaining_bits < 7)
            {
                const unsigned char mask = static_cast<unsigned char>((1u << remaining_bits) - 1u);
                if ((b & 0x7f) > mask)
                {
                    throw std::range_error("varint value out of range for type");
                }
            }

            const uint64_t value = uint64_t(b & 0x7f) << shift;

            temp_result += value;

            shift += 7;
        } while (b >= 0x80);

        const auto result = static_cast<Type>(temp_result);

        if (static_cast<uint64_t>(result) != temp_result)
        {
            throw std::range_error("value is out of range for type");
        }

        return {result, counter - offset};
    }

} // namespace Serialization

#endif
