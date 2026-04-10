// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Fuzz harness: cursor operation interleaving. Stresses the deserializer's
// cursor management by randomly interleaving reads, peeks, skips, resets,
// and compacts on a random buffer. The goal is to ensure none of these
// combinations cause crashes, hangs, or unexpected exception types.

#include "fuzz_common.h"

#include <cstdint>
#include <deserializer_t.h>
#include <serializable_pod.h>
#include <serializer_t.h>

namespace
{
    using Pod8 = SerializablePod<8>;

    enum CursorOp : int
    {
        ReadBool = 0,
        ReadU8,
        ReadU16,
        ReadU32,
        ReadU64,
        ReadU128,
        ReadU256,
        ReadBytes,
        ReadHex,
        ReadVarint32,
        ReadPod8,
        PeekU8,
        PeekU16,
        PeekU32,
        PeekU64,
        PeekBool,
        PeekHex,
        DoSkip,
        DoReset,
        DoCompact,
        DoResetZeroCompact,
        QueryUnread,
        CURSOR_OP_COUNT
    };

    void run_one(fuzz::Rng &rng, fuzz::CrashContext &ctx)
    {
        // Generate a random buffer (0..256 bytes, biased short).
        const auto buf = fuzz::rand_bytes_short_biased(rng, 256);
        Serialization::deserializer_t r(buf);
        ctx.input_bytes = buf;

        const size_t num_ops = static_cast<size_t>(1 + rng() % 32);

        for (size_t op = 0; op < num_ops; ++op)
        {
            const auto which = static_cast<int>(rng() % CURSOR_OP_COUNT);
            const bool be = (rng() & 1u) != 0;

            // Each operation is individually guarded — we expect range_error
            // and friends on short buffers, but nothing else.
            ctx.source = "cursor_op";
            fuzz::guarded(ctx, [&]() {
                switch (which)
                {
                case ReadBool:
                    (void)r.boolean();
                    break;
                case ReadU8:
                    (void)r.uint8();
                    break;
                case ReadU16:
                    (void)r.uint16(false, be);
                    break;
                case ReadU32:
                    (void)r.uint32(false, be);
                    break;
                case ReadU64:
                    (void)r.uint64(false, be);
                    break;
                case ReadU128:
                    (void)r.uint128(false, be);
                    break;
                case ReadU256:
                    (void)r.uint256(false, be);
                    break;
                case ReadBytes:
                {
                    const size_t count = static_cast<size_t>(rng() % 33);
                    (void)r.bytes(count);
                    break;
                }
                case ReadHex:
                {
                    const size_t count = static_cast<size_t>(rng() % 17);
                    (void)r.hex(count);
                    break;
                }
                case ReadVarint32:
                    (void)r.varint<uint32_t>();
                    break;
                case ReadPod8:
                    (void)r.pod<Pod8>();
                    break;
                case PeekU8:
                    (void)r.uint8(true);
                    break;
                case PeekU16:
                    (void)r.uint16(true, be);
                    break;
                case PeekU32:
                    (void)r.uint32(true, be);
                    break;
                case PeekU64:
                    (void)r.uint64(true, be);
                    break;
                case PeekBool:
                    (void)r.boolean(true);
                    break;
                case PeekHex:
                {
                    const size_t count = static_cast<size_t>(rng() % 9);
                    (void)r.hex(count, true);
                    break;
                }
                case DoSkip:
                {
                    const size_t count = static_cast<size_t>(rng() % 33);
                    r.skip(count);
                    break;
                }
                case DoReset:
                {
                    // Sometimes reset to a valid position, sometimes past end.
                    const size_t pos = static_cast<size_t>(rng() % (r.size() + 8));
                    r.reset(pos);
                    break;
                }
                case DoCompact:
                    r.compact();
                    break;
                case DoResetZeroCompact:
                    r.reset(0);
                    r.compact();
                    break;
                case QueryUnread:
                    (void)r.unread_bytes();
                    (void)r.unread_data();
                    break;
                default:
                    break;
                }
            });
        }
    }
} // namespace

int main()
{
    const auto cfg = fuzz::load_config();
    fuzz::print_header("fuzz_cursor_ops", cfg);

    fuzz::Rng rng(cfg.seed);
    fuzz::CrashContext ctx;
    ctx.seed = cfg.seed;

    for (uint64_t i = 0; i < cfg.iters; ++i)
    {
        ctx.iter = i;
        run_one(rng, ctx);
    }

    return fuzz::print_success("fuzz_cursor_ops", cfg);
}
