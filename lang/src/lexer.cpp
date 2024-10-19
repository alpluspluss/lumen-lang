//
// Created by: Al++, 18.10.2024
// This header file is a part of the Nightglow programming language, licensed under the MIT license.
//

#ifdef __AVX2__
#include <immintrin.h>
#elif defined(__SSE2__)
#include <emmintrin.h>
#endif

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#include <stdexcept>
#include <string_view>
#include "../include/lang.h"

using namespace nightglow::lang;

void TokenList::push_back(const token_t &token)
{
    starts.push_back(token.start);
    lengths.push_back(token.length);
    types.push_back(token.type);
    flags.push_back(token.flags);
}

inline size_t TokenList::size() const
{
    return starts.size();
}

lexer::Lexer lexer::create_lexer(const char* src, const size_t length)
{
    if (length > UINT32_MAX)
    {
        throw std::runtime_error("Source file too large (>4GiB)");
    }
    Lexer lexer;
    lexer.src = src;
    lexer.src_length = static_cast<uint32_t>(length);
    lexer.current_pos = 0;
    return lexer;
}

inline void lexer::advance(Lexer &lexer)
{
    lexer.current_pos += lexer.tokens.lengths.back();
}

token_t lexer::peek_next(const Lexer& lexer)
{
    if (lexer.current_pos >= lexer.src_length)
    {
        return {lexer.current_pos, 0, token_i::END_OF_FILE, 0};
    }

    return next_token(const_cast<Lexer&>(lexer));
}

token_t lexer::next_token(Lexer& lexer)
{
    if (lexer.current_pos >= lexer.src_length)
    {
        return { lexer.current_pos, 0, token_i::END_OF_FILE, 0 };
    }

    const token_i token_type = tokenize(lexer);
    const uint32_t token_start = lexer.current_pos;
    constexpr uint16_t token_length = 1;

    const token_t new_token = { token_start, token_length, token_type, 0 };
    lexer.tokens.push_back(new_token);

    advance(lexer);

    return new_token;
}

token_i lexer::tokenize(Lexer& lexer)
{
    skip_whitespace_comment(lexer);
    return token_i();
}

void lexer::skip_whitespace_comment(Lexer& lexer)
{
    const char* src = lexer.src;
    uint32_t& current_pos = lexer.current_pos;
    uint32_t src_length = lexer.src_length;

#ifdef __ARM_NEON
    const uint8x16_t space = vdupq_n_u8(' ');
    const uint8x16_t tab = vdupq_n_u8('\t');
    const uint8x16_t newline = vdupq_n_u8('\n');
    const uint8x16_t carriage_return = vdupq_n_u8('\r');
    const uint8x16_t forward_slash = vdupq_n_u8('/');
    const uint8x16_t asterisk = vdupq_n_u8('*');

    while (current_pos < src_length)
    {
        if (uint32_t remaining = src_length - current_pos; remaining >= 64)
        {
            uint8x16_t chars1 = vld1q_u8(reinterpret_cast<const uint8_t*>(&src[current_pos]));
            uint8x16_t chars2 = vld1q_u8(reinterpret_cast<const uint8_t*>(&src[current_pos + 16]));
            uint8x16_t chars3 = vld1q_u8(reinterpret_cast<const uint8_t*>(&src[current_pos + 32]));
            uint8x16_t chars4 = vld1q_u8(reinterpret_cast<const uint8_t*>(&src[current_pos + 48]));

            uint8x16_t is_whitespace1 = vorrq_u8(
                vorrq_u8(vceqq_u8(chars1, space), vceqq_u8(chars1, tab)),
                vorrq_u8(vceqq_u8(chars1, newline), vceqq_u8(chars1, carriage_return))
            );
            uint8x16_t is_whitespace2 = vorrq_u8(
                vorrq_u8(vceqq_u8(chars2, space), vceqq_u8(chars2, tab)),
                vorrq_u8(vceqq_u8(chars2, newline), vceqq_u8(chars2, carriage_return))
            );
            uint8x16_t is_whitespace3 = vorrq_u8(
                vorrq_u8(vceqq_u8(chars3, space), vceqq_u8(chars3, tab)),
                vorrq_u8(vceqq_u8(chars3, newline), vceqq_u8(chars3, carriage_return))
            );
            uint8x16_t is_whitespace4 = vorrq_u8(
                vorrq_u8(vceqq_u8(chars4, space), vceqq_u8(chars4, tab)),
                vorrq_u8(vceqq_u8(chars4, newline), vceqq_u8(chars4, carriage_return))
            );

            uint64_t mask1 = vgetq_lane_u64(vreinterpretq_u64_u8(is_whitespace1), 0) | vgetq_lane_u64(vreinterpretq_u64_u8(is_whitespace1), 1) << 32;
            uint64_t mask2 = vgetq_lane_u64(vreinterpretq_u64_u8(is_whitespace2), 0) | vgetq_lane_u64(vreinterpretq_u64_u8(is_whitespace2), 1) << 32;
            uint64_t mask3 = vgetq_lane_u64(vreinterpretq_u64_u8(is_whitespace3), 0) | vgetq_lane_u64(vreinterpretq_u64_u8(is_whitespace3), 1) << 32;
            uint64_t mask4 = vgetq_lane_u64(vreinterpretq_u64_u8(is_whitespace4), 0) | vgetq_lane_u64(vreinterpretq_u64_u8(is_whitespace4), 1) << 32;

            if (mask1 == 0xFFFFFFFFFFFFFFFFULL &&
                mask2 == 0xFFFFFFFFFFFFFFFFULL &&
                mask3 == 0xFFFFFFFFFFFFFFFFULL &&
                mask4 == 0xFFFFFFFFFFFFFFFFULL)
            {
                current_pos += 64;
                continue;
            }

            uint8x16_t is_slash1 = vceqq_u8(chars1, forward_slash);
            uint8x16_t is_slash2 = vceqq_u8(chars2, forward_slash);
            uint8x16_t is_slash3 = vceqq_u8(chars3, forward_slash);
            uint8x16_t is_slash4 = vceqq_u8(chars4, forward_slash);

            uint64_t slash_mask1 = vgetq_lane_u64(vreinterpretq_u64_u8(is_slash1), 0) | vgetq_lane_u64(vreinterpretq_u64_u8(is_slash1), 1) << 32;
            uint64_t slash_mask2 = vgetq_lane_u64(vreinterpretq_u64_u8(is_slash2), 0) | vgetq_lane_u64(vreinterpretq_u64_u8(is_slash2), 1) << 32;
            uint64_t slash_mask3 = vgetq_lane_u64(vreinterpretq_u64_u8(is_slash3), 0) | vgetq_lane_u64(vreinterpretq_u64_u8(is_slash3), 1) << 32;
            uint64_t slash_mask4 = vgetq_lane_u64(vreinterpretq_u64_u8(is_slash4), 0) | vgetq_lane_u64(vreinterpretq_u64_u8(is_slash4), 1) << 32;

            uint64_t combined_mask = mask1 | slash_mask1 |
                         (mask2 | slash_mask2) << 16 |
                         (mask3 | slash_mask3) << 32 |
                         (mask4 | slash_mask4) << 48;

            if (combined_mask != 0xFFFFFFFFFFFFFFFFULL)
            {
                int leading = __builtin_ffsll(~combined_mask) - 1;
                current_pos += leading;
                break;
            }

            current_pos += 64;
        }
        else
        {
            while (current_pos < src_length)
            {
                if (char c = src[current_pos]; c == ' ' || c == '\t' || c == '\n' || c == '\r')
                {
                    ++current_pos;
                }
                else if (c == '/' && current_pos + 1 < src_length)
                {
                    if (src[current_pos + 1] == '/')
                    {
                        current_pos += 2;
                        while (current_pos < src_length && src[current_pos] != '\n')
                        {
                            ++current_pos;
                        }
                    }
                    else if (src[current_pos + 1] == '*')
                    {
                        // Multi-line comment
                        current_pos += 2;
                        while (current_pos + 1 < src_length &&
                               !(src[current_pos] == '*' && src[current_pos + 1] == '/'))
                        {
                            ++current_pos;
                        }
                        if (current_pos + 1 < src_length)
                        {
                            current_pos += 2;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
            break;
        }
    }
#else
    while (current_pos < src_length)
    {
        char c = src[current_pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
            ++current_pos;
        }
        else if (c == '/' && current_pos + 1 < src_length)
        {
            if (src[current_pos + 1] == '/')
            {
                // Single-line comment
                current_pos += 2;
                while (current_pos < src_length && src[current_pos] != '\n')
                {
                    ++current_pos;
                }
            }
            else if (src[current_pos + 1] == '*')
            {
                // Multi-line comment
                current_pos += 2;
                while (current_pos + 1 < src_length &&
                       !(src[current_pos] == '*' && src[current_pos + 1] == '/'))
                {
                    ++current_pos;
                }
                if (current_pos + 1 < src_length)
                {
                    current_pos += 2;
                }
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
#endif
}

std::pair<uint32_t, uint32_t> lexer::get_line_col(const Lexer& lexer, const token_t& token)
{
    uint32_t line = 1;
    uint32_t col = 1;

    for (uint32_t i = 0; i < token.start; ++i)
    {
        if (lexer.src[i] == '\n')
        {
            ++line;
            col = 1;
        }
        else
        {
            ++col;
        }
    }

    return {line, col};
}

std::string_view lexer::get_token_value(const Lexer& lexer, const token_t& token)
{
    return { &lexer.src[token.start], token.length };
}
