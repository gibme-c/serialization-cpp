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
#include <serialization_secure_erase.h>
#include <serializer_t.h>
#include <string_helper.h>
#include <vector>

/**
 * Base interface for anything that can be serialized. Inherit from this and implement
 * all the pure virtual methods to plug your type into the serializer/deserializer system,
 * JSON conversion, and string output.
 */
struct Serializable
{
  public:
    virtual ~Serializable() = default;

    /** Reads this object's fields from a deserializer_t reader (cursor-based). */
    virtual void deserialize(Serialization::deserializer_t &reader) = 0;

    /** Reads this object's fields from a raw byte vector. */
    virtual void deserialize(const std::vector<unsigned char> &data) = 0;

    /** Populates this object from a JSON value. */
    virtual JSON_FROM_FUNC(fromJSON) = 0;

    /** Populates this object from a named key inside a JSON object. */
    virtual JSON_FROM_KEY_FUNC(fromJSON) = 0;

    /** Writes this object's fields into a serializer_t writer. */
    virtual void serialize(Serialization::serializer_t &writer) const = 0;

    /** Returns this object as a byte vector. */
    [[nodiscard]] virtual std::vector<unsigned char> serialize() const = 0;

    /** The serialized size in bytes. Use this instead of sizeof(). */
    [[nodiscard]] virtual size_t size() const = 0;

    /** Writes this object as JSON using the provided RapidJSON writer. */
    virtual JSON_TO_FUNC(toJSON) = 0;

    /** Human-readable string representation (typically hex). */
    [[nodiscard]] virtual std::string to_string() const = 0;
};

#endif
