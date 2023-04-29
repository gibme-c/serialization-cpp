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

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string_helper.h>

#pragma warning(disable : 4244)

static const char hex_chars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

static const unsigned char hex_values[256] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

static inline unsigned char char_2_unsigned_char(char character)
{
    unsigned char value = hex_values[static_cast<unsigned char>(character)];

    if (value > 0x0f)
    {
        throw std::runtime_error("invalid hexadecimal character");
    }

    return value;
}

namespace Serialization
{
    std::string to_hex(const void *data, size_t length)
    {
        std::string text;

        for (uint64_t i = 0; i < length; ++i)
        {
            text += hex_chars[static_cast<const unsigned char *>(data)[i] >> 4];

            text += hex_chars[static_cast<const unsigned char *>(data)[i] & 15];
        }

        return text;
    }

    std::vector<unsigned char> from_hex(const std::string &text)
    {
        std::vector<unsigned char> result;

        if ((text.size() & 1) != 0)
        {
            throw std::runtime_error("from_hex: invalid string size");
        }

        const auto text_size = text.size() >> 1;

        result.resize(text_size);

        for (uint64_t i = 0; i < text_size; ++i)
        {
            result[i] = char_2_unsigned_char(text[i << 1]) << 4 | char_2_unsigned_char(text[(i << 1) + 1]);
        }

        return result;
    }

    std::string str_join(const std::vector<std::string> &input, const char &ch)
    {
        std::string result;

        for (const auto &part : input)
        {
            result += part + ch;
        }

        // trim the trailing character that we appended
        result = result.substr(0, result.size() - 1);

        return result;
    }

    std::string str_pad(std::string input, size_t length)
    {
        if (input.length() < length)
        {
            const auto delta = length - input.length();

            for (size_t i = 0; i < delta; ++i)
            {
                input += " ";
            }
        }

        return input;
    }

    std::vector<std::string> str_split(const std::string &input, const char &ch)
    {
        auto pos = input.find(ch);

        uint64_t initial_pos = 0;

        std::vector<std::string> result;

        while (pos != std::string::npos)
        {
            result.push_back(input.substr(initial_pos, pos - initial_pos));

            initial_pos = pos + 1;

            pos = input.find(ch, initial_pos);
        }

        result.push_back(input.substr(initial_pos, std::min(pos, input.size()) - initial_pos + 1));

        return result;
    }

    void str_trim(std::string &str, bool to_lowercase)
    {
        const auto whitespace = "\t\n\r\f\v";

        str.erase(str.find_last_not_of(whitespace) + 1);

        str.erase(0, str.find_first_not_of(whitespace));

        if (to_lowercase)
        {
            std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
        }
    }

} // namespace Serialization
