// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "sharedformat.h"
#include "unifiedcache.h"
#include "unicode/bytestream.h"
#include "unicode/decimfmt.h"
#include "unicode/msgfmtnano.h"
#include "unicode/msgfmtnano_rulebasednumberprovider.h"
#include "unicode/rbnf.h"
#include "unicode/numfmt.h"
#include "unicode/uloc.h"

U_NAMESPACE_BEGIN

namespace {

class RuleBasedNumberFormatKey : public LocaleCacheKey<SharedFormat> {
public:
    RuleBasedNumberFormatKey(
        const Locale& loc,
        URBNFRuleSetTag ruleSetTag,
        const UnicodeString& defaultRuleSet)
            : LocaleCacheKey<SharedFormat>(loc),
              ruleSetTag(ruleSetTag),
              defaultRuleSet(defaultRuleSet) { }
    RuleBasedNumberFormatKey(const RuleBasedNumberFormatKey &other) :
            LocaleCacheKey<SharedFormat>(other),
            ruleSetTag(other.ruleSetTag),
            defaultRuleSet(other.defaultRuleSet) { }
    int32_t hashCode() const {
        int32_t result = 1;
        result = 37u * result + (uint32_t)LocaleCacheKey<SharedFormat>::hashCode();
        result = 37u * result + (uint32_t)ruleSetTag;
        result = 37u * result + (uint32_t)defaultRuleSet.hashCode();
        return result;
    }
    UBool operator==(const CacheKeyBase &other) const {
       // reflexive
       if (this == &other) {
           return TRUE;
       }
       if (!LocaleCacheKey<SharedFormat>::operator==(other)) {
           return FALSE;
       }
       // We know that this and other are of same class if we get this far.
       const RuleBasedNumberFormatKey &realOther =
               static_cast<const RuleBasedNumberFormatKey &>(other);
       return (realOther.ruleSetTag == ruleSetTag && realOther.defaultRuleSet == defaultRuleSet);
    }
    CacheKeyBase *clone() const {
        return new RuleBasedNumberFormatKey(*this);
    }
    const SharedFormat *createObject(
            const void * /*unused*/, UErrorCode &status) const {
        RuleBasedNumberFormat* format = new RuleBasedNumberFormat(ruleSetTag, fLoc, status);
        if (format && !defaultRuleSet.isEmpty()) {
            format->setDefaultRuleSet(defaultRuleSet, status);
        }
        SharedFormat *result = new SharedFormat(format);
        result->addRef();
        return result;
    }

  private:
    const URBNFRuleSetTag ruleSetTag;
    const UnicodeString defaultRuleSet;
};

URBNFRuleSetTag ruleSetTagForNumberFormatType(RuleBasedNumberFormatProvider::RuleBasedNumberFormatType type, UErrorCode& status) {
    switch (type) {
      case RuleBasedNumberFormatProvider::TYPE_SPELLOUT:
        return URBNF_SPELLOUT;
      case RuleBasedNumberFormatProvider::TYPE_DURATION:
        return URBNF_DURATION;
      case RuleBasedNumberFormatProvider::TYPE_ORDINAL:
        return URBNF_ORDINAL;
    }
    status = U_INTERNAL_PROGRAM_ERROR;
    return URBNF_SPELLOUT;
}

class RuleBasedNumberFormatProviderImpl : public RuleBasedNumberFormatProvider {
 public:
    RuleBasedNumberFormatProviderImpl() = default;
    RuleBasedNumberFormatProviderImpl &operator=(RuleBasedNumberFormatProviderImpl&&) = default;
    RuleBasedNumberFormatProviderImpl(RuleBasedNumberFormatProviderImpl&&) = default;
    RuleBasedNumberFormatProviderImpl &operator=(const RuleBasedNumberFormatProviderImpl&) = delete;
    RuleBasedNumberFormatProviderImpl(const RuleBasedNumberFormatProviderImpl&) = delete;

    void formatRuleBasedNumber(const Formattable& number, RuleBasedNumberFormatType type, const Locale& locale, const UnicodeString& defaultRuleSet, UnicodeString& appendTo, UErrorCode& status) const;
};

void RuleBasedNumberFormatProviderImpl::formatRuleBasedNumber(const Formattable& number, RuleBasedNumberFormatType type, const Locale& locale, const UnicodeString& defaultRuleSet, UnicodeString& appendTo, UErrorCode& status) const {
    const UnifiedCache* cache = UnifiedCache::getInstance(status);
    if (U_FAILURE(status)) {
        return;
    }
    const SharedFormat* shared = nullptr;
    URBNFRuleSetTag ruleSetTag = ruleSetTagForNumberFormatType(type, status);
    if (U_FAILURE(status)) {
      return;
    }
    cache->get(RuleBasedNumberFormatKey(locale, ruleSetTag, defaultRuleSet), shared, status);
    if (U_FAILURE(status)) {
        return;
    }
    (*shared)->format(number, appendTo, status);
    shared->removeRef();
}

}  // namespace

LocalPointer<const RuleBasedNumberFormatProvider> RuleBasedNumberFormatProviderNano::createInstance(UErrorCode& success) {
    if (U_FAILURE(success)) {
        return LocalPointer<const RuleBasedNumberFormatProvider>();
    }
    return LocalPointer<const RuleBasedNumberFormatProvider>(new RuleBasedNumberFormatProviderImpl());
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
