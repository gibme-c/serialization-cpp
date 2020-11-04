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

#ifndef SERIALIZATION_SERIALIZABLE_H
#define SERIALIZATION_SERIALIZABLE_H

#include <deserializer_t.h>
#include <json_helper.h>
#include <secure_erase.h>
#include <serializer_t.h>
#include <string_helper.h>
#include <vector>

/**
 * Serialization interface for inheritance
 */
struct Serializable
{
  public:
    /**
     * Deserializes the structure from a deserializer_t
     *
     * @param reader
     */
    virtual void deserialize(Serialization::deserializer_t &reader) = 0;

    /**
     * Deserializes the structure from a std::vector<unsigned char>
     *
     * @param data
     */
    virtual void deserialize(const std::vector<unsigned char> &data) = 0;

    /**
     * Loads the value from JSON
     *
     * @param j
     */
    virtual JSON_FROM_FUNC(fromJSON) = 0;

    /**
     * Loads the value from a JSON value in the specified key of the JSON object
     *
     * @param val
     * @param key
     */
    virtual JSON_FROM_KEY_FUNC(fromJSON) = 0;

    /**
     * Serializes the structure using the supplied serializer_t
     *
     * @param writer
     */
    virtual void serialize(Serialization::serializer_t &writer) const = 0;

    /**
     * Serializes the structure to a std::vector<unsigned char>
     *
     * @return
     */
    [[nodiscard]] virtual std::vector<unsigned char> serialize() const = 0;

    /**
     * Returns the size of the structure
     *
     * @return
     */
    [[nodiscard]] virtual size_t size() const = 0;

    /**
     * Serializes the structure to JSON
     *
     * @param writer
     */
    virtual JSON_TO_FUNC(toJSON) = 0;

    /**
     * Returns the structure as a string
     *
     * @return
     */
    [[nodiscard]] virtual std::string to_string() const = 0;
};

#endif
