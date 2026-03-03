cmake_minimum_required(VERSION 3.14)

project(config_lib
		VERSION 1.0.0
		DESCRIPTION "Parsing and validation of configuration files"
		LANGUAGES CXX
)

add_library(config_lib STATIC
		src/Config.cpp
		src/Lexer.cpp
		src/Parser.cpp
		src/Utils.cpp
		src/Validator.cpp

		includes/AST.hpp
		includes/Config.hpp
		includes/Debug.hpp
		includes/Lexer.hpp
		includes/Parser.hpp
		includes/Token.hpp
		includes/Utils.hpp
		includes/Validator.hpp
)

target_include_directories(config_lib PUBLIC includes)
target_compile_features(config_lib PUBLIC cxx_std_98)