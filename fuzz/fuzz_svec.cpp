// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Fuzz harness: SerializableVector<SerializablePod<32>>::deserialize. This
// is the allocation-bomb path: the first varint in the buffer declares how
// many elements to allocate. The library must reject malformed counts
// without crashing, looping forever, or exhausting memory.

#include "fuzz_common.h"

#include <serializable_pod.h>
#include <serializable_vector.h>

namespace
{
    // SerializableVector<T>::fromJSON constructs each element via T(JSONValue&),
    // so T must have such a ctor. Raw SerializablePod<N> does not.
    template<unsigned SIZE>
    struct JsonPod : SerializablePod<SIZE>
    {
        JsonPod() = default;
        explicit JsonPod(const std::string &s) : SerializablePod<SIZE>(s) {}
        explicit JsonPod(const JSONValue &j)
        {
            JSON_STRING_OR_THROW()
            this->from_string(j.GetString());
        }
    };
    using Pod32 = JsonPod<32>;
    using Pod8 = JsonPod<8>;

    template<typename Pod>
    void fuzz_deserialize(fuzz::CrashContext &ctx, const std::vector<unsigned char> &buf)
    {
        ctx.input_bytes = buf;
        ctx.source = "SerializableVector<Pod>::deserialize";
        fuzz::guarded(ctx, [&]() {
            SerializableVector<Pod> v;
            v.deserialize(buf);
        });
    }
}  // namespace

int main()
{
    const auto cfg = fuzz::load_config();
    fuzz::print_header("fuzz_svec", cfg);

    fuzz::Rng rng(cfg.seed);
    fuzz::CrashContext ctx;
    ctx.seed = cfg.seed;

    // Seeded inputs: crafted varint counts followed by short / empty payloads.
    const std::vector<std::vector<unsigned char>> k_seeds = {
        {},
        {0x00},
        {0x01},                                            // count=1, no payload
        {0x02},                                            // count=2, no payload
        {0xFF, 0xFF, 0xFF, 0xFF, 0x0F},                    // count = uint32 max
        {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F},  // count near uint64 56-bit max
        {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01}, // over-long varint
    };

    for (const auto &s : k_seeds)
    {
        fuzz_deserialize<Pod32>(ctx, s);
        fuzz_deserialize<Pod8>(ctx, s);
        ++ctx.iter;
    }

    for (uint64_t i = 0; i < cfg.iters; ++i)
    {
        ctx.iter = i;
        // Up to 128 bytes to give the outer+inner varints room while
        // capping memory pressure per iteration.
        const auto buf = fuzz::rand_bytes_short_biased(rng, 128);
        fuzz_deserialize<Pod32>(ctx, buf);
        fuzz_deserialize<Pod8>(ctx, buf);
    }

    return fuzz::print_success("fuzz_svec", cfg);
}
