# serialization-cpp

[![CI Build Tests](https://github.com/gibme-c/serialization-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/gibme-c/serialization-cpp/actions/workflows/ci.yml)

A C++17 static library for binary serialization and JSON support. Provides a writer/reader pair (`serializer_t`/`deserializer_t`) for packing typed values into byte vectors, an abstract `Serializable` interface for custom types, and RapidJSON-based JSON helpers with convenience macros.

## Features

- **Binary serialization** — `serializer_t` writes and `deserializer_t` reads typed values (integers, booleans, byte arrays, hex strings, varints) to/from `std::vector<unsigned char>` buffers, with optional big-endian support
- **Variable-length integers** — `encode_varint`/`decode_varint` for compact encoding of small values
- **128/256-bit integers** — native support for `uint128_t` and `uint256_t` via the [uint256_t](https://github.com/calccrypto/uint256_t) library
- **JSON support** — RapidJSON wrapper with typed getters and macros (`LOAD_KEY_FROM_JSON`, `KEY_TO_JSON`, `JSON_OBJECT_CONSTRUCTOR`, etc.) for declarative field-to-JSON mapping
- **Abstract types** — `Serializable` interface, `SerializablePod<N>` fixed-size byte wrapper with hex conversion and secure erasure, `SerializableVector<T>` for vectors of serializable types
- **Secure erasure** — `serialization_secure_erase()` zeroes memory using `SecureZeroMemory` (MSVC) or an inline asm barrier (GCC/Clang) to prevent dead-store elimination
- **String utilities** — `from_hex`, `to_hex`, `str_split`, `str_join`, `str_pad`, `str_trim`
- **Build hardening** — stack protectors, control flow integrity (CET), ASLR, DEP, format security, and symbol visibility across GCC, Clang, MSVC, and MinGW

## Building

Requires CMake 3.10+ and a C++17 compiler. Submodules must be initialized first.

```bash
git clone --recursive https://github.com/gibme-c/serialization-cpp
cd serialization-cpp

# Configure and build
cmake -S . -B build -DSERIALIZATION_BUILD_TESTS=ON
cmake --build build --config Release -j

# Run tests
./build/serialization-tests          # Linux / macOS / MinGW
./build/Release/serialization-tests  # Windows (MSVC)
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `SERIALIZATION_BUILD_TESTS` | `OFF` | Build the unit test executable (`serialization-tests`) |
| `SERIALIZATION_BUILD_FUZZ`  | `OFF` | Build the portable in-process fuzz harnesses |
| `SERIALIZATION_SANITIZE`    | `none` | Enable a sanitizer on tests/fuzz targets (`none`, `address`, `undefined`, `thread`, `address+undefined`). GCC/Clang only; no effect on MSVC. |
| `CMAKE_BUILD_TYPE`          | `Release` | `Debug`, `Release`, or `RelWithDebInfo` |

All three `SERIALIZATION_*` options are only defined when this project is the top-level CMake project. When consumed via `add_subdirectory()`, they are force-off and do nothing — downstream consumers never build our tests or fuzz harnesses.

### As a Dependency

```bash
git submodule add https://github.com/gibme-c/serialization-cpp external/serialization
git submodule update --init --recursive
```

```cmake
add_subdirectory(external/serialization)
target_link_libraries(your_target serialization-static)
```

All compiler warnings, hardening flags, and optimization settings are scoped `PRIVATE` and will not leak into your build. The only things that propagate are the public include paths and the `RAPIDJSON_HAS_STDSTRING` compile definition (required by the JSON headers).

## Usage

Include the umbrella header for everything:

```cpp
#include <serialization.h>
```

Or include individual headers:

```cpp
#include <serializer_t.h>
#include <deserializer_t.h>
#include <json_helper.h>
```

### Binary Serialization

```cpp
#include <serializer_t.h>
#include <deserializer_t.h>

// Write
Serialization::serializer_t writer;
writer.uint32(42);
writer.boolean(true);
writer.hex("deadbeef");
writer.varint(uint64_t(300));

// Read
Serialization::deserializer_t reader(writer.data(), writer.size());
auto val  = reader.uint32();
auto flag = reader.boolean();
auto hex  = reader.hex(4);
auto vi   = reader.varint<uint64_t>();
```

### Custom Serializable Types

Extend `Serializable` and implement the required methods:

```cpp
#include <serialization.h>

struct MyType : Serializable
{
    uint32_t id;
    std::string name;

    std::vector<unsigned char> serialize() const override;
    void deserialize(const std::vector<unsigned char> &data) override;
    size_t size() const override;
    std::string to_string() const override;

    // JSON support
    JSON_FROM_FUNC(fromJSON);
    JSON_TO_FUNC(toJSON);
};
```

For fixed-size data (hashes, keys, etc.), use `SerializablePod<N>`:

```cpp
using Hash256 = SerializablePod<32>;  // 32-byte fixed-size wrapper
```

### JSON Macros

```cpp
JSON_FROM_FUNC(fromJSON)
{
    JSON_PARSE(val, json);
    LOAD_KEY_FROM_JSON(id);
    LOAD_KEY_FROM_JSON(name);
}

JSON_TO_FUNC(toJSON)
{
    JSON_INIT();
    KEY_TO_JSON(id);
    KEY_TO_JSON(name);
    JSON_DUMP(writer);
}
```

## Architecture

### Core Types

| Type | Header | Purpose |
|------|--------|---------|
| `serializer_t` | `serializer_t.h` | Writes typed values into a byte vector |
| `deserializer_t` | `deserializer_t.h` | Reads typed values from a byte buffer with a cursor |
| `Serializable` | `serializable.h` | Abstract interface: `serialize`, `deserialize`, `toJSON`, `fromJSON`, `size`, `to_string` |
| `SerializablePod<N>` | `serializable_pod.h` | Fixed-size byte array wrapper with hex conversion, comparison operators, and secure erasure |
| `SerializableVector<T>` | `serializable_vector.h` | Vector wrapper for `Serializable` types |

### Serialization Pattern

The `serializer_t`/`deserializer_t` pair follows a symmetric writer/reader pattern: serialize fields in order, then deserialize in the same order. The deserializer maintains a cursor (`offset`) that advances with each read. All read methods accept a `peek` parameter to read without advancing.

### Dependencies

| Library | License | Purpose |
|---------|---------|---------|
| [RapidJSON](https://github.com/Tencent/rapidjson) | MIT | JSON parsing and generation (header-only) |
| [uint256_t](https://github.com/calccrypto/uint256_t) | MIT | 128/256-bit unsigned integer types |

Both are included as git submodules under `external/`.

## Testing

Build with `-DSERIALIZATION_BUILD_TESTS=ON` to get the `serialization-tests` executable. The test suite covers:

- **Round-trip tests** — serialize then deserialize for all primitive types (uint8 through uint256, boolean, bytes, hex, varint) in both little-endian and big-endian
- **Pod tests** — `SerializablePod` serialization, JSON round-trip, comparison operators, hex construction
- **Vector tests** — `SerializableVector` append, extend, serialization, JSON round-trip
- **Peek mode** — read without advancing the cursor
- **String helpers** — hex round-trip, split/join, pad, trim
- **Secure erasure** — verify memory is zeroed

## CI

GitHub Actions runs on every push, pull request, release, and daily schedule:

| Platform | Compilers |
|----------|-----------|
| Linux (x86_64) | GCC 11, GCC 12, Clang 14, Clang 15 |
| macOS (ARM64) | AppleClang, Homebrew Clang |
| Windows (x86_64) | MSVC, MinGW GCC |

## License

BSD-3-Clause. See [LICENSE](LICENSE) for the full text.

Dependencies are licensed separately: RapidJSON under MIT, uint256_t under MIT.
