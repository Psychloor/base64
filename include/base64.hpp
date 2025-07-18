#ifndef BASE64_HPP
#define BASE64_HPP

#include <array>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace base64
{
    /**
     * @var base64_chars
     * @brief A constant string view representing the standard Base64 character
     * set.
     *
     * This string contains the 64 characters used for Base64 encoding. The
     * characters are arranged in the required order as per the Base64 encoding
     * standard: uppercase letters (A-Z), lowercase letters (a-z), digits (0-9),
     * and the two special characters '+' and '/'.
     *
     * This variable is used as the default character set for Base64 encoding
     * and decoding operations.
     */
    static constinit std::string_view base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    /**
     * @var base64_chars_url_safe
     * @brief A constant string view representing the URL-safe Base64 character
     * set.
     *
     * This string contains the 64 characters used for Base64 encoding in a
     * URL-safe format. The characters are ordered as follows: uppercase letters
     * (A-Z), lowercase letters (a-z), digits (0-9), and the two special
     * characters '-' and '_', replacing
     * '+' and '/' from the standard Base64 character set.
     *
     * This variable is primarily used for encoding and decoding Base64 data
     * where URL or filename compatibility is required.
     */
    static constinit std::string_view base64_chars_url_safe =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

    /**
     * @enum base64_result
     * @brief Enumeration representing possible outcomes or error codes for
     * Base64 encoding and decoding operations.
     *
     * This enumeration is used to communicate the result or specific errors
     * that might arise during Base64 processing. It includes the following
     * values:
     *
     * - empty_data: Indicates that the input data is empty.
     * - invalid_length: Indicates that the input data has an invalid length for
     * Base64 encoding or decoding.
     * - invalid_character: Indicates the presence of an invalid character in
     * the Base64 input data.
     * - invalid_character_set_length: Indicates that the provided Base64
     * character set does not have the required length of 64 characters.
     * - Invalid_character_set_padding_char_used: Indicates that the padding
     * character '=' was found in the provided Base64 character set, which is
     * invalid.
     */
    enum class base64_result : uint8_t
    {
        empty_data,
        invalid_length,
        invalid_character,
        invalid_character_set_length,
        invalid_character_set_padding_char_used
    };

    /**
     * @brief Encodes a given sequence of bytes into a Base64-encoded string.
     *
     * This function takes a span of bytes as input and converts it into a
     * Base64-encoded string using the provided Base64 character set.
     * Optionally, a custom character set can be supplied. The function returns
     * a standard conformant `std::expected` object, which either contains the
     * resulting Base64 string or an appropriate error code from the
     * `base64_result` enumeration in case of failure.
     *
     * The function performs the following validations before encoding:
     * - Ensures that the input data is not empty.
     * - Checks that the supplied character set has exactly 64 characters.
     * - Verifies that the padding character '=' is not included in the custom
     *   character set.
     *
     * @param input A span of bytes to be encoded in Base64 format.
     * @param chars An optional parameter defining the Base64 character set to
     * be used for encoding. Defaults to `base64_chars`.
     * @return A `std::expected<std::string, base64_result>` containing the
     *         Base64-encoded string on success or an appropriate
     * `base64_result` error.
     *
     * Possible error codes:
     * - base64_result::empty_data: If the input data is empty.
     * - base64_result::invalid_character_set_length: If the character set does
     *   not consist of 64 characters.
     * - Base64_result::invalid_character_set_padding_char_used: If the padding
     *   character '=' is found in the character set.
     */
    [[nodiscard]] inline std::expected<std::string, base64_result>
    base64_encode(const std::span<const std::byte> input,
                  const std::string_view chars = base64_chars)
    {
        if (input.empty())
            return std::unexpected(base64_result::empty_data);

        if (chars.size() != 64)
        {
            return std::unexpected(base64_result::invalid_character_set_length);
        }
        if (chars.find('=') != std::string_view::npos)
        {
            return std::unexpected(
                base64_result::invalid_character_set_padding_char_used);
        }

        std::string result;
        result.reserve(((input.size() + 2) / 3) * 4);

        for (size_t i = 0; i < input.size(); i += 3)
        {
            uint32_t chunk =
                static_cast<uint32_t>(std::to_integer<uint8_t>(input[i])) << 16;
            if (i + 1 < input.size())
            {
                chunk |= static_cast<uint32_t>(
                             std::to_integer<uint8_t>(input[i + 1]))
                    << 8;
            }
            if (i + 2 < input.size())
            {
                chunk |= static_cast<uint32_t>(
                    std::to_integer<uint8_t>(input[i + 2]));
            }

            result.push_back(chars[(chunk & 0x00FC0000) >> 18]);
            result.push_back(chars[(chunk & 0x0003F000) >> 12]);
            result.push_back(
                i + 1 < input.size() ? chars[(chunk & 0x00000FC0) >> 6] : '=');
            result.push_back(i + 2 < input.size() ? chars[(chunk & 0x0000003F)]
                                                  : '=');
        }

        return result;
    }

    /**
     * @brief Decodes a Base64-encoded string into its original binary
     * representation.
     *
     * This function takes an input string encoded in Base64 format and converts
     * it back into its original binary data. It can also use a custom Base64
     * character set for decoding if provided.
     *
     * @param input The Base64-encoded string to decode. Must not be empty and
     * must have a length that is a multiple of 4.
     * @param chars Optional parameter specifying the Base64 character set to be
     * used for decoding. The default value is the standard Base64 character
     * set. This must be a string of exactly 64 unique characters, and it must
     * not contain the padding character '='.
     * @return A std::expected containing the decoded binary data as a vector of
     * std::byte on success, or a base64_result error code indicating why the
     * decoding failed:
     * - base64_result::empty_data: If the input string is empty.
     * - base64_result::invalid_length: If the input string length is not a
     * multiple of 4.
     * - Base64_result::invalid_character: If the input string contains an
     * invalid character for the specified Base64 character set.
     * - Base64_result::invalid_character_set_length: If the provided Base64
     *   character set does not contain exactly 64 characters.
     * - Base64_result::invalid_character_set_padding_char_used: If the padding
     *   character '=' is found in the provided Base64 character set.
     */
    [[nodiscard]] inline std::expected<std::vector<std::byte>, base64_result>
    base64_decode(const std::string_view input,
                  const std::string_view chars = base64_chars)
    {
        if (input.empty())
            return std::unexpected(base64_result::empty_data);
        if (chars.size() != 64)
        {
            return std::unexpected(base64_result::invalid_character_set_length);
        }
        if (chars.find('=') != std::string_view::npos)
        {
            return std::unexpected(
                base64_result::invalid_character_set_padding_char_used);
        }

        // Build decode table from the provided base64 character set
        std::array<uint8_t, 256> decode_table{};
        decode_table.fill(0xFF); // 0xFF = invalid

        for (uint8_t i = 0; i < 64; ++i)
            decode_table[static_cast<uint8_t>(chars[i])] = i;

        decode_table[static_cast<uint8_t>('=')] = 0; // optional padding value

        if (input.empty() || input.size() % 4 != 0)
            return std::unexpected(base64_result::invalid_length);

        std::vector<std::byte> result;
        result.reserve((input.size() * 3) / 4);

        for (size_t i = 0; i < input.size(); i += 4)
        {
            const auto c0 = static_cast<uint8_t>(input[i]);
            const auto c1 = static_cast<uint8_t>(input[i + 1]);
            const auto c2 = static_cast<uint8_t>(input[i + 2]);
            const auto c3 = static_cast<uint8_t>(input[i + 3]);

            const uint8_t v0 = decode_table[c0];
            const uint8_t v1 = decode_table[c1];
            const uint8_t v2 = decode_table[c2];
            const uint8_t v3 = decode_table[c3];

            if (v0 == 0xFF || v1 == 0xFF ||
                (input[i + 2] != '=' && v2 == 0xFF) ||
                (input[i + 3] != '=' && v3 == 0xFF))
            {
                return std::unexpected(base64_result::invalid_character);
            }

            const uint32_t chunk = (static_cast<uint32_t>(v0) << 18) |
                (static_cast<uint32_t>(v1) << 12) |
                (static_cast<uint32_t>(v2) << 6) | static_cast<uint32_t>(v3);

            result.push_back(static_cast<std::byte>((chunk >> 16) & 0xFF));
            if (input[i + 2] != '=')
                result.push_back(static_cast<std::byte>((chunk >> 8) & 0xFF));
            if (input[i + 3] != '=')
                result.push_back(static_cast<std::byte>(chunk & 0xFF));
        }

        return result;
    }
} // namespace base64

#endif // BASE64_HPP
