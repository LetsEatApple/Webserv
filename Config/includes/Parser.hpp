#ifndef PARSER_HPP
#define PARSER_HPP

#include "AST.hpp"
#include "Lexer.hpp"
#include "Token.hpp"
#include <string>
#include <vector>

class Parser
{
public:
	explicit Parser(Lexer& lexer);
	Block parse();

private:
	Lexer& _lexer;
	token::Token _current;

	void advance();
	Block parseBlock(const std::string& name,
					 const std::vector<std::string>& args);
	void parseBlockBody(Block& block);
};

#endif // !PARSER_HPP
