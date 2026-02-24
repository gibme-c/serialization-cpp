cmake_minimum_required(VERSION 3.10...3.31)

project(serializationcpp VERSION 2.0.0 LANGUAGES CXX)

# Default to Release for single-config generators
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

message(STATUS "Mode: ${CMAKE_BUILD_TYPE}")

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ccache support
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Found ccache package... Activating...")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# RapidJSON is header-only; this define enables std::string support
add_definitions(-DRAPIDJSON_HAS_STDSTRING)

option(BUILD_TEST "Build test binary" OFF)
if(DEFINED ENV{BUILD_TEST})
    set(BUILD_TEST $ENV{BUILD_TEST})
endif()
if(BUILD_TEST)
    message(STATUS "Test binary added to targets list")
endif()

# Compiler-specific flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267 /wd4804 /wd4996")
else()
    set(ARCH native CACHE STRING "CPU to build for: -march value or 'default' for none")
    if(NOT "${ARCH}" STREQUAL "default")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${ARCH}")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wuninitialized")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -Og")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O3")
endif()

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
target_link_libraries(serialization-static uint256t)
target_include_directories(serialization-static PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external
    ${CMAKE_CURRENT_SOURCE_DIR}/external/rapidjson/include
)

# Tests
if(BUILD_TEST)
    enable_testing()

    add_executable(serialization-tests test/test.cpp)
    target_link_libraries(serialization-tests serialization-static)
    set_property(TARGET serialization-tests PROPERTY OUTPUT_NAME "serialization-tests")

    add_test(NAME serialization-tests COMMAND serialization-tests)
endif()
