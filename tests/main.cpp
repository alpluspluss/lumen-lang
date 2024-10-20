//
// Created by: Al++, 19.10.2024
// This header file is a part of the Nightglow programming language, licensed under the MIT license.
//

#include "lexer/basic.hpp"
#include "lexer/complex.hpp"

int main()
{
    // Lexing
    basic_tokenization();
    complex_tokenization();

    std::cout << "\n" << GREEN << "\tAll tests passed successfully\n" << RESET;
    return 0;
}
