cmake_minimum_required(VERSION 3.14)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(Webserv)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Werror)

option(ENABLE_TESTING "Enable Unit Testing Build" ON)
option(
	ENABLE_SANITIZERS
	"Enable AddressSanitizer and UndefinedBehaviorSanitizer"
	ON)

if(ENABLE_SANITIZERS)
	add_compile_options(
		-stdlib=libc++
		-fsanitize=address,undefined
		-fno-omit-frame-pointer)
	add_compile_definitions(_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG)
	add_link_options(-stdlib=libc++ -fsanitize=address,undefined)
endif()


set(CONFIG_LIBRARY config_lib)

add_subdirectory(Config)

add_executable(webserv main.cpp)
target_link_libraries(webserv ${CONFIG_LIBRARY})
