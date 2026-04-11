cmake_minimum_required(VERSION 3.10...3.31)

project(serializationcpp VERSION 2.0.0 LANGUAGES CXX)

include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

# Top-level detection. Tests, fuzzers, the sanitizer option, enable_testing(),
# and the Release default all stay behind this gate so a downstream consumer
# pulling us in via add_subdirectory() gets nothing but the library target.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(SERIALIZATION_IS_TOP_LEVEL ON)
else()
    set(SERIALIZATION_IS_TOP_LEVEL OFF)
endif()

if(SERIALIZATION_IS_TOP_LEVEL)
    # Default to Release for single-config generators — only when we own the
    # build. Never override a parent project's choice of build type.
    if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    endif()

    message(STATUS "serialization-cpp mode: ${CMAKE_BUILD_TYPE}")

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

    # ccache support — only when top-level so we don't silently set a
    # launcher on a parent project that didn't ask for one.
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        message(STATUS "Found ccache package... Activating...")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    endif()
endif()

# Options are project-namespaced so they can't collide with a parent project's
# BUILD_TEST / BUILD_FUZZ / SANITIZE. They are only defined and honored when
# we're the top-level project — consumers using add_subdirectory() never get
# our tests or fuzz harnesses, period.
if(SERIALIZATION_IS_TOP_LEVEL)
    option(SERIALIZATION_BUILD_TESTS "Build serialization-cpp unit tests" OFF)
    if(DEFINED ENV{SERIALIZATION_BUILD_TESTS})
        set(SERIALIZATION_BUILD_TESTS $ENV{SERIALIZATION_BUILD_TESTS})
    endif()
    if(SERIALIZATION_BUILD_TESTS)
        message(STATUS "serialization-cpp: unit tests enabled")
    endif()

    option(SERIALIZATION_BUILD_FUZZ "Build portable in-process fuzz harnesses" OFF)
    if(DEFINED ENV{SERIALIZATION_BUILD_FUZZ})
        set(SERIALIZATION_BUILD_FUZZ $ENV{SERIALIZATION_BUILD_FUZZ})
    endif()
    if(SERIALIZATION_BUILD_FUZZ)
        message(STATUS "serialization-cpp: fuzz harnesses enabled")
    endif()

    # Sanitizer selection for tests and fuzzers (not the static library
    # when consumed downstream). Applied PRIVATE so downstream consumers
    # are unaffected. GCC/Clang only.
    set(SERIALIZATION_SANITIZE "none" CACHE STRING
        "Sanitizer: none|address|undefined|thread|address+undefined")
    set_property(CACHE SERIALIZATION_SANITIZE PROPERTY STRINGS
        none address undefined thread address+undefined)
    if(NOT SERIALIZATION_SANITIZE STREQUAL "none")
        message(STATUS "serialization-cpp: sanitizer enabled for test/fuzz targets: ${SERIALIZATION_SANITIZE}")
    endif()
else()
    # Force-off when consumed as a subdirectory. No test/fuzz targets, no
    # sanitizer flags, no CTest registration.
    set(SERIALIZATION_BUILD_TESTS OFF)
    set(SERIALIZATION_BUILD_FUZZ OFF)
    set(SERIALIZATION_SANITIZE "none")
endif()

# Helper: apply the selected sanitizer to a single target. No-op on MSVC or
# when SANITIZE=none. Always PRIVATE — never leaks to downstream.
function(serialization_apply_sanitizer tgt)
    if(SERIALIZATION_SANITIZE STREQUAL "none")
        return()
    endif()
    if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
        OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))
        message(STATUS "SERIALIZATION_SANITIZE=${SERIALIZATION_SANITIZE} requested but compiler "
            "${CMAKE_CXX_COMPILER_ID} does not support it; ignoring for target ${tgt}")
        return()
    endif()
    set(_san_flags "")
    if(SERIALIZATION_SANITIZE MATCHES "address")
        list(APPEND _san_flags -fsanitize=address -fno-omit-frame-pointer)
    endif()
    if(SERIALIZATION_SANITIZE MATCHES "undefined")
        list(APPEND _san_flags -fsanitize=undefined -fno-sanitize-recover=undefined)
    endif()
    if(SERIALIZATION_SANITIZE STREQUAL "thread")
        list(APPEND _san_flags -fsanitize=thread)
    endif()
    target_compile_options(${tgt} PRIVATE ${_san_flags})
    target_link_options(${tgt} PRIVATE ${_san_flags})
    # Windows (clang-cl / Clang targeting MSVC ABI) requires disabling the
    # MSVC STL's string/vector annotations so every TU agrees on the container
    # layout. Without this, link fails with /failifmismatch on 'annotate_string'
    # because ASan adds metadata the uninstrumented STL objects lack.
    if(WIN32 AND SERIALIZATION_SANITIZE MATCHES "address")
        target_compile_definitions(${tgt} PRIVATE
            _DISABLE_STRING_ANNOTATION=1
            _DISABLE_VECTOR_ANNOTATION=1)
    endif()
endfunction()

# Helper: apply the same linker-hardening flags used by serialization-tests to
# any test-or-fuzz executable. Kept here so the fuzz target loop stays short.
function(serialization_apply_exe_hardening tgt)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set_property(TARGET ${tgt} APPEND_STRING PROPERTY LINK_FLAGS
            " -Wl,-z,relro,-z,now -Wl,-z,noexecstack")
    endif()
    if(MINGW)
        set_property(TARGET ${tgt} APPEND_STRING PROPERTY LINK_FLAGS
            " -Wl,--nxcompat -Wl,--dynamicbase -Wl,--high-entropy-va")
    endif()
    if(MSVC)
        target_compile_options(${tgt} PRIVATE /guard:cf)
        set_property(TARGET ${tgt} APPEND_STRING PROPERTY LINK_FLAGS
            " /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA /guard:cf")
        if(HAS_GUARD_EHCONT_SER)
            target_compile_options(${tgt} PRIVATE /guard:ehcont)
            set_property(TARGET ${tgt} APPEND_STRING PROPERTY LINK_FLAGS
                " /guard:ehcont /CETCOMPAT")
        endif()
    endif()
endfunction()

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
# The uint256_t subproject defaults its own BUILD_TESTS option to ON, which would
# unconditionally build upstream's uint256-tests executable even when we only want
# our own test binary. We must force it off before pulling the subdirectory in —
# but if the *parent* project (the one consuming serialization-cpp) happens to use
# a cache var named BUILD_TESTS for its own purposes, we must not clobber it.
# Save, override, add_subdirectory, then restore.
get_property(_serial_prev_BUILD_TESTS_type CACHE BUILD_TESTS PROPERTY TYPE)
if(_serial_prev_BUILD_TESTS_type)
    get_property(_serial_prev_BUILD_TESTS_value CACHE BUILD_TESTS PROPERTY VALUE)
    get_property(_serial_prev_BUILD_TESTS_help  CACHE BUILD_TESTS PROPERTY HELPSTRING)
endif()
set(BUILD_TESTS OFF CACHE BOOL "Disable uint256_t upstream tests" FORCE)
add_subdirectory(external/uint256_t)
if(_serial_prev_BUILD_TESTS_type)
    set(BUILD_TESTS "${_serial_prev_BUILD_TESTS_value}"
        CACHE "${_serial_prev_BUILD_TESTS_type}" "${_serial_prev_BUILD_TESTS_help}" FORCE)
else()
    unset(BUILD_TESTS CACHE)
endif()
unset(_serial_prev_BUILD_TESTS_type)
unset(_serial_prev_BUILD_TESTS_value)
unset(_serial_prev_BUILD_TESTS_help)
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

# For building serialization-static itself — NOT system (you want to see your own warnings)
target_include_directories(serialization-static PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
# For consumers — SYSTEM so they don't get warnings from our headers
target_include_directories(serialization-static SYSTEM INTERFACE
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

# Apply sanitizer (if any) PRIVATELY to the static library so link-time ABI
# matches the sanitized test/fuzz executables. No effect when SANITIZE=none.
serialization_apply_sanitizer(serialization-static)

# Tests
if(SERIALIZATION_BUILD_TESTS)
    enable_testing()

    add_executable(serialization-tests test/test.cpp)
    target_link_libraries(serialization-tests PRIVATE serialization-static)

    set_target_properties(serialization-tests PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        OUTPUT_NAME "serialization-tests"
    )

    serialization_apply_exe_hardening(serialization-tests)
    serialization_apply_sanitizer(serialization-tests)

    add_test(NAME serialization-tests COMMAND serialization-tests)
    set_tests_properties(serialization-tests PROPERTIES TIMEOUT 300)
endif()

# Fuzz harnesses
if(SERIALIZATION_BUILD_FUZZ)
    enable_testing()

    set(_fuzz_sources
        fuzz/fuzz_varint.cpp
        fuzz/fuzz_hex.cpp
        fuzz/fuzz_pack_unpack.cpp
        fuzz/fuzz_deserializer.cpp
        fuzz/fuzz_deserializer_hexctor.cpp
        fuzz/fuzz_pod.cpp
        fuzz/fuzz_svec.cpp
        fuzz/fuzz_json.cpp
        fuzz/fuzz_string_helper.cpp
        fuzz/fuzz_roundtrip.cpp
        fuzz/fuzz_mixed_roundtrip.cpp
        fuzz/fuzz_cursor_ops.cpp
    )

    foreach(_fuzz_src IN LISTS _fuzz_sources)
        get_filename_component(_fuzz_name ${_fuzz_src} NAME_WE)
        add_executable(${_fuzz_name} ${_fuzz_src})
        target_link_libraries(${_fuzz_name} PRIVATE serialization-static)
        target_include_directories(${_fuzz_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/fuzz
        )
        set_target_properties(${_fuzz_name} PROPERTIES
            CXX_STANDARD 17
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
        )

        # Fuzz harnesses use the same warning policy as the library but the
        # random test scaffolding relies on some conversions the library code
        # avoids. Keep -Wall/-Wextra but relax -Wconversion/-Wsign-conversion
        # here so the fuzz drivers don't trip their own guard rails.
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${_fuzz_name} PRIVATE
                -Wall -Wextra -Wpedantic
            )
        elseif(MSVC)
            target_compile_options(${_fuzz_name} PRIVATE
                /W3 /wd4267 /wd4244 /wd4804 /wd4996
            )
        endif()

        serialization_apply_exe_hardening(${_fuzz_name})
        serialization_apply_sanitizer(${_fuzz_name})

        # Register as a CTest case. 5k iterations is a smoke run -- the long
        # fuzz campaign is invoked manually with SERIALIZE_FUZZ_ITERS=1000000.
        add_test(NAME ${_fuzz_name} COMMAND ${_fuzz_name})
        set_tests_properties(${_fuzz_name} PROPERTIES
            ENVIRONMENT "SERIALIZE_FUZZ_ITERS=5000;SERIALIZE_FUZZ_SEED=0xC0FFEE"
            TIMEOUT 180
        )
    endforeach()
endif()
