#ifndef BASE64_TEST_HPP
#define BASE64_TEST_HPP

#include "doctest/doctest.h"
#include "../include/base64.hpp"
#include <string>
#include <vector>

// Test helper functions
namespace test_helpers
{
    inline std::vector<std::byte> string_to_bytes(const std::string& str)
    {
        const auto byte_span = std::as_bytes(std::span{str});
        return {byte_span.begin(), byte_span.end()};
    }

    inline std::string bytes_to_string(const std::vector<std::byte>& bytes)
    {
        return {
            reinterpret_cast<const char*>(bytes.data()),
            bytes.size()
        };
    }
}


#endif // BASE64_TEST_HPP
