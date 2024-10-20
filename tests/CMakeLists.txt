file(GLOB TESTS_SRC
        "lexer/.hpp"
        "main.cpp"
)

add_executable(nightglow-tests ${TESTS_SRC}
        lexer/basic.hpp)

target_link_libraries(nightglow-tests PRIVATE nightglow-lang)

add_test(NAME nightglow-tests COMMAND nightglow-tests)
