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

class U_I18N_API NumberFormatProvider {
public:
    enum NumberFormatType {
        TYPE_NUMBER,
        TYPE_CURRENCY,
        TYPE_PERCENT,
        TYPE_INTEGER,
    };

    NumberFormatProvider() = default;
    NumberFormatProvider(NumberFormatProvider&&) = default;
    NumberFormatProvider &operator=(NumberFormatProvider&&) = default;
    NumberFormatProvider(const NumberFormatProvider&) = delete;
    NumberFormatProvider &operator=(const NumberFormatProvider&) = delete;

    virtual ~NumberFormatProvider() {}

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
};

class U_I18N_API DateTimeFormatProvider {
public:
    DateTimeFormatProvider() = default;
    DateTimeFormatProvider(DateTimeFormatProvider&&) = default;
    DateTimeFormatProvider &operator=(DateTimeFormatProvider&&) = default;
    DateTimeFormatProvider(const DateTimeFormatProvider&) = delete;
    DateTimeFormatProvider &operator=(const DateTimeFormatProvider&) = delete;

    virtual ~DateTimeFormatProvider() {}

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

    virtual const Format* dateTimeFormatForSkeleton(const UnicodeString& /*skeleton*/, const Locale& /*locale*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return nullptr;
    }

    virtual const Format* dateTimeFormat(DateTimeType /*type*/, DateTimeStyle /*style*/, const Locale& /*locale*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return nullptr;
    }
};

class U_I18N_API RuleBasedNumberFormatProvider {
public:
    RuleBasedNumberFormatProvider() = default;
    RuleBasedNumberFormatProvider(RuleBasedNumberFormatProvider&&) = default;
    RuleBasedNumberFormatProvider &operator=(RuleBasedNumberFormatProvider&&) = default;
    RuleBasedNumberFormatProvider(const RuleBasedNumberFormatProvider&) = delete;
    RuleBasedNumberFormatProvider &operator=(const RuleBasedNumberFormatProvider&) = delete;

    virtual ~RuleBasedNumberFormatProvider() {}

    enum RuleBasedNumberFormatType {
        TYPE_SPELLOUT,
        TYPE_DURATION,
        TYPE_ORDINAL,
    };

    virtual const Format* ruleBasedNumberFormat(RuleBasedNumberFormatType /*type*/, const Locale& /*locale*/, const UnicodeString& /*defaultRuleSet*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return nullptr;
    }
};

class U_I18N_API PluralFormatProvider {
public:
    PluralFormatProvider() = default;
    PluralFormatProvider(PluralFormatProvider&&) = default;
    PluralFormatProvider &operator=(PluralFormatProvider&&) = default;
    PluralFormatProvider(const PluralFormatProvider&) = delete;
    PluralFormatProvider &operator=(const PluralFormatProvider&) = delete;

    virtual ~PluralFormatProvider() {}

    enum PluralType {
        TYPE_CARDINAL,
        TYPE_ORDINAL,
    };

    class U_I18N_API Selector {
    public:
        Selector() = default;
        Selector(Selector&&) = default;
        Selector &operator=(Selector&&) = default;
        Selector(const Selector&) = delete;
        Selector &operator=(const Selector&) = delete;

        virtual ~Selector() { }
        virtual UnicodeString select(void *ctx, double number, UErrorCode& ec) const = 0;
    };

    virtual const Selector* pluralSelector(PluralType /*pluralType*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
        return nullptr;
    }

    struct U_I18N_API SelectorContext {
        SelectorContext(
            MessagePattern msgPattern, const NumberFormatProvider& numberFormatProvider,
            Locale locale,
            int32_t start, const UnicodeString &name,
            const Formattable &num, double off, UErrorCode &errorCode)
                : msgPattern(msgPattern), numberFormatProvider(numberFormatProvider),
          locale(locale),
          startIndex(start), argName(name), offset(off),
                  numberArgIndex(-1), formatter(NULL), forReplaceNumber(FALSE) {
            // number needs to be set even when select() is not called.
            // Keep it as a Number/Formattable:
            // For format() methods, and to preserve information (e.g., BigDecimal).
            if(off == 0) {
                number = num;
                fprintf(stderr, "SelectorContext offset zero, number=%f\n", number.getDouble(errorCode));
            } else {
                number = num.getDouble(errorCode) - off;
                fprintf(stderr, "SelectorContext offset %f, number=%f\n", off, number.getDouble(errorCode));
            }
        }

        SelectorContext(SelectorContext&&) = default;
        SelectorContext &operator=(SelectorContext&&) = default;
        SelectorContext(const SelectorContext& other) = delete;
        SelectorContext &operator=(const SelectorContext&) = delete;

        const MessagePattern msgPattern;
        const NumberFormatProvider& numberFormatProvider;
        const Locale locale;

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
     * @param numberFormatProvider Provider and cache for number formats.
     * @param dateTimeFormatProvider Provider and cache for date and time formats.
     * @param ruleBasedNumberFormatProvider Provider and cache for rule-based message formats.
     * @param pluralFormatProvider Provider and cache for number formats.
     * @param parseError Struct to receive information on the position
     *                   of an error within the pattern.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     */
    MessageFormatNano(const UnicodeString& pattern,
                      const Locale& locale,
                      LocalPointer<const NumberFormatProvider> numberFormatProvider,
                      LocalPointer<const DateTimeFormatProvider> dateTimeFormatProvider,
                      LocalPointer<const RuleBasedNumberFormatProvider> ruleBasedNumberFormatProvider,
                      LocalPointer<const PluralFormatProvider> pluralFormatProvider,
                      UParseError& parseError,
                      UErrorCode& status);

    MessageFormatNano(MessageFormatNano&&) = default;
    MessageFormatNano &operator=(MessageFormatNano&&) = default;

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
    MessageFormatNano &operator=(const MessageFormatNano&) = delete;

private:
    const Locale locale;
    const MessagePattern msgPattern;
    const LocalPointer<const NumberFormatProvider> numberFormatProvider;
    const LocalPointer<const DateTimeFormatProvider> dateTimeFormatProvider;
    const LocalPointer<const RuleBasedNumberFormatProvider> ruleBasedNumberFormatProvider;
    const LocalPointer<const PluralFormatProvider> pluralFormatProvider;
};

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // __SOURCE_I18N_UNICODE_MSGFMTNANO_H__
