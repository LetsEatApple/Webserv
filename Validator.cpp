#include "Validator.hpp"
#include "Utils.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stddef.h>
#include <stdexcept>

namespace validator {
static void validateBlock(const Block &b, ContextType ctx);
static DirectiveType directiveType(const std::string &name) {
  if (name == "client_max_body_size")
    return DIR_CLIENT_MAX_BODY_SIZE;
  else if (name == "error_page")
    return DIR_ERROR_PAGE;
  else if (name == "listen")
    return DIR_LISTEN;
  else
    return DIR_UNKNOWN;
}

// TODO: validate path
static void validateErrorPage(const Directive &d) {
  if (d.args.size() < 2)
    throw std::runtime_error(utils::errorMsg(
        "Expected at least 2 arguments for 'error_page' directive", d.line));
  for (size_t i = 1; i < d.args.size() - 1; ++i) {
    int status = std::atoi(d.args[i].c_str());
    if (d.args[i].length() != 3 || status < 300 || status > 599)
      throw std::runtime_error(
          utils::errorMsg("Invalid value '" + d.args[i] + "'", d.line));
  }
}

// TODO: validate measurement units
static void validateClientMaxBodySize(const Directive &d, ValidationState &vs) {
  if (vs.has_client_max_body_size)
    throw std::runtime_error(
        utils::errorMsg("Duplicate '" + d.name + "' directive", d.line));
  if (d.args.size() != 1)
    throw std::runtime_error(utils::errorMsg(
        "Expected exactly 1 argument for '" + d.name + "' directive", d.line));
  vs.has_client_max_body_size = true;
}

// TODO: validate argument
static void validateListen(const Directive &d, ContextType ctx) {
  if (ctx != CTX_SERVER)
    throw std::runtime_error(utils::errorMsg(
        "'" + d.name + "' directive is not allowed here", d.line));
  if (d.args.size() != 1)
    throw std::runtime_error(utils::errorMsg(
        "Expected exactly 1 argument for '" + d.name + "' directive", d.line));
}

static void validateDirective(const Directive &d, ContextType ctx,
                              ValidationState &vs) {
  DirectiveType dir = directiveType(d.name);
  (void)ctx;
  switch (dir) {
  case DIR_ERROR_PAGE:
    validateErrorPage(d);
    break;
  case DIR_CLIENT_MAX_BODY_SIZE:
    validateClientMaxBodySize(d, vs);
    break;
  case DIR_LISTEN:
    validateListen(d, ctx);
  case DIR_UNKNOWN:
    return;
  default:
    return;
  }
};

static void validateHTTP(const Block &b) {
  ValidationState vs = {false, false, false};

  for (size_t i = 0; i < b.directives.size(); ++i) {
    validateDirective(b.directives[i], CTX_HTTP, vs);
  }

  for (size_t i = 0; i < b.blocks.size(); ++i) {
    if (b.blocks[i].name != "server")
      throw std::runtime_error(utils::errorMsg(
          "'" + b.blocks[i].name + "' directive is not allowed here",
          b.blocks[i].line));
    validateBlock(b.blocks[i], CTX_SERVER);
  }
}
static void validateServer(const Block &b) {
  ValidationState vs = {false, false, false};

  for (size_t i = 0; i < b.directives.size(); ++i) {
    validateDirective(b.directives[i], CTX_SERVER, vs);
  }

  for (size_t i = 0; i < b.blocks.size(); ++i) {
    if (b.blocks[i].name != "location")
      throw std::runtime_error(utils::errorMsg(
          "'" + b.blocks[i].name + "' directive is not allowed here",
          b.blocks[i].line));
    validateBlock(b.blocks[i], CTX_LOCATION);
  }
}
static void validateLocation(const Block &b) {
  ValidationState vs = {false, false, false};

  for (size_t i = 0; i < b.directives.size(); ++i) {
    validateDirective(b.directives[i], CTX_SERVER, vs);
  }

  for (size_t i = 0; i < b.blocks.size(); ++i) {
    if (b.blocks[i].name != "location")
      throw std::runtime_error(utils::errorMsg(
          "'" + b.blocks[i].name + "' directive is not allowed here",
          b.blocks[i].line));
    validateBlock(b.blocks[i], CTX_LOCATION);
  }
}

static void validateBlock(const Block &b, ContextType ctx) {
  switch (ctx) {
  case CTX_HTTP:
    validateHTTP(b);
    break;
  case CTX_SERVER:
    validateServer(b);
    break;
  case CTX_LOCATION:
    validateLocation(b);
    break;
  default:
    throw std::runtime_error(
        utils::errorMsg("Unexpected context type", b.line));
  }
}

void validate(const Block &http) {
  if (http.name != "http") {
    std::stringstream err;
    err << "Unexpected root context name '" << http.name << "' line ("
        << http.line << ")";
    throw std::runtime_error(err.str());
  }
  validateBlock(http, CTX_HTTP);
}

} // namespace validator
