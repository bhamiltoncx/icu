// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "shareddateformat.h"
#include "unifiedcache.h"

U_NAMESPACE_BEGIN

template<> U_I18N_API
const SharedDateFormat *LocaleCacheKey<SharedDateFormat>::createObject(
        const void * /*creationContext*/, UErrorCode &status) const {
    status = U_UNSUPPORTED_ERROR;
    return NULL;
}

SharedDateFormat::~SharedDateFormat() { }

U_NAMESPACE_END

#endif  // #!UCONFIG_NO_FORMATTING
