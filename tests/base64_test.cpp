#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "base64_test.hpp"
#include "doctest/parts/doctest_fwd.h"

using namespace test_helpers;

TEST_SUITE("Base64 Encoding and Decoding") {
    TEST_CASE("Encoding basic strings") {
        // Test case 1: Simple ASCII string
        std::string input = "Hello, World!";
        auto result = base64::base64_encode(string_to_bytes(input));
        REQUIRE(result.has_value());
        CHECK(result.value() == "SGVsbG8sIFdvcmxkIQ==");

        // Test case 2: Empty string should return error
        std::vector<std::byte> empty;
        auto empty_result = base64::base64_encode(empty);
        CHECK(!empty_result.has_value());
        CHECK(empty_result.error() == base64::base64_result::empty_data);

        // Test case 3: Single character
        std::string single = "A";
        auto single_result = base64::base64_encode(string_to_bytes(single));
        REQUIRE(single_result.has_value());
        CHECK(single_result.value() == "QQ==");
    }

    TEST_CASE("Decoding basic strings") {
        // Test case 1: Simple ASCII string
        auto result = base64::base64_decode("SGVsbG8sIFdvcmxkIQ==");
        REQUIRE(result.has_value());
        CHECK(bytes_to_string(result.value()) == "Hello, World!");

        // Test case 2: Empty string
        auto empty_result = base64::base64_decode("");
        CHECK(!empty_result.has_value());
        CHECK(empty_result.error() == base64::base64_result::empty_data);

        // Test case 3: Invalid length
        auto invalid_length = base64::base64_decode("SGVsbG8");
        CHECK(!invalid_length.has_value());
        CHECK(invalid_length.error() == base64::base64_result::invalid_length);
    }

    TEST_CASE("URL-safe encoding") {
        std::string input = "Hello?World!/+";
        auto result = base64::base64_encode(string_to_bytes(input), 
                                          base64::base64_chars_url_safe);
        REQUIRE(result.has_value());
        CHECK(result.value().find('+') == std::string::npos);
        CHECK(result.value().find('/') == std::string::npos);
    }

    TEST_CASE("Invalid character handling") {
        auto invalid_char = base64::base64_decode("SGVs!G8=");
        CHECK(!invalid_char.has_value());
        CHECK(invalid_char.error() == base64::base64_result::invalid_character);
    }

    TEST_CASE("Custom character set validation") {
        std::string input = "Test";
        auto result = base64::base64_encode(string_to_bytes(input), "ABC");
        CHECK(!result.has_value());
        CHECK(result.error() == base64::base64_result::invalid_character_set_length);

        result = base64::base64_encode(string_to_bytes(input),
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ=bcdefghijklmnopqrstuvwxyz0123456789+/");
        CHECK(!result.has_value());
        CHECK(result.error() == base64::base64_result::invalid_character_set_padding_char_used);
    }

    TEST_CASE("Round-trip testing") {
        std::vector<std::string> test_strings = {
            "a", "ab", "abc", "abcd", "abcde", "abcdef"
        };

        for(const auto& test_str : test_strings) {
            auto encoded = base64::base64_encode(string_to_bytes(test_str));
            REQUIRE(encoded.has_value());

            auto decoded = base64::base64_decode(encoded.value());
            REQUIRE(decoded.has_value());

            CHECK(bytes_to_string(decoded.value()) == test_str);
        }
    }
}