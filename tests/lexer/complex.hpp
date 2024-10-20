#pragma once

#include <cassert>
#include <iostream>
#include "../lang/include/lexer.h"

inline void complex_tokenization()
{
    constexpr std::string_view input = "@tailrec function() -> void { return; }";
    auto lexer = nightglow::lang::lexer::create_lexer(input.data(), input.size());
    [[maybe_unused]] const nightglow::lang::TokenList* tokens = tokenize(lexer);

    try
    {
        assert(tokens->size() == 11);
        assert(tokens->types[0] == nightglow::lang::token_i::TAIL_REC_ANNOT);
        assert(tokens->types[1] == nightglow::lang::token_i::FUNCTION);
        assert(tokens->types[2] == nightglow::lang::token_i::LEFT_PAREN);
        assert(tokens->types[3] == nightglow::lang::token_i::RIGHT_PAREN);
        assert(tokens->types[4] == nightglow::lang::token_i::ARROW);
        assert(tokens->types[5] == nightglow::lang::token_i::VOID);
        assert(tokens->types[6] == nightglow::lang::token_i::LEFT_BRACE);
        assert(tokens->types[7] == nightglow::lang::token_i::RETURN);
        assert(tokens->types[8] == nightglow::lang::token_i::SEMICOLON);
        assert(tokens->types[9] == nightglow::lang::token_i::RIGHT_BRACE);

        std::cout << GREEN << "[PASSED]: Complex tokenization\n" << RESET;
    }
    catch (const std::exception& e)
    {
        std::cout << RED << "[FAILED]: " << e.what() << RESET << "\n";
    }
}
