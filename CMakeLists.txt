cmake_minimum_required(VERSION 3.8..3.22)

project(minitar LANGUAGES C VERSION 1.1.0)

set(SOURCES
    src/tar.c
    src/util.c
)

add_library(minitar STATIC ${SOURCES})

set_target_properties(minitar PROPERTIES OUTPUT_NAME mtar)

if (MSVC)
    target_compile_options(minitar PRIVATE /W4 /WX)
else()
    target_compile_options(minitar PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

install(TARGETS minitar DESTINATION lib)
install(FILES minitar.h DESTINATION include)