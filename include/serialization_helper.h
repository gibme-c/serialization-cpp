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
    /**
     * Packs the provided value into a byte vector
     * @tparam Type
     * @param value
     * @param big_endian
     * @return
     */
    template<typename Type> std::vector<unsigned char> pack(const Type &value, bool big_endian = false)
    {
        unsigned char bytes[64] = {0};

        std::memcpy(&bytes, &value, sizeof(Type));

        auto result = std::vector<unsigned char>(bytes, bytes + sizeof(Type));

        if (big_endian)
        {
            std::reverse(result.begin(), result.end());
        }

        return result;
    }

    /**
     * Unpacks a value from the provided byte vector starting at the given offset
     * @tparam Type
     * @param packed
     * @param offset
     * @param big_endian
     * @return
     */
    template<typename Type>
    Type unpack(const std::vector<unsigned char> &packed, size_t offset = 0, bool big_endian = false)
    {
        const auto size = sizeof(Type);

        if (offset + size > packed.size())
        {
            throw std::range_error("not enough data to complete request");
        }

        std::vector<unsigned char> bytes(size, 0);

        for (size_t i = offset, j = 0; i < offset + size; ++i, ++j)
        {
            bytes[j] = packed[i];
        }

        Type value = 0;

        if (big_endian)
        {
            std::reverse(bytes.begin(), bytes.end());
        }

        std::memcpy(&value, bytes.data(), bytes.size());

        return value;
    }

    /**
     * Encodes a value into a varint byte vector
     * @tparam Type
     * @param value
     * @return
     */
    template<typename Type> std::vector<unsigned char> encode_varint(const Type &value)
    {
        const auto max_length = sizeof(Type) + 2;

        std::vector<unsigned char> output;

        Type val = value;

        while (val >= 0x80)
        {
            if (output.size() == (max_length - 1))
            {
                throw std::range_error("value is out of range for type");
            }

            const auto val8 = static_cast<unsigned char>(val);

            output.push_back((static_cast<unsigned char>(val8) & 0x7f) | 0x80);

            val >>= 7;
        }

        const auto val8 = static_cast<unsigned char>(val);

        output.push_back(static_cast<unsigned char>(val8));

        return output;
    }

    /**
     * Decodes a value from the provided varint byte vector starting at the given offset
     * @tparam Type
     * @param packed
     * @param offset
     * @return
     */
    template<typename Type>
    std::tuple<Type, size_t> decode_varint(const std::vector<unsigned char> &packed, const size_t offset = 0)
    {
        if (offset > packed.size())
        {
            throw std::range_error("offset exceeds sizes of vector");
        }

        auto counter = offset;

        auto shift = 0;

        Type temp_result = 0;

        unsigned char b;

        do
        {
            if (counter >= packed.size())
            {
                throw std::range_error("could not decode varint");
            }

            b = packed[counter++];

            const auto value = (shift < 28) ? uint64_t(b & 0x7f) << shift : uint64_t(b & 0x7f) * (uint64_t(1) << shift);

            temp_result += Type(value);

            shift += 7;
        } while (b >= 0x80);

        const auto result = Type(temp_result);

        if (result != temp_result)
        {
            throw std::range_error("value is out of range for type");
        }

        return {result, counter - offset};
    }

} // namespace Serialization

#endif
