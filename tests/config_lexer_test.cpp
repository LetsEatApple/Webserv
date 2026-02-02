#include "catch2/catch_test_macros.hpp"
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include "Lexer.hpp"
#include "Token.hpp"

/* ========= FILE HELPERS ========= */

struct TempFile {
  std::string path;

  TempFile(const std::string &p, const std::string &content) : path(p) {
    std::ofstream f(path.c_str());
    f << content;
    f.close();
  }

  ~TempFile() {
    std::remove(path.c_str());
  }
};

TEST_CASE("Empty file produces only EOF token") {
  TempFile f("empty.conf", "");

  Lexer lexer("empty.conf");
  token::Token tok = lexer.next();

  REQUIRE(tok.type == token::TOK_EOF);
}

TEST_CASE("Lexer parses simple block") {
  TempFile f("block.conf", "{ foo; }");

  Lexer lexer("block.conf");
  token::Token t;

  t = lexer.next();
  REQUIRE(t.type == token::TOK_LBRACE);
  
  t = lexer.next();
  REQUIRE(t.type == token::TOK_WORD);
  REQUIRE(t.value == "foo");
  
  t = lexer.next();
  REQUIRE(t.type == token::TOK_SEMICOLON);
  
  t = lexer.next();
  REQUIRE(t.type == token::TOK_RBRACE);
  
  t = lexer.next();
  REQUIRE(t.type == token::TOK_EOF);
}

TEST_CASE("Lexer throws if file cannot be opened") {
  try {
    Lexer lexer("does-not-exist.conf");
    FAIL("Expected exception not thrown");
  } catch (std::runtime_error const &e) {
    std::string msg = e.what();
    REQUIRE(msg.find("Failed to open configuration file at 'does-not-exist.conf'") != std::string::npos);
  }
}

TEST_CASE("Lexer reads words separated by whitespace") {
  TempFile f("words.conf", "foo   bar\tbaz");

  Lexer lexer("words.conf");

  token::Token t;

  t = lexer.next();
  REQUIRE(t.type == token::TOK_WORD);
  REQUIRE(t.value == "foo");

  t = lexer.next();
  REQUIRE(t.value == "bar");

  t = lexer.next();
  REQUIRE(t.value == "baz");

  t = lexer.next();
  REQUIRE(t.type == token::TOK_EOF);
}

TEST_CASE("Lexer skips comments") {
  TempFile f("comments.conf",
            "# this is a comment\n"
            "foo test; # another comment\n"
            "bar test;");

  Lexer lexer("comments.conf");

  token::Token t;

  t = lexer.next();
  REQUIRE(t.value == "foo");
  REQUIRE(t.line == 2);

  t = lexer.next();
  REQUIRE(t.value == "test");
  REQUIRE(t.line == 2);
  
  t = lexer.next();
  REQUIRE(t.type == token::TOK_SEMICOLON);
  REQUIRE(t.line == 2);
  
  t = lexer.next();
  REQUIRE(t.value == "bar");
  REQUIRE(t.line == 3);
  
  t = lexer.next();
  REQUIRE(t.value == "test");
  REQUIRE(t.line == 3);
  
  t = lexer.next();
  REQUIRE(t.type == token::TOK_SEMICOLON);
  REQUIRE(t.line == 3);

  t = lexer.next();
  REQUIRE(t.type == token::TOK_EOF);
}

TEST_CASE("Lexer reads quoted strings") {
  TempFile f("strings.conf", "\"hello world\" 'abc';");

  Lexer lexer("strings.conf");

  token::Token t;

  t = lexer.next();
  REQUIRE(t.type == token::TOK_WORD);
  REQUIRE(t.value == "hello world");

  t = lexer.next();
  REQUIRE(t.value == "abc");

  t = lexer.next();
  REQUIRE(t.type == token::TOK_SEMICOLON);

  t = lexer.next();
  REQUIRE(t.type == token::TOK_EOF);
}

TEST_CASE("Lexer throws on unterminated string") {
  TempFile f("bad_string.conf", "\"unterminated test;");

  Lexer lexer("bad_string.conf");
  token::Token t;

  try {
    t = lexer.next();
    FAIL("Expected exception not thrown");
  } catch (std::runtime_error const &e) {
    std::string msg = e.what();
    REQUIRE(msg.find("Unterminated string at line 1") != std::string::npos);
  }
}

TEST_CASE("Lexer tracks line numbers correctly") {
  TempFile f("lines.conf",
             "foo bar;\n"
             "baz \"qux\";\n");

  Lexer lexer("lines.conf");

  token::Token t;

  t = lexer.next();
  REQUIRE(t.value == "foo");
  REQUIRE(t.line == 1);

  t = lexer.next();
  REQUIRE(t.value == "bar");
  REQUIRE(t.line == 1);

  t = lexer.next();
  REQUIRE(t.type == token::TOK_SEMICOLON);
  REQUIRE(t.line == 1);

  t = lexer.next();
  REQUIRE(t.value == "baz");
  REQUIRE(t.line == 2);

  t = lexer.next();
  REQUIRE(t.value == "qux");
  REQUIRE(t.line == 2);
}

TEST_CASE("Lexer handles adjacent symbols without whitespace") {
  TempFile f("adjacent.conf", "{foo;bar;}");

  Lexer lexer("adjacent.conf");

  REQUIRE(lexer.next().type == token::TOK_LBRACE);
  REQUIRE(lexer.next().value == "foo");
  REQUIRE(lexer.next().type == token::TOK_SEMICOLON);
  REQUIRE(lexer.next().value == "bar");
  REQUIRE(lexer.next().type == token::TOK_SEMICOLON);
  REQUIRE(lexer.next().type == token::TOK_RBRACE);
  REQUIRE(lexer.next().type == token::TOK_EOF);
}
