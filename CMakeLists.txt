cmake_minimum_required(VERSION 3.14)

project(Webserv)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(WARNING "Please compile with Clang.")
endif()

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Werror)

option(ENABLE_TESTING "Enable Unit Testing Build" ON)
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" ON)

if(ENABLE_SANITIZERS)
    add_compile_options(-stdlib=libc++ -fsanitize=address,undefined -fno-omit-frame-pointer)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "18.0")
        message(WARNING "LLVM < 18: _LIBCPP_HARDENING_MODE_DEBUG has no effect")
    else()
        add_compile_definitions(_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG)
    endif()
    add_link_options(-stdlib=libc++ -fsanitize=address,undefined)
endif()


add_compile_options(-Wall -Wextra -Werror)

option(ENABLE_TESTING "Enable Unit Testing Build" ON)
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" ON)

if(ENABLE_SANITIZERS)
    add_compile_options(-stdlib=libc++ -fsanitize=address,undefined -fno-omit-frame-pointer)
    add_compile_definitions(_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG)
    add_link_options(-stdlib=libc++ -fsanitize=address,undefined)
endif()

include(FetchContent)

if(ENABLE_TESTING)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2
        GIT_TAG v3.12.0
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(Catch2)
endif()

set(REACTOR_LIBRARY reactor_lib)
set(HTTP_LIBRARY http_lib)
set(CONFIG_LIBRARY config_lib)

add_subdirectory(Http)
add_subdirectory(Reactor)
add_subdirectory(Config)

add_executable(webserv main.cpp)
target_link_libraries(webserv ${REACTOR_LIBRARY} ${CONFIG_LIBRARY})

if(ENABLE_TESTING)
    include(CTest)
    add_subdirectory(tests)
endif()
