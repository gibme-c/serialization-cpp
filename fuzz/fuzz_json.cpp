// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Fuzz harness: JSON parsing via RapidJSON plus the typed getters. The
// parser should cleanly reject malformed input (throwing invalid_argument)
// and the typed getters should reject type mismatches the same way.

#include "fuzz_common.h"

#include <json_helper.h>
#include <rapidjson/document.h>

namespace
{
    const std::vector<std::string> k_seeds = {
        "",
        "{}",
        "[]",
        "null",
        "true",
        "false",
        "1",
        "1.5",
        "\"hello\"",
        "{\"a\":1,\"b\":\"x\",\"c\":[1,2,3]}",
        "{\"a\":{\"b\":{\"c\":[true,false,null]}}}",
        "{\"a\"",                  // truncated
        "{1:2}",                   // invalid key
        "{\"a\":",                 // truncated value
        "[1,2,3,",                 // truncated array
        "{\"a\":true\"b\":false}", // missing comma
    };

    std::string mutate(fuzz::Rng &rng, std::string s)
    {
        if (s.empty())
        {
            return s;
        }
        const int op = static_cast<int>(rng() % 4);
        const size_t i = static_cast<size_t>(rng() % s.size());
        switch (op)
        {
        case 0:
            s[i] ^= static_cast<char>((rng() & 0xFFu) | 0x01u);
            break;
        case 1:
            s.erase(i, 1);
            break;
        case 2:
            s.insert(i, 1, static_cast<char>(rng() & 0xFFu));
            break;
        case 3:
            s.insert(i, 1, s[i]);
            break;
        }
        return s;
    }

    void exercise(fuzz::CrashContext &ctx, const std::string &s)
    {
        ctx.input_string = s.size() > 200 ? s.substr(0, 200) + "..." : s;
        ctx.input_bytes.clear();

        rapidjson::Document doc;
        if (doc.Parse(s.c_str()).HasParseError())
        {
            return;  // expected — malformed JSON
        }

        // Each getter call lands in its own guarded frame so one thrown
        // type mismatch does not abort the remaining calls, and the
        // noinline catch frame stays out of this function.
        ctx.source = "get_json_bool";
        fuzz::guarded(ctx, [&]() { (void)get_json_bool(doc); });
        ctx.source = "get_json_int64";
        fuzz::guarded(ctx, [&]() { (void)get_json_int64_t(doc); });
        ctx.source = "get_json_uint64";
        fuzz::guarded(ctx, [&]() { (void)get_json_uint64_t(doc); });
        ctx.source = "get_json_uint32";
        fuzz::guarded(ctx, [&]() { (void)get_json_uint32_t(doc); });
        ctx.source = "get_json_double";
        fuzz::guarded(ctx, [&]() { (void)get_json_double(doc); });
        ctx.source = "get_json_string";
        fuzz::guarded(ctx, [&]() { (void)get_json_string(doc); });
        if (doc.IsObject())
        {
            ctx.source = "has_member";
            fuzz::guarded(ctx, [&]() { (void)has_member(doc, std::string("a")); });
            ctx.source = "get_json_value";
            fuzz::guarded(ctx, [&]() { (void)get_json_value(doc, std::string("a")); });
        }
        if (doc.IsArray())
        {
            ctx.source = "get_json_array";
            fuzz::guarded(ctx, [&]() { (void)get_json_array(doc); });
        }
    }
}  // namespace

int main()
{
    const auto cfg = fuzz::load_config();
    fuzz::print_header("fuzz_json", cfg);

    fuzz::Rng rng(cfg.seed);
    fuzz::CrashContext ctx;
    ctx.seed = cfg.seed;

    for (const auto &s : k_seeds)
    {
        exercise(ctx, s);
        ++ctx.iter;
    }

    for (uint64_t i = 0; i < cfg.iters; ++i)
    {
        ctx.iter = i;
        std::string s = k_seeds[rng() % k_seeds.size()];
        const int muts = static_cast<int>(rng() % 4) + 1;
        for (int j = 0; j < muts; ++j)
        {
            s = mutate(rng, s);
            if (s.size() > 1024)
            {
                break;
            }
        }
        exercise(ctx, s);
    }

    return fuzz::print_success("fuzz_json", cfg);
}
