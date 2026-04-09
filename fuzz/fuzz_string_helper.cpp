// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Fuzz harness: the remaining string_helper.h surface — str_split,
// str_join, str_pad, str_trim — against random strings.

#include "fuzz_common.h"

#include <string_helper.h>

namespace
{
    std::string rand_any_string(fuzz::Rng &rng, size_t max_len)
    {
        const size_t len = static_cast<size_t>(rng() % (max_len + 1));
        std::string s;
        s.reserve(len);
        for (size_t i = 0; i < len; ++i)
        {
            s.push_back(static_cast<char>(rng() & 0xFFu));
        }
        return s;
    }
}  // namespace

int main()
{
    const auto cfg = fuzz::load_config();
    fuzz::print_header("fuzz_string_helper", cfg);

    fuzz::Rng rng(cfg.seed);
    fuzz::CrashContext ctx;
    ctx.seed = cfg.seed;

    for (uint64_t i = 0; i < cfg.iters; ++i)
    {
        ctx.iter = i;
        const std::string s = rand_any_string(rng, 256);
        ctx.input_string = (s.size() > 80) ? (s.substr(0, 80) + "...") : s;
        ctx.input_bytes.clear();

        // str_split
        ctx.source = "str_split";
        fuzz::guarded(ctx, [&]() {
            const char delim = static_cast<char>(rng() & 0xFFu);
            (void)Serialization::str_split(s, delim);
        });

        // str_join round-trip (split then join should reassemble)
        ctx.source = "str_split then str_join";
        fuzz::guarded(ctx, [&]() {
            const char delim = ',';
            const auto parts = Serialization::str_split(s, delim);
            const auto joined = Serialization::str_join(parts, delim);
            if (joined != s)
            {
                throw std::runtime_error("split/join not idempotent");
            }
        });

        // str_pad
        ctx.source = "str_pad";
        fuzz::guarded(ctx, [&]() {
            const size_t target = static_cast<size_t>(rng() % 320);
            const auto r = Serialization::str_pad(s, target);
            if (r.size() < s.size())
            {
                throw std::runtime_error("str_pad shrank input");
            }
            if (r.size() < target)
            {
                throw std::runtime_error("str_pad left input under target");
            }
        });

        // str_trim
        ctx.source = "str_trim";
        fuzz::guarded(ctx, [&]() {
            std::string copy = s;
            Serialization::str_trim(copy, (rng() & 1u) != 0);
        });
    }

    return fuzz::print_success("fuzz_string_helper", cfg);
}
