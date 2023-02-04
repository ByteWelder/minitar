add_executable(list EXCLUDE_FROM_ALL list.c)
target_link_libraries(list PRIVATE minitar)

add_executable(untar EXCLUDE_FROM_ALL untar.c)
target_link_libraries(untar PRIVATE minitar)

add_executable(pack EXCLUDE_FROM_ALL pack.c)
target_link_libraries(pack PRIVATE minitar)

add_custom_target(examples DEPENDS list untar pack)