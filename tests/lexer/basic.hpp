#pragma once

#include <cassert>
#include <iostream>
#include "../lang/include/lexer.h"

constexpr auto RESET = "\033[0m";
constexpr auto RED = "\033[31m";
constexpr auto GREEN = "\033[32m";
constexpr auto YELLOW = "\033[33m";

inline void basic_tokenization()
{
    constexpr std::string_view annote = R"(@align(16) function ;  += | "Hello, World!")";
    auto lexer = nightglow::lang::lexer::create_lexer(annote.data(), annote.size());
    [[maybe_unused]] const nightglow::lang::TokenList* tokens = tokenize(lexer);

    try
    {
        assert(tokens->size() == 1);
        assert(tokens->types[0] == nightglow::lang::token_i::ALIGN_ANNOT);
        assert(tokens->types[1] == nightglow::lang::token_i::LEFT_PAREN);
        assert(tokens->types[2] == nightglow::lang::token_i::NUMBER);
        assert(tokens->types[3] == nightglow::lang::token_i::RIGHT_PAREN);
        assert(tokens->types[4] == nightglow::lang::token_i::FUNCTION);
        assert(tokens->types[5] == nightglow::lang::token_i::SEMICOLON);
        assert(tokens->types[6] == nightglow::lang::token_i::PLUS_EQUAL);
        assert(tokens->types[7] == nightglow::lang::token_i::OR);
        assert(tokens->types[8] == nightglow::lang::token_i::STRING);
        std::cout << GREEN << "[PASSED]: Basic tokenization\n" << RESET;
    }
    catch (const std::exception& e)
    {
        std::cout << RED << "[FAILED]: " << e.what() << RESET << "\n";
    }
}