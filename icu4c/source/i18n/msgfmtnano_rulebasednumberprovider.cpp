// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "hash.h"
#include "mutex.h"
#include "unicode/bytestream.h"
#include "unicode/decimfmt.h"
#include "unicode/msgfmtnano.h"
#include "unicode/msgfmtnano_rulebasednumberprovider.h"
#include "unicode/rbnf.h"
#include "unicode/numfmt.h"
#include "unicode/uloc.h"

U_CDECL_BEGIN

static void U_CALLCONV
deleteRuleBasedNumberFormat(void *obj) {
    delete static_cast<icu::RuleBasedNumberFormat *>(obj);
}

U_CDECL_END

U_NAMESPACE_BEGIN

namespace {

// Protects access to |formatters|.
UMutex gMutex;

class RuleBasedNumberFormatProviderImpl : public RuleBasedNumberFormatProvider {
 public:
    RuleBasedNumberFormatProviderImpl(UErrorCode& status) :
        formatters(/*ignoreKeyCase=*/FALSE, status) {
      formatters.setValueDeleter(deleteRuleBasedNumberFormat);
    }
    RuleBasedNumberFormatProviderImpl &operator=(RuleBasedNumberFormatProviderImpl&&) = default;
    RuleBasedNumberFormatProviderImpl(RuleBasedNumberFormatProviderImpl&&) = default;
    RuleBasedNumberFormatProviderImpl &operator=(const RuleBasedNumberFormatProviderImpl&) = delete;
    RuleBasedNumberFormatProviderImpl(const RuleBasedNumberFormatProviderImpl&) = delete;

    void formatRuleBasedNumber(const Formattable& number, RuleBasedNumberFormatType type, const Locale& locale, const UnicodeString& defaultRuleSet, UnicodeString& appendTo, UErrorCode& status) const;

 private:
    mutable Hashtable formatters;
};

void RuleBasedNumberFormatProviderImpl::formatRuleBasedNumber(const Formattable& number, RuleBasedNumberFormatType type, const Locale& locale, const UnicodeString& defaultRuleSet, UnicodeString& appendTo, UErrorCode& status) const {
  std::string key;
  switch (type) {
    case RuleBasedNumberFormatProvider::TYPE_SPELLOUT:
      key.append("spellout|");
      break;
    case RuleBasedNumberFormatProvider::TYPE_DURATION:
      key.append("duration|");
      break;
    case RuleBasedNumberFormatProvider::TYPE_ORDINAL:
      key.append("ordinal|");
      break;
  }
  if (!defaultRuleSet.isEmpty()) {
    key.append("defaultRuleSet=");
    defaultRuleSet.toUTF8String(key);
    key.append("|");
  }
  StringByteSink<std::string> keySink(&key, /*initialAppendCapacity=*/32);
  locale.toLanguageTag(keySink, status);
  UnicodeString ukey(key.data(), key.length(), US_INV);
  RuleBasedNumberFormat* format;
  {
    Mutex lock(&gMutex);
    format = static_cast<RuleBasedNumberFormat*>(formatters.get(ukey));
    if (!format) {
      switch (type) {
        case RuleBasedNumberFormatProvider::TYPE_SPELLOUT:
          format = new RuleBasedNumberFormat(URBNF_SPELLOUT, locale, status);
          break;
        case RuleBasedNumberFormatProvider::TYPE_DURATION:
          format = new RuleBasedNumberFormat(URBNF_DURATION, locale, status);
          break;
        case RuleBasedNumberFormatProvider::TYPE_ORDINAL:
          format = new RuleBasedNumberFormat(URBNF_ORDINAL, locale, status);
          break;
      }
      if (format && !defaultRuleSet.isEmpty()) {
        format->setDefaultRuleSet(defaultRuleSet, status);
      }
      if (format) {
        formatters.put(ukey, format, status);
      }
    }
    if (!format && U_SUCCESS(status)) {
      status = U_INTERNAL_PROGRAM_ERROR;
    }
  }
  if (U_SUCCESS(status)) {
      format->format(number, appendTo, status);
  }
}

}  // namespace

LocalPointer<const RuleBasedNumberFormatProvider> RuleBasedNumberFormatProviderNano::createInstance(UErrorCode& success) {
    if (U_FAILURE(success)) {
        return LocalPointer<const RuleBasedNumberFormatProvider>();
    }
    return LocalPointer<const RuleBasedNumberFormatProvider>(new RuleBasedNumberFormatProviderImpl(success));
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
