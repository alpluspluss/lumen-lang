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

#include "../include/lexer.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

const std::array<uint8_t, 256> char_type = []
{
    std::array<uint8_t, 256> types{};
    for (auto i = 0; i < 256; ++i)
    {
        if (i == ' ' || i == '\t' || i == '\n' || i == '\r')
            types[i] = 1;
        else if (i == '/')
            types[i] = 2;
        else if (i == '*')
            types[i] = 3;
        else if (std::isalpha(i) || i == '_' || i == '@')
            types[i] = 4;
        else if (std::isdigit(i))
            types[i] = 5;
        else
            types[i] = 0;
    }
    return types;
}();

void nightglow::lang::TokenList::push_back(const token_t &token)
{
    starts.emplace_back(token.start);
    lengths.emplace_back(token.length);
    types.emplace_back(token.type);
    flags.emplace_back(token.flags);
}

void nightglow::lang::TokenList::reserve(const uint32_t &n = 10000)
{
    starts.reserve(n);
    lengths.reserve(n);
    types.reserve(n);
    flags.reserve(n);
}

nightglow::lang::lexer::Lexer nightglow::lang::lexer::create_lexer(const std::string_view src, const size_t length)
{
    if (length > UINT32_MAX)
    {
        throw std::runtime_error("Source file too large (>4GiB)");
    }
    Lexer lexer;
    lexer.src = src.data();
    lexer.src_length = static_cast<uint32_t>(length);
    lexer.current_pos = 0;
    lexer.line_starts.push_back(0);
    return lexer;
}

inline void nightglow::lang::lexer::advance(Lexer &lexer)
{
    lexer.current_pos += lexer.tokens.lengths.back();
}

nightglow::lang::token_t nightglow::lang::lexer::peek_next(const lexer::Lexer& lexer)
{
    if (lexer.current_pos >= lexer.src_length)
    {
        return {lexer.current_pos, 0, token_i::END_OF_FILE, 0};
    }

    return next_token(const_cast<lexer::Lexer&>(lexer));
}

nightglow::lang::token_t nightglow::lang::lexer::next_token(Lexer& lexer)
{
    skip_whitespace_comment(lexer, lexer.src, lexer.current_pos, lexer.src_length);
    if (lexer.current_pos >= lexer.src_length)
    {
        return {lexer.current_pos, 0, token_i::END_OF_FILE, 0};
    }

    switch (const char* start = lexer.src + lexer.current_pos; char_type[static_cast<uint8_t>(*start)])
    {
        case 4: return lex_identifier(lexer);
        case 5: return lex_number(lexer);
        default:
        {
            if (lexer.current_pos + 2 < lexer.src_length)
            {
                if (token_map.contains(std::string_view(start, 3)))
                {
                    return {lexer.current_pos, 3, token_map.at(std::string_view(start, 3)), 0};
                }
            }
            if (lexer.current_pos + 1 < lexer.src_length)
            {
                if (token_map.contains(std::string_view(start, 2)))
                {
                    return {lexer.current_pos, 2, token_map.at(std::string_view(start, 2)), 0};
                }
            }
            if (token_map.contains(std::string_view(start, 1)))
            {
                return {lexer.current_pos, 1, token_map.at(std::string_view(start, 1)), 0};
            }
            return {lexer.current_pos, 1, token_i::UNKNOWN, 0};
        }
    }
}

nightglow::lang::TokenList* nightglow::lang::lexer::tokenize(Lexer& lexer)
{
    lexer.tokens.reserve(lexer.src_length / 2);
    while (true)
    {
        token_t token = peek_next(lexer);
        if (token.type == token_i::END_OF_FILE)
        {
            lexer.tokens.push_back(token);
            break;
        }
        if (token.type != token_i::UNKNOWN)
        {
            lexer.tokens.push_back(token);
        }
        advance(lexer);
    }

    return &lexer.tokens;
}

void nightglow::lang::lexer::skip_whitespace_comment(Lexer& lexer, const char *src, uint32_t &current_pos, const uint32_t src_length)
{
    while (current_pos < src_length)
    {
        uint32_t chunk_size = std::min(static_cast<uint32_t>(32), src_length - current_pos);
        // ReSharper disable once CppEntityAssignedButNoRead
        [[maybe_unused]] uint32_t mask = 0;

        #if defined(__ARM_NEON)
        if (chunk_size == 32)
        {
            const uint8x16_t chars1 = vld1q_u8(reinterpret_cast<const uint8_t*>(&src[current_pos]));
            const uint8x16_t chars2 = vld1q_u8(reinterpret_cast<const uint8_t*>(&src[current_pos + 16]));

            uint8x16_t space = vdupq_n_u8(' ');
            uint8x16_t tab = vdupq_n_u8('\t');
            uint8x16_t newline = vdupq_n_u8('\n');
            uint8x16_t carriage_return = vdupq_n_u8('\r');
            uint8x16_t forward_slash = vdupq_n_u8('/');

            const uint8x16_t is_whitespace1 = vorrq_u8(
                vorrq_u8(vceqq_u8(chars1, space), vceqq_u8(chars1, tab)),
                vorrq_u8(vceqq_u8(chars1, newline), vceqq_u8(chars1, carriage_return))
            );
            const uint8x16_t is_whitespace2 = vorrq_u8(
                vorrq_u8(vceqq_u8(chars2, space), vceqq_u8(chars2, tab)),
                vorrq_u8(vceqq_u8(chars2, newline), vceqq_u8(chars2, carriage_return))
            );
            const uint8x16_t is_slash1 = vceqq_u8(chars1, forward_slash);
            const uint8x16_t is_slash2 = vceqq_u8(chars2, forward_slash);

            const uint8x16_t result1 = vorrq_u8(is_whitespace1, is_slash1);
            const uint8x16_t result2 = vorrq_u8(is_whitespace2, is_slash2);

            uint32x4_t bit_mask = vdupq_n_u32(0x01010101u);
            bit_mask = vshlq_n_u32(bit_mask, 1);
            const uint32x4_t mask_vec1 = vreinterpretq_u32_u8(result1);
            const uint32x4_t mask_vec2 = vreinterpretq_u32_u8(result2);
            const uint32x4_t masked1 = vandq_u32(mask_vec1, bit_mask);
            const uint32x4_t masked2 = vandq_u32(mask_vec2, bit_mask);

            const uint32x2_t sum1 = vpadd_u32(vget_low_u32(masked1), vget_high_u32(masked1));
            const uint32x2_t sum2 = vpadd_u32(vget_low_u32(masked2), vget_high_u32(masked2));
            const uint32x2_t final_sum = vpadd_u32(sum1, sum2);

            mask = vget_lane_u32(final_sum, 0) | (vget_lane_u32(final_sum, 1) << 16);
        }
        else if (chunk_size >= 16)
        {
            uint8x16_t chars = vld1q_u8(reinterpret_cast<const uint8_t*>(&src[current_pos]));
            uint8x16_t space = vdupq_n_u8(' ');
            uint8x16_t tab = vdupq_n_u8('\t');
            uint8x16_t newline = vdupq_n_u8('\n');
            uint8x16_t carriage_return = vdupq_n_u8('\r');
            uint8x16_t forward_slash = vdupq_n_u8('/');

            uint8x16_t is_whitespace = vorrq_u8(
                vorrq_u8(vceqq_u8(chars, space), vceqq_u8(chars, tab)),
                vorrq_u8(vceqq_u8(chars, newline), vceqq_u8(chars, carriage_return))
            );
            uint8x16_t is_slash = vceqq_u8(chars, forward_slash);
            const uint8x16_t result = vorrq_u8(is_whitespace, is_slash);

            uint32x4_t bit_mask = vdupq_n_u32(0x01010101u);
            bit_mask = vshlq_n_u32(bit_mask, 1);
            const uint32x4_t mask_vec = vreinterpretq_u32_u8(result);
            const uint32x4_t masked = vandq_u32(mask_vec, bit_mask);

            const uint32x2_t sum = vpadd_u32(vget_low_u32(masked), vget_high_u32(masked));
            mask = vget_lane_u32(sum, 0) | (vget_lane_u32(sum, 1) << 8);
        }
        #elif defined(__AVX2__)
        if (chunk_size == 32)
        {
            __m256i chars = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&src[current_pos]));
            __m256i space = _mm256_set1_epi8(' ');
            __m256i tab = _mm256_set1_epi8('\t');
            __m256i newline = _mm256_set1_epi8('\n');
            __m256i carriage_return = _mm256_set1_epi8('\r');
            __m256i forward_slash = _mm256_set1_epi8('/');

            __m256i is_whitespace = _mm256_or_si256(
                _mm256_or_si256(_mm256_cmpeq_epi8(chars, space), _mm256_cmpeq_epi8(chars, tab)),
                _mm256_or_si256(_mm256_cmpeq_epi8(chars, newline), _mm256_cmpeq_epi8(chars, carriage_return))
            );
            __m256i is_slash = _mm256_cmpeq_epi8(chars, forward_slash);

            mask = _mm256_movemask_epi8(_mm256_or_si256(is_whitespace, is_slash));
        }
        #elif defined(__SSE2__)
        if (chunk_size >= 16)
        {
            __m128i chars_low = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&src[current_pos]));
            __m128i space = _mm_set1_epi8(' ');
            __m128i tab = _mm_set1_epi8('\t');
            __m128i newline = _mm_set1_epi8('\n');
            __m128i carriage_return = _mm_set1_epi8('\r');
            __m128i forward_slash = _mm_set1_epi8('/');

            __m128i is_whitespace_low = _mm_or_si128(
                _mm_or_si128(_mm_cmpeq_epi8(chars_low, space), _mm_cmpeq_epi8(chars_low, tab)),
                _mm_or_si128(_mm_cmpeq_epi8(chars_low, newline), _mm_cmpeq_epi8(chars_low, carriage_return))
            );
            __m128i is_slash_low = _mm_cmpeq_epi8(chars_low, forward_slash);

            mask = _mm_movemask_epi8(_mm_or_si128(is_whitespace_low, is_slash_low));

            if (chunk_size == 32)
            {
                __m128i chars_high = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&src[current_pos + 16]));
                __m128i is_whitespace_high = _mm_or_si128(
                    _mm_or_si128(_mm_cmpeq_epi8(chars_high, space), _mm_cmpeq_epi8(chars_high, tab)),
                    _mm_or_si128(_mm_cmpeq_epi8(chars_high, newline), _mm_cmpeq_epi8(chars_high, carriage_return))
                );
                __m128i is_slash_high = _mm_cmpeq_epi8(chars_high, forward_slash);
                mask |= (_mm_movemask_epi8(_mm_or_si128(is_whitespace_high, is_slash_high)) << 16);
            }
        }
        #endif

        for (uint32_t i = 0; i < chunk_size; ++i)
        {
            const uint8_t type = char_type[static_cast<uint8_t>(src[current_pos])];

            if (type == 0)
                return;

            if (type == 1)
            {
                if (lexer.src[lexer.current_pos] == '\n')
                {
                    lexer.line_starts.push_back(lexer.current_pos + 1);
                }
                ++lexer.current_pos;
                continue;
            }

            if (type == 2 && current_pos + 1 < src_length)
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
                    current_pos += 2;
                    while (current_pos + 1 < src_length)
                    {
                        if (src[current_pos] == '*' && src[current_pos + 1] == '/')
                        {
                            current_pos += 2;
                            break;
                        }
                        ++current_pos;
                    }
                }
                else
                {
                    return;
                }
            }
            else
            {
                return;
            }
        }
    }
}

std::pair<uint32_t, uint32_t> nightglow::lang::lexer::get_line_col(const Lexer& lexer, const token_t& token)
{
    const auto it = std::ranges::upper_bound(lexer.line_starts, token.start) - 1;
    uint32_t line = std::distance(lexer.line_starts.begin(), it) + 1;
    uint32_t col = token.start - *it + 1;
    return { line, col };
}

std::string_view nightglow::lang::lexer::get_token_value(const Lexer& lexer, const token_t& token)
{
    return { &lexer.src[token.start], token.length };
}

nightglow::lang::token_t nightglow::lang::lexer::lex_identifier(const Lexer &lexer)
{
    const char* start = lexer.src + lexer.current_pos;
    const char* current = start;

    if (*current == '@')
    {
        ++current;
        while (current < lexer.src + lexer.src_length && (std::isalnum(*current) || *current == '_'))
        {
            ++current;
        }

        const uint16_t length = current - start;
        if (const std::string_view annotation(start + 1, length - 1); token_map.contains(annotation))
        {
            return {lexer.current_pos, static_cast<uint16_t>(current - start), token_map.at(annotation), 0};
        }

        return {lexer.current_pos, static_cast<uint16_t>(current - start), token_i::UNKNOWN, 0};
    }

    while (current < lexer.src + lexer.src_length && (std::isalnum(*current) || *current == '_'))
    {
        ++current;
    }

    const uint16_t length = current - start;
    std::string_view identifier(start, length);

    if (token_map.contains(identifier))
    {
        return {lexer.current_pos, length, token_map.at(identifier), 0};
    }

    return {lexer.current_pos, length, token_i::IDENTIFIER, 0};
}

nightglow::lang::token_t nightglow::lang::lexer::lex_number(const Lexer &lexer)
{
    const char* start = lexer.src + lexer.current_pos;
    const char* current = start;
    auto is_float = false;
    auto is_hex = false;
    auto is_binary = false;

    if (*current == '0' && current + 1 < lexer.src + lexer.src_length)
    {
        if (*(current + 1) == 'x' || *(current + 1) == 'X')
        {
            is_hex = true;
            current += 2;
        }
        else if (*(current + 1) == 'b' || *(current + 1) == 'B')
        {
            is_binary = true;
            current += 2;
        }
    }

    while (current < lexer.src + lexer.src_length)
    {
        if (is_hex && !std::isxdigit(*current))
            break;
        if (is_binary && *current != '0' && *current != '1')
            break;
        if (!is_hex && !is_binary)
        {
            if (*current == '.')
            {
                if (is_float)
                    break;
                is_float = true;
            }
            else if (!std::isdigit(*current))
            {
                break;
            }
        }
        ++current;
    }

    const uint16_t length = current - start;
    return {lexer.current_pos, length, token_i::NUM_LITERAL, 0};
}

nightglow::lang::token_t nightglow::lang::lexer::lex_string(const Lexer &lexer)
{
    const char* start = lexer.src + lexer.current_pos;
    const char* current = start + 1;

    while (current < lexer.src + lexer.src_length && *current != '"')
    {
        if (*current == '\\' && current + 1 < lexer.src + lexer.src_length)
        {
            ++current;
        }
        ++current;
    }

    if (current >= lexer.src + lexer.src_length || *current != '"')
    {
        return {lexer.current_pos, static_cast<uint16_t>(current - start), token_i::UNKNOWN, 0};
    }

    const uint16_t length = current - start + 1;
    return {lexer.current_pos, length, token_i::STR_LITERAL, 0};
}
