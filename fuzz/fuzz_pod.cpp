// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Fuzz harness: SerializablePod<SIZE>::from_string / deserialize / fromJSON
// against random strings and byte vectors across multiple SIZE
// instantiations.

#include "fuzz_common.h"

#include <rapidjson/document.h>
#include <serializable_pod.h>

namespace
{
    // Caller sets ctx.{input_string,input_bytes} once before the per-size
    // fan-out so only ctx.source changes here.
    template<unsigned SIZE>
    void fuzz_from_string(fuzz::CrashContext &ctx, const std::string &s)
    {
        ctx.source = "SerializablePod::ctor(string)";
        fuzz::guarded(ctx, [&]() { SerializablePod<SIZE> p(s); (void)p; });
    }

    template<unsigned SIZE>
    void fuzz_deserialize_bytes(fuzz::CrashContext &ctx, const std::vector<unsigned char> &buf)
    {
        ctx.source = "SerializablePod::deserialize(bytes)";
        fuzz::guarded(ctx, [&]() {
            SerializablePod<SIZE> p;
            p.deserialize(buf);
        });
    }

    template<unsigned SIZE>
    void fuzz_fromJSON_string_value(fuzz::CrashContext &ctx, const std::string &s)
    {
        ctx.source = "SerializablePod::fromJSON";
        fuzz::guarded(ctx, [&]() {
            // Build a JSON document whose root is the string itself.
            rapidjson::Document doc;
            const std::string wrapped = std::string("\"") + s + "\"";
            if (doc.Parse(wrapped.c_str()).HasParseError())
            {
                return;  // our wrapper produced an unparseable string — skip
            }
            SerializablePod<SIZE> p;
            p.fromJSON(doc);
        });
    }
}  // namespace

int main()
{
    const auto cfg = fuzz::load_config();
    fuzz::print_header("fuzz_pod", cfg);

    fuzz::Rng rng(cfg.seed);
    fuzz::CrashContext ctx;
    ctx.seed = cfg.seed;

    for (uint64_t i = 0; i < cfg.iters; ++i)
    {
        ctx.iter = i;

        const auto s = fuzz::rand_hexish_string(rng, 160);
        ctx.input_string = s;
        ctx.input_bytes.clear();
        fuzz_from_string<8>(ctx, s);
        fuzz_from_string<16>(ctx, s);
        fuzz_from_string<32>(ctx, s);

        const auto buf = fuzz::rand_bytes_short_biased(rng, 96);
        ctx.input_string.clear();
        ctx.input_bytes = buf;
        fuzz_deserialize_bytes<8>(ctx, buf);
        fuzz_deserialize_bytes<16>(ctx, buf);
        fuzz_deserialize_bytes<32>(ctx, buf);
        fuzz_deserialize_bytes<64>(ctx, buf);

        ctx.input_string = s;
        ctx.input_bytes.clear();
        fuzz_fromJSON_string_value<32>(ctx, s);
    }

    return fuzz::print_success("fuzz_pod", cfg);
}
