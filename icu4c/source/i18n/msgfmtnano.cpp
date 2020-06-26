// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "cmemory.h"
#include "messageimpl.h"
#include "uassert.h"
#include "unicode/localpointer.h"
#include "unicode/messagepattern.h"
#include "unicode/msgfmtnano.h"
#include "unicode/uloc.h"
#include "unicode/ustring.h"

//#define U_DEBUG_MSGFMTNANO 1

U_NAMESPACE_BEGIN

namespace {

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
FindKeyword(const UnicodeString& s, const UChar * const *list, size_t listLen)
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
    for (size_t i = 0; i < listLen; ++i) {
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

// Copied from PluralFormat
int32_t
PluralFormatFindSubMessage(const MessagePattern& pattern, int32_t partIndex,
                           const PluralFormatProvider::Selector& selector, void *context,
                           double number, UErrorCode& ec) {
    if (U_FAILURE(ec)) {
        return 0;
    }
#ifdef U_DEBUG_MSGFMTNANO
    std::string s;
    fprintf(stderr, "PluralFormatFindSubMessage pattern=%s partIndex=%d (%f), ec=%s\n", pattern.getPatternString().toUTF8String(s).c_str(), partIndex, number, u_errorName(ec));
    s.clear();
#endif  // U_DEBUG_MSGFMTNANO
    int32_t count=pattern.countParts();
    double offset;
    const MessagePattern::Part* part=&pattern.getPart(partIndex);
    if (MessagePattern::Part::hasNumericValue(part->getType())) {
        offset=pattern.getNumericValue(*part);
        ++partIndex;
    } else {
        offset=0;
    }
    // The keyword is empty until we need to match against a non-explicit, not-"other" value.
    // Then we get the keyword from the selector.
    // (In other words, we never call the selector if we match against an explicit value,
    // or if the only non-explicit keyword is "other".)
    UnicodeString keyword;
    UnicodeString other(u"other", 5);
    // When we find a match, we set msgStart>0 and also set this boolean to true
    // to avoid matching the keyword again (duplicates are allowed)
    // while we continue to look for an explicit-value match.
    UBool haveKeywordMatch=FALSE;
    // msgStart is 0 until we find any appropriate sub-message.
    // We remember the first "other" sub-message if we have not seen any
    // appropriate sub-message before.
    // We remember the first matching-keyword sub-message if we have not seen
    // one of those before.
    // (The parser allows [does not check for] duplicate keywords.
    // We just have to make sure to take the first one.)
    // We avoid matching the keyword twice by also setting haveKeywordMatch=true
    // at the first keyword match.
    // We keep going until we find an explicit-value match or reach the end of the plural style.
    int32_t msgStart=0;
    // Iterate over (ARG_SELECTOR [ARG_INT|ARG_DOUBLE] message) tuples
    // until ARG_LIMIT or end of plural-only pattern.
    do {
        part=&pattern.getPart(partIndex++);
        const UMessagePatternPartType type = part->getType();
        if(type==UMSGPAT_PART_TYPE_ARG_LIMIT) {
            break;
        }
        U_ASSERT (type==UMSGPAT_PART_TYPE_ARG_SELECTOR);
        // part is an ARG_SELECTOR followed by an optional explicit value, and then a message
        if(MessagePattern::Part::hasNumericValue(pattern.getPartType(partIndex))) {
            // explicit value like "=2"
            part=&pattern.getPart(partIndex++);
            if(number==pattern.getNumericValue(*part)) {
                // matches explicit value
                return partIndex;
            }
        } else if(!haveKeywordMatch) {
            // plural keyword like "few" or "other"
            // Compare "other" first and call the selector if this is not "other".
            if(pattern.partSubstringMatches(*part, other)) {
                if(msgStart==0) {
                    msgStart=partIndex;
                    if(0 == keyword.compare(other)) {
                        // This is the first "other" sub-message,
                        // and the selected keyword is also "other".
                        // Do not match "other" again.
                        haveKeywordMatch=TRUE;
                    }
                }
            } else {
                if(keyword.isEmpty()) {
                    keyword=selector.select(context, number-offset, ec);
                    if(msgStart!=0 && (0 == keyword.compare(other))) {
                        // We have already seen an "other" sub-message.
                        // Do not match "other" again.
                        haveKeywordMatch=TRUE;
                        // Skip keyword matching but do getLimitPartIndex().
                    }
                }
                if(!haveKeywordMatch && pattern.partSubstringMatches(*part, keyword)) {
                    // keyword matches
                    msgStart=partIndex;
                    // Do not match this keyword again.
                    haveKeywordMatch=TRUE;
                }
            }
        }
        partIndex=pattern.getLimitPartIndex(partIndex);
    } while(++partIndex<count);
#ifdef U_DEBUG_MSGFMTNANO
    fprintf(stderr, "findSubMessage msgStart=%d ec=%s\n", msgStart, u_errorName(ec));
#endif  // U_DEBUG_MSGFMTNANO
    return msgStart;
}

// Copied from SelectFormat
int32_t SelectFormatFindSubMessage(const MessagePattern& pattern, int32_t partIndex,
                                   const UnicodeString& keyword, UErrorCode& ec) {
    if (U_FAILURE(ec)) {
        return 0;
    }
    UnicodeString other(u"other", 5);
    int32_t count = pattern.countParts();
    int32_t msgStart=0;
    // Iterate over (ARG_SELECTOR, message) pairs until ARG_LIMIT or end of select-only pattern.
    do {
        const MessagePattern::Part& part=pattern.getPart(partIndex++);
        const UMessagePatternPartType type=part.getType();
        if(type==UMSGPAT_PART_TYPE_ARG_LIMIT) {
            break;
        }
        // part is an ARG_SELECTOR followed by a message
        if(pattern.partSubstringMatches(part, keyword)) {
            // keyword matches
            return partIndex;
        } else if(msgStart==0 && pattern.partSubstringMatches(part, other)) {
            msgStart=partIndex;
        }
        partIndex=pattern.getLimitPartIndex(partIndex);
    } while(++partIndex<count);
    return msgStart;
}

class FormatOperation {
 public:
    FormatOperation(const MessageFormatNano::FormatParams& params,
                    const MessagePattern& msgPattern,
                    const NumberFormatProvider& numberFormatProvider,
                    const DateTimeFormatProvider& dateTimeFormatProvider,
                    const RuleBasedNumberFormatProvider& ruleBasedNumberFormatProvider,
                    const PluralFormatProvider& pluralFormatProvider) :
            params(params),
            msgPattern(msgPattern),
            numberFormatProvider(numberFormatProvider),
            dateTimeFormatProvider(dateTimeFormatProvider),
            ruleBasedNumberFormatProvider(ruleBasedNumberFormatProvider),
            pluralFormatProvider(pluralFormatProvider)
    {
    }

    FormatOperation(const FormatOperation& other) = delete;
    FormatOperation(FormatOperation&&) = delete;
    FormatOperation &operator=(const FormatOperation&) = delete;
    FormatOperation &operator=(FormatOperation&&) = delete;

    void format(int32_t msgStart,
                const PluralFormatProvider::SelectorContext *plNumber,
                UnicodeString& appendTo,
                UErrorCode& success) const;
private:
    void formatComplexSubMessage(int32_t msgStart,
                                 const PluralFormatProvider::SelectorContext *plNumber,
                                 UnicodeString& appendTo,
                                 UErrorCode& success) const;

    void formatArgWithExplicitType(const Formattable& arg,
                                   const UnicodeString& type,
                                   const UnicodeString& style,
                                   UnicodeString& appendTo,
                                   UErrorCode& ec) const;
    const MessageFormatNano::FormatParams& params;
    const MessagePattern& msgPattern;
    const NumberFormatProvider& numberFormatProvider;
    const DateTimeFormatProvider& dateTimeFormatProvider;
    const RuleBasedNumberFormatProvider& ruleBasedNumberFormatProvider;
    const PluralFormatProvider& pluralFormatProvider;
};

void FormatOperation::format(
    int32_t msgStart,
    const PluralFormatProvider::SelectorContext *plNumber,
    UnicodeString& appendTo,
    UErrorCode& success) const {
    if (U_FAILURE(success)) {
        return;
    }

#ifdef U_DEBUG_MSGFMTNANO
    std::string s;
    std::string s2;
    fprintf(stderr, "format msgStart=%d msgPattern=%s plNumber=%p cnt=%d appendTo=[%s] success=%s\n", msgStart, msgPattern.getPatternString().toUTF8String(s).c_str(), plNumber, params.count, appendTo.toUTF8String(s2).c_str(), u_errorName(success));
    s.clear();
    s2.clear();
#endif   // U_DEBUG_MSGFMTNANO
    const UnicodeString& msgString = msgPattern.getPatternString();
    int32_t prevIndex = msgPattern.getPart(msgStart).getLimit();
    for (int32_t i = msgStart + 1; U_SUCCESS(success) ; ++i) {
        const MessagePattern::Part* part = &msgPattern.getPart(i);
        const UMessagePatternPartType type = part->getType();
        int32_t index = part->getIndex();
        appendTo.append(msgString, prevIndex, index - prevIndex);
#ifdef U_DEBUG_MSGFMTNANO
        fprintf(stderr, "format i=%d partType=%d index=%d appendTo=[%s]\n", i, type, index, appendTo.toUTF8String(s).c_str());
        s.clear();
#endif   // U_DEBUG_MSGFMTNANO
        if (type == UMSGPAT_PART_TYPE_MSG_LIMIT) {
            return;
        }
        prevIndex = part->getLimit();
        if (type == UMSGPAT_PART_TYPE_REPLACE_NUMBER && plNumber) {
            if (plNumber->forReplaceNumber) {
                appendTo.append(plNumber->numberString);
            } else {
                numberFormatProvider.formatNumber(plNumber->number, NumberFormatProvider::TYPE_NUMBER, params.locale, appendTo, success);
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
        if (params.argumentNames == nullptr) {
            int32_t argNumber = part->getValue();  // ARG_NUMBER
            if (0 <= argNumber && argNumber < params.count) {
                arg = params.arguments + argNumber;
            } else {
                arg = nullptr;
                noArg = TRUE;
            }
        } else {
          arg = nullptr;
          for (int32_t i = 0; i < params.count; ++i) {
            if (0 == params.argumentNames[i].compare(argName)) {
              arg = params.arguments + i;
#ifdef U_DEBUG_MSGFMTNANO
              fprintf(stderr, "Found argName=[%s], arg type=%d\n", argName.toUTF8String(s).c_str(), arg->getType());
              s.clear();
#endif  // U_DEBUG_MSGFMTNANO
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
            if(plNumber->offset == 0 && plNumber->forReplaceNumber) {
                appendTo.append(plNumber->numberString);
            } else {
                // Do not use the formatted (number-offset) string for a named argument
                // that formats the number without subtracting the offset.
                numberFormatProvider.formatNumber(*arg, NumberFormatProvider::TYPE_NUMBER, params.locale, appendTo, success);
            }
        }
        else {
            switch (argType) {
                case UMSGPAT_ARG_TYPE_NONE: {
                    if (arg->isNumeric()) {
                        numberFormatProvider.formatNumber(*arg, NumberFormatProvider::TYPE_NUMBER, params.locale, appendTo, success);
                    } else if (arg->getType() == Formattable::kDate) {
                        dateTimeFormatProvider.formatDateTime(*arg, /*dateStyle=*/DateTimeFormatProvider::STYLE_SHORT, /*timeStyle=*/DateTimeFormatProvider::STYLE_SHORT, params.locale, params.timeZone.getAlias(), appendTo, success);
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
                    formatArgWithExplicitType(*arg, explicitType, style, appendTo, success);
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
                    format(subMsgStart, plNumber, appendTo, success);
                    break;
                }
                case UMSGPAT_ARG_TYPE_PLURAL:
                case UMSGPAT_ARG_TYPE_SELECTORDINAL: {
                    if (!arg->isNumeric()) {
                        success = U_ILLEGAL_ARGUMENT_ERROR;
                        return;
                    }
                    const PluralFormatProvider::Selector* selector =
                            pluralFormatProvider.pluralSelector(
                                argType == UMSGPAT_ARG_TYPE_PLURAL ? PluralFormatProvider::TYPE_CARDINAL : PluralFormatProvider::TYPE_ORDINAL,
                                success);
                    if (U_FAILURE(success)) {
                        return;
                    }
                    // We must use the Formattable::getDouble() variant with the UErrorCode parameter
                    // because only this one converts non-double numeric types to double.
                    double offset = msgPattern.getPluralOffset(i);
                    PluralFormatProvider::SelectorContext context(msgPattern, numberFormatProvider, params.locale, i, argName, *arg, offset, success);
                    int32_t subMsgStart = PluralFormatFindSubMessage(
                        msgPattern, i, *selector, &context, arg->getDouble(success), success);
                    formatComplexSubMessage(subMsgStart, &context, appendTo, success);
                    break;
                }
                case UMSGPAT_ARG_TYPE_SELECT: {
                    int32_t subMsgStart = SelectFormatFindSubMessage(msgPattern, i, arg->getString(success), success);
                    if (U_FAILURE(success)) {
                        return;
                    }
                    formatComplexSubMessage(subMsgStart, /*context=*/nullptr, appendTo, success);
                    break;
                }
            }
        }
        prevIndex = msgPattern.getPart(argLimit).getLimit();
        i = argLimit;
    }
}

void FormatOperation::formatComplexSubMessage(int32_t msgStart,
                                              const PluralFormatProvider::SelectorContext *plNumber,
                                              UnicodeString& appendTo,
                                              UErrorCode& success) const {
#ifdef U_DEBUG_MSGFMTNANO
    std::string s;
    std::string s2;
    fprintf(stderr, "formatComplexSubMessage msgStart=%d msgPattern=%s plNumber=%p cnt=%d appendTo=[%s] success=%s\n", msgStart, msgPattern.getPatternString().toUTF8String(s).c_str(), plNumber, params.count, appendTo.toUTF8String(s2).c_str(), u_errorName(success));
#endif  // U_DEBUG_MSGFMTNANO
    if (U_FAILURE(success)) {
        return;
    }

    if (!MessageImpl::jdkAposMode(msgPattern)) {
        format(msgStart, plNumber, appendTo, success);
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
                    numberFormatProvider.formatNumber(plNumber->number, NumberFormatProvider::TYPE_NUMBER, params.locale, sb, success);
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
        FormatOperation subFormatOperation(params, subMessagePattern, numberFormatProvider, dateTimeFormatProvider, ruleBasedNumberFormatProvider, pluralFormatProvider);
        subFormatOperation.format(/*msgStart=*/0, /*plNumber=*/nullptr, appendTo, success);
    } else {
        appendTo.append(sb);
    }
}

// Copied from msgfmt.cpp
void FormatOperation::formatArgWithExplicitType(const Formattable& arg, const UnicodeString& type, const UnicodeString& style,
                                                UnicodeString& appendTo, UErrorCode& ec) const {
    if (U_FAILURE(ec)) {
        return;
    }

    // MessageFormat
    constexpr char16_t const* TYPE_IDS[] = {
        u"number",
        u"date",
        u"time",
        u"spellout",
        u"ordinal",
        u"duration",
    };

    // NumberFormat
    constexpr char16_t const* const NUMBER_STYLE_IDS[] = {
        u"",
        u"currency",
        u"percent",
        u"integer",
    };

    // DateFormat
    constexpr char16_t const* const DATE_STYLE_IDS[] = {
        u"",
        u"short",
        u"medium",
        u"long",
        u"full",
    };

    constexpr DateTimeFormatProvider::DateTimeStyle DATE_STYLES[] = {
      DateTimeFormatProvider::STYLE_DEFAULT,
      DateTimeFormatProvider::STYLE_SHORT,
      DateTimeFormatProvider::STYLE_MEDIUM,
      DateTimeFormatProvider::STYLE_LONG,
      DateTimeFormatProvider::STYLE_FULL,
    };

    switch (int32_t typeID = FindKeyword(type, TYPE_IDS, UPRV_LENGTHOF(TYPE_IDS))) {
        case 0: // number
            switch (FindKeyword(style, NUMBER_STYLE_IDS, UPRV_LENGTHOF(NUMBER_STYLE_IDS))) {
                case 0: // default
                    numberFormatProvider.formatNumber(arg, NumberFormatProvider::TYPE_NUMBER, params.locale, appendTo, ec);
                    return;
                case 1: // currency
                    numberFormatProvider.formatNumber(arg, NumberFormatProvider::TYPE_CURRENCY, params.locale, appendTo, ec);
                    return;
                case 2: // percent
                    numberFormatProvider.formatNumber(arg, NumberFormatProvider::TYPE_PERCENT, params.locale, appendTo, ec);
                    return;
                case 3: // integer
                    numberFormatProvider.formatNumber(arg, NumberFormatProvider::TYPE_INTEGER, params.locale, appendTo, ec);
                    return;
                default: { // pattern or skeleton
                    int32_t firstNonSpace = SkipWhiteSpace(style, 0);
                    if (style.compare(firstNonSpace, 2, u"::", 0, 2) == 0) {
                        // Skeleton
                        UnicodeString skeleton = style.tempSubString(firstNonSpace + 2);
                        numberFormatProvider.formatNumberWithSkeleton(arg, skeleton, params.locale, appendTo, ec);
                    }
                    // Pattern
                    numberFormatProvider.formatDecimalNumberWithPattern(arg, style, params.locale, appendTo, ec);
                    return;
                }
            }
            break;
        case 1:   // date
        case 2: { // time
            int32_t firstNonSpace = SkipWhiteSpace(style, 0);
            if (style.compare(firstNonSpace, 2, u"::", 0, 2) == 0) {
                // Skeleton
                UnicodeString skeleton = style.tempSubString(firstNonSpace + 2);
                dateTimeFormatProvider.formatDateTimeWithSkeleton(arg, skeleton, params.locale, params.timeZone.getAlias(), appendTo, ec);
                return;
            }
            // Pattern
            int32_t styleID = FindKeyword(style, DATE_STYLE_IDS, UPRV_LENGTHOF(DATE_STYLE_IDS));
            DateTimeFormatProvider::DateTimeStyle dateTimeStyle = (styleID >= 0) ? DATE_STYLES[styleID] : DateTimeFormatProvider::STYLE_DEFAULT;

            if (typeID == 1) {
                dateTimeFormatProvider.formatDateTime(arg, /*dateStyle=*/dateTimeStyle, /*timeStyle=*/DateTimeFormatProvider::STYLE_NONE, params.locale, params.timeZone.getAlias(), appendTo, ec);
                return;
            }
            dateTimeFormatProvider.formatDateTime(arg, /*dateStyle=*/DateTimeFormatProvider::STYLE_NONE, /*timeStyle=*/dateTimeStyle, params.locale, params.timeZone.getAlias(), appendTo, ec);
            return;
        }
        case 3: // spellout
            ruleBasedNumberFormatProvider.formatRuleBasedNumber(arg, RuleBasedNumberFormatProvider::TYPE_SPELLOUT, params.locale, style, appendTo, ec);
            return;
        case 4: // ordinal
            ruleBasedNumberFormatProvider.formatRuleBasedNumber(arg, RuleBasedNumberFormatProvider::TYPE_ORDINAL, params.locale, style, appendTo, ec);
            return;
        case 5: // duration
            ruleBasedNumberFormatProvider.formatRuleBasedNumber(arg, RuleBasedNumberFormatProvider::TYPE_DURATION, params.locale, style, appendTo, ec);
            return;
    }

    ec = U_ILLEGAL_ARGUMENT_ERROR;
}

}  // namespace

//--------------------------------------------------------------------

MessageFormatNano::MessageFormatNano(const UnicodeString& pattern,
                                     UParseError& parseError,
                                     UErrorCode& success)
        : msgPattern(pattern, &parseError, success),
          numberFormatProvider(new NumberFormatProvider()),
          dateTimeFormatProvider(new DateTimeFormatProvider()),
          ruleBasedNumberFormatProvider(new RuleBasedNumberFormatProvider()),
          pluralFormatProvider(new PluralFormatProvider()) {
}

MessageFormatNano::MessageFormatNano(const UnicodeString& pattern,
                                     LocalPointer<const NumberFormatProvider> numberFormatProvider,
                                     LocalPointer<const DateTimeFormatProvider> dateTimeFormatProvider,
                                     LocalPointer<const RuleBasedNumberFormatProvider> ruleBasedNumberFormatProvider,
                                     LocalPointer<const PluralFormatProvider> pluralFormatProvider,
                                     UParseError& parseError,
                                     UErrorCode& status)
        : msgPattern(pattern, &parseError, status),
          numberFormatProvider(std::move(numberFormatProvider)),
          dateTimeFormatProvider(std::move(dateTimeFormatProvider)),
          ruleBasedNumberFormatProvider(std::move(ruleBasedNumberFormatProvider)),
          pluralFormatProvider(std::move(pluralFormatProvider))
{
}

UnicodeString& MessageFormatNano::format(const FormatParams& formatParams,
                                         UnicodeString& appendTo,
                                         UErrorCode& success) const {
    FormatOperation formatOperation(formatParams, msgPattern, *numberFormatProvider, *dateTimeFormatProvider, *ruleBasedNumberFormatProvider, *pluralFormatProvider);
    formatOperation.format(/*msgStart=*/0, /*plNumber=*/nullptr, appendTo, success);
    return appendTo;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
