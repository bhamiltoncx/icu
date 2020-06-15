// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "uassert.h"
#include "unicode/localpointer.h"
#include "unicode/messagepattern.h"
#include "unicode/msgfmtnano.h"
#include "unicode/uloc.h"
#include "unicode/ustring.h"

#define LEFT_CURLY_BRACE  ((UChar)0x007B)
#define LESS_THAN    ((UChar)0x003C)   /*<*/
#define RIGHT_CURLY_BRACE ((UChar)0x007D)

static const UChar NULL_STRING[] = {
    0x6E, 0x75, 0x6C, 0x6C, 0  // "null"
};

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
        if (boundaryChar == LESS_THAN ? !(number > boundary) : !(number >= boundary)) {
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
int32_t PluralFormatFindSubMessage(const MessagePattern& pattern, int32_t partIndex,
                                     const PluralSelector& selector, void *context,
                                     double number, UErrorCode& ec) {
    if (U_FAILURE(ec)) {
        return 0;
    }
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
    UnicodeString other(FALSE, OTHER_STRING, 5);
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
    return msgStart;
}

struct PluralSelectorFormatOperationContext {
    PluralSelectorFormatOperationContext(
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

    PluralSelectorFormatOperationContext(const PluralSelectorFormatOperationContext& other) = delete;
    PluralSelectorFormatOperationContext(PluralSelectorFormatOperationContext&&) = delete;
    PluralSelectorFormatOperationContext &operator=(const PluralSelectorFormatOperationContext&) = delete;
    PluralSelectorFormatOperationContext &operator=(PluralSelectorFormatOperationContext&&) = delete;

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

class FormatOperation {
 public:
    FormatOperation(const Locale& locale, const MessagePattern& messagePattern, const FormatProvider& formatProvider) :
    locale(locale), messagePattern(messagePattern), formatProvider(formatProvider) { }

    FormatOperation(const FormatOperation& other) = delete;
    FormatOperation(FormatOperation&&) = delete;
    FormatOperation &operator=(const FormatOperation&) = delete;
    FormatOperation &operator=(FormatOperation&&) = delete;

    void format(int32_t msgStart,
                const PluralSelectorFormatOperationContext *plNumber,
                const Formattable* arguments,
                const UnicodeString* argumentNames,
                int32_t cnt,
                UnicodeString& appendTo,
                FieldPosition* pos,
                UErrorCode& success) const;

    void formatComplexSubMessage(int32_t msgStart,
                                 const PluralSelectorFormatOperationContext *plNumber,
                                 const Formattable* arguments,
                                 const UnicodeString *argumentNames,
                                 int32_t cnt,
                                 UnicodeString& appendTo,
                                 UErrorCode& success) const;

private:
    const Locale& locale;
    const MessagePattern& msgPattern;
    const FormatProvider& formatProvider,
};

void FormatOperation::format(
    int32_t msgStart,
    const PluralSelectorFormatOperationContext *plNumber,
    const Formattable* arguments,
    const UnicodeString *argumentNames,
    int32_t cnt,
    UnicodeString& appendTo,
    FieldPosition* pos,
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
        if (type == UMSGPAT_PART_TYPE_REPLACE_NUMBER) {
            if (plNumber.forReplaceNumber) {
                // number-offset was already formatted.
                FormatAndAppend(
                    pluralNumber.formatter,
                    pluralNumber.number, appendTo, pluralNumber.numberString, success);
            } else {
                const Format* nf = formatProvider.numberFormat(MFP_NUMBER_FORMAT_TYPE_NUMBER, locale, status);
                if (U_SUCCESS(success)) {
                    nf->append(pluralNumber.number, appendTo, success);
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
        if (argumentNames == NULL) {
            int32_t argNumber = part->getValue();  // ARG_NUMBER
            if (0 <= argNumber && argNumber < cnt) {
                arg = arguments + argNumber;
            } else {
                arg = NULL;
                noArg = TRUE;
            }
        } else {
          arg = NULL;
          for (int32_t i = 0; i < cnt; ++i) {
            if (0 == argumentNames[i].compare(argName)) {
              arg = arguments + i;
              break;
            }
          }
          if (arg == NULL) {
            noArg = TRUE;
          }
        }
        ++i;
        if (noArg) {
            appendTo.append(
                UnicodeString(LEFT_CURLY_BRACE).append(argName).append(RIGHT_CURLY_BRACE));
        } else if (arg == NULL) {
            appendTo.append(NULL_STRING, 4);
        }
        else if(plNumber!=NULL &&
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
                        const Format* format = formatProvider.numberFormat(MFP_NUMBER_FORMAT_TYPE_NUMBER, locale, status);
                        if (format) {
                            format->format(appendTo, *arg, success);
                        }
                    } else if (arg->getType() == Formattable::kDate) {
                        const Format* format = formatProvider.timeDateFormat(MFP_TIMEDATE_TYPE_DATE, MFP_TIMEDATE_STYLE_SHORT, locale, success);
                        if (format) {
                            format->format(appendTo, *arg, success);
                        }
                    } else {
                        appendTo.append(arg->getString(success));
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
                    format(subMsgStart, arguments, argumentNames,
                           cnt, appendTo, success);
                    break;
                }
                case UMSGPAT_ARG_TYPE_PLURAL:
                case UMSGPAT_ARG_TYPE_SELECTORDINAL: {
                    if (!arg->isNumeric()) {
                        success = U_ILLEGAL_ARGUMENT_ERROR;
                        return;
                    }
                    // const PluralSelectorProvider &selector =
                    //     argType == UMSGPAT_ARG_TYPE_PLURAL ? pluralProvider : ordinalProvider;
                    // // We must use the Formattable::getDouble() variant with the UErrorCode parameter
                    // // because only this one converts non-double numeric types to double.
                    // double offset = msgPattern.getPluralOffset(i);
                    // PluralSelectorContext context(i, argName, *arg, offset, success);
                    // int32_t subMsgStart = PluralFormat::findSubMessage(
                    //         msgPattern, i, selector, &context, arg->getDouble(success), success);
                    // formatComplexSubMessage(subMsgStart, &context, arguments, argumentNames,
                    //                         cnt, appendTo, success);
                    break;
                }
                case UMSGPAT_ARG_TYPE_SELECT: {
                    // int32_t subMsgStart = SelectFormat::findSubMessage(msgPattern, i, arg->getString(success), success);
                    // formatComplexSubMessage(subMsgStart, NULL, arguments, argumentNames,
                    //                         cnt, appendTo, success);
                    break;
                }
            }
        }
        prevIndex = msgPattern.getPart(argLimit).getLimit();
        i = argLimit;
    }
}

void FormatOperation::formatComplexSubMessage(int32_t msgStart,
                                              const PluralSelectorFormatOperationContext *plNumber,
                                              const Formattable* arguments,
                                              const UnicodeString *argumentNames,
                                              int32_t cnt,
                                              UnicodeString& appendTo,
                                              UErrorCode& success) const {
    // TODO
    success = U_UNSUPPORTED_ERROR;
}

}  // namespace

//--------------------------------------------------------------------

MessageFormatNano::MessageFormatNano(const UnicodeString& pattern,
                                     const Locale& newLocale,
                                     UParseError& parseError,
                                     UErrorCode& success)
        : locale(newLocale),
          msgPattern(pattern, &parseError, success),
          messageFormatProvider(new MessageFormatProvider())
{
}

MessageFormatNano::MessageFormatNano(const UnicodeString& pattern,
                                     const Locale& newLocale,
                                     LocalPointer<const MessageFormatProvider> messageFormatProvider,
                                     UParseError& parseError,
                                     UErrorCode& success)
        : locale(newLocale),
          msgPattern(pattern, &parseError, success),
          messageFormatProvider(std::move(messageFormatProvider))
{
}

void MessageFormatNano::format(const Formattable* arguments,
                               const UnicodeString *argumentNames,
                               int32_t cnt,
                               UnicodeString& appendTo,
                               UErrorCode& success) const {
    FormatOperation formatOperation(locale, msgPattern, *messageFormatProvider);
    formatOperation.format(/*msgStart=*/0, arguments, argumentNames, cnt, appendTo, success);
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

//eof
