//
// Created by mkugan on 3/3/26.
//

#ifndef WEBSERV_DIRECTIVEVALIDATORS_HPP
#define WEBSERV_DIRECTIVEVALIDATORS_HPP

#include "AST.hpp"
#include "Validator.hpp"

namespace validator
{
    void validateErrorPage(const Directive& d, ValidationState& vs);
    void validateClientMaxBodySize(const Directive& d, ValidationState& vs);
    void validateListen(const Directive& d, ContextType ctx, ValidationState& vs);
    void validateServerName(const Directive& d, ContextType ctx, ValidationState& vs);
    void validateLimitExcept(const Directive& d, ContextType ctx, ValidationState& vs);
    void validateReturn(const Directive& d, ContextType ctx, ValidationState& vs);
    void validateCGI(const Directive& d, ContextType ctx, ValidationState& vs);
    void validateAlias(const Directive& d, ContextType ctx, ValidationState& vs);
    void validateAutoindex(const Directive& d, ValidationState& vs);
    void validateIndex(const Directive& d, ValidationState& vs);
    void validateUpload(const Directive& d, ValidationState& vs);
} // namespace validator

#endif //WEBSERV_DIRECTIVEVALIDATORS_HPP
