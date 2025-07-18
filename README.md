# Modern C++ Base64 Library

A header-only C++23 implementation of Base64 encoding and decoding with extensive error handling and URL-safe support.

## Features

- Header-only implementation
- Modern C++23 features (`std::expected`, `std::span`, `constinit`)
- Comprehensive error handling
- URL-safe encoding support
- Custom character set support
- Extensive test coverage
- Zero dependencies (beyond C++23 standard library)
- CMake installation support with proper config files

## Requirements

- C++23 compatible compiler
- CMake 3.31 or higher

## Installation

### Method 1: Header-only
This is a header-only library. Simply copy `include/base64.hpp` to your project and include it.

### Method 2: CMake FetchContent
```
cmake
include(FetchContent)
FetchContent_Declare(
base64
GIT_REPOSITORY https://github.com/Psychloor/base64.git
GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(base64)

target_link_libraries(your_target PRIVATE base64::base64)
```
### Method 3: CMake Subdirectory
```
cmake
add_subdirectory(base64)
target_link_libraries(your_target PRIVATE base64::base64)
```
### Method 4: System Installation
```
bash
# Clone the repository
git clone https://github.com/Psychloor/base64.git
cd base64

# Create build directory
mkdir build && cd build

# Configure with custom install location (optional)
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local

# Build and install
cmake --build .
sudo cmake --install .
```
Then in your project's CMakeLists.txt:
```
cmake
find_package(base64 REQUIRED)
target_link_libraries(your_target PRIVATE base64::base64)
```
## Usage
```
cpp
#include <base64.hpp>

// Basic encoding
std::string data = "Hello, World!";
auto bytes = std::as_bytes(std::span{data});
auto encoded = base64::base64_encode(bytes);
if (encoded) {
std::cout << "Encoded: " << encoded.value() << '\n';
// Output: "SGVsbG8sIFdvcmxkIQ=="
}

// Basic decoding
auto decoded = base64::base64_decode("SGVsbG8sIFdvcmxkIQ==");
if (decoded) {
std::string result(reinterpret_cast<const char*>(decoded.value().data()),
decoded.value().size());
std::cout << "Decoded: " << result << '\n';
// Output: "Hello, World!"
}

// URL-safe encoding
auto url_safe = base64::base64_encode(bytes, base64::base64_chars_url_safe);
```
## Error Handling

The library uses `std::expected` for error handling. Possible errors:

- `empty_data`: Input data is empty
- `invalid_length`: Input has invalid length (for decoding)
- `invalid_character`: Invalid character in input
- `invalid_character_set_length`: Custom character set isn't 64 characters
- `invalid_character_set_padding_char_used`: Padding character in custom set

## Building and Testing
```
bash
# Clone the repository
git clone https://github.com/Psychloor/base64.git
cd base64

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .

# Run tests
ctest --output-on-failure

# Or use the convenience target
cmake --build . --target run_tests
```
## Project Structure
```

base64/
├── .gitignore
├── CMakeLists.txt
├── README.md
├── cmake/
│   └── base64Config.cmake.in
├── include/
│   └── base64.hpp
└── tests/
├── base64_test.cpp
└── base64_test.hpp
```
## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

MIT License

## Version History

- 1.0.0
  - Initial release
  - Base64 encoding/decoding with error handling
  - URL-safe encoding support
  - CMake installation support
