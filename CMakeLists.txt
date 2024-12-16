cmake_minimum_required(VERSION 3.10)
project(twmailer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# Add compile definitions for client and server modes
add_executable(twmailer-server main.cpp server.cpp)
target_compile_definitions(twmailer-server PRIVATE SERVER_MODE)
target_link_libraries(twmailer-server wsock32 ws2_32)
set_target_properties(twmailer-server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(twmailer-client main.cpp client.cpp)
target_compile_definitions(twmailer-client PRIVATE CLIENT_MODE)
target_link_libraries(twmailer-client wsock32 ws2_32)
set_target_properties(twmailer-client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)