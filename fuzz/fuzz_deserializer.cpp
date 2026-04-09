// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Fuzz harness: deserializer_t random-walk. Construct a deserializer from
// random bytes, then dispatch a pseudo-random sequence of read operations
// over it. Any unexpected crash or exception is a bug.

#include "fuzz_common.h"

#include <deserializer_t.h>
#include <serializable_pod.h>
#include <serializer_t.h>

namespace
{
    using Pod = SerializablePod<8>;

    void step(fuzz::Rng &rng, Serialization::deserializer_t &r, fuzz::CrashContext &ctx)
    {
        const int op = static_cast<int>(rng() % 14);
        fuzz::guarded(ctx, [&]() {
            switch (op)
            {
            case 0:
                (void)r.boolean();
                break;
            case 1:
                (void)r.uint8();
                break;
            case 2:
                (void)r.uint16(false, (rng() & 1u) != 0);
                break;
            case 3:
                (void)r.uint32(false, (rng() & 1u) != 0);
                break;
            case 4:
                (void)r.uint64(false, (rng() & 1u) != 0);
                break;
            case 5:
                (void)r.uint128(false, (rng() & 1u) != 0);
                break;
            case 6:
                (void)r.uint256(false, (rng() & 1u) != 0);
                break;
            case 7:
                (void)r.bytes(static_cast<size_t>(rng() % 16));
                break;
            case 8:
                (void)r.hex(static_cast<size_t>(rng() % 16));
                break;
            case 9:
                (void)r.varint<uint32_t>();
                break;
            case 10:
                (void)r.varint<uint64_t>();
                break;
            case 11:
                (void)r.pod<Pod>();
                break;
            case 12:
                (void)r.podV<Pod>();
                break;
            case 13:
                (void)r.podVV<Pod>();
                break;
            }
        });
    }
}  // namespace

int main()
{
    const auto cfg = fuzz::load_config();
    fuzz::print_header("fuzz_deserializer", cfg);

    fuzz::Rng rng(cfg.seed);
    fuzz::CrashContext ctx;
    ctx.seed = cfg.seed;

    for (uint64_t i = 0; i < cfg.iters; ++i)
    {
        ctx.iter = i;
        auto bytes = fuzz::rand_bytes_short_biased(rng, 128);
        ctx.input_bytes = bytes;
        ctx.source = "deserializer_t random walk";

        Serialization::deserializer_t r(bytes);
        // Do up to ~16 random operations, breaking once the buffer is
        // exhausted enough that reads keep throwing.
        const int ops = static_cast<int>(rng() % 16) + 1;
        for (int j = 0; j < ops; ++j)
        {
            step(rng, r, ctx);
        }
        // Exercise cursor maintenance in between.
        if ((rng() & 0x7u) == 0)
        {
            fuzz::guarded(ctx, [&]() {
                const auto n = static_cast<size_t>(rng() % 8);
                r.skip(n);
            });
        }
        if ((rng() & 0x7u) == 0)
        {
            fuzz::guarded(ctx, [&]() { r.compact(); });
        }
        if ((rng() & 0xFu) == 0)
        {
            r.reset();
        }
    }

    return fuzz::print_success("fuzz_deserializer", cfg);
}
