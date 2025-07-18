#ifndef BASE64_HPP
#define BASE64_HPP

#include <array>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace base64
{
    /**
     * @brief Enumeration representing various error codes for Base64 operations.
     *
     * Provides detailed error feedback for issues encountered during
     * Base64 encoding or decoding processes.
     *
     * @enum success                    Operation completed successfully.
     * @enum empty_data                 Input data is empty.
     * @enum invalid_length             Input data length is not valid for Base64 processing.
     * @enum invalid_character          Invalid character encountered in input.
     * @enum invalid_character_set_length Character set must contain exactly 64 characters.
     * @enum invalid_character_set_padding_char_used The Padding character '=' is not allowed in the character set.
     * @enum file_not_found             A specified file was not found.
     * @enum file_not_readable          File is not readable due to permissions or other restrictions.
     * @enum file_too_large             File size exceeds the maximum allowed size for processing.
     * @enum io_error                   General I/O error encountered while accessing a file.
     */
    enum class error : uint8_t
    {
        success = 0,
        empty_data,
        invalid_length,
        invalid_character,
        invalid_character_set_length,
        invalid_character_set_padding_char_used,
        file_not_found,
        file_not_readable,
        file_too_large,
        io_error
    };

    namespace detail
    {
        struct error_category final : std::error_category
        {
            [[nodiscard]] const char* name() const noexcept override
            {
                return "base64";
            }

            [[nodiscard]] std::string message(int ev) const override
            {
                switch (static_cast<error>(ev))
                {
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
                    return
                        "Padding character '=' is not allowed in character set";
                case file_not_found:
                    return "File not found";
                case file_not_readable:
                    return "File is not readable";
                case file_too_large:
                    return "File is too large to process";
                case io_error:
                    return "I/O error while reading file";

                default:
                    return "Unknown error";
                }
            }
        };

        [[nodiscard]] inline const std::error_category&
        get_error_category() noexcept
        {
            static const error_category category{};
            return category;
        }
    } // namespace detail

    [[nodiscard]] inline std::error_code make_error_code(error e) noexcept
    {
        return {static_cast<int>(e), detail::get_error_category()};
    }

    // Character sets
    inline constexpr std::string_view base64_chars{
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
    };

    inline constexpr std::string_view base64_chars_url_safe{
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"
    };

    // Main functionality
    using encode_result = std::expected<std::string, std::error_code>;
    using decode_result = std::expected<std::vector<std::byte>, std::error_code>
    ;

    namespace detail
    {
        [[nodiscard]] constexpr bool validate_charset(
            std::string_view chars) noexcept
        {
            return chars.size() == 64 && chars.find('=') ==
                std::string_view::npos;
        }

        template <typename T>
        [[nodiscard]] constexpr auto make_unexpected(error e)
        {
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
                input.empty()
                    ? error::empty_data
                    : chars.size() != 64
                    ? error::invalid_character_set_length
                    : error::invalid_character_set_padding_char_used);

        std::string result;
        result.reserve(((input.size() + 2) / 3) * 4);

        for (size_t i = 0; i < input.size(); i += 3)
        {
            uint32_t chunk = static_cast<uint32_t>(std::to_integer<uint8_t>(
                input[i])) << 16;

            if (i + 1 < input.size())
                chunk |= static_cast<uint32_t>(std::to_integer<uint8_t>(
                    input[i + 1])) << 8;
            if (i + 2 < input.size())
                chunk |= static_cast<uint32_t>(std::to_integer<uint8_t>(
                    input[i + 2]));

            result += chars[(chunk & 0x00FC0000) >> 18];
            result += chars[(chunk & 0x0003F000) >> 12];
            result += i + 1 < input.size()
                          ? chars[(chunk & 0x00000FC0) >> 6]
                          : '=';
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
            return detail::make_unexpected<std::vector<std::byte>>(
                error::empty_data);

        if (!detail::validate_charset(chars))
            return detail::make_unexpected<std::vector<std::byte>>(
                chars.size() != 64
                    ? error::invalid_character_set_length
                    : error::invalid_character_set_padding_char_used);

        if (input.size() % 4 != 0)
            return detail::make_unexpected<std::vector<std::byte>>(
                error::invalid_length);

        // Create decode lookup table
        std::array<uint8_t, 256> decode_table{};
        decode_table.fill(0xFF);
        for (uint8_t i = 0; i < 64; ++i)
            decode_table[static_cast<uint8_t>(chars[i])] = i;
        decode_table[static_cast<uint8_t>('=')] = 0;

        std::vector<std::byte> result;
        result.reserve((input.size() * 3) / 4);

        for (size_t i = 0; i < input.size(); i += 4)
        {
            const std::array<uint8_t, 4> v{
                decode_table[static_cast<uint8_t>(input[i])],
                decode_table[static_cast<uint8_t>(input[i + 1])],
                decode_table[static_cast<uint8_t>(input[i + 2])],
                decode_table[static_cast<uint8_t>(input[i + 3])]
            };

            if (v[0] == 0xFF || v[1] == 0xFF ||
                (input[i + 2] != '=' && v[2] == 0xFF) ||
                (input[i + 3] != '=' && v[3] == 0xFF))
            {
                return detail::make_unexpected<std::vector<std::byte>>(
                    error::invalid_character);
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

    namespace detail
    {
        // Optimal chunk size (multiple of 3 for base64 encoding efficiency)
        constexpr size_t default_chunk_size = 48 * 1024; // 48KB chunks

        class stream_encoder
        {
            std::vector<std::byte> buffer_;
            std::string result_;
            const std::string_view chars_;
            const size_t chunk_size_;

        public:
            explicit stream_encoder(const size_t reserved_size,
                                    const std::string_view chars = base64_chars,
                                    const size_t chunk_size =
                                        default_chunk_size)
                : buffer_(chunk_size)
                  , chars_(chars)
                  , chunk_size_(chunk_size)
            {
                // Reserve estimated final size plus some padding
                result_.reserve((reserved_size + 2) / 3 * 4);
            }

            void process_chunk(const std::span<const std::byte> chunk)
            {
                for (size_t i = 0; i < chunk.size(); i += 3)
                {
                    uint32_t triple = static_cast<uint32_t>(std::to_integer<
                        uint8_t>(chunk[i])) << 16;

                    if (i + 1 < chunk.size())
                        triple |= static_cast<uint32_t>(std::to_integer<
                            uint8_t>(chunk[i + 1])) << 8;
                    if (i + 2 < chunk.size())
                        triple |= static_cast<uint32_t>(std::to_integer<
                            uint8_t>(chunk[i + 2]));

                    result_ += chars_[(triple & 0x00FC0000) >> 18];
                    result_ += chars_[(triple & 0x0003F000) >> 12];
                    result_ += (i + 1 < chunk.size())
                                   ? chars_[(triple & 0x00000FC0) >> 6]
                                   : '=';
                    result_ += (i + 2 < chunk.size())
                                   ? chars_[(triple & 0x0000003F)]
                                   : '=';
                }
            }

            [[nodiscard]] std::string&& finalize() &&
            {
                return std::move(result_);
            }

            [[nodiscard]] std::span<std::byte> get_buffer() noexcept
            {
                return {buffer_.data(), buffer_.size()};
            }
        };
    } // namespace detail

    /**
     * @brief Encodes a file into a Base64-encoded string using streaming.
     *
     * This implementation:
     * - Uses chunked reading for memory efficiency
     * - Pre-allocates buffers for optimal performance
     * - Avoids unnecessary memory reallocations
     * - Processes data in a streaming fashion
     *
     * @param path Path to the file to encode
     * @param chars Character set to use (default: standard Base64)
     * @param chunk_size Size of chunks to read (default: 48KB)
     * @param max_size Maximum file size to process (default: 100MB)
     * @return encode_result Encoded string or error
     */
    [[nodiscard]] inline encode_result base64_encode_file(
        const std::filesystem::path& path,
        const std::string_view chars = base64_chars,
        const size_t chunk_size = detail::default_chunk_size,
        const std::uintmax_t max_size = 100 * 1024 * 1024)
    {
        // Validate input parameters
        if (!detail::validate_charset(chars))
            return detail::make_unexpected<std::string>(
                chars.size() != 64
                    ? error::invalid_character_set_length
                    : error::invalid_character_set_padding_char_used);

        // Validate file
        std::error_code ec;
        if (!std::filesystem::exists(path))
            return detail::make_unexpected<std::string>(error::file_not_found);

        const auto file_size = std::filesystem::file_size(path, ec);
        if (ec)
            return detail::make_unexpected<std::string>(error::io_error);

        if (file_size == 0)
            return detail::make_unexpected<std::string>(error::empty_data);

        if (file_size > max_size)
            return detail::make_unexpected<std::string>(error::file_too_large);

        // Open a file with exception handling
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            return detail::make_unexpected<std::string>(
                error::file_not_readable);

        try
        {
            // Setup streaming encoder with pre-allocated buffers
            detail::stream_encoder encoder(file_size, chars, chunk_size);

            // Process file in chunks
            while (file && !file.eof())
            {
                auto buffer = encoder.get_buffer();
                file.read(reinterpret_cast<char*>(buffer.data()),
                          static_cast<std::streamsize>(buffer.size()));

                if (const auto bytes_read = file.gcount(); bytes_read > 0)
                {
                    encoder.process_chunk(std::span(buffer.data(), bytes_read));
                }
            }

            // Check for read errors
            if (file.bad())
                return detail::make_unexpected<std::string>(error::io_error);

            // Finalize and return the result
            return std::move(encoder).finalize();
        }
        catch (const std::exception&)
        {
            return detail::make_unexpected<std::string>(error::io_error);
        }
    }

    /**
     * @brief Encodes a file into Base64 and writes to an output file using streaming.
     *
     * @param input_path Path to the input file
     * @param output_path Path where to write the encoded result
     * @param chars Character set to use (default: standard Base64)
     * @param chunk_size Size of chunks to read (default: 48KB)
     * @param max_size Maximum file size to process (default: 100MB)
     * @return std::error_code Error code (empty if successful)
     */
    [[nodiscard]] inline std::error_code base64_encode_file_to_file(
        const std::filesystem::path& input_path,
        const std::filesystem::path& output_path,
        const std::string_view chars = base64_chars,
        const size_t chunk_size = detail::default_chunk_size,
        const std::uintmax_t max_size = 100 * 1024 * 1024)
    {
        try
        {
            std::ofstream output(output_path);
            if (!output.is_open())
                return make_error_code(error::io_error);

            // Validate input parameters
            if (!detail::validate_charset(chars))
                return make_error_code(
                    chars.size() != 64
                        ? error::invalid_character_set_length
                        : error::invalid_character_set_padding_char_used);

            // Validate file
            std::error_code ec;
            if (!std::filesystem::exists(input_path))
                return make_error_code(error::file_not_found);

            const auto file_size = std::filesystem::file_size(input_path, ec);
            if (ec)
                return make_error_code(error::io_error);

            if (file_size == 0)
                return make_error_code(error::empty_data);

            if (file_size > max_size)
                return make_error_code(error::file_too_large);

            std::ifstream input(input_path, std::ios::binary);
            if (!input.is_open())
                return make_error_code(error::file_not_readable);

            // Setup streaming encoder
            detail::stream_encoder encoder(chunk_size, chars, chunk_size);

            // Process the file in chunks and write directly to the output
            while (input && !input.eof())
            {
                auto buffer = encoder.get_buffer();
                input.read(reinterpret_cast<char*>(buffer.data()),
                           static_cast<std::streamsize>(buffer.size()));

                const auto bytes_read = input.gcount();
                if (bytes_read > 0)
                {
                    encoder.process_chunk(std::span(buffer.data(), bytes_read));
                }

                // Write an encoded chunk to an output file
                output << std::move(encoder).finalize();

                if (!output)
                    return make_error_code(error::io_error);
            }

            // Check for read errors
            if (input.bad())
                return make_error_code(error::io_error);

            return {};
        }
        catch (const std::exception&)
        {
            return make_error_code(error::io_error);
        }
    }
} // namespace base64

// Enable automatic conversion to std::error_code
template <>
struct std::is_error_code_enum<base64::error> : std::true_type
{
};

#endif // BASE64_HPP
