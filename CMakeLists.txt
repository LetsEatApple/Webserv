cmake_minimum_required(VERSION 3.14)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(Webserv)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Werror)

option(ENABLE_TESTING "Enable Unit Testing Build" ON)
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" ON)

if(ENABLE_SANITIZERS)
    set(SANITIZER_FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer)

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-stdlib=libc++ ${SANITIZER_FLAGS})
        add_compile_definitions(_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG)
        add_link_options(-stdlib=libc++ -fsanitize=address,undefined)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(${SANITIZER_FLAGS})
        add_link_options(-fsanitize=address,undefined)
        message(WARNING "libc++ hardening mode not available with GCC/libstdc++")
    else()
        message(WARNING "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}, sanitizers not enabled")
    endif()
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
set(CONFIG_LIBRARY config_lib)

#add_subdirectory(Config)
add_subdirectory(Reactor)

add_executable(webserv main.cpp)
target_link_libraries(webserv ${REACTOR_LIBRARY} ${CONFIG_LIBRARY})
