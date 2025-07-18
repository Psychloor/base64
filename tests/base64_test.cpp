#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "base64_test.hpp"
#include "doctest/parts/doctest_fwd.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <random>
#include <thread>

using namespace test_helpers;

TEST_SUITE("Base64 Encoding and Decoding")
{
    TEST_CASE("Encoding basic strings")
    {
        // Test case 1: Simple ASCII string
        std::string input = "Hello, World!";
        auto result = base64::base64_encode(string_to_bytes(input));
        REQUIRE(result.has_value());
        CHECK(result.value() == "SGVsbG8sIFdvcmxkIQ==");

        // Test case 2: Empty string should return error
        std::vector<std::byte> empty;
        auto empty_result = base64::base64_encode(empty);
        CHECK(!empty_result.has_value());
        CHECK(empty_result.error() == base64::error::empty_data);

        // Test case 3: Single character
        std::string single = "A";
        auto single_result = base64::base64_encode(string_to_bytes(single));
        REQUIRE(single_result.has_value());
        CHECK(single_result.value() == "QQ==");
    }

    TEST_CASE("Decoding basic strings")
    {
        // Test case 1: Simple ASCII string
        auto result = base64::base64_decode("SGVsbG8sIFdvcmxkIQ==");
        REQUIRE(result.has_value());
        CHECK(bytes_to_string(result.value()) == "Hello, World!");

        // Test case 2: Empty string
        auto empty_result = base64::base64_decode("");
        CHECK(!empty_result.has_value());
        CHECK(empty_result.error() == base64::error::empty_data);

        // Test case 3: Invalid length
        auto invalid_length = base64::base64_decode("SGVsbG8");
        CHECK(!invalid_length.has_value());
        CHECK(invalid_length.error() == base64::error::invalid_length);
    }

    TEST_CASE("URL-safe encoding")
    {
        std::string input = "Hello?World!/+";
        auto result = base64::base64_encode(string_to_bytes(input),
                                            base64::base64_chars_url_safe);
        REQUIRE(result.has_value());
        CHECK(result.value().find('+') == std::string::npos);
        CHECK(result.value().find('/') == std::string::npos);
    }

    TEST_CASE("Invalid character handling")
    {
        auto invalid_char = base64::base64_decode("SGVs!G8=");
        CHECK(!invalid_char.has_value());
        CHECK(invalid_char.error() == base64::error::invalid_character);
    }

    TEST_CASE("Custom character set validation")
    {
        std::string input = "Test";
        auto result = base64::base64_encode(string_to_bytes(input), "ABC");
        CHECK(!result.has_value());
        CHECK(result.error() == base64::error::invalid_character_set_length);

        result = base64::base64_encode(string_to_bytes(input),
                                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ=bcdefghijklmnopqrstuvwxyz0123456789+/");
        CHECK(!result.has_value());
        CHECK(result.error() == base64::error::
            invalid_character_set_padding_char_used);
    }

    TEST_CASE("Round-trip testing")
    {
        std::vector<std::string> test_strings = {
            "a", "ab", "abc", "abcd", "abcde", "abcdef"
        };

        for (const auto& test_str : test_strings)
        {
            auto encoded = base64::base64_encode(string_to_bytes(test_str));
            REQUIRE(encoded.has_value());

            auto decoded = base64::base64_decode(encoded.value());
            REQUIRE(decoded.has_value());

            CHECK(bytes_to_string(decoded.value()) == test_str);
        }
    }

    TEST_SUITE("File Operations")
    {
        class temp_file
        {
            std::filesystem::path path_;

        public:
            explicit temp_file(const std::vector<std::byte>& content)
            {
                auto now = std::chrono::system_clock::now().time_since_epoch();
                auto count = std::chrono::duration_cast<
                    std::chrono::nanoseconds>(now).count();

                path_ = std::filesystem::temp_directory_path() /
                    ("base64_test_" + std::to_string(count));

                std::ofstream file(path_, std::ios::binary);
                if (!file.is_open())
                {
                    throw std::runtime_error("Failed to create temporary file");
                }

                if (!content.empty())
                {
                    file.write(reinterpret_cast<const char*>(content.data()),
                               static_cast<std::streamsize>(content.size()));
                }
                file.close();

                // Verify the file was created
                if (!std::filesystem::exists(path_))
                {
                    throw std::runtime_error("Failed to create temporary file");
                }
            }

            ~temp_file()
            {
                std::error_code ec;
                std::filesystem::remove(path_, ec);
            }

            [[nodiscard]] const auto& path() const { return path_; }
        };

        TEST_CASE("Basic file operations")
        {
            const std::string content = "Hello, World!";
            temp_file test_file(string_to_bytes(content));

            auto result = base64::base64_encode_file(test_file.path());
            REQUIRE(result.has_value());
            CHECK(*result == "SGVsbG8sIFdvcmxkIQ==");
        }

        TEST_CASE("Empty and small files")
        {
            temp_file empty_file({});
            auto result = base64::base64_encode_file(empty_file.path());
            CHECK(!result.has_value());

            // The error could be either io_error or empty_data depending on whether
            // the file was created successfully but is empty, or if there was an IO error
            CHECK((result.error() == base64::error::io_error ||
                result.error() == base64::error::empty_data));

            temp_file one_byte_file({std::byte{65}});
            result = base64::base64_encode_file(one_byte_file.path());
            REQUIRE(result.has_value());
            CHECK(*result == "QQ==");
        }

        TEST_CASE("Error conditions")
        {
            CHECK(!base64::base64_encode_file("nonexistent.file").has_value());

            temp_file test_file(string_to_bytes("test"));
            auto result = base64::base64_encode_file(
                test_file.path(), "invalid");
            CHECK(!result.has_value());
            CHECK(result.error() == base64::error::invalid_character_set_length)
            ;
        }

        TEST_CASE("File to file encoding")
        {
            const std::string content = "Hello, World!";
            temp_file input_file(string_to_bytes(content));
            temp_file output_file({});

            auto error = base64::base64_encode_file_to_file(
                input_file.path(), output_file.path());
            CHECK(!error);
        }

        TEST_CASE("Binary data handling")
        {
            std::vector<std::byte> binary_data{
                std::byte{0xFF}, std::byte{0x00}, std::byte{0x80},
                std::byte{0x7F}
            };

            auto encoded = base64::base64_encode(binary_data);
            REQUIRE(encoded.has_value());

            auto decoded = base64::base64_decode(encoded.value());
            REQUIRE(decoded.has_value());
            CHECK(decoded.value() == binary_data);
        }

        TEST_CASE("Large data handling")
        {
            // Create 1MB test data
            constexpr size_t size = 1024 * 1024;
            std::vector<std::byte> large_data(size);

            // Fill with a repeating pattern
            for (size_t i = 0; i < size; ++i)
            {
                large_data[i] = std::byte{static_cast<unsigned char>(i % 256)};
            }

            auto encoded = base64::base64_encode(large_data);
            REQUIRE(encoded.has_value());

            auto decoded = base64::base64_decode(encoded.value());
            REQUIRE(decoded.has_value());
            CHECK(decoded.value() == large_data);
        }

        TEST_CASE("Padding variations")
        {
            // Test different input lengths to cover all padding cases
            std::vector<std::vector<std::byte>> test_cases = {
                {std::byte{'f'}}, // 2 padding chars
                {std::byte{'f'}, std::byte{'o'}}, // 1 padding char
                {std::byte{'f'}, std::byte{'o'}, std::byte{'o'}} // no padding
            };

            for (const auto& test_case : test_cases)
            {
                auto encoded = base64::base64_encode(test_case);
                REQUIRE(encoded.has_value());

                auto decoded = base64::base64_decode(encoded.value());
                REQUIRE(decoded.has_value());
                CHECK(decoded.value() == test_case);

                // Check the padding length is correct
                const size_t padding_count = std::ranges::count(
                    encoded.value(), '=');
                CHECK(padding_count == (3 - (test_case.size() % 3)) % 3);
            }
        }

        TEST_CASE("Unicode string handling")
        {
            std::string unicode_str = "Hello, 世界! 🌍";  // Most other compilers default to UTF-8

            auto bytes = string_to_bytes(unicode_str);

            auto encoded = base64::base64_encode(bytes);
            REQUIRE(encoded.has_value());

            auto decoded = base64::base64_decode(encoded.value());
            REQUIRE(decoded.has_value());
            CHECK(bytes_to_string(decoded.value()) == unicode_str);
        }

        // Add these inside the "File Operations" test suite
        TEST_SUITE("File Operations")
        {
            // ... existing code ...

            TEST_CASE("Large file operations")
            {
                // Create 2MB of test data
                std::vector<std::byte> large_data(2 * 1024 * 1024);
                std::mt19937 gen(std::random_device{}());
                std::uniform_int_distribution<> dis(0, 255);
                std::ranges::generate(large_data,
                                      [&]()
                                      {
                                          return std::byte{
                                              static_cast<unsigned char>(dis(
                                                  gen))
                                          };
                                      });

                temp_file large_file(large_data);

                auto result = base64::base64_encode_file(large_file.path());
                REQUIRE(result.has_value());

                // Verify the encoded data
                auto decoded = base64::base64_decode(result.value());
                REQUIRE(decoded.has_value());
                CHECK(decoded.value() == large_data);
            }

            TEST_CASE("Chunk size variations")
            {
                std::string content =
                    "Hello, World! This is a test of different chunk sizes.";
                temp_file test_file(string_to_bytes(content));

                // Test with different chunk sizes
                std::vector<size_t> chunk_sizes = {
                    1, // Minimum
                    16, // Small
                    1024, // Medium
                    65536 // Large
                };

                for (size_t chunk_size : chunk_sizes)
                {
                    auto result = base64::base64_encode_file(test_file.path(),
                        base64::base64_chars,
                        chunk_size);
                    REQUIRE(result.has_value());

                    // Verify the encoded content is correct regardless of chunk size
                    auto decoded = base64::base64_decode(result.value());
                    REQUIRE(decoded.has_value());
                    CHECK(bytes_to_string(decoded.value()) == content);
                }
            }

            TEST_CASE("File to file with different character sets")
            {
                std::string content = "Hello+World/This?Is=A+Test/";
                temp_file input_file(string_to_bytes(content));
                temp_file output_file({});

                // Test with URL-safe character set
                auto error = base64::base64_encode_file_to_file(
                    input_file.path(),
                    output_file.path(),
                    base64::base64_chars_url_safe);

                CHECK(!error);

                // Read and verify the output file
                std::ifstream file(output_file.path());
                std::string encoded((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());

                CHECK(encoded.find('+') == std::string::npos);
                CHECK(encoded.find('/') == std::string::npos);
            }
        }
    }
}
