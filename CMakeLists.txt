cmake_minimum_required(VERSION 3.10...3.31)

project(serializationcpp VERSION 2.0.0 LANGUAGES CXX)

include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

# Default to Release for single-config generators
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Mode: ${CMAKE_BUILD_TYPE}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ccache support
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Found ccache package... Activating...")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

option(BUILD_TEST "Build test binary" OFF)
if(DEFINED ENV{BUILD_TEST})
    set(BUILD_TEST $ENV{BUILD_TEST})
endif()
if(BUILD_TEST)
    message(STATUS "Test binary added to targets list")
endif()

# --- Detect MinGW environment ---
# Only use CMake's built-in MINGW (set for GCC with MinGW). Clang on Windows can
# target either MinGW or MSVC ABI, and the linker varies (GNU ld vs lld-link) —
# applying the wrong linker flags breaks the build. We let Clang on Windows rely
# on the OS defaults for DEP/ASLR (enabled by default on modern Windows) rather
# than guessing the linker style.

# ---------------------------------------------------------------------------
# Hardening flags
# ---------------------------------------------------------------------------

# Helper variables for generator expressions
set(_is_gcc "$<CXX_COMPILER_ID:GNU>")
set(_is_clang "$<CXX_COMPILER_ID:Clang>")
set(_is_appleclang "$<CXX_COMPILER_ID:AppleClang>")
set(_is_msvc "$<CXX_COMPILER_ID:MSVC>")
set(_is_gnu_like "$<OR:${_is_gcc},${_is_clang},${_is_appleclang}>")
set(_is_debug "$<CONFIG:Debug>")
set(_is_release "$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>")

# Dependencies
add_subdirectory(external/uint256_t)
set_property(TARGET uint256t PROPERTY FOLDER "external/uint256")

# Library
set(SOURCES
    src/deserializer_t.cpp
    src/serialization_secure_erase.cpp
    src/serializer_t.cpp
    src/string_helper.cpp
)

add_library(serialization-static STATIC ${SOURCES})

# C++17 — set as target properties (not global) to avoid leaking to consumers
set_target_properties(serialization-static PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_link_libraries(serialization-static PUBLIC uint256t)

# Mark uint256t's public includes as SYSTEM so warnings in external headers
# (e.g., -Wundef in endianness.h) don't fire when compiling our code.
get_target_property(_uint256t_includes uint256t INTERFACE_INCLUDE_DIRECTORIES)
if(_uint256t_includes)
    set_target_properties(uint256t PROPERTIES
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_uint256t_includes}")
endif()

# RapidJSON is header-only; this define enables std::string support.
# PUBLIC because downstream consumers including our JSON headers need it too.
target_compile_definitions(serialization-static PUBLIC RAPIDJSON_HAS_STDSTRING)

# Our own headers: PUBLIC (non-SYSTEM) so warnings are visible
target_include_directories(serialization-static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
# External/third-party headers: SYSTEM to suppress warnings we don't control
target_include_directories(serialization-static SYSTEM PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/rapidjson/include>
)

# --- Hardening: GCC/Clang ---
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(serialization-static PRIVATE
        -Wall -Wextra -Wpedantic -Werror
        -Wconversion -Wsign-conversion -Wshadow
        -Wcast-align -Wundef
        -Wformat=2 -Wformat-security
        -fno-strict-overflow
    )
    # Stack protector: requires libssp or compiler-rt at link time.
    # Probe with try_compile to catch Clang+MinGW toolchains that lack libssp.
    set(CMAKE_REQUIRED_FLAGS "-fstack-protector-strong")
    check_cxx_source_compiles("int main() { char buf[64]; buf[0] = 0; return buf[0]; }" HAS_STACK_PROTECTOR_STRONG_SER)
    unset(CMAKE_REQUIRED_FLAGS)
    if(HAS_STACK_PROTECTOR_STRONG_SER)
        target_compile_options(serialization-static PRIVATE -fstack-protector-strong)
    endif()
    # -fPIC is implied on Windows PE; only needed on ELF/Mach-O
    if(NOT WIN32)
        target_compile_options(serialization-static PRIVATE -fPIC)
        # Stack clash protection (Linux only — not supported on macOS/Apple Clang)
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            target_compile_options(serialization-static PRIVATE -fstack-clash-protection)
        endif()
    endif()
    # Intel CET control flow integrity (x86_64 only)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        check_cxx_compiler_flag("-fcf-protection=full" HAS_CF_PROTECTION_SER)
        if(HAS_CF_PROTECTION_SER)
            target_compile_options(serialization-static PRIVATE -fcf-protection=full)
        endif()
    endif()
    # GCC-specific
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(serialization-static PRIVATE -Wstrict-aliasing=2)
        # libstdc++ container assertions in Debug (bounds checks on operator[], etc.)
        target_compile_options(serialization-static PRIVATE
            $<${_is_debug}:-D_GLIBCXX_ASSERTIONS>
        )
    endif()
    # _FORTIFY_SOURCE requires optimization; not reliably supported on MinGW
    if(NOT WIN32)
        target_compile_options(serialization-static PRIVATE
            $<$<NOT:${_is_debug}>:-D_FORTIFY_SOURCE=2>
        )
    endif()
    # Optimization flags (PRIVATE)
    target_compile_options(serialization-static PRIVATE
        $<${_is_debug}:-g3 -Og>
        $<${_is_release}:-O3>
    )
elseif(MSVC)
    target_compile_options(serialization-static PRIVATE
        /W4 /WX
        /sdl
        /GS
        /guard:cf
        /wd4267 /wd4804 /wd4996
    )
    check_cxx_compiler_flag("/Qspectre" HAS_QSPECTRE_SER)
    if(HAS_QSPECTRE_SER)
        target_compile_options(serialization-static PRIVATE /Qspectre)
    endif()
    # CET shadow stack compatibility
    check_cxx_compiler_flag("/guard:ehcont" HAS_GUARD_EHCONT_SER)
    if(HAS_GUARD_EHCONT_SER)
        target_compile_options(serialization-static PRIVATE /guard:ehcont)
    endif()
endif()

# Tests
if(BUILD_TEST)
    enable_testing()

    add_executable(serialization-tests test/test.cpp)
    target_link_libraries(serialization-tests PRIVATE serialization-static)

    set_target_properties(serialization-tests PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        OUTPUT_NAME "serialization-tests"
    )

    # --- Linker hardening (test executable) ---
    # Linux (ELF)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set_property(TARGET serialization-tests APPEND_STRING PROPERTY LINK_FLAGS
            " -Wl,-z,relro,-z,now -Wl,-z,noexecstack")
    endif()
    # MinGW linker hardening (DEP + ASLR) — only for GCC+MinGW (known GNU ld)
    if(MINGW)
        set_property(TARGET serialization-tests APPEND_STRING PROPERTY LINK_FLAGS
            " -Wl,--nxcompat -Wl,--dynamicbase -Wl,--high-entropy-va")
    endif()
    # MSVC hardening (test executable)
    if(MSVC)
        target_compile_options(serialization-tests PRIVATE /guard:cf)
        set_property(TARGET serialization-tests APPEND_STRING PROPERTY LINK_FLAGS
            " /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA /guard:cf")
        if(HAS_GUARD_EHCONT_SER)
            target_compile_options(serialization-tests PRIVATE /guard:ehcont)
            set_property(TARGET serialization-tests APPEND_STRING PROPERTY LINK_FLAGS
                " /guard:ehcont /CETCOMPAT")
        endif()
    endif()

    add_test(NAME serialization-tests COMMAND serialization-tests)
endif()
