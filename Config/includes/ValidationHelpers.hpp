//
// Created by mkugan on 3/3/26.
//

#ifndef VALIDATIONHELPERS_HPP
#define VALIDATIONHELPERS_HPP

#include "AST.hpp"
#include "BlockValidators.hpp"

namespace validator
{
    void rejectDuplicate(const ValidationFlag& flag, const ValidationMask& seen, const Directive& d);
    void requireExactArgs(const Directive& d, std::size_t expected);
    void requireArgsRange(const Directive& d, std::size_t min, std::size_t max);
    void requireAtLeastArgs(const Directive& d, std::size_t min);
    void requireContext(const Directive& d, ContextType actual, ContextMask allowed);
} // namespace validator

#endif // !VALIDATIONHELPERS_HPP
