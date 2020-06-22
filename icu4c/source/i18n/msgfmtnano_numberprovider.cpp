// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "hash.h"
#include "mutex.h"
#include "unicode/bytestream.h"
#include "unicode/decimfmt.h"
#include "unicode/msgfmtnano.h"
#include "unicode/msgfmtnano_numberprovider.h"
#include "unicode/numberformatter.h"
#include "unicode/numfmt.h"
#include "unicode/uloc.h"

U_CDECL_BEGIN

static void U_CALLCONV
deleteFormat(void *obj) {
    delete (icu::Format *) obj;
}

U_CDECL_END

U_NAMESPACE_BEGIN

namespace {

// Protects access to |formatters|.
UMutex gMutex;

class NumberFormatProviderImpl : public NumberFormatProvider {
 public:
    NumberFormatProviderImpl(UErrorCode& status) :
        formatters(/*ignoreKeyCase=*/FALSE, status) {
      formatters.setValueDeleter(deleteFormat);
    }
    NumberFormatProviderImpl &operator=(NumberFormatProviderImpl&&) = default;
    NumberFormatProviderImpl(NumberFormatProviderImpl&&) = default;
    NumberFormatProviderImpl &operator=(const NumberFormatProviderImpl&) = delete;
    NumberFormatProviderImpl(const NumberFormatProviderImpl&) = delete;

    const Format* numberFormat(NumberFormatType type, const Locale& locale, UErrorCode& status) const;
    const Format* numberFormatForSkeleton(const UnicodeString& skeleton, const Locale& locale, UErrorCode& status) const;
    const Format* decimalFormatWithPattern(const UnicodeString& pattern, const Locale& locale, UErrorCode& status) const;

 private:
    mutable Hashtable formatters;
};

const Format* NumberFormatProviderImpl::numberFormat(NumberFormatType type, const Locale& locale, UErrorCode& status) const {
  std::string key;
  switch (type) {
    case NumberFormatProvider::TYPE_NUMBER:
      key.append("number|");
      break;
    case NumberFormatProvider::TYPE_CURRENCY:
      key.append("currency|");
      break;
    case NumberFormatProvider::TYPE_PERCENT:
      key.append("percent|");
      break;
    case NumberFormatProvider::TYPE_INTEGER:
      key.append("integer|");
      break;
  }
  StringByteSink<std::string> keySink(&key, /*initialAppendCapacity=*/32);
  locale.toLanguageTag(keySink, status);
  UnicodeString ukey(key.data(), key.length(), US_INV);
  {
    Mutex lock(&gMutex);
    Format* format = static_cast<Format*>(formatters.get(ukey));
    if (!format) {
      switch (type) {
        case NumberFormatProvider::TYPE_NUMBER:
          format = NumberFormat::createInstance(locale, status);
          break;
        case NumberFormatProvider::TYPE_CURRENCY:
          format = NumberFormat::createCurrencyInstance(locale, status);
          break;
        case NumberFormatProvider::TYPE_PERCENT:
          format = NumberFormat::createPercentInstance(locale, status);
          break;
        case NumberFormatProvider::TYPE_INTEGER: {
          format = number::NumberFormatter::withLocale(locale).precision(number::Precision::maxFraction(0)).toFormat(status);
          break;
        }
      }
      if (format) {
        formatters.put(ukey, format, status);
      }
    }
    if (!format && U_SUCCESS(status)) {
      status = U_INTERNAL_PROGRAM_ERROR;
    }
    return format;
  }
}

const Format* NumberFormatProviderImpl::numberFormatForSkeleton(const UnicodeString& skeleton, const Locale& locale, UErrorCode& status) const {
  std::string key("skeleton|");
  StringByteSink<std::string> keySink(&key, /*initialAppendCapacity=*/32);
  locale.toLanguageTag(keySink, status);
  key.append("|");
  skeleton.toUTF8(keySink);
  UnicodeString ukey(UnicodeString::fromUTF8(StringPiece(key)));
  {
    Mutex lock(&gMutex);
    Format* format = static_cast<Format*>(formatters.get(ukey));
    if (!format) {
      format = number::NumberFormatter::forSkeleton(skeleton, status).locale(locale).toFormat(status);
      if (format) {
        formatters.put(ukey, format, status);
      }
    }
    if (!format && U_SUCCESS(status)) {
      status = U_INTERNAL_PROGRAM_ERROR;
    }
    return format;
  }
}

const Format* NumberFormatProviderImpl::decimalFormatWithPattern(const UnicodeString& pattern, const Locale& locale, UErrorCode& status) const {
  std::string key("decimalpattern|");
  StringByteSink<std::string> keySink(&key, /*initialAppendCapacity=*/32);
  locale.toLanguageTag(keySink, status);
  key.append("|");
  pattern.toUTF8(keySink);
  UnicodeString ukey(UnicodeString::fromUTF8(StringPiece(key)));
  {
    Mutex lock(&gMutex);
    Format* format = static_cast<Format*>(formatters.get(ukey));
    if (!format) {
      format = NumberFormat::createInstance(locale, status);
      DecimalFormat* decimalFormat = dynamic_cast<DecimalFormat*>(format);
      if (decimalFormat) {
        decimalFormat->applyPattern(pattern, status);
      }
      if (format) {
        formatters.put(ukey, format, status);
      }
    }
    if (!format && U_SUCCESS(status)) {
      status = U_INTERNAL_PROGRAM_ERROR;
    }
    return format;
  }
}

}  // namespace

LocalPointer<const NumberFormatProvider> NumberFormatProviderNano::createInstance(UErrorCode& success) {
    if (U_FAILURE(success)) {
        return LocalPointer<const NumberFormatProvider>();
    }
    return LocalPointer<const NumberFormatProvider>(new NumberFormatProviderImpl(success));
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
