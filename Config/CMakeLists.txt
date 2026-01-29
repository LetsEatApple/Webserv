set(CONFIG_SOURCES
	Config.cpp
	Lexer.cpp
	Parser.cpp
	Utils.cpp
	Validator.cpp
)

add_library(${CONFIG_LIBRARY} STATIC ${CONFIG_SOURCES})
