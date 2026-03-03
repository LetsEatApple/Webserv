//
// Created by mkugan on 3/3/26.
//

#include "DirectiveValidators.hpp"
#include "Utils.hpp"
#include <stdexcept>

namespace validator
{
    void validateErrorPage(const Directive& d, ValidationState& vs)
    {
        if (d.args.size() < 2)
            throw std::runtime_error(utils::errorMsg(
                "Expected at least 2 arguments for '" + d.name + "' directive",
                d.line));
        vs.has_error_page = true;
    }

    void validateClientMaxBodySize(const Directive& d, ValidationState& vs)
    {
        if (vs.has_client_max_body_size)
            throw std::runtime_error(
                utils::errorMsg("Duplicate '" + d.name + "' directive", d.line));
        if (d.args.size() != 1)
            throw std::runtime_error(utils::errorMsg(
                "Expected exactly 1 argument for '" + d.name + "' directive", d.line));
        vs.has_client_max_body_size = true;
    }

    void validateListen(const Directive& d, ContextType ctx,
                               ValidationState& vs)
    {
        if (ctx != CTX_SERVER)
            throw std::runtime_error(utils::errorMsg(
                "'" + d.name + "' directive is not allowed here", d.line));
        if (d.args.size() != 1)
            throw std::runtime_error(utils::errorMsg(
                "Expected exactly 1 argument for '" + d.name + "' directive", d.line));
        vs.has_listen = true;
    }

    void validateServerName(const Directive& d, ContextType ctx,
                            ValidationState& vs)
    {
        if (ctx != CTX_SERVER)
            throw std::runtime_error(utils::errorMsg(
                "'" + d.name + "' directive is not allowed here", d.line));
        if (d.args.empty())
            throw std::runtime_error(utils::errorMsg(
                "Expected at least 1 argument for '" + d.name + "' directive", d.line));
        vs.has_listen = true;
    }

    void validateLimitExcept(const Directive& d, ContextType ctx,
                             ValidationState& vs)
    {
        if (vs.has_limit_exept)
            throw std::runtime_error(
                utils::errorMsg("Duplicate '" + d.name + "' directive", d.line));
        if (ctx != CTX_LOCATION)
            throw std::runtime_error(utils::errorMsg(
                "'" + d.name + "' directive is not allowed here", d.line));
        if (d.args.empty() || d.args.size() > 3)
            throw std::runtime_error(utils::errorMsg(
                "Expected between 1 and 3 arguments for '" + d.name + "' directive",
                d.line));
        vs.has_limit_exept = true;
    }

    void validateReturn(const Directive& d, ContextType ctx,
                        ValidationState& vs)
    {
        if (ctx != CTX_SERVER && ctx != CTX_LOCATION)
            throw std::runtime_error(utils::errorMsg(
                "'" + d.name + "' directive is not allowed here", d.line));
        if (d.args.size() != 2)
            throw std::runtime_error(utils::errorMsg(
                "Expected exactly 2 arguments for '" + d.name + "' directive", d.line));
        vs.has_return = true;
    }

    void validateCGI(const Directive& d, ContextType ctx,
                     ValidationState& vs)
    {
        if (ctx != CTX_SERVER && ctx != CTX_LOCATION)
            throw std::runtime_error(utils::errorMsg(
                "'" + d.name + "' directive is not allowed here", d.line));
        if (d.args.size() != 2)
            throw std::runtime_error(utils::errorMsg(
                "Expected exactly 2 arguments for '" + d.name + "' directive", d.line));
        vs.has_cgi = true;
    }

    void validateAlias(const Directive& d, ContextType ctx,
                       ValidationState& vs)
    {
        if (vs.has_alias)
            throw std::runtime_error(
                utils::errorMsg("Duplicate '" + d.name + "' directive", d.line));
        if (ctx != CTX_LOCATION)
            throw std::runtime_error(utils::errorMsg(
                "'" + d.name + "' directive is not allowed here", d.line));
        if (d.args.size() != 1)
            throw std::runtime_error(utils::errorMsg(
                "Expected exactly 1 argument for '" + d.name + "' directive", d.line));
        vs.has_alias = true;
    }

    void validateAutoindex(const Directive& d, ValidationState& vs)
    {
        if (vs.has_autoindex)
            throw std::runtime_error(
                utils::errorMsg("Duplicate '" + d.name + "' directive", d.line));
        if (d.args.size() != 1)
            throw std::runtime_error(utils::errorMsg(
                "Expected exactly 1 argument for '" + d.name + "' directive", d.line));
        vs.has_autoindex = true;
    }

    void validateIndex(const Directive& d, ValidationState& vs)
    {
        if (vs.has_index)
            throw std::runtime_error(
                utils::errorMsg("Duplicate '" + d.name + "' directive", d.line));
        if (d.args.size() != 1)
            throw std::runtime_error(utils::errorMsg(
                "Expected exactly 1 argument for '" + d.name + "' directive", d.line));
        vs.has_index = true;
    }

    void validateUpload(const Directive& d, ValidationState& vs)
    {
        if (vs.has_upload)
            throw std::runtime_error(
                utils::errorMsg("Duplicate '" + d.name + "' directive", d.line));
        if (d.args.size() != 1)
            throw std::runtime_error(utils::errorMsg(
                "Expected exactly 1 argument for '" + d.name + "' directive", d.line));
        vs.has_upload = true;
    }
} // namespace validator
