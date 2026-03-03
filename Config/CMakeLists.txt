set(CONFIG_SOURCES
	Config.cpp
	Lexer.cpp
	Parser.cpp
	Utils.cpp
	Validator.cpp
)

project(config VERSION 1.0.0 DESCRIPTION "Parsing and validation of configuration files")

add_library(${CONFIG_LIBRARY} STATIC ${CONFIG_SOURCES})
target_include_directories(${CONFIG_LIBRARY} PUBLIC .)