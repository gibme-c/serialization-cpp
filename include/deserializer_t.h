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
    /**
     * Reads typed values from a byte buffer using an internal cursor. Each read advances
     * the cursor unless peek=true. Call the typed methods in the same order they were
     * written by serializer_t to get your data back.
     */
    struct deserializer_t final
    {
        explicit deserializer_t(const serializer_t &writer);

        deserializer_t(std::initializer_list<unsigned char> input);

        explicit deserializer_t(const std::vector<unsigned char> &input);

        /** Accepts a hex string, decodes it to bytes, then reads from that. */
        explicit deserializer_t(const std::string &input);

        /** Reads a single boolean (one byte). */
        bool boolean(bool peek = false);

        /** Reads the next `count` raw bytes from the buffer. */
        std::vector<unsigned char> bytes(size_t count = 1, bool peek = false);

        /** Drops already-read bytes from memory to free up space. */
        void compact();

        /** Direct pointer to the underlying buffer data. */
        [[nodiscard]] const unsigned char *data() const;

        /** Reads `length` bytes and returns them as a hex string. */
        std::string hex(size_t length = 1, bool peek = false);

        /** Reads a single Serializable object by size, then calls its deserialize(). */
        template<typename Type> Type pod(bool peek = false)
        {
            Type result;

            const auto data = bytes(result.size(), peek);

            result.deserialize(data);

            return result;
        }

        /** Reads a varint-prefixed vector of Serializable objects. */
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

        /** Reads a varint-prefixed nested (2D) vector of Serializable objects. */
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

        /** Moves the read cursor back to the given position (default: the beginning). */
        void reset(size_t position = 0);

        /** Total size of the buffer in bytes. Use this instead of sizeof(). */
        [[nodiscard]] size_t size() const;

        /** Jumps the cursor forward by `count` bytes without returning anything. */
        void skip(size_t count = 1);

        /** Returns the entire buffer as a hex string. */
        [[nodiscard]] std::string to_string() const;

        /** Reads a single byte. */
        unsigned char uint8(bool peek = false);

        /** Reads a 16-bit unsigned int. Pass big_endian=true to flip the byte order. */
        uint16_t uint16(bool peek = false, bool big_endian = false);

        /** Reads a 32-bit unsigned int. Pass big_endian=true to flip the byte order. */
        uint32_t uint32(bool peek = false, bool big_endian = false);

        /** Reads a 64-bit unsigned int. Pass big_endian=true to flip the byte order. */
        uint64_t uint64(bool peek = false, bool big_endian = false);

        /** Reads a 128-bit unsigned int. Pass big_endian=true to flip the byte order. */
        uint128_t uint128(bool peek = false, bool big_endian = false);

        /** Reads a 256-bit unsigned int. Pass big_endian=true to flip the byte order. */
        uint256_t uint256(bool peek = false, bool big_endian = false);

        /** Reads a variable-length encoded integer. */
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

        /** Reads a varint-prefixed vector of varints. */
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

        /** How many bytes are left to read. */
        [[nodiscard]] size_t unread_bytes() const;

        /** Returns a copy of the bytes that haven't been read yet. */
        [[nodiscard]] std::vector<unsigned char> unread_data() const;

      private:
        std::vector<unsigned char> buffer;

        size_t offset = 0;
    };

} // namespace Serialization

#endif
