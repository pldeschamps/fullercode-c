cmake_minimum_required(VERSION 3.10)
project(fullercode C)

set(CMAKE_C_STANDARD 99)

# Library
add_library(fullercode STATIC fullercode.c)
target_include_directories(fullercode PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(fullercode m)

# Demo/test executable
add_executable(fullercode_demo main.c)
target_link_libraries(fullercode_demo fullercode m)

# Install targets
install(TARGETS fullercode ARCHIVE DESTINATION lib)
install(FILES fullercode.h DESTINATION include)
