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

#ifndef SERIALIZATION_DESERIALIZER_T
#define SERIALIZATION_DESERIALIZER_T

#include <serializer_t.h>
#include <string_helper.h>

namespace Serialization
{
    struct deserializer_t final
    {
        explicit deserializer_t(const serializer_t &writer);

        deserializer_t(std::initializer_list<unsigned char> input);

        explicit deserializer_t(const std::vector<unsigned char> &input);

        explicit deserializer_t(const std::string &input);

        /**
         * Decodes a value from the byte vector
         * @param peek
         * @return
         */
        bool boolean(bool peek = false);

        /**
         * Returns a byte vector of the given length from the byte vector
         * @param count
         * @param peek
         * @return
         */
        std::vector<unsigned char> bytes(size_t count = 1, bool peek = false);

        /**
         * Trims read dead from the byte vector thus reducing its memory footprint
         */
        void compact();

        /**
         * Returns a pointer to the underlying structure data
         * @return
         */
        [[nodiscard]] const unsigned char *data() const;

        /**
         * Decodes a hex encoded string of the given length from the byte vector
         * @param length
         * @param peek
         * @return
         */
        std::string hex(size_t length = 1, bool peek = false);

        /**
         * Decodes a value from the byte vector
         *
         * @tparam Type
         * @param peek
         * @return
         */
        template<typename Type> Type pod(bool peek = false)
        {
            Type result;

            const auto data = bytes(result.size(), peek);

            result.deserialize(data);

            return result;
        }

        /**
         * Decodes a vector of values from the byte vector
         *
         * @tparam Type
         * @param peek
         * @return
         */
        template<typename Type> std::vector<Type> podV(bool peek = false)
        {
            const auto start = offset;

            const auto count = varint<uint64_t>();

            std::vector<Type> result;

            const Type temp;

            for (uint64_t i = 0; i < count; ++i)
            {
                result.push_back(pod<Type>());
            }

            if (peek)
            {
                reset(start);
            }

            return result;
        }

        /**
         * Decodes a nested vector of values from the byte vector
         *
         * @tparam Type
         * @param peek
         * @return
         */
        template<typename Type> std::vector<std::vector<Type>> podVV(bool peek = false)
        {
            const auto start = offset;

            const auto level1_count = varint<uint64_t>();

            std::vector<std::vector<Type>> result;

            for (uint64_t i = 0; i < level1_count; ++i)
            {
                const auto count = varint<uint64_t>();

                std::vector<Type> level2;

                for (uint64_t j = 0; j < count; ++j)
                {
                    level2.push_back(pod<Type>());
                }

                result.push_back(level2);
            }

            if (peek)
            {
                reset(start);
            }

            return result;
        }

        /**
         * Resets the reader to the given position (default 0)
         * @param position
         */
        void reset(size_t position = 0);

        /**
         * Use this method instead of sizeof() to get the resulting
         * size of the structure in bytes
         * @return
         */
        [[nodiscard]] size_t size() const;

        /**
         * Skips the next specified bytes while reading
         * @param count
         */
        void skip(size_t count = 1);

        /**
         * Returns the hex encoding of the underlying byte vector
         * @return
         */
        [[nodiscard]] std::string to_string() const;

        /**
         * Decodes a value from the byte vector
         * @param peek
         * @return
         */
        unsigned char uint8(bool peek = false);

        /**
         * Decodes a value from the byte vector
         * @param peek
         * @param big_endian
         * @return
         */
        uint16_t uint16(bool peek = false, bool big_endian = false);

        /**
         * Decodes a value from the byte vector
         * @param peek
         * @param big_endian
         * @return
         */
        uint32_t uint32(bool peek = false, bool big_endian = false);

        /**
         * Decodes a value from the byte vector
         * @param peek
         * @param big_endian
         * @return
         */
        uint64_t uint64(bool peek = false, bool big_endian = false);

        /**
         * Decodes a value from the byte vector
         * @param peek
         * @param big_endian
         * @return
         */
        uint128_t uint128(bool peek = false, bool big_endian = false);

        /**
         * Decodes a value from the byte vector
         * @param peek
         * @param big_endian
         * @return
         */
        uint256_t uint256(bool peek = false, bool big_endian = false);

        /**
         * Decodes a value from the byte vector
         * @tparam Type
         * @param peek
         * @return
         */
        template<typename Type> Type varint(bool peek = false)
        {
            const auto start = offset;

            const auto [result, length] = decode_varint<Type>(buffer, start);

            if (!peek)
            {
                offset += length;
            }

            return result;
        }

        /**
         * Decodes a vector of values from the byte vector
         * @tparam Type
         * @param peek
         * @return
         */
        template<typename Type> std::vector<Type> varintV(bool peek = false)
        {
            const auto start = offset;

            const auto count = varint<uint64_t>();

            std::vector<Type> result;

            for (uint64_t i = 0; i < count; ++i)
            {
                const auto temp = varint<Type>();

                result.push_back(temp);
            }

            if (peek)
            {
                reset(start);
            }

            return result;
        }

        /**
         * Returns the remaining number of bytes that have not been read from the byte vector
         * @return
         */
        [[nodiscard]] size_t unread_bytes() const;

        /**
         * Returns a byte vector copy of the remaining number of bytes that have not been read from the byte vector
         * @return
         */
        [[nodiscard]] std::vector<unsigned char> unread_data() const;

      private:
        std::vector<unsigned char> buffer;

        size_t offset = 0;
    };

} // namespace Serialization

#endif
