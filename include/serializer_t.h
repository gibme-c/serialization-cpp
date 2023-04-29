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
    struct serializer_t final
    {
        serializer_t() = default;

        serializer_t(const serializer_t &writer);

        serializer_t(std::initializer_list<unsigned char> input);

        explicit serializer_t(const std::vector<unsigned char> &input);

        unsigned char &operator[](int i);

        unsigned char operator[](int i) const;

        /**
         * Encodes the value into the vector
         * @param value
         */
        void boolean(bool value);

        /**
         * Encodes the value into the vector
         * @param data
         * @param length
         */
        void bytes(const void *data, size_t length);

        /**
         * Encodes the value into the vector
         * @param value
         */
        void bytes(const std::vector<unsigned char> &value);

        /**
         * Returns a pointer to the underlying structure data
         * @return
         */
        [[nodiscard]] const unsigned char *data() const;

        /**
         * Encodes the value into the vector
         * @param value
         */
        void hex(const std::string &value);

        /**
         * Encodes the value into the vector
         *
         * @param value
         */
        template<typename Type> void pod(const Type &value)
        {
            const auto bytes = value.serialize();

            extend(bytes);
        }

        /**
         * Encodes the vector of values into the vector
         *
         * @param values
         */
        template<typename Type> void pod(const std::vector<Type> &values)
        {
            varint(values.size());

            for (const auto &value : values)
            {
                pod<Type>(value);
            }
        }

        /**
         * Encodes the nested vector of values into the vector
         *
         * @param values
         */
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

        /**
         * Clears the underlying byte vector
         */
        void reset();

        /**
         * Use this method instead of sizeof() to get the resulting
         * size of the structure in bytes
         * @return
         */
        [[nodiscard]] size_t size() const;

        /**
         * Returns the hex encoding of the underlying byte vector
         * @return
         */
        [[nodiscard]] std::string to_string() const;

        /**
         * Encodes the value into the vector
         * @param value
         */
        void uint8(const unsigned char &value);

        /**
         * Encodes the value into the vector
         * @param value
         * @param big_endian
         */
        void uint16(const uint16_t &value, bool big_endian = false);

        /**
         * Encodes the value into the vector
         * @param value
         * @param big_endian
         */
        void uint32(const uint32_t &value, bool big_endian = false);

        /**
         * Encodes the value into the vector
         * @param value
         * @param big_endian
         */
        void uint64(const uint64_t &value, bool big_endian = false);

        /**
         * Encodes the value into the vector
         * @param value
         * @param big_endian
         */
        void uint128(const uint128_t &value, bool big_endian = false);

        /**
         * Encodes the value into the vector
         * @param value
         * @param big_endian
         */
        void uint256(const uint256_t &value, bool big_endian = false);

        /**
         * Encodes the value into the vector as a varint
         * @tparam Type
         * @param value
         */
        template<typename Type> void varint(const Type &value)
        {
            const auto bytes = encode_varint(value);

            extend(bytes);
        }

        /**
         * Encodes the vector of values into the vector as a varint
         * @tparam Type
         * @param values
         */
        template<typename Type> void varint(const std::vector<Type> &values)
        {
            varint(values.size());

            for (const auto &value : values)
            {
                varint(value);
            }
        }

        /**
         * Returns a copy of the underlying vector
         * @return
         */
        [[nodiscard]] std::vector<unsigned char> vector() const;

      private:
        void extend(const std::vector<unsigned char> &vector);

        std::vector<unsigned char> buffer;
    };

} // namespace Serialization

#endif
