file(GLOB LANG_SRC "src/*.cpp")

add_library(nightglow-lang STATIC ${LANG_SRC}
        include/lang.h
        include/lexer.h
        ../extern/robin_hood.h)

target_include_directories(nightglow-lang PUBLIC
        ${CMAKE_SOURCE_DIR}/common
        ${CMAKE_SOURCE_DIR}/lang/include)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(nightglow-lang PRIVATE -O3)
endif()

install(TARGETS nightglow-lang DESTINATION lib)
install(DIRECTORY include/
        DESTINATION include/nightglow-lang
        FILES_MATCHING PATTERN "*.h" PATTERN "*.intl"
)