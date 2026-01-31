#ifndef VALIDATOR_HPP
#define VALIDATOR_HPP

#include "AST.hpp"

namespace validator {
enum ContextType { CTX_HTTP, CTX_SERVER, CTX_LOCATION };
enum DirectiveType {
  DIR_LISTEN,
  DIR_SERVER_NAME,
  DIR_CLIENT_MAX_BODY_SIZE,
  DIR_ERROR_PAGE,
  DIR_LIMIT_EXCEPT,
  DIR_ALIAS,
  DIR_AUTOINDEX,
  DIR_INDEX,
  DIR_RETURN,
  DIR_UPLOAD,
  DIR_UNKNOWN
};

struct ValidationState {
  bool has_client_max_body_size;
  bool has_server;
  bool has_location;
  bool has_listen;
  bool has_limit_exept;
  bool has_server_name;
  bool has_error_page;
  bool has_alias;
  bool has_autoindex;
  bool has_index;
  bool has_return;
  bool has_upload;
};

void validate(const Block &http);
}; // namespace validator

#endif // !VALIDATOR_HPP
