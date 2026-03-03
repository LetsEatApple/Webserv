#include "AST.hpp"
#include "Config.hpp"
#include "BlockValidators.hpp"
#include "catch2/catch_test_macros.hpp"
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

/*
  bool autoindex;
  size_t client_max_body_size;
  std::string path;
  std::string alias;
  std::string index;
  std::string upload;
  std::map<int, std::string> ret;
  std::set<std::string> limit_except;
  std::map<int, std::string> error_page;

  #This configuration is for demonstration purpose only.
  
  # HTTP (root, main) context. It MAY contain simple directives and MUST contain at least one server context.
  # It MUST NOT contain any location context.
  
  # Directives:
  # path has implicit default value "/"
  # autoindex "on" shows listing for current directory if index wasn't found, no duplicates
  # index filename, no duplicates
  # alias only inside location, no duplicates 
  # upload path where files will be uploaded to, no duplicates, "" means no upload
  # return (ret) directives set rules for different returning codes, only in server or location
  # limit_except sets allowed HTTP methods for location (only inside location)
  # error_page directives set path to error pages
  # client_max_body_size limit to body size of POST requests
  # could be specified in bytes: 4096, kilobytes: 4k or 4K, megabytes: 16m or 16M
 
  
  # server_name virtual hosts, only inside server
  # listen address:port pair, only inside server
  
  autoindex off;
  index index.html;
  upload /;
  error_page 404 /not_found.html;
  error_page 500 501 502 503 504 505 /5xx.html;
  client_max_body_size 1m;
  return 200 OK;
  return 301 /redirect;
  
  # empty server, default listen, all directives are inhereted
  server {}

  listen specifies address and port on which server should listen for incoming requests
  0.0.0.0:8000 - default value
  localhost = 127.0.0.1:8000
  * = 0.0.0.0:8000
  *:8080 = 0.0.0.0:8080
  8000 = 127.0.0.1:8000

  
*/

static Block makeBlock(
    std::string name,
    std::vector<std::string> args)
{
    Block b;
    b.name = name;
    b.args = args;
    b.line = 1;
    b.blocks = std::vector<Block>{};
    b.directives = std::vector<Directive>{};
    return b;
}

static Directive makeDirective(
    std::string name,
    std::vector<std::string> args)
{
    Directive d;
    d.name = name;
    d.args = args;
    d.line = 1;
    return d;
}

static Block makeValidConfig() {
  Block http = makeBlock("http", {});
  http.directives.push_back(makeDirective("autoindex", {"off"}));
  http.directives.push_back(makeDirective("index", {"index.html"}));
  http.directives.push_back(makeDirective("upload", {"/"}));
  http.directives.push_back(makeDirective("error_page", {"404", "/not_found.html"}));
  http.directives.push_back(makeDirective("error_page", {"500",  "501", "502", "503", "504", "505", "/5xx.html"}));
  http.directives.push_back(makeDirective("client_max_body_size", {"1m"}));

  Block server1 = makeBlock("server", {});
  http.blocks.push_back(server1);

  Block location1 = makeBlock("location", {"/"});
  Block location2 = makeBlock("location", {"/upload"});
  location2.directives.push_back(makeDirective("autoindex", {"off"}));
  location2.directives.push_back(makeDirective("index", {"index.html"}));
  location2.directives.push_back(makeDirective("upload", {"/"}));
  location2.directives.push_back(makeDirective("error_page", {"404", "/not_found.html"}));
  location2.directives.push_back(makeDirective("error_page", {"500",  "501", "502", "503", "/5xx.html"}));
  location2.directives.push_back(makeDirective("client_max_body_size", {"4096"}));
  location2.directives.push_back(makeDirective("return", {"301", "/redirect"}));
  location2.directives.push_back(makeDirective("alias", {"/var/www/temp"}));
  location2.directives.push_back(makeDirective("limit_except", {"GET", "POST"}));
  Block server2 = makeBlock("server", {});
  server2.blocks.push_back(location1);
  server2.blocks.push_back(location2);
  server2.directives.push_back(makeDirective("listen", {"localhost"}));
  server2.directives.push_back(makeDirective("autoindex", {"on"}));
  server2.directives.push_back(makeDirective("index", {"index.php"}));
  server2.directives.push_back(makeDirective("upload", {"/upload"}));
  server2.directives.push_back(makeDirective("error_page", {"404", "/404.html"}));
  server2.directives.push_back(makeDirective("error_page", {"500",  "501", "502", "503", "504", "505", "/server_errors.html"}));
  server2.directives.push_back(makeDirective("client_max_body_size", {"10K"}));
  server2.directives.push_back(makeDirective("server_name", {"google.com", "www.google.com"}));
  server2.directives.push_back(makeDirective("return", {"200", "OK"}));
  server2.directives.push_back(makeDirective("cgi", {".py", "/usr/bin/python3"}));
  http.blocks.push_back(server2);

  return http;
}

TEST_CASE("Parse valid config") {
  Block config = makeValidConfig();
  REQUIRE_NOTHROW(validator::validate(config));
}

TEST_CASE("error: wrong name for http block") {
  Block config = makeValidConfig();
  config.name = "server";
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: no server in http") {
  Block config = makeValidConfig();
  config.blocks.clear();
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: location in HTTP") {
  Block config = makeValidConfig();
  config.blocks.push_back(makeBlock("location", {}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: server_name in HTTP") {
  Block config = makeValidConfig();
  config.directives.push_back(makeDirective("server_name", {"test", "test2"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: return in HTTP") {
  Block config = makeValidConfig();
  config.directives.push_back(makeDirective("return", {"200", "OK"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: limit_except in HTTP") {
  Block config = makeValidConfig();
  config.directives.push_back(makeDirective("limit_except", {"GET"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: alias in HTTP") {
  Block config = makeValidConfig();
  config.directives.push_back(makeDirective("alias", {"GET"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: cgi in HTTP") {
  Block config = makeValidConfig();
  config.directives.push_back(makeDirective("cgi", {".py", "/usr/bin/python3"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: SERVER block in SERVER") {
  Block config = makeValidConfig();
  config.blocks[0].blocks.push_back(makeBlock("server", {}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: HTTP block in SERVER") {
  Block config = makeValidConfig();
  config.blocks[0].blocks.push_back(makeBlock("http", {}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: limit_except in SERVER") {
  Block config = makeValidConfig();
  config.blocks[0].directives.push_back(makeDirective("limit_except", {"GET"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: alias in SERVER") {
  Block config = makeValidConfig();
  config.blocks[0].directives.push_back(makeDirective("alias", {"GET"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: location too few arguments") {
  Block config = makeValidConfig();
  config.blocks[0].blocks.push_back(makeBlock("location", {}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: location too many arguments") {
  Block config = makeValidConfig();
  config.blocks[0].blocks.push_back(makeBlock("location", {"/", "/"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: HTTP block in LOCATION") {
  Block config = makeValidConfig();
  config.blocks[1].blocks[0].blocks.push_back(makeBlock("http", {}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: SERVER block in LOCATION") {
  Block config = makeValidConfig();
  config.blocks[1].blocks[0].blocks.push_back(makeBlock("server", {}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: LOCATION block in LOCATION") {
  Block config = makeValidConfig();
  config.blocks[1].blocks[0].blocks.push_back(makeBlock("location", {}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: server_name in LOCATION") {
  Block config = makeValidConfig();
  config.blocks[1].blocks[0].directives.push_back(makeDirective("server_name", {"test"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: listen in LOCATION") {
  Block config = makeValidConfig();
  config.blocks[1].blocks[0].directives.push_back(makeDirective("listen", {"8080"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: error_page too few arguments") {
  Block config = makeValidConfig();
  config.directives.push_back(makeDirective("error_page", {"400"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error_page many arguments") {
  Block config = makeValidConfig();
  config.directives.push_back(makeDirective("error_page", {"400", "401", "402", "403", "404", "/error.html"}));
  REQUIRE_NOTHROW(validator::validate(config));
}

TEST_CASE("error: duplicate client_max_body_size") {
  Block config = makeValidConfig();
  config.directives.push_back(makeDirective("client_max_body_size", {"400"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: client_max_body_size too many arguments") {
  Block config = makeValidConfig();
  config.blocks[0].directives.push_back(makeDirective("client_max_body_size", {"400", "500m"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: client_max_body_size too few arguments") {
  Block config = makeValidConfig();
  config.blocks[0].directives.push_back(makeDirective("client_max_body_size", {}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: listen too many arguments") {
  Block config = makeValidConfig();
  config.blocks[0].directives.push_back(makeDirective("listen", {"400", "500m"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: listen too few arguments") {
  Block config = makeValidConfig();
  config.blocks[0].directives.push_back(makeDirective("listen", {}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("server_name many arguments") {
  Block config = makeValidConfig();
  config.blocks[0].directives.push_back(makeDirective("server_name", {"testtest", "test"}));
  REQUIRE_NOTHROW(validator::validate(config));
}

TEST_CASE("error: server_name too few arguments") {
  Block config = makeValidConfig();
  config.blocks[0].directives.push_back(makeDirective("server_name", {}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: limit_except too few arguments") {
  Block config = makeValidConfig();
  config.blocks[1].blocks[0].directives.push_back(makeDirective("limit_except", {}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: limit_except too many arguments") {
  Block config = makeValidConfig();
  config.blocks[1].blocks[0].directives.push_back(makeDirective("limit_except", {"one", "two", "three", "four"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: duplicate limit_except") {
  Block config = makeValidConfig();
  config.blocks[1].blocks[1].directives.push_back(makeDirective("limit_except", {"GET"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: return too few arguments") {
  Block config = makeValidConfig();
  config.blocks[1].blocks[0].directives.push_back(makeDirective("return", {}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: return too many arguments") {
  Block config = makeValidConfig();
  config.blocks[1].blocks[0].directives.push_back(makeDirective("return", {"one", "two", "four"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: cgi too few arguments") {
  Block config = makeValidConfig();
  config.blocks[1].blocks[0].directives.push_back(makeDirective("cgi", {".py"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: cgi too many arguments") {
  Block config = makeValidConfig();
  config.blocks[1].blocks[0].directives.push_back(makeDirective("cgi", {".php", "/usr/bin/php8.2", "four"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: duplicate alias") {
  Block config = makeValidConfig();
  config.blocks[1].blocks[1].directives.push_back(makeDirective("alias", {"/"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: alias too few arguments") {
  Block config = makeValidConfig();
  config.blocks[1].blocks[0].directives.push_back(makeDirective("alias", {}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: alias too many arguments") {
  Block config = makeValidConfig();
  config.blocks[1].blocks[0].directives.push_back(makeDirective("alias", {"one", "two"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: duplicate autoindex") {
  Block config = makeValidConfig();
  config.directives.push_back(makeDirective("autoindex", {"on"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: autoindex too few arguments") {
  Block config = makeValidConfig();
  config.blocks[0].directives.push_back(makeDirective("autoindex", {}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: autoindex too many arguments") {
  Block config = makeValidConfig();
  config.blocks[0].directives.push_back(makeDirective("autoindex", {"one", "two"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: duplicate index") {
  Block config = makeValidConfig();
  config.directives.push_back(makeDirective("index", {"test.html"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: index too few arguments") {
  Block config = makeValidConfig();
  config.blocks[0].directives.push_back(makeDirective("index", {}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: index too many arguments") {
  Block config = makeValidConfig();
  config.blocks[0].directives.push_back(makeDirective("index", {"one", "two"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: duplicate upload") {
  Block config = makeValidConfig();
  config.directives.push_back(makeDirective("upload", {"/test"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: upload too few arguments") {
  Block config = makeValidConfig();
  config.blocks[0].directives.push_back(makeDirective("upload", {}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}

TEST_CASE("error: upload too many arguments") {
  Block config = makeValidConfig();
  config.blocks[0].directives.push_back(makeDirective("upload", {"one", "two"}));
  REQUIRE_THROWS_AS(validator::validate(config), std::runtime_error);
}
