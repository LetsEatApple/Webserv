cmake_minimum_required(VERSION 3.14)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(Webserv)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" ON)

if(ENABLE_SANITIZERS)
    add_compile_options(-stdlib=libc++ -fsanitize=address,undefined -fno-omit-frame-pointer)
    add_compile_definitions(_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG)
    add_link_options(-stdlib=libc++ -fsanitize=address,undefined)
endif()

set(REACTOR_LIBRARY reactor_lib)
set(REQUEST_LIBRARY request_lib)
set(RESPONSE_LIBRARY response_lib)
set(SERVER_LIBRARY server_lib)

add_subdirectory(Http)
add_subdirectory(Reactor)

add_executable(webserv main.cpp)
target_link_libraries(webserv ${REACTOR_LIBRARY})
