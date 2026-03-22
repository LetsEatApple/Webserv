#include "Lexer.hpp"
#include <cctype>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include "Token.hpp"
#include "Utils.hpp"

Lexer::Lexer(const std::string& path) : _file(path.c_str()), _line(1)
{
	if (!_file.is_open())
		throw std::runtime_error("Failed to open configuration file at '" + path +
			"'");
}

int Lexer::get()
{
	const int c = _file.get();
	if (c == '\n')
		_line++;
	return c;
}

void Lexer::skipWhitespaceAndComments()
{
	for (;;)
	{
		int c = get();
		if (c == EOF)
			return;
		if (std::isspace(static_cast<unsigned char>(c)))
			continue;
		if (c == '#')
		{
			while ((c = get()) != EOF && c != '\n')
			{
			}
			continue;
		}
		_file.unget();
		return;
	}
}

token::Token Lexer::next()
{
	skipWhitespaceAndComments();

	int c = get();
	if (c == EOF)
	{
		token::Token tok = {token::TOK_EOF, "", _line};
		return tok;
	}

	if (c == '{')
	{
		token::Token tok = {token::TOK_LBRACE, "{", _line};
		return tok;
	}

	if (c == '}')
	{
		token::Token tok = {token::TOK_RBRACE, "}", _line};
		return tok;
	}
	if (c == ';')
	{
		token::Token tok = {token::TOK_SEMICOLON, ";", _line};
		return tok;
	}

	std::string word;
	if (c == '"' || c == '\'')
	{
		const int quote = c;
		while ((c = get()) != quote)
		{
			if (c == EOF)
				throw std::runtime_error(utils::errorMsg("Unterminated string", _line));
			word += static_cast<char>(c);
		}
		token::Token tok = {token::TOK_WORD, word, _line};
		return tok;
	}

	while (c != EOF
		&& !std::isspace(static_cast<unsigned char>(c))
		&& c != '{' && c != '}' && c != ';')
	{
		word += static_cast<char>(c);
		c = get();
	}

	if (c == '{' || c == '}' || c == ';')
		_file.unget();

	token::Token tok = {token::TOK_WORD, word, _line};
	return tok;
}
