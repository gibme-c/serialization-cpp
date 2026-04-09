// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Fuzz harness: Serialization::deserializer_t(const std::string &) + reads.
// This path mirrors how callers ingest untrusted hex strings and is the
// single highest-value surface in the library. The constructor feeds the
// string to from_hex() then parses bytes through every primitive reader.

#include "fuzz_common.h"

#include <deserializer_t.h>
#include <optional>
#include <serializable_pod.h>

namespace
{
    using Pod = SerializablePod<32>;
}

int main()
{
    const auto cfg = fuzz::load_config();
    fuzz::print_header("fuzz_deserializer_hexctor", cfg);

    fuzz::Rng rng(cfg.seed);
    fuzz::CrashContext ctx;
    ctx.seed = cfg.seed;

    for (uint64_t i = 0; i < cfg.iters; ++i)
    {
        ctx.iter = i;
        std::string s = fuzz::rand_hexish_string(rng, 256);
        ctx.input_string = s;
        ctx.input_bytes.clear();

        // Construct the reader under guarded(); if the hex-string ctor
        // throws an expected error, skip the rest of this iteration.
        std::optional<Serialization::deserializer_t> r_opt;
        ctx.source = "deserializer_t(string)";
        fuzz::guarded(ctx, [&]() { r_opt.emplace(s); });
        if (!r_opt)
        {
            continue;
        }
        auto &r = *r_opt;

        const int ops = static_cast<int>(rng() % 8) + 1;
        for (int j = 0; j < ops; ++j)
        {
            const int op = static_cast<int>(rng() % 9);
            switch (op)
            {
            case 0:
                ctx.source = "uint32";
                fuzz::guarded(ctx, [&]() { (void)r.uint32(); });
                break;
            case 1:
                ctx.source = "uint64";
                fuzz::guarded(ctx, [&]() { (void)r.uint64(); });
                break;
            case 2:
                ctx.source = "uint16";
                fuzz::guarded(ctx, [&]() { (void)r.uint16(); });
                break;
            case 3:
            {
                const size_t n = static_cast<size_t>(rng() % 32);
                ctx.source = "bytes";
                fuzz::guarded(ctx, [&]() { (void)r.bytes(n); });
                break;
            }
            case 4:
                ctx.source = "varint<u64>";
                fuzz::guarded(ctx, [&]() { (void)r.varint<uint64_t>(); });
                break;
            case 5:
                ctx.source = "pod<Pod32>";
                fuzz::guarded(ctx, [&]() { (void)r.pod<Pod>(); });
                break;
            case 6:
                ctx.source = "podV<Pod32>";
                fuzz::guarded(ctx, [&]() { (void)r.podV<Pod>(); });
                break;
            case 7:
                ctx.source = "varint<u32>";
                fuzz::guarded(ctx, [&]() { (void)r.varint<uint32_t>(); });
                break;
            case 8:
            {
                const size_t n = static_cast<size_t>(rng() % 16);
                ctx.source = "hex";
                fuzz::guarded(ctx, [&]() { (void)r.hex(n); });
                break;
            }
            }
        }
    }

    return fuzz::print_success("fuzz_deserializer_hexctor", cfg);
}
