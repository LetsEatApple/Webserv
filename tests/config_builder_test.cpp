#include "catch2/catch_test_macros.hpp"
#include "Config.hpp"
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <utility>
#include <vector>

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

 struct TestCase {
    std::string name;
    std::string input;
    std::pair<std::string, uint16_t> expected;
  };

 struct TestFailCase {
    std::string name;
    std::string input;
  };

TEST_CASE("Valid - Port only") {

  // listen 1;
  // listen 80;
  // listen 443;
  // listen 8000;
  // listen 65535;
 
  std::vector<TestCase> cases = {
    {"valid-port-only-1","server{listen 1;}", {"0.0.0.0", 1}},
    {"valid-port-only-80", "server{listen 80;}", {"0.0.0.0", 80}},
    {"valid-port-only-443","server{listen 443;}", {"0.0.0.0", 443}},
    {"valid-port-only-8000", "server{listen 8000;}", {"0.0.0.0", 8000}},
    {"valid-port-only-65535","server{listen 65535;}", {"0.0.0.0", 65535}},
  };

  for (std::vector<TestCase>::iterator it = cases.begin(); it != cases.end(); ++it) {
    TempFile f(it->name, it->input);
    config::Listeners listeners;
    REQUIRE_NOTHROW(listeners = config::buildConfig(it->name));
    REQUIRE(listeners.count(it->expected) == 1);
  }
}


TEST_CASE("Valid - IPv4 only") {
  
  // listen 127.0.0.1;
  // listen 0.0.0.0;
  // listen 192.168.1.1;
  // listen localhost;
  // listen *;
 
  std::vector<TestCase> cases = {
    {"valid-ipv4-only-127001","server{listen 127.0.0.1;}", {"127.0.0.1", 8000}},
    {"valid-ipv4-only-0000","server{listen 0.0.0.0;}", {"0.0.0.0", 8000}},
    {"valid-ipv4-only-19216811","server{listen 192.168.1.1;}", {"192.168.1.1", 8000}},
    {"valid-ipv4-only-localhost","server{listen localhost;}", {"127.0.0.1", 8000}},
    {"valid-ipv4-only-wildcard","server{listen *;}", {"0.0.0.0", 8000}},
  };

  for (std::vector<TestCase>::iterator it = cases.begin(); it != cases.end(); ++it) {
    TempFile f(it->name, it->input);
    config::Listeners listeners;
    REQUIRE_NOTHROW(listeners = config::buildConfig(it->name));
    REQUIRE(listeners.count(it->expected) == 1);
  }
}

TEST_CASE("Valid - IPv4 + Port") {
  
  // listen 127.0.0.1:80;
  // listen 127.0.0.1:8000;
  // listen 0.0.0.0:8080;
  // listen localhost:3000;
  // listen *:8080;
  // listen 192.168.1.1:65535;
 
  std::vector<TestCase> cases = {
    {"valid-ipv4-127001-80","server{listen 127.0.0.1:80;}", {"127.0.0.1", 80}},
    {"valid-ipv4-127001-8000","server{listen 127.0.0.1:8000;}", {"127.0.0.1", 8000}},
    {"valid-ipv4-0000-8000","server{listen 0.0.0.0:8000;}", {"0.0.0.0", 8000}},
    {"valid-ipv4-localhost-3000","server{listen localhost:3000;}", {"127.0.0.1", 3000}},
    {"valid-ipv4-wildcard-8080","server{listen *:8080;}", {"0.0.0.0", 8080}},
    {"valid-ipv4-19216811-65535","server{listen 192.168.1.1:65535;}", {"192.168.1.1", 65535}},
  };

  for (std::vector<TestCase>::iterator it = cases.begin(); it != cases.end(); ++it) {
    TempFile f(it->name, it->input);
    config::Listeners listeners;
    REQUIRE_NOTHROW(listeners = config::buildConfig(it->name));
    REQUIRE(listeners.count(it->expected) == 1);
  }
}

TEST_CASE("Valid - IPv6 only") {
  
  // listen [::1];
  // listen [::];
  // listen [2001:db8::1];
  // listen [fe80::1];
  // listen [::ffff:127.0.0.1];
 
  std::vector<TestCase> cases = {
    {"valid-ipv6-only-::1","server{listen [::1];}", {"::1", 8000}},
    {"valid-ipv6-only-::","server{listen [::];}", {"::", 8000}},
    {"valid-ipv6-only-2001:db8::1","server{listen [2001:db8::1];}", {"2001:db8::1", 8000}},
    {"valid-ipv6-only-fe80::1","server{listen [fe80::1];}", {"fe80::1", 8000}},
    {"valid-ipv6-only-::ffff:127001","server{listen [::ffff:127.0.0.1];}", {"::ffff:127.0.0.1", 8000}},
  };

  for (std::vector<TestCase>::iterator it = cases.begin(); it != cases.end(); ++it) {
    TempFile f(it->name, it->input);
    config::Listeners listeners;
    REQUIRE_NOTHROW(listeners = config::buildConfig(it->name));
    REQUIRE(listeners.count(it->expected) == 1);
  }
}



TEST_CASE("Valid - IPv6 + Port") {

  // listen [::1]:80;
  // listen [::]:8000;
  // listen [2001:db8::1]:443;
  // listen [fe80::1]:65535;
  // listen [::ffff:127.0.0.1]:8080;`
 
  std::vector<TestCase> cases = {
    {"valid-ipv6-::1","server{listen [::1]:80;}", {"::1", 80}},
    {"valid-ipv6-::","server{listen [::]:8000;}", {"::", 8000}},
    {"valid-ipv6-2001:db8::1","server{listen [2001:db8::1]:443;}", {"2001:db8::1", 443}},
    {"valid-ipv6-fe80::1","server{listen [fe80::1]:65535;}", {"fe80::1", 65535}},
    {"valid-ipv6-::ffff:127001","server{listen [::ffff:127.0.0.1]:8080;}", {"::ffff:127.0.0.1", 8080}},
  };

  for (std::vector<TestCase>::iterator it = cases.begin(); it != cases.end(); ++it) {
    TempFile f(it->name, it->input);
    config::Listeners listeners;
    REQUIRE_NOTHROW(listeners = config::buildConfig(it->name));
    REQUIRE(listeners.count(it->expected) == 1);
  }
}

TEST_CASE("Invalid - IPv6 must be bracketed") {

  // listen ::1;
  // listen ::;
  // listen 2001:db8::1;
  // listen fe80::1;
  // listen ::1:80;
 
  std::vector<TestFailCase> cases = {
    {"ipv6-withoutbrackets-::1","server{listen ::1;}"},
    {"ipv6-withoutbrackets-::","server{listen ::;}"},
    {"ipv6-withoutbrackets-2001:db8::1","server{listen 2001:db8::1;}"},
    {"ipv6-withoutbrackets-fe80::1","server{listen fe80::1;}"},
    {"ipv6-withoutbrackets-::1:80","server{listen ::1::80;}"},
  };

  for (std::vector<TestFailCase>::iterator it = cases.begin(); it != cases.end(); ++it) {
    TempFile f(it->name, it->input);
    config::Listeners listeners;
    REQUIRE_THROWS(listeners = config::buildConfig(it->name));
  }
}

TEST_CASE("Invalid - IPv6 malformed brackets") {

  // listen [;
  // listen ];
  // listen [];
  // listen [::1;
  // listen ::1];
  // listen [::1]80;
  // listen [::1]abc;
  // listen [::1]::80;
  // listen [::1]:;
  // listen [::1]:abc;
 
  std::vector<TestFailCase> cases = {
    {"ipv6-malformedbrackets-1","server{listen [;}"},
    {"ipv6-malformedbrackets-2","server{listen ];}"},
    {"ipv6-malformedbrackets-3","server{listen [];}"},
    {"ipv6-malformedbrackets-4","server{listen [::1;}"},
    {"ipv6-malformedbrackets-5","server{listen ::1];}"},
    {"ipv6-malformedbrackets-6","server{listen [::1]80;}"},
    {"ipv6-malformedbrackets-7","server{listen [::1]abc;}"},
    {"ipv6-malformedbrackets-8","server{listen [::1]::80;}"},
    {"ipv6-malformedbrackets-9","server{listen [::1]::;}"},
    {"ipv6-malformedbrackets-10","server{listen [::1]:abc;}"},
  };

  for (std::vector<TestFailCase>::iterator it = cases.begin(); it != cases.end(); ++it) {
    TempFile f(it->name, it->input);
    config::Listeners listeners;
    REQUIRE_THROWS(listeners = config::buildConfig(it->name));
  }
}

TEST_CASE("Invalid - multiple colons") {

  // listen 127.0.0.1:80:90;
  // listen localhost:80:90;
  // listen *:80:90;
 
  std::vector<TestFailCase> cases = {
    {"multiplecolons-1","server{listen 127.0.0.1:80:90;}"},
    {"multiplecolons-2","server{listen localhost:80:90;}"},
    {"multiplecolons-3","server{listen *:80:90;}"},
  };

  for (std::vector<TestFailCase>::iterator it = cases.begin(); it != cases.end(); ++it) {
    TempFile f(it->name, it->input);
    config::Listeners listeners;
    REQUIRE_THROWS(listeners = config::buildConfig(it->name));
  }
}

TEST_CASE("Invalid - bad ports") {

  // listen 0;
  // listen 65536;
  // listen -1;
  // listen 999999;
  // listen abc;
  // listen 127.0.0.1:0;
  // listen 127.0.0.1:65536;
  // listen 127.0.0.1:-1;
  // listen 127.0.0.1:abc;
  // listen [::1]:0;
  // listen [::1]:65536;
  // listen [::1]:-1;
  // listen [::1]:abc;
 
  std::vector<TestFailCase> cases = {
    {"badpots-1","server{listen 0;}"},
    {"badpots-2","server{listen 65536;}"},
    {"badpots-3","server{listen -1;}"},
    {"badpots-4","server{listen 999999;}"},
    {"badpots-5","server{listen abc;}"},
    {"badpots-6","server{listen 127.0.0.1:0;}"},
    {"badpots-7","server{listen 127.0.0.1:65536;}"},
    {"badpots-8","server{listen 127.0.0.1:-1;}"},
    {"badpots-9","server{listen 127.0.0.1:abc;}"},
    {"badpots-10","server{listen [::1]:0;}"},
    {"badpots-11","server{listen [::1]:65536;}"},
    {"badpots-12","server{listen [::1]:-1;}"},
    {"badpots-13","server{listen [::1]:abc;}"},
  };

  for (std::vector<TestFailCase>::iterator it = cases.begin(); it != cases.end(); ++it) {
    TempFile f(it->name, it->input);
    config::Listeners listeners;
    REQUIRE_THROWS(listeners = config::buildConfig(it->name));
  }
}

TEST_CASE("Invalid - wildcard misuse") {

  // listen [*];
  // listen [*]:80;
  // listen *::80;
 
  std::vector<TestFailCase> cases = {
    {"wildcardmisuse-1","server{listen [*];}"},
    {"wildcardmisuse-2","server{listen [*]:80;}"},
    {"wildcardmisuse-3","server{listen *::80;}"},
  };

  for (std::vector<TestFailCase>::iterator it = cases.begin(); it != cases.end(); ++it) {
    TempFile f(it->name, it->input);
    config::Listeners listeners;
    REQUIRE_THROWS(listeners = config::buildConfig(it->name));
  }
}

TEST_CASE("Valid - edge cases") {

  // listen 080;
  // listen 00080;
  // listen 127.0.0.1:080;
 
  std::vector<TestCase> cases = {
    {"edgecases-1","server{listen 080;}", {"0.0.0.0", 80}},
    {"edgecases-2","server{listen 00080;}", {"0.0.0.0", 80}},
    {"edgecases-3","server{listen 127.0.0.1:080;}", {"127.0.0.1", 80}},
  };

  for (std::vector<TestCase>::iterator it = cases.begin(); it != cases.end(); ++it) {
    TempFile f(it->name, it->input);
    config::Listeners listeners;
    REQUIRE_NOTHROW(listeners = config::buildConfig(it->name));
    REQUIRE(listeners.count(it->expected) == 1);
  }
}

TEST_CASE("Valid - weird cases") {
  // listen 1.1.1.1:1; 
  // listen 255.255.255.255:65535;
  // listen [::ffff:192.168.0.1]:1234;
  // listen localhost:1;
  // listen *:1;`
 
  std::vector<TestCase> cases = {
    {"weird-1","server{listen 1.1.1.1:1;}", {"1.1.1.1", 1}},
    {"weird-2","server{listen 255.255.255.255:65535;}", {"255.255.255.255", 65535}},
    {"weird-3","server{listen [::ffff:192.168.0.1]:1234;}", {"::ffff:192.168.0.1", 1234}},
    {"weird-4","server{listen localhost:1;}", {"127.0.0.1", 1}},
    {"weird-5","server{listen *:1;}", {"0.0.0.0", 1}},
  };

  for (std::vector<TestCase>::iterator it = cases.begin(); it != cases.end(); ++it) {
    TempFile f(it->name, it->input);
    config::Listeners listeners;
    REQUIRE_NOTHROW(listeners = config::buildConfig(it->name));
    REQUIRE(listeners.count(it->expected) == 1);
  }
}
