// Copyright (c) 2020-2024, Brandon Lehmann
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <iostream>
#include <stdexcept>
#include <string>

static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define ASSERT_TRUE(expr)                                                                      \
    do                                                                                         \
    {                                                                                          \
        if (!(expr))                                                                           \
        {                                                                                      \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__)     \
                + ": ASSERT_TRUE failed: " #expr);                                             \
        }                                                                                      \
    } while (0)

#define ASSERT_FALSE(expr)                                                                     \
    do                                                                                         \
    {                                                                                          \
        if ((expr))                                                                            \
        {                                                                                      \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__)     \
                + ": ASSERT_FALSE failed: " #expr);                                            \
        }                                                                                      \
    } while (0)

#define ASSERT_EQ(a, b)                                                                        \
    do                                                                                         \
    {                                                                                          \
        if (!((a) == (b)))                                                                     \
        {                                                                                      \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__)     \
                + ": ASSERT_EQ failed: " #a " == " #b);                                       \
        }                                                                                      \
    } while (0)

#define ASSERT_NE(a, b)                                                                        \
    do                                                                                         \
    {                                                                                          \
        if (!((a) != (b)))                                                                     \
        {                                                                                      \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__)     \
                + ": ASSERT_NE failed: " #a " != " #b);                                       \
        }                                                                                      \
    } while (0)

#define ASSERT_THROWS(expr)                                                                    \
    do                                                                                         \
    {                                                                                          \
        bool caught = false;                                                                   \
        try                                                                                    \
        {                                                                                      \
            expr;                                                                              \
        }                                                                                      \
        catch (...)                                                                            \
        {                                                                                      \
            caught = true;                                                                     \
        }                                                                                      \
        if (!caught)                                                                           \
        {                                                                                      \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__)     \
                + ": ASSERT_THROWS failed: " #expr " did not throw");                          \
        }                                                                                      \
    } while (0)

#define RUN_TEST(func)                                                      \
    do                                                                      \
    {                                                                       \
        std::cout << "  " #func "... " << std::flush;                       \
        try                                                                 \
        {                                                                   \
            func();                                                         \
            std::cout << "PASSED" << std::endl;                             \
            ++g_tests_passed;                                               \
        }                                                                   \
        catch (const std::exception &e)                                     \
        {                                                                   \
            std::cout << "FAILED" << std::endl;                             \
            std::cerr << "    " << e.what() << std::endl;                   \
            ++g_tests_failed;                                               \
        }                                                                   \
    } while (0)

static inline int test_summary()
{
    std::cout << std::endl
              << "Results: " << g_tests_passed << " passed, " << g_tests_failed << " failed, "
              << (g_tests_passed + g_tests_failed) << " total" << std::endl;

    return g_tests_failed == 0 ? 0 : 1;
}

#endif
