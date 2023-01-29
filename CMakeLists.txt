cmake_minimum_required(VERSION 3.8..3.22)

project(minitar LANGUAGES C VERSION 1.6.1)

option(MINITAR_IGNORE_UNSUPPORTED_TYPES "Skip past entries that have unsupported types instead of panicking (deprecated)" OFF)

set(SOURCES
    src/tar.c
    src/util.c
)

add_library(minitar STATIC ${SOURCES})

if(MINITAR_IGNORE_UNSUPPORTED_TYPES)
message(WARNING "MINITAR_IGNORE_UNSUPPORTED_TYPES is deprecated, since there are no unsupported types anymore")
endif()

target_include_directories(minitar PUBLIC ${CMAKE_CURRENT_LIST_DIR}) # for minitar.h

set_target_properties(minitar PROPERTIES OUTPUT_NAME mtar)

set_target_properties(minitar PROPERTIES C_STANDARD 11)
set_target_properties(minitar PROPERTIES C_STANDARD_REQUIRED ON)

if (MSVC)
    target_compile_options(minitar PRIVATE /W4 /WX)
else()
    target_compile_options(minitar PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

install(TARGETS minitar DESTINATION lib)
install(FILES minitar.h DESTINATION include)

add_subdirectory(examples)