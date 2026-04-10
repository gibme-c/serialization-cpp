// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Unit tests for string_helper.h — from_hex, to_hex, str_split, str_join,
// str_pad, str_trim. Covers every error path plus boundary sizes and
// idempotence invariants.

#ifndef SERIALIZATION_UNIT_STRING_HELPER_INL
#define SERIALIZATION_UNIT_STRING_HELPER_INL

#include <string>
#include <string_helper.h>
#include <vector>

// ---------- from_hex ----------
static void test_from_hex_valid_lowercase()
{
    const auto r = Serialization::from_hex("deadbeef");
    const std::vector<unsigned char> expected = {0xDE, 0xAD, 0xBE, 0xEF};
    ASSERT_BYTES_EQ(r, expected);
}

static void test_from_hex_valid_uppercase()
{
    const auto r = Serialization::from_hex("DEADBEEF");
    const std::vector<unsigned char> expected = {0xDE, 0xAD, 0xBE, 0xEF};
    ASSERT_BYTES_EQ(r, expected);
}

static void test_from_hex_mixed_case()
{
    const auto r = Serialization::from_hex("DeAdBeEf");
    const std::vector<unsigned char> expected = {0xDE, 0xAD, 0xBE, 0xEF};
    ASSERT_BYTES_EQ(r, expected);
}

static void test_from_hex_all_digits()
{
    const auto r = Serialization::from_hex("0123456789abcdef");
    const std::vector<unsigned char> expected = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    ASSERT_BYTES_EQ(r, expected);
}

static void test_from_hex_empty_string_returns_empty()
{
    const auto r = Serialization::from_hex("");
    ASSERT_EQ(r.size(), static_cast<size_t>(0));
}

static void test_from_hex_odd_length_throws_length_error()
{
    ASSERT_THROWS_TYPE(Serialization::from_hex("abc"), std::length_error);
}

static void test_from_hex_odd_length_single_char_throws()
{
    ASSERT_THROWS_TYPE(Serialization::from_hex("a"), std::length_error);
}

static void test_from_hex_invalid_char_throws_invalid_argument()
{
    ASSERT_THROWS_TYPE(Serialization::from_hex("zz"), std::invalid_argument);
}

static void test_from_hex_invalid_char_mid_string_throws()
{
    ASSERT_THROWS_TYPE(Serialization::from_hex("deadbXef"), std::invalid_argument);
}

static void test_from_hex_space_char_throws()
{
    // Whitespace is not a hex digit. Use an even-length string (4 chars)
    // so the odd-length guard doesn't fire first — "de  " has two spaces
    // after the valid byte, which must be rejected as invalid hex chars.
    ASSERT_THROWS_TYPE(Serialization::from_hex("de  "), std::invalid_argument);
}

static void test_from_hex_null_byte_throws()
{
    std::string s = "de\x00ad";
    s.resize(4);
    s[2] = '\0';
    s[3] = '\0';
    ASSERT_THROWS_TYPE(Serialization::from_hex(s), std::invalid_argument);
}

static void test_from_hex_high_ascii_throws()
{
    // Byte > 0x7f must be rejected through the hex_values lookup table.
    std::string s;
    s.push_back(static_cast<char>(0xFE));
    s.push_back(static_cast<char>(0xFE));
    ASSERT_THROWS_TYPE(Serialization::from_hex(s), std::invalid_argument);
}

static void test_from_hex_idempotent_via_to_hex()
{
    const std::vector<unsigned char> bytes = {0x00, 0x01, 0x7F, 0x80, 0xFF};
    const auto hex = Serialization::to_hex(bytes.data(), bytes.size());
    const auto r = Serialization::from_hex(hex);
    ASSERT_BYTES_EQ(r, bytes);
}

// ---------- to_hex ----------
static void test_to_hex_empty()
{
    const auto r = Serialization::to_hex(nullptr, 0);
    ASSERT_EQ(r, std::string(""));
}

static void test_to_hex_single_byte()
{
    const unsigned char b = 0x5A;
    const auto r = Serialization::to_hex(&b, 1);
    ASSERT_EQ(r, std::string("5a"));
}

static void test_to_hex_produces_lowercase()
{
    const std::vector<unsigned char> bytes = {0xDE, 0xAD, 0xBE, 0xEF};
    const auto r = Serialization::to_hex(bytes.data(), bytes.size());
    ASSERT_EQ(r, std::string("deadbeef"));
}

static void test_to_hex_length_is_twice_input()
{
    std::vector<unsigned char> bytes(37, 0xAB);
    const auto r = Serialization::to_hex(bytes.data(), bytes.size());
    ASSERT_EQ(r.size(), bytes.size() * 2);
}

// ---------- str_split ----------
static void test_str_split_basic()
{
    const auto r = Serialization::str_split("one two three", ' ');
    ASSERT_EQ(r.size(), static_cast<size_t>(3));
    ASSERT_EQ(r[0], std::string("one"));
    ASSERT_EQ(r[1], std::string("two"));
    ASSERT_EQ(r[2], std::string("three"));
}

static void test_str_split_single_token_no_delim()
{
    const auto r = Serialization::str_split("alone", ' ');
    ASSERT_EQ(r.size(), static_cast<size_t>(1));
    ASSERT_EQ(r[0], std::string("alone"));
}

static void test_str_split_trailing_delim()
{
    const auto r = Serialization::str_split("a,b,", ',');
    ASSERT_EQ(r.size(), static_cast<size_t>(3));
    ASSERT_EQ(r[0], std::string("a"));
    ASSERT_EQ(r[1], std::string("b"));
    ASSERT_EQ(r[2], std::string(""));
}

static void test_str_split_leading_delim()
{
    const auto r = Serialization::str_split(",a,b", ',');
    ASSERT_EQ(r.size(), static_cast<size_t>(3));
    ASSERT_EQ(r[0], std::string(""));
    ASSERT_EQ(r[1], std::string("a"));
    ASSERT_EQ(r[2], std::string("b"));
}

static void test_str_split_consecutive_delims()
{
    const auto r = Serialization::str_split("a,,b", ',');
    ASSERT_EQ(r.size(), static_cast<size_t>(3));
    ASSERT_EQ(r[0], std::string("a"));
    ASSERT_EQ(r[1], std::string(""));
    ASSERT_EQ(r[2], std::string("b"));
}

// ---------- str_join ----------
static void test_str_join_basic()
{
    const std::vector<std::string> parts = {"a", "b", "c"};
    const auto r = Serialization::str_join(parts, ',');
    ASSERT_EQ(r, std::string("a,b,c"));
}

static void test_str_join_empty_vector()
{
    const std::vector<std::string> parts;
    const auto r = Serialization::str_join(parts, ',');
    ASSERT_EQ(r, std::string(""));
}

static void test_str_join_single_element()
{
    const std::vector<std::string> parts = {"only"};
    const auto r = Serialization::str_join(parts, ',');
    ASSERT_EQ(r, std::string("only"));
}

static void test_str_join_empty_strings_keep_delims()
{
    const std::vector<std::string> parts = {"", "", ""};
    const auto r = Serialization::str_join(parts, '-');
    ASSERT_EQ(r, std::string("--"));
}

static void test_str_join_default_delim_space()
{
    const std::vector<std::string> parts = {"a", "b", "c"};
    const auto r = Serialization::str_join(parts);
    ASSERT_EQ(r, std::string("a b c"));
}

static void test_str_split_join_roundtrip_no_empty_parts()
{
    const std::string original = "alpha|beta|gamma|delta";
    const auto split = Serialization::str_split(original, '|');
    const auto joined = Serialization::str_join(split, '|');
    ASSERT_EQ(joined, original);
}

// ---------- str_pad ----------
static void test_str_pad_shorter_input()
{
    const auto r = Serialization::str_pad("ab", 5);
    ASSERT_EQ(r, std::string("ab   "));
}

static void test_str_pad_equal_length()
{
    const auto r = Serialization::str_pad("abcde", 5);
    ASSERT_EQ(r, std::string("abcde"));
}

static void test_str_pad_longer_input()
{
    const auto r = Serialization::str_pad("abcdefg", 5);
    ASSERT_EQ(r, std::string("abcdefg"));
}

static void test_str_pad_zero_length()
{
    const auto r = Serialization::str_pad("hello", 0);
    ASSERT_EQ(r, std::string("hello"));
}

static void test_str_pad_empty_input()
{
    const auto r = Serialization::str_pad("", 4);
    ASSERT_EQ(r, std::string("    "));
}

// ---------- str_trim ----------
static void test_str_trim_basic()
{
    std::string s = "\t  hello  \n";
    Serialization::str_trim(s);
    ASSERT_EQ(s, std::string("hello"));
}

static void test_str_trim_spaces_only()
{
    std::string s = "     ";
    Serialization::str_trim(s);
    ASSERT_EQ(s, std::string(""));
}

static void test_str_trim_only_tabs_and_newlines()
{
    std::string s = "\t\n\rhello world\r\n\t";
    Serialization::str_trim(s);
    ASSERT_EQ(s, std::string("hello world"));
}

static void test_str_trim_formfeed_vtab()
{
    std::string s = "\f\vhello\v\f";
    Serialization::str_trim(s);
    ASSERT_EQ(s, std::string("hello"));
}

static void test_str_trim_all_whitespace()
{
    std::string s = "\t\n\r\f\v";
    Serialization::str_trim(s);
    ASSERT_EQ(s, std::string(""));
}

static void test_str_trim_empty()
{
    std::string s;
    Serialization::str_trim(s);
    ASSERT_EQ(s, std::string(""));
}

static void test_str_trim_no_whitespace()
{
    std::string s = "abc";
    Serialization::str_trim(s);
    ASSERT_EQ(s, std::string("abc"));
}

static void test_str_trim_lowercase_path()
{
    std::string s = "\tHELLO World\n";
    Serialization::str_trim(s, /*to_lowercase=*/true);
    ASSERT_EQ(s, std::string("hello world"));
}

static void test_str_trim_lowercase_only_letters_changed()
{
    std::string s = "ABC123!@#";
    Serialization::str_trim(s, true);
    ASSERT_EQ(s, std::string("abc123!@#"));
}

static void test_str_trim_preserves_internal_tabs_no()
{
    // Internal tabs are NOT touched; only leading/trailing ones are removed.
    std::string s = "\tleading\tinner\ttrailing\t";
    Serialization::str_trim(s);
    ASSERT_EQ(s, std::string("leading\tinner\ttrailing"));
}

#endif
