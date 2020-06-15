// © 2019 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef __SOURCE_I18N_UNICODE_MSGFMTNANO_H__
#define __SOURCE_I18N_UNICODE_MSGFMTNANO_H__

#include "unicode/utypes.h"

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

class U_I18N_API FormatProvider {
public:
    enum NumberFormatType {
        TYPE_NUMBER,
        TYPE_CURRENCY,
        TYPE_PERCENT,
        TYPE_INTEGER,
    };

    enum RuleBasedNumberFormatType {
        TYPE_SPELLOUT,
        TYPE_DURATION,
        TYPE_ORDINAL,
    };

    enum DateTimeType {
        TYPE_TIME,
        TYPE_DATE,
        TYPE_DATETIME,
    };

    enum DateTimeStyle {
        STYLE_DEFAULT,
        STYLE_SHORT,
        STYLE_MEDIUM,
        STYLE_LONG,
        STYLE_FULL,
    };

    FormatProvider(const FormatProvider&) = delete;
    FormatProvider(FormatProvider&&) = delete;
    FormatProvider &operator=(const FormatProvider&) = delete;
    FormatProvider &operator=(FormatProvider&&) = delete;

    FormatProvider() = default;
    virtual ~FormatProvider() {}

    virtual const Format* numberFormat(NumberFormatType /*type*/, const Locale& /*locale*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return nullptr;
    }

    virtual const Format* numberFormatForSkeleton(const UnicodeString& /*skeleton*/, const Locale& /*locale*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return nullptr;
    }

    virtual const Format* decimalFormatWithPattern(const UnicodeString& /*pattern*/, const Locale& /*locale*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return nullptr;
    }

    virtual const Format* dateTimeFormatForSkeleton(const UnicodeString& /*skeleton*/, const Locale& /*locale*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return nullptr;
    }

    virtual const Format* dateTimeFormat(DateTimeType /*type*/, DateTimeStyle /*style*/, const Locale& /*locale*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return nullptr;
    }

    virtual const Format* ruleBasedNumberFormat(RuleBasedNumberFormatType /*type*/, const Locale& /*locale*/, const UnicodeString& /*defaultRuleSet*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return nullptr;
    }

    class PluralSelectorProvider {
  public:
        PluralSelectorProvider(const PluralSelectorProvider&) = delete;
        PluralSelectorProvider(PluralSelectorProvider&&) = delete;
        PluralSelectorProvider &operator=(const PluralSelectorProvider&) = delete;
        PluralSelectorProvider &operator=(PluralSelectorProvider&&) = delete;
        
        virtual ~PluralSelectorProvider();
        virtual UnicodeString select(void *ctx, double number, UErrorCode& ec) const = 0;
    };

    virtual const PluralSelectorProvider* pluralSelectorProviderForArgType(UMessagePatternArgType /*argType*/, UErrorCode& status) const {
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
    const LocalPointer<const FormatProvider> formatProvider;
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // __SOURCE_I18N_UNICODE_MSGFMTNANO_H__
