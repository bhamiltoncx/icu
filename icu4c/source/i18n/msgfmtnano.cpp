// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "messageimpl.h"
#include "uassert.h"
#include "unicode/localpointer.h"
#include "unicode/messagepattern.h"
#include "unicode/msgfmtnano.h"
#include "unicode/uloc.h"
#include "unicode/ustring.h"

U_NAMESPACE_BEGIN

namespace {

void FormatAndAppend(const Format* formatter, const Formattable& arg,
                     const UnicodeString &argString, UnicodeString& appendTo, UErrorCode& ec) {
    if (U_FAILURE(ec)) {
        return;
    }
    if (!argString.isEmpty()) {
        appendTo.append(argString);
    } else {
        formatter->format(arg, appendTo, ec);
    }
}

// Copied from patternprops.cpp (that file brings in huge tables of data this
// doesn't need)
UBool
IsWhiteSpace(UChar32 c) {
    if(c<0) {
        return FALSE;
    } else if(c<=0xff) {
        return (c >= 0x09 && c <= 0x0D) || c == 0x85;
    } else if(0x200e<=c && c<=0x2029) {
        return c<=0x200f || 0x2028<=c;
    } else {
        return FALSE;
    }
}

int32_t
SkipWhiteSpace(const UnicodeString& s, int32_t start) {
    int32_t i = start;
    int32_t length = s.length();
    while(i<length && IsWhiteSpace(s.charAt(i))) {
        ++i;
    }
    return i;
}

const UChar *
TrimWhiteSpace(const UChar *s, int32_t &length) {
    if(length<=0 || (!IsWhiteSpace(s[0]) && !IsWhiteSpace(s[length-1]))) {
        return s;
    }
    int32_t start=0;
    int32_t limit=length;
    while(start<limit && IsWhiteSpace(s[start])) {
        ++start;
    }
    if(start<limit) {
        // There is non-white space at start; we will not move limit below that,
        // so we need not test start<limit in the loop.
        while(IsWhiteSpace(s[limit-1])) {
            --limit;
        }
    }
    length=limit-start;
    return s+start;
}

// Copied from msgfmt.cpp
int32_t
FindKeyword(const UnicodeString& s,
            const UChar * const *list)
{
    if (s.isEmpty()) {
        return 0; // default
    }

    int32_t length = s.length();
    const UChar *ps = TrimWhiteSpace(s.getBuffer(), length);
    UnicodeString buffer(FALSE, ps, length);
    // Trims the space characters and turns all characters
    // in s to lower case.
    buffer.toLower("");
    for (int32_t i = 0; list[i]; ++i) {
        if (!buffer.compare(list[i], u_strlen(list[i]))) {
            return i;
        }
    }
    return -1;
}

// Copied from ChoiceFormat
int32_t
ChoiceFormatFindSubMessage(const MessagePattern &pattern, int32_t partIndex, double number) {
    int32_t count = pattern.countParts();
    int32_t msgStart;
    // Iterate over (ARG_INT|DOUBLE, ARG_SELECTOR, message) tuples
    // until ARG_LIMIT or end of choice-only pattern.
    // Ignore the first number and selector and start the loop on the first message.
    partIndex += 2;
    for (;;) {
        // Skip but remember the current sub-message.
        msgStart = partIndex;
        partIndex = pattern.getLimitPartIndex(partIndex);
        if (++partIndex >= count) {
            // Reached the end of the choice-only pattern.
            // Return with the last sub-message.
            break;
        }
        const MessagePattern::Part &part = pattern.getPart(partIndex++);
        UMessagePatternPartType type = part.getType();
        if (type == UMSGPAT_PART_TYPE_ARG_LIMIT) {
            // Reached the end of the ChoiceFormat style.
            // Return with the last sub-message.
            break;
        }
        // part is an ARG_INT or ARG_DOUBLE
        U_ASSERT(MessagePattern::Part::hasNumericValue(type));
        double boundary = pattern.getNumericValue(part);
        // Fetch the ARG_SELECTOR character.
        int32_t selectorIndex = pattern.getPatternIndex(partIndex++);
        UChar boundaryChar = pattern.getPatternString().charAt(selectorIndex);
        if (boundaryChar == u'<' ? !(number > boundary) : !(number >= boundary)) {
            // The number is in the interval between the previous boundary and the current one.
            // Return with the sub-message between them.
            // The !(a>b) and !(a>=b) comparisons are equivalent to
            // (a<=b) and (a<b) except they "catch" NaN.
            break;
        }
    }
    return msgStart;
}

class FormatOperation {
 public:
    FormatOperation(const Locale& locale,
                    const MessagePattern& msgPattern,
                    const FormatProvider& formatProvider) :
            locale(locale), msgPattern(msgPattern), formatProvider(formatProvider) {
    }

    FormatOperation(const FormatOperation& other) = delete;
    FormatOperation(FormatOperation&&) = delete;
    FormatOperation &operator=(const FormatOperation&) = delete;
    FormatOperation &operator=(FormatOperation&&) = delete;

    void format(int32_t msgStart,
                const FormatProvider::PluralSelectorContext *plNumber,
                const Formattable* arguments,
                const UnicodeString* argumentNames,
                int32_t cnt,
                UnicodeString& appendTo,
                UErrorCode& success) const;
private:
    void formatComplexSubMessage(int32_t msgStart,
                                 const FormatProvider::PluralSelectorContext *plNumber,
                                 const Formattable* arguments,
                                 const UnicodeString *argumentNames,
                                 int32_t cnt,
                                 UnicodeString& appendTo,
                                 UErrorCode& success) const;

    const Format* createAppropriateFormat(const UnicodeString& type,
                                          const UnicodeString& style,
                                          UErrorCode& ec) const;
    const Locale& locale;
    const MessagePattern& msgPattern;
    const FormatProvider& formatProvider;
};

void FormatOperation::format(
    int32_t msgStart,
    const FormatProvider::PluralSelectorContext *plNumber,
    const Formattable* arguments,
    const UnicodeString *argumentNames,
    int32_t cnt,
    UnicodeString& appendTo,
    UErrorCode& success) const {
    if (U_FAILURE(success)) {
        return;
    }

    const UnicodeString& msgString = msgPattern.getPatternString();
    int32_t prevIndex = msgPattern.getPart(msgStart).getLimit();
    for (int32_t i = msgStart + 1; U_SUCCESS(success) ; ++i) {
        const MessagePattern::Part* part = &msgPattern.getPart(i);
        const UMessagePatternPartType type = part->getType();
        int32_t index = part->getIndex();
        appendTo.append(msgString, prevIndex, index - prevIndex);
        if (type == UMSGPAT_PART_TYPE_MSG_LIMIT) {
            return;
        }
        prevIndex = part->getLimit();
        if (type == UMSGPAT_PART_TYPE_REPLACE_NUMBER && plNumber) {
            if (plNumber->forReplaceNumber) {
                // number-offset was already formatted.
                FormatAndAppend(
                    plNumber->formatter,
                    plNumber->number, plNumber->numberString, appendTo, success);
            } else {
                const Format* nf = formatProvider.numberFormat(FormatProvider::TYPE_NUMBER, locale, success);
                if (U_SUCCESS(success)) {
                    nf->format(plNumber->number, appendTo, success);
                }
            }
            continue;
        }
        if (type != UMSGPAT_PART_TYPE_ARG_START) {
            continue;
        }
        int32_t argLimit = msgPattern.getLimitPartIndex(i);
        UMessagePatternArgType argType = part->getArgType();
        part = &msgPattern.getPart(++i);
        const Formattable* arg;
        UBool noArg = FALSE;
        UnicodeString argName = msgPattern.getSubstring(*part);
        if (argumentNames == nullptr) {
            int32_t argNumber = part->getValue();  // ARG_NUMBER
            if (0 <= argNumber && argNumber < cnt) {
                arg = arguments + argNumber;
            } else {
                arg = nullptr;
                noArg = TRUE;
            }
        } else {
          arg = nullptr;
          for (int32_t i = 0; i < cnt; ++i) {
            if (0 == argumentNames[i].compare(argName)) {
              arg = arguments + i;
              break;
            }
          }
          if (arg == nullptr) {
            noArg = TRUE;
          }
        }
        ++i;
        if (noArg) {
            appendTo.append(u"{", 1).append(argName).append(u"}", 1);
        } else if (arg == nullptr) {
            appendTo.append(u"null", 4);
        }
        else if(plNumber!=nullptr &&
                plNumber->numberArgIndex==(i-2)) {
            if(plNumber->offset == 0) {
                // The number was already formatted with this formatter.
                FormatAndAppend(plNumber->formatter, plNumber->number,
                                plNumber->numberString, appendTo, success);
            } else {
                // Do not use the formatted (number-offset) string for a named argument
                // that formats the number without subtracting the offset.
                plNumber->formatter->format(*arg, appendTo, success);
            }
        }
        else {
            switch (argType) {
                case UMSGPAT_ARG_TYPE_NONE: {
                    if (arg->isNumeric()) {
                        const Format* format = formatProvider.numberFormat(FormatProvider::TYPE_NUMBER, locale, success);
                        if (format) {
                            format->format(*arg, appendTo, success);
                        }
                    } else if (arg->getType() == Formattable::kDate) {
                        const Format* format = formatProvider.dateTimeFormat(FormatProvider::TYPE_DATE, FormatProvider::STYLE_SHORT, locale, success);
                        if (format) {
                            format->format(*arg, appendTo, success);
                        }
                    } else {
                        appendTo.append(arg->getString(success));
                    }
                    break;
                }
                case UMSGPAT_ARG_TYPE_SIMPLE: {
                    i += 2;
                    UnicodeString explicitType = msgPattern.getSubstring(msgPattern.getPart(i++));
                    UnicodeString style;
                    if ((part = &msgPattern.getPart(i))->getType() == UMSGPAT_PART_TYPE_ARG_STYLE) {
                        style = msgPattern.getSubstring(*part);
                        ++i;
                    }
                    const Format* format = createAppropriateFormat(explicitType, style, success);
                    if (format) {
                        format->format(*arg, appendTo, success);
                    }
                    break;
                }
                case UMSGPAT_ARG_TYPE_CHOICE: {
                    if (!arg->isNumeric()) {
                        success = U_ILLEGAL_ARGUMENT_ERROR;
                        return;
                    }
                    // We must use the Formattable::getDouble() variant with the UErrorCode parameter
                    // because only this one converts non-double numeric types to double.
                    const double number = arg->getDouble(success);
                    int32_t subMsgStart = ChoiceFormatFindSubMessage(msgPattern, i, number);
                    format(subMsgStart, plNumber, arguments, argumentNames, cnt, appendTo, success);
                    break;
                }
                case UMSGPAT_ARG_TYPE_PLURAL:
                case UMSGPAT_ARG_TYPE_SELECTORDINAL: {
                    if (!arg->isNumeric()) {
                        success = U_ILLEGAL_ARGUMENT_ERROR;
                        return;
                    }
                    const FormatProvider::PluralSelector* selector =
                            formatProvider.pluralSelector(
                                argType == UMSGPAT_ARG_TYPE_PLURAL ? FormatProvider::TYPE_PLURAL : FormatProvider::TYPE_SELECTORDINAL,
                                success);
                    if (U_FAILURE(success)) {
                        return;
                    }
                    // We must use the Formattable::getDouble() variant with the UErrorCode parameter
                    // because only this one converts non-double numeric types to double.
                    double offset = msgPattern.getPluralOffset(i);
                    FormatProvider::PluralSelectorContext context(i, argName, *arg, offset, success);
                    int32_t subMsgStart = formatProvider.pluralFindSubMessage(
                        msgPattern, i, *selector, &context, arg->getDouble(success), success);
                    formatComplexSubMessage(subMsgStart, &context, arguments, argumentNames,
                                            cnt, appendTo, success);
                    break;
                }
                case UMSGPAT_ARG_TYPE_SELECT: {
                    int32_t subMsgStart = formatProvider.selectFindSubMessage(msgPattern, i, arg->getString(success), success);
                    if (U_FAILURE(success)) {
                        return;
                    }
                    formatComplexSubMessage(subMsgStart, /*context=*/nullptr, arguments, argumentNames,
                                            cnt, appendTo, success);
                    break;
                }
            }
        }
        prevIndex = msgPattern.getPart(argLimit).getLimit();
        i = argLimit;
    }
}

void FormatOperation::formatComplexSubMessage(int32_t msgStart,
                                              const FormatProvider::PluralSelectorContext *plNumber,
                                              const Formattable* arguments,
                                              const UnicodeString *argumentNames,
                                              int32_t cnt,
                                              UnicodeString& appendTo,
                                              UErrorCode& success) const {
    if (U_FAILURE(success)) {
        return;
    }

    if (!MessageImpl::jdkAposMode(msgPattern)) {
        format(msgStart, plNumber, arguments, argumentNames, cnt, appendTo, success);
        return;
    }

    // JDK compatibility mode: (see JDK MessageFormat.format() API docs)
    // - remove SKIP_SYNTAX; that is, remove half of the apostrophes
    // - if the result string contains an open curly brace '{' then
    //   instantiate a temporary MessageFormat object and format again;
    //   otherwise just append the result string
    const UnicodeString& msgString = msgPattern.getPatternString();
    UnicodeString sb;
    int32_t prevIndex = msgPattern.getPart(msgStart).getLimit();
    for (int32_t i = msgStart;;) {
        const MessagePattern::Part& part = msgPattern.getPart(++i);
        const UMessagePatternPartType type = part.getType();
        int32_t index = part.getIndex();
        if (type == UMSGPAT_PART_TYPE_MSG_LIMIT) {
            sb.append(msgString, prevIndex, index - prevIndex);
            break;
        } else if (type == UMSGPAT_PART_TYPE_REPLACE_NUMBER || type == UMSGPAT_PART_TYPE_SKIP_SYNTAX) {
            sb.append(msgString, prevIndex, index - prevIndex);
            if (type == UMSGPAT_PART_TYPE_REPLACE_NUMBER) {
                if(plNumber->forReplaceNumber) {
                    // number-offset was already formatted.
                    sb.append(plNumber->numberString);
                } else {
                    const Format* format = formatProvider.numberFormat(FormatProvider::TYPE_NUMBER, locale, success);
                    if (format) {
                        sb.append(format->format(plNumber->number, sb, success));
                    }
                }
            }
            prevIndex = part.getLimit();
        } else if (type == UMSGPAT_PART_TYPE_ARG_START) {
            sb.append(msgString, prevIndex, index - prevIndex);
            prevIndex = index;
            i = msgPattern.getLimitPartIndex(i);
            index = msgPattern.getPart(i).getLimit();
            MessageImpl::appendReducedApostrophes(msgString, prevIndex, index, sb);
            prevIndex = index;
        }
    }
    if (sb.indexOf(u'{') >= 0) {
        MessagePattern subMessagePattern(UMSGPAT_APOS_DOUBLE_REQUIRED, success);
        subMessagePattern.parse(sb, /*parseError=*/nullptr, success);
        FormatOperation subFormatOperation(locale, subMessagePattern, formatProvider);
        subFormatOperation.format(/*msgStart=*/0, /*plNumber=*/nullptr, arguments, argumentNames, cnt, appendTo, success);
    } else {
        appendTo.append(sb);
    }
}

// Copied from msgfmt.cpp
const Format* FormatOperation::createAppropriateFormat(const UnicodeString& type, const UnicodeString& style,
                                                       UErrorCode& ec) const {
    if (U_FAILURE(ec)) {
        return nullptr;
    }

    // MessageFormat
    constexpr char16_t const* TYPE_IDS[] = {
        u"number",
        u"date",
        u"time",
        u"spellout",
        u"ordinal",
        u"duration",
        nullptr,
    };

    // NumberFormat
    constexpr char16_t const* const NUMBER_STYLE_IDS[] = {
        u"",
        u"currency",
        u"percent",
        u"integer",
        nullptr,
    };

    // DateFormat
    constexpr char16_t const* const DATE_STYLE_IDS[] = {
        u"",
        u"short",
        u"medium",
        u"long",
        u"full",
        nullptr,
    };

    constexpr FormatProvider::DateTimeStyle DATE_STYLES[] = {
      FormatProvider::STYLE_DEFAULT,
      FormatProvider::STYLE_SHORT,
      FormatProvider::STYLE_MEDIUM,
      FormatProvider::STYLE_LONG,
      FormatProvider::STYLE_FULL,
    };

    switch (int32_t typeID = FindKeyword(type, TYPE_IDS)) {
        case 0: // number
            switch (FindKeyword(style, NUMBER_STYLE_IDS)) {
                case 0: // default
                    return formatProvider.numberFormat(FormatProvider::TYPE_NUMBER, locale, ec);
                case 1: // currency
                    return formatProvider.numberFormat(FormatProvider::TYPE_CURRENCY, locale, ec);
                case 2: // percent
                    return formatProvider.numberFormat(FormatProvider::TYPE_PERCENT, locale, ec);
                case 3: // integer
                    return formatProvider.numberFormat(FormatProvider::TYPE_INTEGER, locale, ec);
                default: { // pattern or skeleton
                    int32_t firstNonSpace = SkipWhiteSpace(style, 0);
                    if (style.compare(firstNonSpace, 2, u"::", 0, 2) == 0) {
                        // Skeleton
                        UnicodeString skeleton = style.tempSubString(firstNonSpace + 2);
                        return formatProvider.numberFormatForSkeleton(skeleton, locale, ec);
                    }
                    // Pattern
                    return formatProvider.decimalFormatWithPattern(style, locale, ec);
                }
            }
            break;
        case 1:   // date
        case 2: { // time
            int32_t firstNonSpace = SkipWhiteSpace(style, 0);
            if (style.compare(firstNonSpace, 2, u"::", 0, 2) == 0) {
                // Skeleton
                UnicodeString skeleton = style.tempSubString(firstNonSpace + 2);
                return formatProvider.dateTimeFormatForSkeleton(skeleton, locale, ec);
            }
            // Pattern
            int32_t styleID = FindKeyword(style, DATE_STYLE_IDS);
            FormatProvider::DateTimeStyle dateStyle = (styleID >= 0) ? DATE_STYLES[styleID] : FormatProvider::STYLE_DEFAULT;

            if (typeID == 1) {
                return formatProvider.dateTimeFormat(FormatProvider::TYPE_DATE, dateStyle, locale, ec);
            }
            return formatProvider.dateTimeFormat(FormatProvider::TYPE_TIME, dateStyle, locale, ec);
        }
        case 3: // spellout
            return formatProvider.ruleBasedNumberFormat(FormatProvider::TYPE_SPELLOUT, locale, style, ec);
        case 4: // ordinal
            return formatProvider.ruleBasedNumberFormat(FormatProvider::TYPE_ORDINAL, locale, style, ec);
        case 5: // duration
            return formatProvider.ruleBasedNumberFormat(FormatProvider::TYPE_DURATION, locale, style, ec);
    }

    ec = U_ILLEGAL_ARGUMENT_ERROR;
    return nullptr;
}

}  // namespace

//--------------------------------------------------------------------

MessageFormatNano::MessageFormatNano(const UnicodeString& pattern,
                                     const Locale& newLocale,
                                     UParseError& parseError,
                                     UErrorCode& success)
        : locale(newLocale),
          msgPattern(pattern, &parseError, success),
          formatProvider(new FormatProvider())
{
}

MessageFormatNano::MessageFormatNano(const UnicodeString& pattern,
                                     const Locale& newLocale,
                                     LocalPointer<const FormatProvider> formatProvider,
                                     UParseError& parseError,
                                     UErrorCode& success)
        : locale(newLocale),
          msgPattern(pattern, &parseError, success),
          formatProvider(std::move(formatProvider))
{
}

UnicodeString& MessageFormatNano::format(const UnicodeString* argumentNames,
                                         const Formattable* arguments,
                                         int32_t cnt,
                                         UnicodeString& appendTo,
                                         UErrorCode& success) const {
    FormatOperation formatOperation(locale, msgPattern, *formatProvider);
    formatOperation.format(/*msgStart=*/0, /*plNumber=*/nullptr, arguments, argumentNames, cnt, appendTo, success);
    return appendTo;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
