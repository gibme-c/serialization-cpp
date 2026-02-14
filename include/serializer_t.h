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

#ifndef SERIALIZATION_SERIALIZER_T
#define SERIALIZATION_SERIALIZER_T

#include <serialization_helper.h>
#include <string_helper.h>
#include <uint256_t/uint128_t.h>
#include <uint256_t/uint256_t.h>

namespace Serialization
{
    /**
     * Writes typed values into a byte buffer. Call the typed methods (uint8, uint32, bytes, etc.)
     * in order, then grab the result with vector() or to_string(). The matching deserializer_t
     * reads them back in the same order.
     */
    struct serializer_t final
    {
        serializer_t() = default;

        serializer_t(const serializer_t &writer);

        serializer_t(std::initializer_list<unsigned char> input);

        explicit serializer_t(const std::vector<unsigned char> &input);

        unsigned char &operator[](size_t i);

        unsigned char operator[](size_t i) const;

        /** Writes a single boolean as one byte (0x00 or 0x01). */
        void boolean(bool value);

        /** Writes raw bytes from a pointer + length into the buffer. */
        void bytes(const void *data, size_t length);

        /** Writes the contents of a byte vector into the buffer. */
        void bytes(const std::vector<unsigned char> &value);

        /** Direct pointer to the underlying buffer data. */
        [[nodiscard]] const unsigned char *data() const;

        /** Decodes a hex string and writes the resulting bytes into the buffer. */
        void hex(const std::string &value);

        /** Writes a single Serializable object by calling its serialize() method. */
        template<typename Type> void pod(const Type &value)
        {
            const auto bytes = value.serialize();

            extend(bytes);
        }

        /** Writes a vector of Serializable objects, prefixed with a varint count. */
        template<typename Type> void pod(const std::vector<Type> &values)
        {
            varint(values.size());

            for (const auto &value : values)
            {
                pod<Type>(value);
            }
        }

        /** Writes a nested (2D) vector of Serializable objects, each level prefixed with a varint count. */
        template<typename Type> void pod(const std::vector<std::vector<Type>> &values)
        {
            varint(values.size());

            for (const auto &level1 : values)
            {
                varint(level1.size());

                for (const auto &value : level1)
                {
                    pod<Type>(value);
                }
            }
        }

        /** Clears the buffer so this writer can be reused. */
        void reset();

        /** Number of bytes written so far. Use this instead of sizeof(). */
        [[nodiscard]] size_t size() const;

        /** Returns the buffer contents as a hex string. */
        [[nodiscard]] std::string to_string() const;

        /** Writes a single byte. */
        void uint8(const unsigned char &value);

        /** Writes a 16-bit unsigned int. Pass big_endian=true to flip the byte order. */
        void uint16(const uint16_t &value, bool big_endian = false);

        /** Writes a 32-bit unsigned int. Pass big_endian=true to flip the byte order. */
        void uint32(const uint32_t &value, bool big_endian = false);

        /** Writes a 64-bit unsigned int. Pass big_endian=true to flip the byte order. */
        void uint64(const uint64_t &value, bool big_endian = false);

        /** Writes a 128-bit unsigned int. Pass big_endian=true to flip the byte order. */
        void uint128(const uint128_t &value, bool big_endian = false);

        /** Writes a 256-bit unsigned int. Pass big_endian=true to flip the byte order. */
        void uint256(const uint256_t &value, bool big_endian = false);

        /** Writes a value using variable-length integer encoding (smaller values = fewer bytes). */
        template<typename Type> void varint(const Type &value)
        {
            const auto bytes = encode_varint(value);

            extend(bytes);
        }

        /** Writes a vector of varints, prefixed with a varint count. */
        template<typename Type> void varint(const std::vector<Type> &values)
        {
            varint(values.size());

            for (const auto &value : values)
            {
                varint(value);
            }
        }

        /** Returns a copy of the underlying byte buffer. */
        [[nodiscard]] std::vector<unsigned char> vector() const;

      private:
        void extend(const std::vector<unsigned char> &vector);

        std::vector<unsigned char> buffer;
    };

} // namespace Serialization

#endif
