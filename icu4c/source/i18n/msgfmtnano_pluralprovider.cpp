// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

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

U_NAMESPACE_BEGIN

namespace {

// Protects access to |rules| and |locale|.
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
    PluralSelectorImpl(UPluralType pluralType) : pluralType(pluralType) { }
    PluralSelectorImpl &operator=(PluralSelectorImpl&&) = default;
    PluralSelectorImpl(PluralSelectorImpl&&) = default;
    PluralSelectorImpl &operator=(const PluralSelectorImpl&) = delete;
    PluralSelectorImpl(const PluralSelectorImpl&) = delete;
    UnicodeString select(void *ctx, double number, UErrorCode& ec) const;

private:
    const PluralRules* rulesForLocaleLocked(const Locale& rulesLocale, UErrorCode& ec) const;
    const UPluralType pluralType;
    mutable Locale locale;
    mutable LocalPointer<PluralRules> rules;
};

class PluralFormatProviderImpl : public PluralFormatProvider {
public:
    PluralFormatProviderImpl() :
            cardinalSelector(new PluralSelectorImpl(UPLURAL_TYPE_CARDINAL)),
            ordinalSelector(new PluralSelectorImpl(UPLURAL_TYPE_ORDINAL)) { }
    PluralFormatProviderImpl &operator=(PluralFormatProviderImpl&&) = default;
    PluralFormatProviderImpl(PluralFormatProviderImpl&&) = default;
    PluralFormatProviderImpl &operator=(const PluralFormatProviderImpl&) = delete;
    PluralFormatProviderImpl(const PluralFormatProviderImpl&) = delete;

    const PluralFormatProvider::Selector* pluralSelector(PluralFormatProvider::PluralType /*pluralType*/, UErrorCode& status) const;
    int32_t selectFindSubMessage(const MessagePattern& /*pattern*/, int32_t /*partIndex*/, const UnicodeString& /*keyword*/, UErrorCode& status) const;

private:
    const LocalPointer<const PluralFormatProvider::Selector> cardinalSelector;
    const LocalPointer<const PluralFormatProvider::Selector> ordinalSelector;
};

const PluralRules* PluralSelectorImpl::rulesForLocaleLocked(const Locale& rulesLocale, UErrorCode& ec) const {
  // gMutex must be locked before invoking this.
  if (rules.isValid() && locale == rulesLocale) {
    return rules.getAlias();
  }
  locale = rulesLocale;
  rules.adoptInstead(PluralRules::forLocale(rulesLocale, pluralType, ec));
  if (U_FAILURE(ec)) {
    return nullptr;
  }
  return rules.getAlias();
}

UnicodeString PluralSelectorImpl::select(void *ctx, double number, UErrorCode& ec) const {
    fprintf(stderr, "PluralSelectorImpl::select number=%f ec=%s\n", number, u_errorName(ec));
    if (U_FAILURE(ec)) {
        return UnicodeString(u"other", 5);
    }
    fprintf(stderr, "ctx=%p\n", ctx);
    PluralFormatProvider::SelectorContext &context = *static_cast<PluralFormatProvider::SelectorContext *>(ctx);
    // Select a sub-message according to how the number is formatted,
    // which is specified in the selected sub-message.
    // We avoid this circle by looking at how
    // the number is formatted in the "other" sub-message
    // which must always be present and usually contains the number.
    // Message authors should be consistent across sub-messages.
    fprintf(stderr, "context.number.getType -> %d, getDouble -> %f ec=%s\n", context.number.getType(), context.number.getDouble(ec), u_errorName(ec));
    std::string s;
    fprintf(stderr, "context.argName=%s\n", context.argName.toUTF8String(s).c_str());
    s.clear();
    int32_t otherIndex = findOtherSubMessage(context.msgPattern, context.startIndex);
    fprintf(stderr, "otherIndex=%d\n", otherIndex);
    context.numberArgIndex = findFirstPluralNumberArg(context.msgPattern, otherIndex, context.argName);
    fprintf(stderr, "context.numberArgIndex=%d\n", context.numberArgIndex);
    if (!context.formatter) {
        context.formatter = context.numberFormatProvider.numberFormat(NumberFormatProvider::TYPE_NUMBER, locale, ec);
        if (U_FAILURE(ec)) {
          return UnicodeString(u"other", 5);
        }
        fprintf(stderr, "context.formatter = %p ec = %s\n", context.formatter, u_errorName(ec));
        context.forReplaceNumber = TRUE;
    }
    fprintf(stderr, "context.number.getType -> %d, getDouble -> %f\n", context.number.getType(), context.number.getDouble(ec));
    if (context.number.getDouble(ec) != number) {
      fprintf(stderr, "ERROR: context.number.getDouble -> %f, number -> %f\n", context.number.getDouble(ec), number);
        ec = U_INTERNAL_PROGRAM_ERROR;
        return UnicodeString(u"other", 5);
    }
    context.formatter->format(context.number, context.numberString, ec);
    auto* decFmt = dynamic_cast<const DecimalFormat *>(context.formatter);
    if(decFmt != NULL) {
        number::impl::DecimalQuantity dq;
        decFmt->formatToDecimalQuantity(context.number, dq, ec);
        if (U_FAILURE(ec)) {
            return UnicodeString(u"other", 5);
        }
        {
          Mutex lock(&gMutex);
          const PluralRules *rules = rulesForLocaleLocked(context.locale, ec);
          if (rules) {
            return rules->select(dq);
          }
        }
    } else {
      Mutex lock(&gMutex);
      const PluralRules *rules = rulesForLocaleLocked(context.locale, ec);
      if (rules) {
        return rules->select(number);
      }
    }
    if (U_SUCCESS(ec)) {
      ec = U_INTERNAL_PROGRAM_ERROR;
    }
    return UnicodeString(u"other", 5);
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

int32_t PluralFormatProviderImpl::selectFindSubMessage(const MessagePattern& /*pattern*/, int32_t /*partIndex*/, const UnicodeString& /*keyword*/, UErrorCode& status) const {
    status = U_UNSUPPORTED_ERROR;
    return -1;
}

}  // namespace

LocalPointer<const PluralFormatProvider> PluralFormatProviderNano::createInstance(UErrorCode& success) {
    if (U_FAILURE(success)) {
        return LocalPointer<const PluralFormatProvider>();
    }
    return LocalPointer<const PluralFormatProvider>(new PluralFormatProviderImpl());
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
