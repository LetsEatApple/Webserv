//
// Created by mkugan on 3/5/26.
//

#ifndef DIRECTIVEAPPLIER_HPP
#define DIRECTIVEAPPLIER_HPP

#include "AST.hpp"
#include "Config.hpp"

namespace config
{
    void applyLocationPath(Location& loc, Block const& b);
    void applyAlias(Location& loc, Directive const& d);
    void applyAutoindex(Location& loc, Directive const& d);
    void applyIndex(Location& loc, Directive const& d);
    void applyUpload(Location& loc, Directive const& d);
    void applyLimitExcept(Location& loc, Directive const& d);
    void applyClientMaxBodySize(Location& loc, Directive const& d);
    void applyErrorPages(Location& loc, Directive const& d);
    void applyReturn(Location& loc, Directive const& d);
    void applyCGI(Location& loc, Directive const& d);
} // namespace config

#endif // !DIRECTIVEAPPLIER_HPP
