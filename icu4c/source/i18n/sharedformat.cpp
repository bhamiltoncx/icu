// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "sharedformat.h"
#include "unifiedcache.h"

U_NAMESPACE_BEGIN

template<> U_I18N_API
const SharedFormat *LocaleCacheKey<SharedFormat>::createObject(
        const void * /*creationContext*/, UErrorCode &status) const {
    status = U_UNSUPPORTED_ERROR;
    return NULL;
}

SharedFormat::~SharedFormat() { }

U_NAMESPACE_END

#endif  // #!UCONFIG_NO_FORMATTING
