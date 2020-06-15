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

    enum PluralType {
        TYPE_PLURAL,
        TYPE_SELECTORDINAL,
    };

    FormatProvider() = default;
    FormatProvider(const FormatProvider&) = delete;
    FormatProvider(FormatProvider&&) = delete;
    FormatProvider &operator=(const FormatProvider&) = delete;
    FormatProvider &operator=(FormatProvider&&) = delete;

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

    class PluralSelector {
    public:
        PluralSelector() = default;
        PluralSelector(const PluralSelector&) = delete;
        PluralSelector(PluralSelector&&) = delete;
        PluralSelector &operator=(const PluralSelector&) = delete;
        PluralSelector &operator=(PluralSelector&&) = delete;
        
        virtual ~PluralSelector();
        virtual UnicodeString select(void *ctx, double number, UErrorCode& ec) const = 0;
    };

    virtual const PluralSelector* pluralSelector(PluralType /*pluralType*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return nullptr;
    }

    virtual int32_t pluralFindSubMessage(const MessagePattern& /*pattern*/, int32_t /*partIndex*/, const PluralSelector& /*selectorProvider*/, void* /*context*/, double /*number*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return -1;
    }

    virtual int32_t selectFindSubMessage(const MessagePattern& /*pattern*/, int32_t /*partIndex*/, const UnicodeString& /*keyword*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return -1;
    }

    struct PluralSelectorContext {
        PluralSelectorContext(
            int32_t start, const UnicodeString &name,
            const Formattable &num, double off, UErrorCode &errorCode)
                : startIndex(start), argName(name), offset(off),
                  numberArgIndex(-1), formatter(NULL), forReplaceNumber(FALSE) {
            // number needs to be set even when select() is not called.
            // Keep it as a Number/Formattable:
            // For format() methods, and to preserve information (e.g., BigDecimal).
            if(off == 0) {
                number = num;
            } else {
                number = num.getDouble(errorCode) - off;
            }
        }

        PluralSelectorContext(const PluralSelectorContext& other) = delete;
        PluralSelectorContext(PluralSelectorContext&&) = delete;
        PluralSelectorContext &operator=(const PluralSelectorContext&) = delete;
        PluralSelectorContext &operator=(PluralSelectorContext&&) = delete;

        // Input values for plural selection with decimals.
        int32_t startIndex;
        const UnicodeString &argName;
        /** argument number - plural offset */
        Formattable number;
        double offset;
        // Output values for plural selection with decimals.
        /** -1 if REPLACE_NUMBER, 0 arg not found, >0 ARG_START index */
        int32_t numberArgIndex;
        const Format *formatter;
        /** formatted argument number - plural offset */
        UnicodeString numberString;
        /** TRUE if number-offset was formatted with the stock number formatter */
        UBool forReplaceNumber;
    };
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
