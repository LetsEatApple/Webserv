#include "AST.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "catch2/catch_test_macros.hpp"
#include <stdexcept>
#include <string>
#include "test_helpers.hpp"

static Block parseFile(std::string const &path) {
  Lexer lexer(path);
  Parser parser(lexer);
  return parser.parse();
}

TEST_CASE("Parser rejects empty file") {
  TempFile f("empty.conf", "");

  try {
    Lexer lexer("empty.conf");
    Parser parser(lexer);
    parser.parse();
    FAIL("Expected exception not thrown");
  } catch (std::runtime_error const &e) {
    std::string msg = e.what();
    REQUIRE(msg.find("Unexpected token 'EOF' at top level at line 1") != std::string::npos);
  }
}

TEST_CASE("Parser rejects unexpected first token") {
  TempFile f("start_with_semicolon.conf", "; server { }");
  TempFile f2("start_with_lbrace.conf", "{ server { }");
  TempFile f3("start_with_rbrace.conf", "} server { }");

  try {
    Lexer lexer("start_with_semicolon.conf");
    Parser parser(lexer);
    parser.parse();
    FAIL("Expected exception not thrown");
  } catch (std::runtime_error const &e) {
    std::string msg = e.what();
    REQUIRE(msg.find("Unexpected token at top level at line 1") != std::string::npos);
  }
  
  try {
    Lexer lexer("start_with_lbrace.conf");
    Parser parser(lexer);
    parser.parse();
    FAIL("Expected exception not thrown");
  } catch (std::runtime_error const &e) {
    std::string msg = e.what();
    REQUIRE(msg.find("Unexpected token at top level at line 1") != std::string::npos);
  }
  
  try {
    Lexer lexer("start_with_rbrace.conf");
    Parser parser(lexer);
    parser.parse();
    FAIL("Expected exception not thrown");
  } catch (std::runtime_error const &e) {
    std::string msg = e.what();
    REQUIRE(msg.find("Unexpected token at top level at line 1") != std::string::npos);
  }
}

TEST_CASE("Parser rejects unclosed directive") {
  TempFile f("unclosed.conf", "listen localhost:8000");

  try {
    Lexer lexer("unclosed.conf");
    Parser parser(lexer);
    parser.parse();
    FAIL("Expected exception not thrown");
  } catch (std::runtime_error const &e) {
    std::string msg = e.what();
    REQUIRE(msg.find("Expected ';' or '{' at line 1") != std::string::npos);
  }
}

TEST_CASE("Parser rejects unclosed block") {
  TempFile f("unclosed.conf", "server { listen 80;");

  try {
    Lexer lexer("unclosed.conf");
    Parser parser(lexer);
    parser.parse();
    FAIL("Expected exception not thrown");
  } catch (std::runtime_error const &e) {
    std::string msg = e.what();
    REQUIRE(msg.find("Unexpected token inside block at line 1") != std::string::npos);
  }
}

TEST_CASE("Parser rejects unexpected token at start of block") {
  TempFile f("unexpected_block_start.conf", "server { ; }");

  try {
    Lexer lexer("unexpected_block_start.conf");
    Parser parser(lexer);
    parser.parse();
    FAIL("Expected exception not thrown");
  } catch (std::runtime_error const &e) {
    std::string msg = e.what();
    REQUIRE(msg.find("Unexpected token inside block at line 1") != std::string::npos);
  }
}

TEST_CASE("Parser parses top-level directive") {
  TempFile f("top.conf", "client_max_body_size 10M;");

  Block root = parseFile("top.conf");

  REQUIRE(root.directives.size() == 1);
  REQUIRE(root.directives[0].name == "client_max_body_size");
  REQUIRE(root.directives[0].args.size() == 1);
  REQUIRE(root.directives[0].args[0] == "10M");
}

TEST_CASE("Parser parses simple block") {
  TempFile f("block.conf",
             "server {\n"
             "  listen 8080;\n"
             "}\n");

  Block root = parseFile("block.conf");

  REQUIRE(root.blocks.size() == 1);
  REQUIRE(root.blocks[0].name == "server");
  REQUIRE(root.blocks[0].directives.size() == 1);
  REQUIRE(root.blocks[0].directives[0].name == "listen");
}

TEST_CASE("Parser parses nested blocks") {
  TempFile f("nested.conf",
      "server {\n"
      "  listen one.example.com;\n"
      "  server_name one.example.com  www.one.example.com;\n"
      "  location / {\n"
      "    client_max_body_size       10m;\n"
      "  }\n"
      "  error_page  404  /404.html;\n"
      "}\n"
      "server {\n"
      "  listen 127.0.0.1;\n"
      "}");
  
  Block root = parseFile("nested.conf");

  REQUIRE(root.blocks.size() == 2);
  Block &server1 = root.blocks[0];
  Block &server2 = root.blocks[1];
  REQUIRE(root.directives.size() == 0);
  REQUIRE(server1.name == "server");
  REQUIRE(server1.directives.size() == 3);
  REQUIRE(server1.directives[0].name == "listen");
  REQUIRE(server1.directives[0].args.size() == 1);
  REQUIRE(server1.directives[0].args[0] == "one.example.com");
  REQUIRE(server1.directives[1].name == "server_name");
  REQUIRE(server1.directives[1].args.size() == 2);
  REQUIRE(server1.directives[1].args[0] == "one.example.com");
  REQUIRE(server1.directives[1].args[1] == "www.one.example.com");
  REQUIRE(server1.directives[2].name == "error_page");
  REQUIRE(server1.directives[2].args.size() == 2);
  REQUIRE(server1.directives[2].args[0] == "404");
  REQUIRE(server1.directives[2].args[1] == "/404.html");
  REQUIRE(server1.blocks.size() == 1);
  Block &location = server1.blocks[0];
  REQUIRE(location.name == "location");
  REQUIRE(location.args.size() == 1);
  REQUIRE(location.args[0] == "/");
  REQUIRE(location.blocks.size() == 0);
  REQUIRE(location.directives.size() == 1);
  REQUIRE(location.directives[0].name == "client_max_body_size");
  REQUIRE(location.directives[0].args.size() == 1);
  REQUIRE(location.directives[0].args[0] == "10m");
  REQUIRE(server2.name == "server");
  REQUIRE(server2.directives.size() == 1);
  REQUIRE(server2.directives[0].name == "listen");
  REQUIRE(server2.directives[0].args.size() == 1);
  REQUIRE(server2.directives[0].args[0] == "127.0.0.1");
}
