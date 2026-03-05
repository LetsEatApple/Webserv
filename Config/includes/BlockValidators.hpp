#ifndef VALIDATOR_HPP
#define VALIDATOR_HPP

#include "AST.hpp"

namespace validator {
typedef unsigned int ContextMask;
typedef unsigned int ValidationMask;

enum ContextType { CTX_HTTP = 1 << 0, CTX_SERVER = 1 << 1, CTX_LOCATION = 1 << 2 };

enum DirectiveType {
  DIR_LISTEN,
  DIR_SERVER_NAME,
  DIR_CLIENT_MAX_BODY_SIZE,
  DIR_LIMIT_EXCEPT,
  DIR_ERROR_PAGE,
  DIR_ALIAS,
  DIR_AUTOINDEX,
  DIR_INDEX,
  DIR_RETURN,
  DIR_UPLOAD,
  DIR_CGI,
  DIR_UNKNOWN
};

enum ValidationFlag {
	HAS_CLIENT_MAX_BODY_SIZE = 1 << 0,
	HAS_SERVER = 1 << 1,
	HAS_LOCATION = 1 << 2,
	HAS_LISTEN = 1 << 3,
	HAS_LIMIT_EXCEPT = 1 << 4,
  HAS_SERVER_NAME = 1 << 5,
	HAS_ERROR_PAGE = 1 << 6,
	HAS_ALIAS = 1 << 7,
  HAS_AUTOINDEX = 1 << 8,
	HAS_INDEX = 1 << 9,
	HAS_RETURN = 1 << 10,
	HAS_UPLOAD = 1 << 11,
	HAS_CGI = 1 << 12
};

void validate(const Block &http);
};// namespace validator

#endif // !VALIDATOR_HPP
