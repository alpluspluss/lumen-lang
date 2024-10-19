//
// Created by: Al++, 18.10.2024
// This header file is a part of the Nightglow programming language, licensed under the MIT license.
//

#ifndef LANG_H
#define LANG_H

#include <cstdint>
#include <vector>

namespace nightglow::lang
{
    /**
     * @brief Represents the type of token that the lexer has found.
     */
    enum class token_i : uint8_t
    {
        KEYWORD,
        IDENTIFIER,
        NUM_LITERAL,
        STR_LITERAL,
        OPERATOR,
        PUNCTUAL,
        ANNOTATION,
        TYPE,
        NULLABLE_TYPE,
        ARRAY_TYPE,
        UNKNOWN,
        END_OF_FILE
    };

    /**
     * @brief Represents a token that the lexer has found.
     */
    struct alignas(8) token_t
    {
        uint32_t start;  // 4 bytes: index into source
        uint16_t length; // 2 bytes: length of token
        token_i type;    // 1 byte
        uint8_t flags;   // 1 byte: can store additional information
    };

    /**
     * @brief A structure to efficiently store tokens.
     */
    struct alignas(8) TokenList
    {
        std::vector<uint32_t> starts;
        std::vector<uint16_t> lengths;
        std::vector<token_i> types;
        std::vector<uint8_t> flags;

        void push_back(const token_t& token);
        [[nodiscard]] inline size_t size() const;
    };
}

namespace nightglow::lang::lexer
{
    /**
     * @brief The lexer class.
     */
    struct alignas(8) Lexer
    {
     const char* src{};        // 8 bytes
     TokenList tokens;         // 32 bytes (8 bytes per vector pointer)
     uint32_t current_pos{};   // 4 bytes
     uint32_t src_length{};    // 4 bytes
    };

    /**
     * @brief Creates a lexer object.
     * @param src The source code to tokenize.
     * @param length The length of the source code.
     * @return Lexer The lexer object.
     * @throws std::runtime_error if the source file is too large (>4GiB).
     */
    Lexer create_lexer(const char* src, size_t length);

    /**
     * @brief Peek the next token without advancing.
     * @param lexer The lexer object.
     * @return token_t The next token.
     */
    token_t peek_next(const Lexer& lexer);

    /**
     * @brief Advances the lexer to the next token.
     * @param lexer The lexer object.
     */
    inline void advance(Lexer& lexer);

    /**
     * @brief Returns the next token.
     * @param lexer The lexer object.
     * @return token_t The next token.
     */
    token_t next_token(Lexer& lexer);

    /**
     * @brief Tokenizes the source code and returns the token type.
     * @param lexer The lexer object.
     * @return token_i The token type.
     */
    token_i tokenize(Lexer& lexer);

    /**
     * @brief Skips whitespace and comments using SIMD instructions.
     * @param lexer The lexer object.
     */
    void skip_whitespace_comment(Lexer& lexer);

    /**
     * @brief Get the line and column for a given token.
     * @param lexer The lexer object.
     * @param token The token.
     * @return std::pair<uint32_t, uint32_t> The line and column.
     */
    std::pair<uint32_t, uint32_t> get_line_col(const Lexer& lexer, const token_t& token);
    /**
     * @brief Get the string value of a token.
     * @param lexer The lexer object.
     * @param token The token.
     * @return std::string_view The string value of the token.
     */
    std::string_view get_token_value(const Lexer& lexer, const token_t& token);
}

#endif
