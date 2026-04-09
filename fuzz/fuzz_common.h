// Copyright (c) 2020-2024, Brandon Lehmann
// SPDX-License-Identifier: BSD-3-Clause
//
// Portable in-process fuzz harness infrastructure. Every fuzz target is a
// standalone executable that:
//
//   - Runs SERIALIZE_FUZZ_ITERS iterations (default 200000, overridable via
//     environment variable so CTest can run a short smoke pass)
//   - Uses a deterministic std::mt19937_64 seeded from SERIALIZE_FUZZ_SEED
//     (default 0xC0FFEEULL) so every crash is reproducible
//   - Loads an optional on-disk corpus of hand-crafted seed inputs before
//     switching to the PRNG stream
//   - Enforces the invariant: parser must NEVER terminate(), segfault, or
//     throw anything other than std::range_error / std::invalid_argument /
//     std::length_error / std::out_of_range. Any other exception type is a
//     bug.
//
// On a failing iteration, the harness prints:
//   SEED=0x...
//   ITER=N
//   INPUT=<hex>
//   WHAT=<exception what()>
//
// ...so the exact reproducer is in the log.

#ifndef SERIALIZATION_FUZZ_COMMON_H
#define SERIALIZATION_FUZZ_COMMON_H

// MSVC (and Clang in MSVC-compatible mode on Windows) tags std::getenv as
// deprecated under /sdl. The fuzz harness only reads harness-control env
// vars, not user secrets — silence the warning for fuzz code only.
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <string_helper.h>
#include <vector>

namespace fuzz
{
    // ------------------------------------------------------------------
    // Environment / configuration
    // ------------------------------------------------------------------

    inline uint64_t parse_u64_env(const char *name, uint64_t fallback)
    {
        const char *env = std::getenv(name);
        if (env == nullptr || env[0] == '\0')
        {
            return fallback;
        }
        try
        {
            if ((std::strlen(env) > 2) && env[0] == '0' && (env[1] == 'x' || env[1] == 'X'))
            {
                return std::stoull(env + 2, nullptr, 16);
            }
            return std::stoull(env, nullptr, 10);
        }
        catch (...)
        {
            return fallback;
        }
    }

    struct Config
    {
        uint64_t seed;
        uint64_t iters;
    };

    inline Config load_config()
    {
        Config c;
        c.seed = parse_u64_env("SERIALIZE_FUZZ_SEED", 0xC0FFEEULL);
        c.iters = parse_u64_env("SERIALIZE_FUZZ_ITERS", 200000ULL);
        return c;
    }

    // ------------------------------------------------------------------
    // Random byte / string generators (deterministic, seeded)
    // ------------------------------------------------------------------

    using Rng = std::mt19937_64;

    inline std::vector<unsigned char> rand_bytes(Rng &rng, size_t len)
    {
        std::vector<unsigned char> out(len);
        for (size_t i = 0; i < len; ++i)
        {
            out[i] = static_cast<unsigned char>(rng() & 0xFFu);
        }
        return out;
    }

    // Random-length bytes in [0, max_len] biased toward short inputs.
    inline std::vector<unsigned char> rand_bytes_short_biased(Rng &rng, size_t max_len)
    {
        const uint64_t r = rng();
        const size_t len = static_cast<size_t>(r % (max_len + 1));
        return rand_bytes(rng, len);
    }

    // Byte layouts designed to tickle varint decoders: long 0x80 runs, all
    // 0xFF, alternating 0x80/0x7F, single bytes.
    inline std::vector<unsigned char> rand_bytes_varint_shaped(Rng &rng, size_t max_len)
    {
        const uint64_t r = rng();
        const size_t len = static_cast<size_t>(r % (max_len + 1));
        std::vector<unsigned char> out(len);
        const int shape = static_cast<int>(rng() % 6);
        for (size_t i = 0; i < len; ++i)
        {
            switch (shape)
            {
            case 0:  // all high-bit-set
                out[i] = 0x80 | static_cast<unsigned char>(rng() & 0x7Fu);
                break;
            case 1:  // all 0xFF
                out[i] = 0xFF;
                break;
            case 2:  // alternating 0x80 / 0x01
                out[i] = (i & 1u) ? 0x01 : 0x80;
                break;
            case 3:  // biased low (0..0x7F)
                out[i] = static_cast<unsigned char>(rng() & 0x7Fu);
                break;
            case 4:  // last byte terminator, rest continuation
                out[i] = (i + 1 == len) ? static_cast<unsigned char>(rng() & 0x7Fu)
                                        : (0x80 | static_cast<unsigned char>(rng() & 0x7Fu));
                break;
            default:
                out[i] = static_cast<unsigned char>(rng() & 0xFFu);
                break;
            }
        }
        return out;
    }

    inline std::string rand_hexish_string(Rng &rng, size_t max_len)
    {
        const uint64_t r = rng();
        const size_t len = static_cast<size_t>(r % (max_len + 1));
        std::string out;
        out.reserve(len);
        const int shape = static_cast<int>(rng() % 5);
        for (size_t i = 0; i < len; ++i)
        {
            switch (shape)
            {
            case 0:
            {  // valid hex
                static const char hex[] = "0123456789abcdef";
                out.push_back(hex[rng() % 16]);
                break;
            }
            case 1:
            {  // mixed case hex
                static const char hex[] = "0123456789abcdefABCDEF";
                out.push_back(hex[rng() % 22]);
                break;
            }
            case 2:  // arbitrary ASCII
                out.push_back(static_cast<char>((rng() % (0x7F - 0x20)) + 0x20));
                break;
            case 3:  // with some invalid chars
                if (rng() % 3 == 0)
                {
                    out.push_back(static_cast<char>(rng() % 256));
                }
                else
                {
                    static const char hex[] = "0123456789abcdef";
                    out.push_back(hex[rng() % 16]);
                }
                break;
            default:  // any byte
                out.push_back(static_cast<char>(rng() % 256));
                break;
            }
        }
        return out;
    }

    // ------------------------------------------------------------------
    // Crash reporter
    // ------------------------------------------------------------------

    struct CrashContext
    {
        uint64_t seed;
        uint64_t iter;
        std::string source;
        std::vector<unsigned char> input_bytes;
        std::string input_string;
    };

    [[noreturn]] inline void report_crash(const CrashContext &ctx, const char *what_str)
    {
        std::fprintf(stderr, "\n==== FUZZ CRASH ====\n");
        std::fprintf(stderr, "SEED=0x%llx\n", static_cast<unsigned long long>(ctx.seed));
        std::fprintf(stderr, "ITER=%llu\n", static_cast<unsigned long long>(ctx.iter));
        std::fprintf(stderr, "SOURCE=%s\n", ctx.source.c_str());
        if (!ctx.input_string.empty())
        {
            std::fprintf(stderr, "INPUT_STRING=%s\n", ctx.input_string.c_str());
        }
        if (!ctx.input_bytes.empty())
        {
            std::string hex = Serialization::to_hex(ctx.input_bytes.data(), ctx.input_bytes.size());
            std::fprintf(stderr, "INPUT_HEX=%s\n", hex.c_str());
        }
        std::fprintf(stderr, "WHAT=%s\n", what_str);
        std::fprintf(stderr, "====================\n");
        std::fflush(stderr);
        std::exit(1);
    }

    // ------------------------------------------------------------------
    // Header print and summary
    // ------------------------------------------------------------------

    inline void print_header(const char *name, const Config &c)
    {
        std::printf("[%s] seed=0x%llx iters=%llu\n", name,
                    static_cast<unsigned long long>(c.seed),
                    static_cast<unsigned long long>(c.iters));
        std::fflush(stdout);
    }

    inline int print_success(const char *name, const Config &c)
    {
        std::printf("[%s] OK %llu iters (seed=0x%llx)\n", name,
                    static_cast<unsigned long long>(c.iters),
                    static_cast<unsigned long long>(c.seed));
        std::fflush(stdout);
        return 0;
    }

    // Multi-catch exception dispatch. Kept non-inline + out-of-line so the
    // catch clauses live in one frame, and avoids dynamic_cast which
    // intermittently access-violates under Clang+ASan on Windows.
    enum class GuardedVerdict
    {
        Expected,
        CrashBadAlloc,
        CrashOther,
    };

    struct GuardedResult
    {
        GuardedVerdict verdict;
        std::string what;
    };

#if defined(__GNUC__) || defined(__clang__)
#define SER_FUZZ_NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define SER_FUZZ_NOINLINE __declspec(noinline)
#else
#define SER_FUZZ_NOINLINE
#endif

    // NOINLINE is load-bearing under ASan on Windows — inlining the
    // catches at every call site blows the default 1 MB stack.
    SER_FUZZ_NOINLINE inline GuardedResult classify_current_exception() noexcept
    {
        try
        {
            throw;  // rethrow currently-active exception
        }
        catch (const std::range_error &)
        {
            return {GuardedVerdict::Expected, {}};
        }
        catch (const std::length_error &)
        {
            return {GuardedVerdict::Expected, {}};
        }
        catch (const std::invalid_argument &)
        {
            return {GuardedVerdict::Expected, {}};
        }
        catch (const std::out_of_range &)
        {
            return {GuardedVerdict::Expected, {}};
        }
        catch (const std::bad_alloc &e)
        {
            return {GuardedVerdict::CrashBadAlloc, e.what()};
        }
        catch (const std::exception &e)
        {
            return {GuardedVerdict::CrashOther, e.what()};
        }
        catch (...)
        {
            return {GuardedVerdict::CrashOther, "non-std::exception thrown"};
        }
    }

    // NOINLINE so callers' stack frames don't absorb the try/catch —
    // same ASan reasoning as classify_current_exception above.
    template<typename Fn>
    SER_FUZZ_NOINLINE void guarded(CrashContext &ctx, Fn &&fn)
    {
        try
        {
            fn();
        }
        catch (...)
        {
            const auto result = classify_current_exception();
            if (result.verdict != GuardedVerdict::Expected)
            {
                report_crash(ctx, result.what.c_str());
            }
        }
    }

}  // namespace fuzz

#endif
