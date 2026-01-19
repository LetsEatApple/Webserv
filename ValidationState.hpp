#ifndef VALIDATION_STATE_HPP
#define VALIDATION_STATE_HPP

namespace Validator {
struct ValidationState {
  bool has_client_max_body_size;
  bool has_server;
  bool has_location;
};
}; // namespace Validator

#endif // !VALIDATION_STATE_HPP
