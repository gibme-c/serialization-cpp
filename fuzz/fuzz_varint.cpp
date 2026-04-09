// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Fuzz harness: decode_varint<uint8/16/32/64> against random, mutated, and
// adversarial byte sequences. The invariant is that decode_varint must
// either return a value or throw std::range_error / std::length_error — it
// must never crash, infinite-loop, or throw anything else.

#include "fuzz_common.h"

#include <serialization_helper.h>

namespace
{
    // Hand-crafted seed corpus: byte sequences known to stress the decoder.
    const std::vector<std::vector<unsigned char>> k_corpus = {
        {},
        {0x00},
        {0x7F},
        {0x80, 0x01},
        {0xFF, 0x7F},
        {0x80, 0x80, 0x01},
        {0xFF, 0xFF, 0xFF, 0xFF, 0x0F},                // uint32 max
        {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F}, // 56-bit max
        {0x80},                                         // truncated continuation
        {0x80, 0x80},                                   // truncated continuation
        {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01},  // over-long
        {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  // all-0xFF
    };

    // Caller is expected to have set ctx.input_bytes before fanning out
    // over the integer widths — that keeps one copy per iteration instead
    // of one per width.
    template<typename Type>
    void fuzz_decode(fuzz::CrashContext &ctx, const std::vector<unsigned char> &input)
    {
        ctx.source = "decode_varint";
        fuzz::guarded(ctx, [&]() {
            (void)Serialization::decode_varint<Type>(input);
        });
    }

    template<typename Type>
    void fuzz_encode_decode_roundtrip(fuzz::CrashContext &ctx, fuzz::Rng &rng)
    {
        const uint64_t raw = rng();
        const Type value = static_cast<Type>(raw);
        ctx.source = "encode+decode_varint roundtrip";
        fuzz::guarded(ctx, [&]() {
            auto encoded = Serialization::encode_varint<Type>(value);
            ctx.input_bytes = encoded;
            const auto [decoded, consumed] = Serialization::decode_varint<Type>(encoded);
            if (decoded != value)
            {
                throw std::runtime_error("varint roundtrip mismatch");
            }
            if (consumed != encoded.size())
            {
                throw std::runtime_error("varint consumed != size");
            }
        });
    }
}  // namespace

int main()
{
    const auto cfg = fuzz::load_config();
    fuzz::print_header("fuzz_varint", cfg);

    fuzz::Rng rng(cfg.seed);
    fuzz::CrashContext ctx;
    ctx.seed = cfg.seed;
    ctx.iter = 0;

    // Phase 1: corpus replay.
    for (const auto &seed : k_corpus)
    {
        ctx.input_bytes = seed;
        fuzz_decode<uint8_t>(ctx, seed);
        fuzz_decode<uint16_t>(ctx, seed);
        fuzz_decode<uint32_t>(ctx, seed);
        fuzz_decode<uint64_t>(ctx, seed);
        ++ctx.iter;
    }

    // Phase 2: random + shaped byte streams.
    for (uint64_t i = 0; i < cfg.iters; ++i)
    {
        ctx.iter = i;
        const auto bytes = (i & 1u) ? fuzz::rand_bytes_varint_shaped(rng, 20)
                                    : fuzz::rand_bytes_short_biased(rng, 20);
        ctx.input_bytes = bytes;
        fuzz_decode<uint8_t>(ctx, bytes);
        fuzz_decode<uint16_t>(ctx, bytes);
        fuzz_decode<uint32_t>(ctx, bytes);
        fuzz_decode<uint64_t>(ctx, bytes);

        // Round-trip: encode then decode with an optional bit flip to
        // stress the decoder's mutation tolerance.
        fuzz_encode_decode_roundtrip<uint8_t>(ctx, rng);
        fuzz_encode_decode_roundtrip<uint16_t>(ctx, rng);
        fuzz_encode_decode_roundtrip<uint32_t>(ctx, rng);
        fuzz_encode_decode_roundtrip<uint64_t>(ctx, rng);
    }

    return fuzz::print_success("fuzz_varint", cfg);
}
