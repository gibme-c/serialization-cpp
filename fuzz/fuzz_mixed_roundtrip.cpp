// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Fuzz harness: mixed-type serialization round-trips. Each iteration builds
// a random sequence of typed write operations (bool, uint8..uint256, varint,
// bytes, pod), serializes them into a single buffer, then deserializes with
// the matching read sequence and verifies byte-exact round-trip fidelity.

#include "fuzz_common.h"

#include <cstdint>
#include <deserializer_t.h>
#include <functional>
#include <serializable_pod.h>
#include <serializer_t.h>
#include <vector>

namespace
{
    using Pod8 = SerializablePod<8>;

    enum OpType : int
    {
        OpBool = 0,
        OpU8,
        OpU16,
        OpU32,
        OpU64,
        OpU128,
        OpU256,
        OpVarint32,
        OpVarint64,
        OpBytes,
        OpPod8,
        OP_COUNT
    };

    void run_one(fuzz::Rng &rng, fuzz::CrashContext &ctx)
    {
        Serialization::serializer_t w;

        // Each verifier reads from the deserializer and throws on mismatch.
        std::vector<std::function<void(Serialization::deserializer_t &)>> verifiers;

        const size_t num_ops = static_cast<size_t>(1 + rng() % 20);

        for (size_t op = 0; op < num_ops; ++op)
        {
            const auto op_type = static_cast<int>(rng() % OP_COUNT);
            const bool big_endian = (rng() & 1u) != 0;

            switch (op_type)
            {
            case OpBool:
            {
                const bool val = (rng() & 1u) != 0;
                w.boolean(val);
                verifiers.emplace_back([val](Serialization::deserializer_t &r) {
                    if (r.boolean() != val)
                    {
                        throw std::runtime_error("bool mismatch");
                    }
                });
                break;
            }
            case OpU8:
            {
                const auto val = static_cast<unsigned char>(rng() & 0xFFu);
                w.uint8(val);
                verifiers.emplace_back([val](Serialization::deserializer_t &r) {
                    if (r.uint8() != val)
                    {
                        throw std::runtime_error("uint8 mismatch");
                    }
                });
                break;
            }
            case OpU16:
            {
                const auto val = static_cast<uint16_t>(rng() & 0xFFFFu);
                w.uint16(val, big_endian);
                verifiers.emplace_back([val, big_endian](Serialization::deserializer_t &r) {
                    if (r.uint16(false, big_endian) != val)
                    {
                        throw std::runtime_error("uint16 mismatch");
                    }
                });
                break;
            }
            case OpU32:
            {
                const auto val = static_cast<uint32_t>(rng());
                w.uint32(val, big_endian);
                verifiers.emplace_back([val, big_endian](Serialization::deserializer_t &r) {
                    if (r.uint32(false, big_endian) != val)
                    {
                        throw std::runtime_error("uint32 mismatch");
                    }
                });
                break;
            }
            case OpU64:
            {
                const auto val = static_cast<uint64_t>(rng());
                w.uint64(val, big_endian);
                verifiers.emplace_back([val, big_endian](Serialization::deserializer_t &r) {
                    if (r.uint64(false, big_endian) != val)
                    {
                        throw std::runtime_error("uint64 mismatch");
                    }
                });
                break;
            }
            case OpU128:
            {
                const uint128_t val(rng(), rng());
                w.uint128(val, big_endian);
                verifiers.emplace_back([val, big_endian](Serialization::deserializer_t &r) {
                    if (!(r.uint128(false, big_endian) == val))
                    {
                        throw std::runtime_error("uint128 mismatch");
                    }
                });
                break;
            }
            case OpU256:
            {
                const uint256_t val(uint128_t(rng(), rng()), uint128_t(rng(), rng()));
                w.uint256(val, big_endian);
                verifiers.emplace_back([val, big_endian](Serialization::deserializer_t &r) {
                    if (!(r.uint256(false, big_endian) == val))
                    {
                        throw std::runtime_error("uint256 mismatch");
                    }
                });
                break;
            }
            case OpVarint32:
            {
                const auto val = static_cast<uint32_t>(rng());
                w.varint<uint32_t>(val);
                verifiers.emplace_back([val](Serialization::deserializer_t &r) {
                    if (r.varint<uint32_t>() != val)
                    {
                        throw std::runtime_error("varint32 mismatch");
                    }
                });
                break;
            }
            case OpVarint64:
            {
                const auto val = static_cast<uint64_t>(rng());
                w.varint<uint64_t>(val);
                verifiers.emplace_back([val](Serialization::deserializer_t &r) {
                    if (r.varint<uint64_t>() != val)
                    {
                        throw std::runtime_error("varint64 mismatch");
                    }
                });
                break;
            }
            case OpBytes:
            {
                const size_t len = static_cast<size_t>(rng() % 17);
                auto val = fuzz::rand_bytes(rng, len);
                w.bytes(val);
                verifiers.emplace_back([val, len](Serialization::deserializer_t &r) {
                    auto got = r.bytes(len);
                    if (got != val)
                    {
                        throw std::runtime_error("bytes mismatch");
                    }
                });
                break;
            }
            case OpPod8:
            {
                Pod8 val;
                for (size_t k = 0; k < 8; ++k)
                {
                    val[k] = static_cast<unsigned char>(rng() & 0xFFu);
                }
                w.pod(val);
                verifiers.emplace_back([val](Serialization::deserializer_t &r) {
                    auto got = r.pod<Pod8>();
                    if (!(got == val))
                    {
                        throw std::runtime_error("pod8 mismatch");
                    }
                });
                break;
            }
            default:
                break;
            }
        }

        // Now deserialize and verify.
        ctx.input_bytes = w.vector();
        Serialization::deserializer_t r(w);
        for (auto &verify : verifiers)
        {
            verify(r);
        }
        if (r.unread_bytes() != 0)
        {
            throw std::runtime_error("trailing unread bytes after round-trip");
        }
    }
} // namespace

int main()
{
    const auto cfg = fuzz::load_config();
    fuzz::print_header("fuzz_mixed_roundtrip", cfg);

    fuzz::Rng rng(cfg.seed);
    fuzz::CrashContext ctx;
    ctx.seed = cfg.seed;

    for (uint64_t i = 0; i < cfg.iters; ++i)
    {
        ctx.iter = i;
        ctx.source = "mixed-type serialization round-trip";
        fuzz::guarded(ctx, [&]() { run_one(rng, ctx); });
    }

    return fuzz::print_success("fuzz_mixed_roundtrip", cfg);
}
