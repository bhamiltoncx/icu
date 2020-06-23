// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "hash.h"
#include "mutex.h"
#include "unicode/bytestream.h"
#include "unicode/msgfmtnano.h"
#include "unicode/msgfmtnano_datetimeprovider.h"
#include "unicode/datefmt.h"
#include "unicode/uloc.h"

U_CDECL_BEGIN

static void U_CALLCONV
deleteFormat(void *obj) {
    delete static_cast<icu::Format *>(obj);
}

U_CDECL_END

U_NAMESPACE_BEGIN

namespace {

DateFormat::EStyle dateTimeStyleToDateFormatStyle(DateTimeFormatProvider::DateTimeStyle style, UErrorCode& status) {
    switch (style) {
        case DateTimeFormatProvider::STYLE_NONE:
            return DateFormat::kNone;
        case DateTimeFormatProvider::STYLE_DEFAULT:
            return DateFormat::kDefault;
        case DateTimeFormatProvider::STYLE_SHORT:
            return DateFormat::kShort;
        case DateTimeFormatProvider::STYLE_MEDIUM:
            return DateFormat::kMedium;
        case DateTimeFormatProvider::STYLE_LONG:
            return DateFormat::kLong;
        case DateTimeFormatProvider::STYLE_FULL:
            return DateFormat::kFull;
    }
    status = U_INTERNAL_PROGRAM_ERROR;
    return DateFormat::kDefault;
}

void appendDateTimeStyleString(DateTimeFormatProvider::DateTimeStyle style, std::string& key) {
    switch (style) {
        case DateTimeFormatProvider::STYLE_NONE:
            key.append("none");
            break;
        case DateTimeFormatProvider::STYLE_DEFAULT:
            key.append("default");
            break;
        case DateTimeFormatProvider::STYLE_SHORT:
            key.append("short");
            break;
        case DateTimeFormatProvider::STYLE_MEDIUM:
            key.append("medium");
            break;
        case DateTimeFormatProvider::STYLE_LONG:
            key.append("long");
            break;
        case DateTimeFormatProvider::STYLE_FULL:
            key.append("full");
            break;
    }
}

// Protects access to |formatters|.
UMutex gMutex;

class DateTimeFormatProviderImpl : public DateTimeFormatProvider {
 public:
    DateTimeFormatProviderImpl(UErrorCode& status) :
        formatters(/*ignoreKeyCase=*/FALSE, status) {
      formatters.setValueDeleter(deleteFormat);
    }
    DateTimeFormatProviderImpl &operator=(DateTimeFormatProviderImpl&&) = default;
    DateTimeFormatProviderImpl(DateTimeFormatProviderImpl&&) = default;
    DateTimeFormatProviderImpl &operator=(const DateTimeFormatProviderImpl&) = delete;
    DateTimeFormatProviderImpl(const DateTimeFormatProviderImpl&) = delete;

    const Format* dateTimeFormat(DateTimeStyle dateStyle, DateTimeStyle timeStyle, const Locale& locale, UErrorCode& status) const;
    const Format* dateTimeFormatForSkeleton(const UnicodeString& skeleton, const Locale& locale, UErrorCode& status) const;

 private:
    mutable Hashtable formatters;
};

const Format* DateTimeFormatProviderImpl::dateTimeFormat(DateTimeStyle dateStyle, DateTimeStyle timeStyle, const Locale& locale, UErrorCode& status) const {
  std::string key;
  key.append("dateStyle=");
  appendDateTimeStyleString(dateStyle, key);
  key.append("|timeStyle=");
  appendDateTimeStyleString(timeStyle, key);
  key.append("|");
  StringByteSink<std::string> keySink(&key, /*initialAppendCapacity=*/32);
  locale.toLanguageTag(keySink, status);
  UnicodeString ukey(key.data(), key.length(), US_INV);
  DateFormat::EStyle dateFormatStyle = dateTimeStyleToDateFormatStyle(dateStyle, status);
  DateFormat::EStyle timeFormatStyle = dateTimeStyleToDateFormatStyle(timeStyle, status);
  {
    Mutex lock(&gMutex);
    Format* format = static_cast<Format*>(formatters.get(ukey));
    if (!format) {
        format = DateFormat::createDateTimeInstance(dateFormatStyle, timeFormatStyle, locale);
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

const Format* DateTimeFormatProviderImpl::dateTimeFormatForSkeleton(const UnicodeString& skeleton, const Locale& locale, UErrorCode& status) const {
  std::string key("skeleton|");
  StringByteSink<std::string> keySink(&key, /*initialAppendCapacity=*/32);
  skeleton.toUTF8(keySink);
  key.append("|");
  locale.toLanguageTag(keySink, status);
  UnicodeString ukey(UnicodeString::fromUTF8(StringPiece(key)));
  {
    Mutex lock(&gMutex);
    Format* format = static_cast<Format*>(formatters.get(ukey));
    if (!format) {
        format = DateFormat::createInstanceForSkeleton(skeleton, locale, status);
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

LocalPointer<const DateTimeFormatProvider> DateTimeFormatProviderNano::createInstance(UErrorCode& success) {
    if (U_FAILURE(success)) {
        return LocalPointer<const DateTimeFormatProvider>();
    }
    return LocalPointer<const DateTimeFormatProvider>(new DateTimeFormatProviderImpl(success));
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
