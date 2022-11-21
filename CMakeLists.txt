cmake_minimum_required(VERSION 3.8..3.22)

project(minitar C)

set(SOURCES
    src/tar.c
    src/util.c
)

add_library(minitar STATIC ${SOURCES})

set_target_properties(minitar PROPERTIES OUTPUT_NAME mtar)

include(CheckSourceCompiles)

# Check for signedness of standard types
check_source_compiles(C "#include <sys/types.h>\nint main(int argc, char** argv){ char is_unsigned_size_type[(size_t)0 < (size_t)-1 ? 1 : -1]; }" UNSIGNED_SIZE_TYPE)
check_source_compiles(C "#include <sys/types.h>\nint main(int argc, char** argv){ char is_unsigned_time_type[(time_t)0 < (time_t)-1 ? 1 : -1]; }" UNSIGNED_TIME_TYPE)
check_source_compiles(C "#include <sys/types.h>\nint main(int argc, char** argv){ char is_unsigned_mode_type[(mode_t)0 < (mode_t)-1 ? 1 : -1]; }" UNSIGNED_MODE_TYPE)
check_source_compiles(C "#include <sys/types.h>\nint main(int argc, char** argv){ char is_unsigned_gid_type[(gid_t)0 < (gid_t)-1 ? 1 : -1]; }" UNSIGNED_GID_TYPE)
check_source_compiles(C "#include <sys/types.h>\nint main(int argc, char** argv){ char is_unsigned_uid_type[(uid_t)0 < (uid_t)-1 ? 1 : -1]; }" UNSIGNED_UID_TYPE)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

target_include_directories(minitar PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

if (MSVC)
    target_compile_options(minitar PRIVATE /W4 /WX)
else()
    target_compile_options(minitar PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

install(TARGETS minitar DESTINATION lib)
install(FILES minitar.h DESTINATION include)