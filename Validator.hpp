#ifndef VALIDATOR_HPP
#define VALIDATOR_HPP

#include "AST.hpp"

namespace validator {
enum ContextType { CTX_HTTP, CTX_SERVER, CTX_LOCATION };
enum DirectiveType {
  DIR_LISTEN,
  DIR_CLIENT_MAX_BODY_SIZE,
  DIR_ERROR_PAGE,
  DIR_ROOT,
  DIR_METHODS,
  DIR_AUTOINDEX,
  DIR_RETURN,
  DIR_UNKNOWN
};
struct ValidationState {
  bool has_client_max_body_size;
  bool has_server;
  bool has_location;
};

void validate(const Block &http);
}; // namespace validator

#endif // !VALIDATOR_HPP
