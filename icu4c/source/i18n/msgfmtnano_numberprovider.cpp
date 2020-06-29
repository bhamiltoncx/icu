// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "sharedformat.h"
#include "unifiedcache.h"
#include "unicode/bytestream.h"
#include "unicode/decimfmt.h"
#include "unicode/msgfmtnano.h"
#include "unicode/msgfmtnano_numberprovider.h"
#include "unicode/numberformatter.h"
#include "unicode/numfmt.h"
#include "unicode/uloc.h"

U_NAMESPACE_BEGIN

namespace {

class NumberFormatWithTypeKey : public LocaleCacheKey<SharedFormat> {
public:
    NumberFormatWithTypeKey(
        const Locale &loc,
        NumberFormatProvider::NumberFormatType type)
            : LocaleCacheKey<SharedFormat>(loc),
              type(type) { }
    NumberFormatWithTypeKey(const NumberFormatWithTypeKey &other) :
            LocaleCacheKey<SharedFormat>(other),
            type(other.type) { }
    int32_t hashCode() const {
        return (int32_t)(37u * (uint32_t)LocaleCacheKey<SharedFormat>::hashCode() + (uint32_t)type);
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
       const NumberFormatWithTypeKey &realOther =
               static_cast<const NumberFormatWithTypeKey &>(other);
       return (realOther.type == type);
    }
    CacheKeyBase *clone() const {
        return new NumberFormatWithTypeKey(*this);
    }
    const SharedFormat *createObject(
            const void * /*unused*/, UErrorCode &status) const {
        Format *format = nullptr;
        switch (type) {
            case NumberFormatProvider::TYPE_NUMBER:
                format = NumberFormat::createInstance(fLoc, status);
                break;
            case NumberFormatProvider::TYPE_CURRENCY:
                format = NumberFormat::createCurrencyInstance(fLoc, status);
                break;
            case NumberFormatProvider::TYPE_PERCENT:
                format = NumberFormat::createPercentInstance(fLoc, status);
                break;
            case NumberFormatProvider::TYPE_INTEGER: {
                format = number::NumberFormatter::withLocale(fLoc).precision(number::Precision::maxFraction(0)).toFormat(status);
                break;
            }
        }
        if (!format && U_SUCCESS(status)) {
            status = U_INTERNAL_PROGRAM_ERROR;
        }
        SharedFormat *result = new SharedFormat(format);
        result->addRef();
        return result;
    }

  private:
    const NumberFormatProvider::NumberFormatType type;
};

class NumberFormatWithSkeletonKey : public LocaleCacheKey<SharedFormat> {
public:
    NumberFormatWithSkeletonKey(
        const Locale& loc,
        const UnicodeString& skeleton)
            : LocaleCacheKey<SharedFormat>(loc),
              skeleton(skeleton) { }
    NumberFormatWithSkeletonKey(const NumberFormatWithSkeletonKey &other) :
            LocaleCacheKey<SharedFormat>(other),
            skeleton(other.skeleton) { }
    int32_t hashCode() const {
        return (int32_t)(37u * (uint32_t)LocaleCacheKey<SharedFormat>::hashCode() + (uint32_t)skeleton.hashCode());
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
       const NumberFormatWithSkeletonKey &realOther =
               static_cast<const NumberFormatWithSkeletonKey &>(other);
       return (realOther.skeleton == skeleton);
    }
    CacheKeyBase *clone() const {
        return new NumberFormatWithSkeletonKey(*this);
    }
    const SharedFormat *createObject(
            const void * /*unused*/, UErrorCode &status) const {
        Format* format = number::NumberFormatter::forSkeleton(skeleton, status).locale(fLoc).toFormat(status);
        if (!format && U_SUCCESS(status)) {
            status = U_INTERNAL_PROGRAM_ERROR;
        }
        SharedFormat *result = new SharedFormat(format);
        result->addRef();
        return result;
    }

  private:
    const UnicodeString skeleton;
};

class NumberFormatWithDecimalPatternKey : public LocaleCacheKey<SharedFormat> {
public:
    NumberFormatWithDecimalPatternKey(
        const Locale& loc,
        const UnicodeString& pattern)
            : LocaleCacheKey<SharedFormat>(loc),
              pattern(pattern) { }
    NumberFormatWithDecimalPatternKey(const NumberFormatWithDecimalPatternKey &other) :
            LocaleCacheKey<SharedFormat>(other),
            pattern(other.pattern) { }
    int32_t hashCode() const {
        return (int32_t)(37u * (uint32_t)LocaleCacheKey<SharedFormat>::hashCode() + (uint32_t)pattern.hashCode());
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
       const NumberFormatWithDecimalPatternKey &realOther =
               static_cast<const NumberFormatWithDecimalPatternKey &>(other);
       return (realOther.pattern == pattern);
    }
    CacheKeyBase *clone() const {
        return new NumberFormatWithDecimalPatternKey(*this);
    }
    const SharedFormat *createObject(
            const void * /*unused*/, UErrorCode &status) const {
        Format* format = NumberFormat::createInstance(fLoc, status);
        DecimalFormat* decimalFormat = dynamic_cast<DecimalFormat*>(format);
        if (decimalFormat) {
            decimalFormat->applyPattern(pattern, status);
        }
        SharedFormat *result = new SharedFormat(format);
        result->addRef();
        return result;
    }

  private:
    const UnicodeString pattern;
};

class NumberFormatProviderImpl : public NumberFormatProvider {
 public:
    NumberFormatProviderImpl() = default;
    NumberFormatProviderImpl &operator=(NumberFormatProviderImpl&&) = default;
    NumberFormatProviderImpl(NumberFormatProviderImpl&&) = default;
    NumberFormatProviderImpl &operator=(const NumberFormatProviderImpl&) = delete;
    NumberFormatProviderImpl(const NumberFormatProviderImpl&) = delete;

    void formatNumber(const Formattable& number, NumberFormatType type, const Locale& locale, UnicodeString& appendTo, UErrorCode& status) const;
    void formatNumberWithSkeleton(const Formattable& number, const UnicodeString& skeleton, const Locale& locale, UnicodeString& appendTo, UErrorCode& status) const;
    void formatDecimalNumberWithPattern(const Formattable& number, const UnicodeString& pattern, const Locale& locale, UnicodeString& appendTo, UErrorCode& status) const;
};

void NumberFormatProviderImpl::formatNumber(const Formattable& number, NumberFormatType type, const Locale& locale, UnicodeString& appendTo, UErrorCode& status) const {
    const UnifiedCache* cache = UnifiedCache::getInstance(status);
    if (U_FAILURE(status)) {
        return;
    }
    const SharedFormat* shared = nullptr;
    cache->get(NumberFormatWithTypeKey(locale, type), shared, status);
    if (U_FAILURE(status)) {
        return;
    }
    (*shared)->format(number, appendTo, status);
}

void NumberFormatProviderImpl::formatNumberWithSkeleton(const Formattable& number, const UnicodeString& skeleton, const Locale& locale, UnicodeString& appendTo, UErrorCode& status) const {
    const UnifiedCache* cache = UnifiedCache::getInstance(status);
    if (U_FAILURE(status)) {
        return;
    }
    const SharedFormat* shared = nullptr;
    cache->get(NumberFormatWithSkeletonKey(locale, skeleton), shared, status);
    if (U_FAILURE(status)) {
        return;
    }
    (*shared)->format(number, appendTo, status);
}

void NumberFormatProviderImpl::formatDecimalNumberWithPattern(const Formattable& number, const UnicodeString& pattern, const Locale& locale, UnicodeString& appendTo, UErrorCode& status) const {
    const UnifiedCache* cache = UnifiedCache::getInstance(status);
    if (U_FAILURE(status)) {
        return;
    }
    const SharedFormat* shared = nullptr;
    cache->get(NumberFormatWithDecimalPatternKey(locale, pattern), shared, status);
    if (U_FAILURE(status)) {
        return;
    }
    (*shared)->format(number, appendTo, status);
}

}  // namespace

LocalPointer<const NumberFormatProvider> NumberFormatProviderNano::createInstance(UErrorCode& success) {
    if (U_FAILURE(success)) {
        return LocalPointer<const NumberFormatProvider>();
    }
    return LocalPointer<const NumberFormatProvider>(new NumberFormatProviderImpl());
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
