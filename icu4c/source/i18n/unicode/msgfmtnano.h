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
#include "unicode/timezone.h"
#include "unicode/unistr.h"
#include "unicode/uobject.h"

U_NAMESPACE_BEGIN

class U_I18N_API NumberFormatProvider : public UObject {
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

    ~NumberFormatProvider();

    virtual void formatNumber(const Formattable& /*number*/, NumberFormatType /*type*/, const Locale& /*locale*/, UnicodeString& /*appendTo*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
    }

    virtual void formatNumberWithSkeleton(const Formattable& /*number*/, const UnicodeString& /*skeleton*/, const Locale& /*locale*/, UnicodeString& /*appendTo*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
    }

    virtual void formatDecimalNumberWithPattern(const Formattable& /*number*/, const UnicodeString& /*pattern*/, const Locale& /*locale*/, UnicodeString& /*appendTo*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
    }
};

class U_I18N_API DateTimeFormatProvider : public UObject {
public:
    DateTimeFormatProvider() = default;
    DateTimeFormatProvider(DateTimeFormatProvider&&) = default;
    DateTimeFormatProvider &operator=(DateTimeFormatProvider&&) = default;
    DateTimeFormatProvider(const DateTimeFormatProvider&) = delete;
    DateTimeFormatProvider &operator=(const DateTimeFormatProvider&) = delete;

    ~DateTimeFormatProvider();

    enum DateTimeStyle {
        STYLE_NONE,
        STYLE_DEFAULT,
        STYLE_SHORT,
        STYLE_MEDIUM,
        STYLE_LONG,
        STYLE_FULL,
    };

    virtual void formatDateTime(const Formattable& /*date*/, DateTimeStyle /*dateStyle*/, DateTimeStyle /*timeStyle*/, const Locale& /*locale*/, const TimeZone* /*timeZone*/, UnicodeString& /*appendTo*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
    }
    
    virtual void formatDateTimeWithSkeleton(const Formattable& /*date*/, const UnicodeString& /*skeleton*/, const Locale& /*locale*/, const TimeZone* /*timeZone*/, UnicodeString& /*appendTo*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
    }
};

class U_I18N_API RuleBasedNumberFormatProvider : public UObject {
public:
    RuleBasedNumberFormatProvider() = default;
    RuleBasedNumberFormatProvider(RuleBasedNumberFormatProvider&&) = default;
    RuleBasedNumberFormatProvider &operator=(RuleBasedNumberFormatProvider&&) = default;
    RuleBasedNumberFormatProvider(const RuleBasedNumberFormatProvider&) = delete;
    RuleBasedNumberFormatProvider &operator=(const RuleBasedNumberFormatProvider&) = delete;

    ~RuleBasedNumberFormatProvider();

    enum RuleBasedNumberFormatType {
        TYPE_SPELLOUT,
        TYPE_DURATION,
        TYPE_ORDINAL,
    };

    virtual void formatRuleBasedNumber(const Formattable& /*number*/, RuleBasedNumberFormatType /*type*/, const Locale& /*locale*/, const UnicodeString& /*defaultRuleSet*/, UnicodeString& /*appendTo*/, UErrorCode& status) const {
        status = U_UNSUPPORTED_ERROR;
    }
};

class U_I18N_API PluralFormatProvider : public UObject {
public:
    PluralFormatProvider() = default;
    PluralFormatProvider(PluralFormatProvider&&) = default;
    PluralFormatProvider &operator=(PluralFormatProvider&&) = default;
    PluralFormatProvider(const PluralFormatProvider&) = delete;
    PluralFormatProvider &operator=(const PluralFormatProvider&) = delete;

    ~PluralFormatProvider();

    enum PluralType {
        TYPE_CARDINAL,
        TYPE_ORDINAL,
    };

    class U_I18N_API Selector : public UObject {
    public:
        Selector() = default;
        Selector(Selector&&) = default;
        Selector &operator=(Selector&&) = default;
        Selector(const Selector&) = delete;
        Selector &operator=(const Selector&) = delete;

        ~Selector();
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
                  numberArgIndex(-1), forReplaceNumber(FALSE) {
            // number needs to be set even when select() is not called.
            // Keep it as a Number/Formattable:
            // For format() methods, and to preserve information (e.g., BigDecimal).
            if(off == 0) {
                number = num;
            } else {
                number = num.getDouble(errorCode) - off;
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
        /** formatted argument number - plural offset */
        UnicodeString numberString;
        /** TRUE if number-offset was formatted with the stock number formatter */
        UBool forReplaceNumber;
    };
};

class U_I18N_API MessageFormatNano : public UObject {
public:
    /**
     * Constructs a new MessageFormatNano using the given pattern and locale.
     * @param pattern   Pattern used to construct object.
     * @param formatProvider Provider and cache for formats. By default,
     *                       supports {@link ChoiceFormat}, {@link PluralFormat},
     *                       and {@link SelectFormat}.
     * @param parseError Struct to receive information on the position
     *                   of an error within the pattern.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     */
    MessageFormatNano(const UnicodeString& pattern,
                      UParseError& parseError,
                      UErrorCode& status);

    /**
     * Constructs a new MessageFormatNano using the given pattern and locale.
     * @param pattern   Pattern used to construct object.
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
                      LocalPointer<const NumberFormatProvider> numberFormatProvider,
                      LocalPointer<const DateTimeFormatProvider> dateTimeFormatProvider,
                      LocalPointer<const RuleBasedNumberFormatProvider> ruleBasedNumberFormatProvider,
                      LocalPointer<const PluralFormatProvider> pluralFormatProvider,
                      UParseError& parseError,
                      UErrorCode& status);

    MessageFormatNano(MessageFormatNano&&) = default;
    MessageFormatNano &operator=(MessageFormatNano&&) = default;
    ~MessageFormatNano();

    class FormatParamsBuilder;

    struct FormatParams {
        const UnicodeString* const argumentNames;
        const Formattable* const arguments;
        int32_t count;
        Locale locale;
        UDate date;
        LocalPointer<const TimeZone> timeZone;
        
        FormatParams(const FormatParams&) = delete;
        FormatParams &operator=(const FormatParams&) = delete;
        FormatParams(FormatParams&&) = default;
        FormatParams &operator=(FormatParams&&) = default;
        
      private:
        FormatParams(const UnicodeString* const argumentNames,
                     const Formattable* const arguments,
                     int32_t count,
                     Locale locale,
                     UDate date,
                     LocalPointer<const TimeZone> timeZone) :
            argumentNames(argumentNames),
            arguments(arguments),
            count(count),
            locale(locale),
            date(date),
            timeZone(std::move(timeZone)) { }
        
        friend class FormatParamsBuilder;
    };

    class FormatParamsBuilder {
      public:
        FormatParamsBuilder(const FormatParamsBuilder&) = delete;
        FormatParamsBuilder &operator=(const FormatParamsBuilder&) = delete;
        FormatParamsBuilder(FormatParamsBuilder&&) = default;
        FormatParamsBuilder &operator=(FormatParamsBuilder&&) = default;
        
        static FormatParamsBuilder withNamedArguments(const UnicodeString* const argumentNames, const Formattable* const arguments, int32_t count) {
            return FormatParamsBuilder(argumentNames, arguments, count);
        }
        static FormatParamsBuilder withArguments(const Formattable* arguments, int32_t count) {
            return FormatParamsBuilder(/*argumentNames=*/nullptr, arguments, count);
        }

        FormatParamsBuilder& setLocale(const Locale& locale) {
            this->locale = locale;
            return *this;
        }
        FormatParamsBuilder& setDate(UDate date) {
            this->date = date;
            return *this;
        }
        FormatParamsBuilder& setTimeZone(LocalPointer<const TimeZone> timeZone) {
            this->timeZone = std::move(timeZone);
            return *this;
        }
        
        FormatParams build() {
            return FormatParams(argumentNames, arguments, count, locale, date, std::move(timeZone));
        }
        
  private:
      FormatParamsBuilder(const UnicodeString* const argumentNames, const Formattable* const arguments, int32_t count) :
        argumentNames(argumentNames), arguments(arguments), count(count) { }
        
        const UnicodeString* const argumentNames;
        const Formattable* const arguments;
        int32_t count;
        Locale locale;
        UDate date;
        LocalPointer<const TimeZone> timeZone;
    };

    /**
     * Formats the given array of arguments into a user-defined argument name
     * array. This function supports both named and numbered
     * arguments-- if numbered, the formatName is the
     * corresponding UnicodeStrings (e.g. "0", "1", "2"...).
     *
     * @param formatParams Parameters for the format.
     * @param appendTo  Output parameter to receive result.
     *                  Result is appended to existing contents.
     * @param status    Input/output error code.  If the
     *                  pattern cannot be parsed, set to failure code.
     * @return          Reference to 'appendTo' parameter.
     * @stable ICU 4.0
     */
    UnicodeString& format(const FormatParams& formatParams,
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
