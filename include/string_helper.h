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

#ifndef SERIALIZATION_STRING_HELPER_H
#define SERIALIZATION_STRING_HELPER_H

#include <cstdint>
#include <string>
#include <vector>

namespace Serialization
{
    /**
     * Converts a hexadecimal string to a vector of unsigned char
     *
     * @param text
     * @return
     */
    std::vector<unsigned char> from_hex(const std::string &text);

    /**
     * Converts a void pointer of the given length into a hexadecimal string
     *
     * @param data
     * @param length
     * @return
     */
    std::string to_hex(const void *data, size_t length);

    /**
     * Joins a vector of strings together using the specified character as the delimiter
     *
     * @param input
     * @param ch
     * @return
     */
    std::string str_join(const std::vector<std::string> &input, const char &ch = ' ');

    /**
     * Pads a string with blank spaces up to the specified length
     *
     * @param input
     * @param length
     * @return
     */
    std::string str_pad(std::string input, size_t length = 0);

    /**
     * Splits a string into a vector of strings using the specified character as a delimiter
     *
     * @param input
     * @param ch
     * @return
     */
    std::vector<std::string> str_split(const std::string &input, const char &ch = ' ');

    /**
     * Trims any whitespace from both the start and end of the given string
     *
     * @param str
     * @param to_lowercase
     */
    void str_trim(std::string &str, bool to_lowercase = false);

} // namespace Serialization

#endif
