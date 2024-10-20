file(GLOB CLI_SRC "src/*.cpp")

add_executable(Nightglow ${CLI_SRC}
        main.cpp
)

target_include_directories(Nightglow PRIVATE
        ${CMAKE_SOURCE_DIR}/common
        ${CMAKE_SOURCE_DIR}/lang/include
)

target_link_libraries(Nightglow PRIVATE nightglow-lang)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(Nightglow PRIVATE -O3)
endif()

install(TARGETS Nightglow DESTINATION build)