// Copyright (c) 2020-2023, Brandon Lehmann
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

#include <iostream>
#include <serialization.h>

typedef SerializablePod<32> value_t;

const auto input = std::string("974506601a60dc465e6e9acddb563889e63471849ec4198656550354b8541fcb");

int main()
{
    auto value = value_t(input);

    std::cout << "Expected: " << input << std::endl;
    std::cout << "Received: " << value << std::endl;

    if (input != value.to_string())
    {
        std::cout << "std::string MISMATCH!!" << std::endl;

        exit(1);
    }

    {
        auto writer = Serialization::serializer_t();

        value.serialize(writer);

        writer.pod(value);

        auto reader = Serialization::deserializer_t(writer);

        auto read = reader.pod<value_t>();

        std::cout << "Writer:   " << read << std::endl;

        if (read.to_string() != input)
        {
            std::cout << "serializer_t MISMATCH!!" << std::endl;

            exit(1);
        }
    }

    {
        JSON_INIT_BUFFER(buffer, writer);

        value.toJSON(writer);

        std::cout << "JSON:    " << buffer.GetString() << std::endl;

        JSON_DUMP_BUFFER(buffer, output);

        JSON_PARSE(output);

        auto value2 = value_t();

        value2.fromJSON(body);

        if (value2.to_string() != input)
        {
            std::cout << "JSON MISMATCH!!" << std::endl;

            exit(1);
        }
    }
}
