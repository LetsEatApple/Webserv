#ifndef VALIDATOR_TYPES_HPP
#define VALIDATOR_TYPES_HPP

namespace Validator {
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

}; // namespace Validator
#endif // !VALIDATOR_TYPES_HPP
