# serialization-cpp: A simple C++ serialization library

The following *features* have been baked into this library:

* Builds with CMake making it easy to add to other projects
* GibHub Actions verifies that it builds on Ubuntu, Windows, and MacOS using various compilers
* Provides a `serializer_t` and `deserializer_t` structure for writing/reading complex data structures 
  packed into `unsigned char` (byte) vectors
* Includes [RapidJSON](https://github.com/Tencent/rapidjson) support for serializing/de-serializing to/from JSON
  * Including helper MACROS for common patterns
* Includes support for [uint256_t & uint128_t](https://github.com/calccrypto/uint256_t) value serialization
* Includes an abstract `Serializable` struct that other structures/classes should extend to ensure support with this
  library
* Includes an abstract `SerializablePod<#>` that creates a structured wrapper around a C++ POD
  * Templated constructor for various underlying byte size(s)
  * For more complex data structures, most methods should be overridden (or build your own)
* Includes a `serialization_secure_erase()` method that sets the underlying data storage to 0s without being optimized
  away by the compiler
* Includes various string helper methods such as:
  * `from_hex()` converts a hex encoded string to a vector of unsigned char (bytes)
  * `to_hex()` converts a data structure to a hex encoded string
  * `str_join()` joins a vector of strings together using the supplied delimiter
  * `str_pad()` pads a string with blank spaces up to the specified length
  * `str_split()` splits a string into a vector of strings using the specified delimiter
  * `str_trim()` trims any whitespace from both the start and end of the given string

## Documentation

C++ API documentation can be found in the headers (.h)

### Example Use

See `test/test.cpp` for a high level example

### Cloning the Repository

This repository uses submodules, make sure you pull those before doing anything if you are cloning the project.

```bash
git clone --recursive https://github.com/gibme-c/serialization-cpp
```

### As a dependency

```bash
git submodule add https://github.com/gibme-c/serialization-cpp external/serialization
git submodule update --init --recursive
```

## License

This wrapper is provided under the [BSD-3-Clause license](https://en.wikipedia.org/wiki/BSD_licenses) found in LICENSE.

* RapidJSON itself is licensed under the [MIT License](https://en.wikipedia.org/wiki/MIT_License)
  * Dependencies of RapidJSON are provided the under following license(s):
    * The [BSD-3-Clause license](https://en.wikipedia.org/wiki/BSD_licenses)
* The uint256_t library is licensed under the [MIT License](https://en.wikipedia.org/wiki/MIT_License)
