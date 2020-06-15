// © 2019 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef __SOURCE_I18N_UNICODE_MSGFMTNANO_H__
#define __SOURCE_I18N_UNICODE_MSGFMTNANO_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#if U_SHOW_CPLUSPLUS_API

/**
 * \file
 * \brief C++ API: Formats messages in a language-neutral way.
 */

#if !UCONFIG_NO_FORMATTING

#include "unicode/format.h"
#include "unicode/localpointer.h"
#include "unicode/locid.h"
#include "unicode/messagepattern.h"
#include "unicode/parseerr.h"
#include "unicode/unistr.h"

U_NAMESPACE_BEGIN

class U_I18N_API MessageFormatProvider {
public:
    enum NumberFormatType {
        MFP_NUMBER_FORMAT_TYPE_NUMBER,
        MFP_NUMBER_FORMAT_TYPE_CURRENCY,
        MFP_NUMBER_FORMAT_TYPE_PERCENT,
    };

    enum RuleBasedNumberFormatType {
        MFP_RBNF_TYPE_SPELLOUT,
        MFP_RBNF_TYPE_DURATION,
        MFP_RBNF_TYPE_ORDINAL,
    };

    enum TimeDateType {
        MFP_TIMEDATE_TYPE_TIME,
        MFP_TIMEDATE_TYPE_DATE,
        MFP_TIMEDATE_TYPE_DATETIME,
    };

    enum TimeDateStyle {
        MFP_TIMEDATE_STYLE_SHORT,
        MFP_TIMEDATE_STYLE_MEDIUM,
        MFP_TIMEDATE_STYLE_LONG,
        MFP_TIMEDATE_STYLE_FULL,
    };

    MessageFormatProvider(const MessageFormatProvider&) = delete;
    MessageFormatProvider(MessageFormatProvider&&) = delete;
    MessageFormatProvider &operator=(const MessageFormatProvider&) = delete;
    MessageFormatProvider &operator=(MessageFormatProvider&&) = delete;

    virtual ~MessageFormatProvider() {}

    virtual const Format* numberFormat(NumberFormatType type, const Locale& locale, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return nullptr;
    }

    virtual const Format* numberFormatForSkeleton(const UnicodeString& skeleton, const Locale& locale, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return nullptr;
    }

    virtual const Format* decimalFormatWithPattern(const UnicodeString& pattern, const Locale& locale, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return nullptr;
    }

    virtual const Format* timeDateFormatForSkeleton(TimeDateType type, const UnicodeString& skeleton, const Locale& locale, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return nullptr;
    }

    virtual const Format* timeDateFormat(TimeDateType type, TimeDateStyle style, const Locale& locale, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return nullptr;
    }

    virtual const Format* ruleBasedNumberFormat(RuleBasedNumberFormatType type, const Locale& locale, const UnicodeString& defaultRuleSet, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return nullptr;
    }
};

class U_I18N_API MessageFormatNano {
public:
    /**
     * Constructs a new MessageFormatNano using the given pattern and locale.
     * @param pattern   Pattern used to construct object.
     * @param locale The locale to use for formatting dates and numbers.
     * @param formatProvider Provider and cache for formats. By default,
     *                       supports {@link ChoiceFormat}, {@link PluralFormat},
     *                       and {@link SelectFormat}.
     * @param parseError Struct to receive information on the position
     *                   of an error within the pattern.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     */
    MessageFormatNano(const UnicodeString& pattern,
                      const Locale& locale,
                      UParseError& parseError,
                      UErrorCode& status);

    /**
     * Constructs a new MessageFormatNano using the given pattern and locale.
     * @param pattern   Pattern used to construct object.
     * @param locale The locale to use for formatting dates and numbers.
     * @param formatProvider Provider and cache for formats. By default,
     *                       supports {@link ChoiceFormat}, {@link PluralFormat},
     *                       and {@link SelectFormat}.
     * @param parseError Struct to receive information on the position
     *                   of an error within the pattern.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     */
    MessageFormatNano(const UnicodeString& pattern,
                      const Locale& locale,
                      LocalPointer<const FormatProvider> formatProvider,
                      UParseError& parseError,
                      UErrorCode& status);

    /**
     * Formats the given array of arguments into a user-defined argument name
     * array. This function supports both named and numbered
     * arguments-- if numbered, the formatName is the
     * corresponding UnicodeStrings (e.g. "0", "1", "2"...).
     *
     * @param argumentNames argument name array
     * @param arguments An array of objects to be formatted.
     * @param count     The number of elements of 'argumentNames' and
     *                  arguments.  The number of argumentNames and arguments
     *                  must be the same.
     * @param appendTo  Output parameter to receive result.
     *                  Result is appended to existing contents.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     * @return          Reference to 'appendTo' parameter.
     * @stable ICU 4.0
     */
    UnicodeString& format(const UnicodeString* argumentNames,
                          const Formattable* arguments,
                          int32_t count,
                          UnicodeString& appendTo,
                          UErrorCode& status) const;
    
    MessageFormatNano(const MessageFormatNano&) = delete;
    MessageFormatNano(MessageFormatNano&&) = delete;
    MessageFormatNano &operator=(const MessageFormatNano&) = delete;
    MessageFormatNano &operator=(MessageFormatNano&&) = delete;
    
private:
    const Locale locale;
    const MessagePattern msgPattern;
    const LocalPointer<const FormatProvider> formatProvider,
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // __SOURCE_I18N_UNICODE_MSGFMTNANO_H__
//eof
