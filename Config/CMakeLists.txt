cmake_minimum_required(VERSION 3.14)

project(config_lib
		VERSION 1.0.0
		DESCRIPTION "Parsing and validation of configuration files"
		LANGUAGES CXX
)

set(CONFIG_SOURCES
		src/Config.cpp
		src/Lexer.cpp
		src/Parser.cpp
		src/Utils.cpp
		src/BlockValidators.cpp
		src/DirectiveValidators.cpp
		src/ValidationHelpers.cpp
		src/ListenParser.cpp
		src/ConfigPrinter.cpp
		src/DirectiveApplier.cpp
)
add_library(${CONFIG_LIBRARY} STATIC ${CONFIG_SOURCES})

target_include_directories(${CONFIG_LIBRARY} PUBLIC includes)
target_compile_features(${CONFIG_LIBRARY} PUBLIC cxx_std_98)