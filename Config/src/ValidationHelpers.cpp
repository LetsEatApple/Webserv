#include "ValidationHelpers.hpp"

#include <AST.hpp>
#include <stdexcept>
#include <Utils.hpp>

void validator::rejectDuplicate(const ValidationFlag& flag, const ValidationMask& seen, const Directive& d)
{
    if (flag & seen)
        throw std::runtime_error(
            utils::errorMsg("Duplicate '" + d.name + "' directive", d.line));
}

void validator::requireExactArgs(const Directive& d, const std::size_t expected)
{
    if (d.args.size() != expected)
        throw std::runtime_error(
            utils::errorMsg(
                "Expected exactly " + utils::toString(expected) + " argument(s) for '" + d.name + "' directive",
                d.line));
}

void validator::requireArgsRange(const Directive& d, const std::size_t min, const std::size_t max)
{
    if (d.args.size() < min || d.args.size() > max)
        throw std::runtime_error(
            utils::errorMsg(
                "Expected between " + utils::toString(min) + " and " + utils::toString(max) + " argument for '" + d.name
                + "'", d.line));
}

void validator::requireAtLeastArgs(const Directive& d, const std::size_t min)
{
    if (d.args.size() < min)
        throw std::runtime_error(
            utils::errorMsg("Expected at least 1 argument for '" + d.name + "' directive", d.line));
}

void validator::requireContext(const Directive& d, const ContextType actual, const ContextMask allowed)
{
    if (!(actual & allowed))
        throw std::runtime_error(utils::errorMsg(
            "'" + d.name + "' directive is not allowed here", d.line));
}
