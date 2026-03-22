#include "DirectiveValidators.hpp"
#include "ValidationHelpers.hpp"

namespace validator
{
    void validateErrorPage(const Directive& d, ValidationMask& seen)
    {
        seen |= HAS_ERROR_PAGE;
        requireAtLeastArgs(d, 2);
    }

    void validateClientMaxBodySize(const Directive& d, ValidationMask& seen)
    {
        rejectDuplicate(HAS_CLIENT_MAX_BODY_SIZE, seen, d);
        seen |= HAS_CLIENT_MAX_BODY_SIZE;
        requireExactArgs(d, 1);
    }

    void validateListen(const Directive& d, const ContextType ctx,
                        ValidationMask& seen)
    {
        seen |= HAS_LISTEN;
        requireContext(d, ctx, CTX_SERVER);
        requireExactArgs(d, 1);
    }

    void validateServerName(const Directive& d, const ContextType ctx,
                            ValidationMask& seen)
    {
        seen |= HAS_SERVER_NAME;
        requireContext(d, ctx, CTX_SERVER);
        requireAtLeastArgs(d, 1);
    }

    void validateLimitExcept(const Directive& d, const ContextType ctx,
                             ValidationMask& seen)
    {
        rejectDuplicate(HAS_LIMIT_EXCEPT, seen, d);
        seen |= HAS_LIMIT_EXCEPT;
        requireContext(d, ctx, CTX_LOCATION);
        requireArgsRange(d, 1, 3);
    }

    void validateReturn(const Directive& d, const ContextType ctx,
                        ValidationMask& seen)
    {
        seen |= HAS_RETURN;
        requireExactArgs(d, 2);
        requireContext(d, ctx, CTX_SERVER | CTX_LOCATION);
    }

    void validateCGI(const Directive& d, const ContextType ctx,
                     ValidationMask& seen)
    {
        seen |= HAS_CGI;
        requireExactArgs(d, 2);
        requireContext(d, ctx, CTX_SERVER | CTX_LOCATION);
    }

    void validateAlias(const Directive& d, const ContextType ctx,
                       ValidationMask& seen)
    {
        rejectDuplicate(HAS_ALIAS, seen, d);
        seen |= HAS_ALIAS;
        requireExactArgs(d, 1);
        requireContext(d, ctx, CTX_LOCATION);
    }

    void validateAutoindex(const Directive& d, ValidationMask& seen)
    {
        rejectDuplicate(HAS_AUTOINDEX, seen, d);
        seen |= HAS_AUTOINDEX;
        requireExactArgs(d, 1);
    }

    void validateIndex(const Directive& d, ValidationMask& seen)
    {
        rejectDuplicate(HAS_INDEX, seen, d);
        seen |= HAS_INDEX;
        requireExactArgs(d, 1);
    }

    void validateUpload(const Directive& d, ValidationMask& seen)
    {
        rejectDuplicate(HAS_UPLOAD, seen, d);
        seen |= HAS_UPLOAD;
        requireExactArgs(d, 1);
    }
} // namespace validator
