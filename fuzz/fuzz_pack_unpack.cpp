// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Fuzz harness: Serialization::unpack<T> at random offsets into random
// buffers across every supported integer width. ASan is relied upon to
// detect any out-of-bounds read the bounds-check function may miss; the
// harness itself only asserts that expected exceptions are thrown and no
// unexpected exceptions escape.

#include "fuzz_common.h"

#include <serialization_helper.h>

namespace
{
    // Caller sets ctx.input_bytes once per outer iteration; this fan-out
    // only updates ctx.source.
    template<typename T>
    void fuzz_unpack_at(fuzz::CrashContext &ctx, const std::vector<unsigned char> &buf, size_t offset, bool be)
    {
        ctx.source = "unpack<T>";
        fuzz::guarded(ctx, [&]() { (void)Serialization::unpack<T>(buf, offset, be); });
    }

    template<typename T>
    void fuzz_pack_unpack_roundtrip(fuzz::CrashContext &ctx, fuzz::Rng &rng)
    {
        const uint64_t raw = rng();
        const T v = static_cast<T>(raw);
        ctx.source = "pack/unpack<T> roundtrip";
        fuzz::guarded(ctx, [&]() {
            const auto le = Serialization::pack<T>(v);
            const auto be = Serialization::pack<T>(v, true);
            if (Serialization::unpack<T>(le) != v)
            {
                throw std::runtime_error("pack/unpack LE mismatch");
            }
            if (Serialization::unpack<T>(be, 0, true) != v)
            {
                throw std::runtime_error("pack/unpack BE mismatch");
            }
        });
    }
}  // namespace

int main()
{
    const auto cfg = fuzz::load_config();
    fuzz::print_header("fuzz_pack_unpack", cfg);

    fuzz::Rng rng(cfg.seed);
    fuzz::CrashContext ctx;
    ctx.seed = cfg.seed;

    for (uint64_t i = 0; i < cfg.iters; ++i)
    {
        ctx.iter = i;

        // Random buffer of length 0..64.
        const auto buf = fuzz::rand_bytes_short_biased(rng, 64);
        ctx.input_bytes = buf;
        const size_t max_off = buf.empty() ? 64 : (buf.size() + 16);
        const size_t off = static_cast<size_t>(rng() % (max_off + 1));
        const bool be = (rng() & 1u) != 0;

        fuzz_unpack_at<uint8_t>(ctx, buf, off, be);
        fuzz_unpack_at<uint16_t>(ctx, buf, off, be);
        fuzz_unpack_at<uint32_t>(ctx, buf, off, be);
        fuzz_unpack_at<uint64_t>(ctx, buf, off, be);
        fuzz_unpack_at<int8_t>(ctx, buf, off, be);
        fuzz_unpack_at<int16_t>(ctx, buf, off, be);
        fuzz_unpack_at<int32_t>(ctx, buf, off, be);
        fuzz_unpack_at<int64_t>(ctx, buf, off, be);

        // Round-trip: pack then unpack a random value — must always
        // recover the original exactly.
        fuzz_pack_unpack_roundtrip<uint8_t>(ctx, rng);
        fuzz_pack_unpack_roundtrip<uint16_t>(ctx, rng);
        fuzz_pack_unpack_roundtrip<uint32_t>(ctx, rng);
        fuzz_pack_unpack_roundtrip<uint64_t>(ctx, rng);
        fuzz_pack_unpack_roundtrip<int32_t>(ctx, rng);
        fuzz_pack_unpack_roundtrip<int64_t>(ctx, rng);
    }

    return fuzz::print_success("fuzz_pack_unpack", cfg);
}
