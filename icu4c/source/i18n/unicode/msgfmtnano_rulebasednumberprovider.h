// © 2019 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef __SOURCE_I18N_UNICODE_MSGFMTNANO_RULEBASEDNUMBERPROVIDER_H__
#define __SOURCE_I18N_UNICODE_MSGFMTNANO_RULEBASEDNUMBERPROVIDER_H__

#include "unicode/utypes.h"

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/msgfmtnano.h"
#include "unicode/localpointer.h"
#include "unicode/uloc.h"

U_NAMESPACE_BEGIN

class U_I18N_API RuleBasedNumberFormatProviderNano {
    public:
    RuleBasedNumberFormatProviderNano() = delete;
    RuleBasedNumberFormatProviderNano &operator=(const RuleBasedNumberFormatProviderNano&) = delete;
    RuleBasedNumberFormatProviderNano &operator=(RuleBasedNumberFormatProviderNano&&) = delete;
    RuleBasedNumberFormatProviderNano(const RuleBasedNumberFormatProviderNano&) = delete;
    RuleBasedNumberFormatProviderNano(RuleBasedNumberFormatProviderNano&&) = delete;

    static LocalPointer<const RuleBasedNumberFormatProvider> U_EXPORT2 createInstance(UErrorCode& success);
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // __SOURCE_I18N_UNICODE_MSGFMTNANO_RULEBASEDNUMBERPROVIDER_H__
