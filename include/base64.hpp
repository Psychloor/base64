
#ifndef BASE64_HPP
#define BASE64_HPP

#include <array>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace base64 {

// Error handling
enum class error : uint8_t {
    success = 0,
    empty_data,
    invalid_length,
    invalid_character,
    invalid_character_set_length,
    invalid_character_set_padding_char_used
};

namespace detail {
    struct error_category final : std::error_category {
        [[nodiscard]] const char* name() const noexcept override {
            return "base64";
        }

        [[nodiscard]] std::string message(int ev) const override {
            switch (static_cast<error>(ev)) {
                using enum error;
                case empty_data:
                    return "Input data is empty";
                case invalid_length:
                    return "Invalid input length";
                case invalid_character:
                    return "Invalid character in input";
                case invalid_character_set_length:
                    return "Character set must be 64 characters";
                case invalid_character_set_padding_char_used:
                    return "Padding character '=' is not allowed in character set";
                default:
                    return "Unknown error";
            }
        }
    };

    [[nodiscard]] inline const std::error_category& get_error_category() noexcept {
        static const error_category category{};
        return category;
    }
} // namespace detail

[[nodiscard]] inline std::error_code make_error_code(error e) noexcept {
    return {static_cast<int>(e), detail::get_error_category()};
}

// Character sets
inline constexpr std::string_view base64_chars{
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};

inline constexpr std::string_view base64_chars_url_safe{
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"};

// Main functionality
using encode_result = std::expected<std::string, std::error_code>;
using decode_result = std::expected<std::vector<std::byte>, std::error_code>;

namespace detail {
    [[nodiscard]] constexpr bool validate_charset(std::string_view chars) noexcept {
        return chars.size() == 64 && chars.find('=') == std::string_view::npos;
    }

    template<typename T>
    [[nodiscard]] constexpr auto make_unexpected(error e) {
        return std::unexpected(make_error_code(e));
    }
} // namespace detail

/**
 * @brief Encodes a sequence of bytes into a Base64-encoded string.
 * 
 * @param input Bytes to encode
 * @param chars Character set to use (default: standard Base64)
 * @return result Encoded string or error
 */
[[nodiscard]] inline encode_result base64_encode(
    const std::span<const std::byte> input,
    const std::string_view chars = base64_chars)
{
    if (input.empty())
        return detail::make_unexpected<std::string>(error::empty_data);

    if (!detail::validate_charset(chars))
        return detail::make_unexpected<std::string>(
            input.empty() ? error::empty_data :
            chars.size() != 64 ? error::invalid_character_set_length :
            error::invalid_character_set_padding_char_used);

    std::string result;
    result.reserve(((input.size() + 2) / 3) * 4);

    for (size_t i = 0; i < input.size(); i += 3) {
        uint32_t chunk = static_cast<uint32_t>(std::to_integer<uint8_t>(input[i])) << 16;
        
        if (i + 1 < input.size())
            chunk |= static_cast<uint32_t>(std::to_integer<uint8_t>(input[i + 1])) << 8;
        if (i + 2 < input.size())
            chunk |= static_cast<uint32_t>(std::to_integer<uint8_t>(input[i + 2]));

        result += chars[(chunk & 0x00FC0000) >> 18];
        result += chars[(chunk & 0x0003F000) >> 12];
        result += i + 1 < input.size() ? chars[(chunk & 0x00000FC0) >> 6] : '=';
        result += i + 2 < input.size() ? chars[(chunk & 0x0000003F)] : '=';
    }

    return result;
}

/**
 * @brief Decodes a Base64-encoded string into bytes.
 * 
 * @param input Base64-encoded string
 * @param chars Character set to use (default: standard Base64)
 * @return decode_result Decoded bytes or error
 */
[[nodiscard]] inline decode_result base64_decode(
    const std::string_view input,
    const std::string_view chars = base64_chars)
{
    if (input.empty())
        return detail::make_unexpected<std::vector<std::byte>>(error::empty_data);

    if (!detail::validate_charset(chars))
        return detail::make_unexpected<std::vector<std::byte>>(
            chars.size() != 64 ? error::invalid_character_set_length :
            error::invalid_character_set_padding_char_used);

    if (input.size() % 4 != 0)
        return detail::make_unexpected<std::vector<std::byte>>(error::invalid_length);

    // Create decode lookup table
    std::array<uint8_t, 256> decode_table{};
    decode_table.fill(0xFF);
    for (uint8_t i = 0; i < 64; ++i)
        decode_table[static_cast<uint8_t>(chars[i])] = i;
    decode_table[static_cast<uint8_t>('=')] = 0;

    std::vector<std::byte> result;
    result.reserve((input.size() * 3) / 4);

    for (size_t i = 0; i < input.size(); i += 4) {
        const std::array<uint8_t, 4> v{
            decode_table[static_cast<uint8_t>(input[i])],
            decode_table[static_cast<uint8_t>(input[i + 1])],
            decode_table[static_cast<uint8_t>(input[i + 2])],
            decode_table[static_cast<uint8_t>(input[i + 3])]
        };

        if (v[0] == 0xFF || v[1] == 0xFF ||
            (input[i + 2] != '=' && v[2] == 0xFF) ||
            (input[i + 3] != '=' && v[3] == 0xFF)) {
            return detail::make_unexpected<std::vector<std::byte>>(error::invalid_character);
        }

        const uint32_t chunk = (static_cast<uint32_t>(v[0]) << 18) |
                              (static_cast<uint32_t>(v[1]) << 12) |
                              (static_cast<uint32_t>(v[2]) << 6) |
                               static_cast<uint32_t>(v[3]);

        result.push_back(static_cast<std::byte>((chunk >> 16) & 0xFF));
        if (input[i + 2] != '=')
            result.push_back(static_cast<std::byte>((chunk >> 8) & 0xFF));
        if (input[i + 3] != '=')
            result.push_back(static_cast<std::byte>(chunk & 0xFF));
    }

    return result;
}

} // namespace base64

// Enable automatic conversion to std::error_code
template<>
struct std::is_error_code_enum<base64::error> : std::true_type {};

#endif // BASE64_HPP