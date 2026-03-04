//
// Created by mkugan on 3/3/26.
//

#ifndef WEBSERV_DIRECTIVEVALIDATORS_HPP
#define WEBSERV_DIRECTIVEVALIDATORS_HPP

#include "AST.hpp"
#include "BlockValidators.hpp"

namespace validator
{
    void validateErrorPage(const Directive& d, ValidationMask &seen);
    void validateClientMaxBodySize(const Directive& d, ValidationMask &seen);
    void validateListen(const Directive& d, ContextType ctx, ValidationMask &seen);
    void validateServerName(const Directive& d, ContextType ctx, ValidationMask &seen);
    void validateLimitExcept(const Directive& d, ContextType ctx, ValidationMask &seen);
    void validateReturn(const Directive& d, ContextType ctx, ValidationMask &seen);
    void validateCGI(const Directive& d, ContextType ctx, ValidationMask &seen);
    void validateAlias(const Directive& d, ContextType ctx, ValidationMask &seen);
    void validateAutoindex(const Directive& d, ValidationMask &seen);
    void validateIndex(const Directive& d, ValidationMask &seen);
    void validateUpload(const Directive& d, ValidationMask &seen);
} // namespace validator

#endif //WEBSERV_DIRECTIVEVALIDATORS_HPP
