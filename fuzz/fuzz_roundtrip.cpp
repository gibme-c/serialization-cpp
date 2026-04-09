// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Fuzz harness: serialize/deserialize full-pipeline involution. Builds a
// random SerializableVector<SerializablePod<32>>, serializes, deserializes,
// and verifies the round trip is byte-exact.

#include "fuzz_common.h"

#include <serializable_pod.h>
#include <serializable_vector.h>

namespace
{
    // See fuzz_svec.cpp for why T needs a JSONValue ctor.
    struct Pod : SerializablePod<32>
    {
        Pod() = default;
        explicit Pod(const std::string &s) : SerializablePod<32>(s) {}
        explicit Pod(const JSONValue &j)
        {
            JSON_STRING_OR_THROW()
            from_string(j.GetString());
        }
    };
}

int main()
{
    const auto cfg = fuzz::load_config();
    fuzz::print_header("fuzz_roundtrip", cfg);

    fuzz::Rng rng(cfg.seed);
    fuzz::CrashContext ctx;
    ctx.seed = cfg.seed;

    for (uint64_t i = 0; i < cfg.iters; ++i)
    {
        ctx.iter = i;

        // Build a vector with a random number of elements (0..16) filled
        // with random content.
        const size_t count = static_cast<size_t>(rng() % 17);
        SerializableVector<Pod> v;
        for (size_t j = 0; j < count; ++j)
        {
            Pod p;
            auto *raw = *p;
            for (size_t k = 0; k < 32; ++k)
            {
                raw[k] = static_cast<unsigned char>(rng() & 0xFFu);
            }
            v.append(p);
        }

        ctx.source = "SerializableVector<Pod32> serialize/deserialize involution";
        fuzz::guarded(ctx, [&]() {
            const auto encoded = v.serialize();
            ctx.input_bytes = encoded;
            SerializableVector<Pod> decoded;
            decoded.deserialize(encoded);
            if (!(decoded == v))
            {
                throw std::runtime_error("roundtrip content mismatch");
            }
            // Re-encode the decoded copy and compare byte-for-byte.
            const auto re_encoded = decoded.serialize();
            if (re_encoded != encoded)
            {
                throw std::runtime_error("roundtrip byte mismatch");
            }
        });
    }

    return fuzz::print_success("fuzz_roundtrip", cfg);
}
