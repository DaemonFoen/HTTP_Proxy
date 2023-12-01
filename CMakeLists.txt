cmake_minimum_required(VERSION 3.16)
project(HTTP_Proxy C)

set(CMAKE_C_STANDARD 11)

# Add the executable target
add_executable(HTTP_Proxy main.c)

# Find and link the pthread library
find_package(Threads REQUIRED)
target_link_libraries(HTTP_Proxy PRIVATE Threads::Threads)
