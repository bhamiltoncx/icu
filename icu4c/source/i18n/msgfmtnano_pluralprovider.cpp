// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "hash.h"
#include "messageimpl.h"
#include "mutex.h"
#include "number_decimalquantity.h"
#include "uassert.h"
#include "unicode/localpointer.h"
#include "unicode/messagepattern.h"
#include "unicode/msgfmtnano.h"
#include "unicode/msgfmtnano_pluralprovider.h"
#include "unicode/plurfmt.h"
#include "unicode/plurrule.h"
#include "unicode/uloc.h"
#include "unicode/ustring.h"
#include "unicode/upluralrules.h"

//#define U_DEBUG_MSGFMTNANO 1

U_CDECL_BEGIN

static void U_CALLCONV
deletePluralRules(void *obj) {
    delete static_cast<icu::PluralRules *>(obj);
}

U_CDECL_END

U_NAMESPACE_BEGIN

namespace {

// Protects access to |rules|.
UMutex gMutex;

int32_t findOtherSubMessage(const MessagePattern& msgPattern, int32_t partIndex) {
    int32_t count=msgPattern.countParts();
    const MessagePattern::Part *part = &msgPattern.getPart(partIndex);
    if(MessagePattern::Part::hasNumericValue(part->getType())) {
        ++partIndex;
    }
    // Iterate over (ARG_SELECTOR [ARG_INT|ARG_DOUBLE] message) tuples
    // until ARG_LIMIT or end of plural-only pattern.
    UnicodeString other(u"other", 5);
    do {
        part=&msgPattern.getPart(partIndex++);
        UMessagePatternPartType type=part->getType();
        if(type==UMSGPAT_PART_TYPE_ARG_LIMIT) {
            break;
        }
        U_ASSERT(type==UMSGPAT_PART_TYPE_ARG_SELECTOR);
        // part is an ARG_SELECTOR followed by an optional explicit value, and then a message
        if(msgPattern.partSubstringMatches(*part, other)) {
            return partIndex;
        }
        if(MessagePattern::Part::hasNumericValue(msgPattern.getPartType(partIndex))) {
            ++partIndex;  // skip the numeric-value part of "=1" etc.
        }
        partIndex=msgPattern.getLimitPartIndex(partIndex);
    } while(++partIndex<count);
    return 0;
}

int32_t
findFirstPluralNumberArg(const MessagePattern& msgPattern, int32_t msgStart, const UnicodeString &argName) {
    for(int32_t i=msgStart+1;; ++i) {
        const MessagePattern::Part &part=msgPattern.getPart(i);
        UMessagePatternPartType type=part.getType();
        if(type==UMSGPAT_PART_TYPE_MSG_LIMIT) {
            return 0;
        }
        if(type==UMSGPAT_PART_TYPE_REPLACE_NUMBER) {
            return -1;
        }
        if(type==UMSGPAT_PART_TYPE_ARG_START) {
            UMessagePatternArgType argType=part.getArgType();
            if(!argName.isEmpty() && (argType==UMSGPAT_ARG_TYPE_NONE || argType==UMSGPAT_ARG_TYPE_SIMPLE)) {
                // ARG_NUMBER or ARG_NAME
                if(msgPattern.partSubstringMatches(msgPattern.getPart(i+1), argName)) {
                    return i;
                }
            }
            i=msgPattern.getLimitPartIndex(i);
        }
    }
}

class PluralSelectorImpl : public PluralFormatProvider::Selector {
public:
    PluralSelectorImpl(UPluralType pluralType, UErrorCode& status) : pluralType(pluralType), pluralRules(/*ignoreKeycase=*/FALSE, status) {
        pluralRules.setValueDeleter(deletePluralRules);
    }
    PluralSelectorImpl &operator=(PluralSelectorImpl&&) = default;
    PluralSelectorImpl(PluralSelectorImpl&&) = default;
    PluralSelectorImpl &operator=(const PluralSelectorImpl&) = delete;
    PluralSelectorImpl(const PluralSelectorImpl&) = delete;
    UnicodeString select(void *ctx, double number, UErrorCode& ec) const;

private:
    const PluralRules* pluralRulesForLocale(const Locale& locale, UErrorCode& ec) const;
    const UPluralType pluralType;
    mutable Hashtable pluralRules;
};

class PluralFormatProviderImpl : public PluralFormatProvider {
public:
    PluralFormatProviderImpl(UErrorCode& status) :
            cardinalSelector(new PluralSelectorImpl(UPLURAL_TYPE_CARDINAL, status)),
            ordinalSelector(new PluralSelectorImpl(UPLURAL_TYPE_ORDINAL, status)) { }
    PluralFormatProviderImpl &operator=(PluralFormatProviderImpl&&) = default;
    PluralFormatProviderImpl(PluralFormatProviderImpl&&) = default;
    PluralFormatProviderImpl &operator=(const PluralFormatProviderImpl&) = delete;
    PluralFormatProviderImpl(const PluralFormatProviderImpl&) = delete;

    const PluralFormatProvider::Selector* pluralSelector(PluralFormatProvider::PluralType pluralType, UErrorCode& status) const;

private:
    const LocalPointer<const PluralFormatProvider::Selector> cardinalSelector;
    const LocalPointer<const PluralFormatProvider::Selector> ordinalSelector;
};

const PluralRules* PluralSelectorImpl::pluralRulesForLocale(const Locale& locale, UErrorCode& ec) const {
    if (U_FAILURE(ec)) {
        return nullptr;
    }
    std::string key;
    StringByteSink<std::string> keySink(&key, /*initialAppendCapacity=*/32);
    locale.toLanguageTag(keySink, ec);
    UnicodeString ukey(key.data(), key.length(), US_INV);
    {
        Mutex lock(&gMutex);
        PluralRules* rules = static_cast<PluralRules*>(pluralRules.get(ukey));
        if (!rules) {
            rules = PluralRules::forLocale(locale, pluralType, ec);
            pluralRules.put(ukey, rules, ec);
        }
        return rules;
    }
}

UnicodeString PluralSelectorImpl::select(void *ctx, double number, UErrorCode& ec) const {
#ifdef U_DEBUG_MSGFMTNANO
    fprintf(stderr, "PluralSelectorImpl::select number=%f ec=%s\n", number, u_errorName(ec));
#endif  // U_DEBUG_MSGFMTNANO
    UnicodeString other(u"other", 5);
    if (U_FAILURE(ec)) {
        return other;
    }
    PluralFormatProvider::SelectorContext &context = *static_cast<PluralFormatProvider::SelectorContext *>(ctx);
    // Select a sub-message according to how the number is formatted,
    // which is specified in the selected sub-message.
    // We avoid this circle by looking at how
    // the number is formatted in the "other" sub-message
    // which must always be present and usually contains the number.
    // Message authors should be consistent across sub-messages.
    int32_t otherIndex = findOtherSubMessage(context.msgPattern, context.startIndex);
    context.numberArgIndex = findFirstPluralNumberArg(context.msgPattern, otherIndex, context.argName);
    if (!context.forReplaceNumber) {
        context.forReplaceNumber = TRUE;
    }
    if (context.number.getDouble(ec) != number) {
        ec = U_INTERNAL_PROGRAM_ERROR;
        return other;
    }
    context.numberFormatProvider.formatNumber(context.number, NumberFormatProvider::TYPE_NUMBER, context.locale, context.numberString, ec);
    const PluralRules *rules = pluralRulesForLocale(context.locale, ec);
    if (rules) {
        UnicodeString result = rules->select(number);
#ifdef U_DEBUG_MSGFMTNANO
        std::string s;
        fprintf(stderr, "PluralSelectorImpl::select(double) result=[%s]\n", result.toUTF8String(s).c_str());
#endif  // U_DEBUG_MSGFMTNANO
        return result;
    }
    if (U_SUCCESS(ec)) {
      ec = U_INTERNAL_PROGRAM_ERROR;
    }
    return other;
}

const PluralFormatProvider::Selector* PluralFormatProviderImpl::pluralSelector(PluralFormatProvider::PluralType pluralType, UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return nullptr;
    }
    switch (pluralType) {
        case PluralFormatProvider::TYPE_CARDINAL:
            return cardinalSelector.getAlias();
        case PluralFormatProvider::TYPE_ORDINAL:
            return ordinalSelector.getAlias();
    }
    status = U_INTERNAL_PROGRAM_ERROR;
    return nullptr;
}

}  // namespace

LocalPointer<const PluralFormatProvider> PluralFormatProviderNano::createInstance(UErrorCode& success) {
    if (U_FAILURE(success)) {
        return LocalPointer<const PluralFormatProvider>();
    }
    return LocalPointer<const PluralFormatProvider>(new PluralFormatProviderImpl(success));
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
