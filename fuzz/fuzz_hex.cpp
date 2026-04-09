// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Fuzz harness: Serialization::from_hex and to_hex. Exercises the hex codec
// with random strings (valid/invalid/odd-length/empty/huge), plus the
// from_hex(to_hex(x)) idempotence invariant on random byte vectors.

#include "fuzz_common.h"

#include <string_helper.h>

namespace
{
    const std::vector<std::string> k_string_seeds = {
        "",
        "0",
        "00",
        "de",
        "dead",
        "DeAdBeEf",
        "abc",       // odd length
        "zz",        // invalid char
        "de ad",     // space injection
        "0\xff\x00",
        "0123456789abcdef",
        "974506601a60dc465e6e9acddb563889e63471849ec4198656550354b8541fcb",
    };

    void fuzz_from_hex(fuzz::CrashContext &ctx, const std::string &s)
    {
        ctx.source = "from_hex";
        ctx.input_string = s;
        ctx.input_bytes.clear();
        fuzz::guarded(ctx, [&]() { (void)Serialization::from_hex(s); });
    }

    void fuzz_to_hex_from_hex_idempotent(fuzz::CrashContext &ctx,
                                         const std::vector<unsigned char> &bytes)
    {
        ctx.source = "to_hex then from_hex idempotent";
        ctx.input_string.clear();
        ctx.input_bytes = bytes;
        fuzz::guarded(ctx, [&]() {
            const auto hex = Serialization::to_hex(bytes.data(), bytes.size());
            const auto back = Serialization::from_hex(hex);
            if (back != bytes)
            {
                throw std::runtime_error("to_hex/from_hex idempotence broken");
            }
        });
    }
}  // namespace

int main()
{
    const auto cfg = fuzz::load_config();
    fuzz::print_header("fuzz_hex", cfg);

    fuzz::Rng rng(cfg.seed);
    fuzz::CrashContext ctx;
    ctx.seed = cfg.seed;

    // Corpus phase
    for (const auto &s : k_string_seeds)
    {
        fuzz_from_hex(ctx, s);
        ++ctx.iter;
    }

    // Randomized phase
    for (uint64_t i = 0; i < cfg.iters; ++i)
    {
        ctx.iter = i;
        const auto s = fuzz::rand_hexish_string(rng, 128);
        fuzz_from_hex(ctx, s);

        // Idempotence check on a random byte vector.
        const auto bytes = fuzz::rand_bytes_short_biased(rng, 256);
        fuzz_to_hex_from_hex_idempotent(ctx, bytes);
    }

    // One-off: a single huge string to exercise allocation paths.
    {
        ctx.iter = cfg.iters;
        std::string huge(64 * 1024, '\0');
        for (auto &c : huge)
        {
            c = "0123456789abcdef"[rng() % 16];
        }
        fuzz_from_hex(ctx, huge);
    }

    return fuzz::print_success("fuzz_hex", cfg);
}
