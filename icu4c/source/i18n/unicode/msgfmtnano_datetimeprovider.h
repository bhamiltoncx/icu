// © 2019 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef __SOURCE_I18N_UNICODE_MSGFMTNANO_DATETIMEPROVIDER_H__
#define __SOURCE_I18N_UNICODE_MSGFMTNANO_DATETIMEPROVIDER_H__

#include "unicode/utypes.h"

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/msgfmtnano.h"
#include "unicode/localpointer.h"

U_NAMESPACE_BEGIN

class U_I18N_API DateTimeFormatProviderNano {
    public:
    DateTimeFormatProviderNano() = delete;
    DateTimeFormatProviderNano &operator=(const DateTimeFormatProviderNano&) = delete;
    DateTimeFormatProviderNano &operator=(DateTimeFormatProviderNano&&) = delete;
    DateTimeFormatProviderNano(const DateTimeFormatProviderNano&) = delete;
    DateTimeFormatProviderNano(DateTimeFormatProviderNano&&) = delete;

    static LocalPointer<const DateTimeFormatProvider> U_EXPORT2 createInstance(UErrorCode& success);
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // __SOURCE_I18N_UNICODE_MSGFMTNANO_DATETIMEPROVIDER_H__
