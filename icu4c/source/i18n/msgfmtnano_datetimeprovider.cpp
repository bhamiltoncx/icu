// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "shareddateformat.h"
#include "unifiedcache.h"
#include "unicode/bytestream.h"
#include "unicode/calendar.h"
#include "unicode/datefmt.h"
#include "unicode/msgfmtnano.h"
#include "unicode/msgfmtnano_datetimeprovider.h"
#include "unicode/timezone.h"
#include "unicode/uloc.h"

U_NAMESPACE_BEGIN

namespace {

class DateFormatWithStylesKey : public LocaleCacheKey<SharedDateFormat> {
public:
    DateFormatWithStylesKey(
        const Locale &loc,
        DateFormat::EStyle dateStyle,
        DateFormat::EStyle timeStyle)
            : LocaleCacheKey<SharedDateFormat>(loc),
              dateStyle(dateStyle),
              timeStyle(timeStyle) { }
    DateFormatWithStylesKey(const DateFormatWithStylesKey &other) :
            LocaleCacheKey<SharedDateFormat>(other),
            dateStyle(other.dateStyle),
            timeStyle(other.timeStyle) { }
    int32_t hashCode() const {
      int32_t result = 1;
      result = 37u * result + (uint32_t)LocaleCacheKey<SharedDateFormat>::hashCode();
      result = 37u * result + (uint32_t)dateStyle;
      result = 37u * result + (uint32_t)timeStyle;
      return result;
    }
    UBool operator==(const CacheKeyBase &other) const {
       // reflexive
       if (this == &other) {
           return TRUE;
       }
       if (!LocaleCacheKey<SharedDateFormat>::operator==(other)) {
           return FALSE;
       }
       // We know that this and other are of same class if we get this far.
       const DateFormatWithStylesKey &realOther =
               static_cast<const DateFormatWithStylesKey &>(other);
       return (realOther.dateStyle == dateStyle && realOther.timeStyle == timeStyle);
    }
    CacheKeyBase *clone() const {
        return new DateFormatWithStylesKey(*this);
    }
    const SharedDateFormat *createObject(
        const void * /*unused*/, UErrorCode &/*status*/) const {
        DateFormat *format = DateFormat::createDateTimeInstance(dateStyle, timeStyle, fLoc);
        SharedDateFormat *result = new SharedDateFormat(format);
        result->addRef();
        return result;
    }

  private:
    const DateFormat::EStyle dateStyle;
    const DateFormat::EStyle timeStyle;
};

class DateFormatWithSkeletonKey : public LocaleCacheKey<SharedDateFormat> {
public:
    DateFormatWithSkeletonKey(
        const Locale& loc,
        const UnicodeString& skeleton)
            : LocaleCacheKey<SharedDateFormat>(loc),
              skeleton(skeleton) { }
    DateFormatWithSkeletonKey(const DateFormatWithSkeletonKey &other) :
            LocaleCacheKey<SharedDateFormat>(other),
            skeleton(other.skeleton) { }
    int32_t hashCode() const {
        return (int32_t)(37u * (uint32_t)LocaleCacheKey<SharedDateFormat>::hashCode() + (uint32_t)skeleton.hashCode());
    }
    UBool operator==(const CacheKeyBase &other) const {
       // reflexive
       if (this == &other) {
           return TRUE;
       }
       if (!LocaleCacheKey<SharedDateFormat>::operator==(other)) {
           return FALSE;
       }
       // We know that this and other are of same class if we get this far.
       const DateFormatWithSkeletonKey &realOther =
               static_cast<const DateFormatWithSkeletonKey &>(other);
       return (realOther.skeleton == skeleton);
    }
    CacheKeyBase *clone() const {
        return new DateFormatWithSkeletonKey(*this);
    }
    const SharedDateFormat *createObject(
            const void * /*unused*/, UErrorCode &status) const {
        DateFormat* format = DateFormat::createInstanceForSkeleton(skeleton, fLoc, status);
        SharedDateFormat *result = new SharedDateFormat(format);
        result->addRef();
        return result;
    }

  private:
    const UnicodeString skeleton;
};

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

void formattableToUDate(const Formattable& date, UDate& udate, UErrorCode& status) {
    switch (date.getType()) {
    case Formattable::kDate:
        udate = date.getDate();
        return;
    case Formattable::kDouble:
        udate = (UDate)date.getDouble();
        return;
    case Formattable::kLong:
        udate = (UDate)date.getLong();
        return;
    default:
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
}

LocalPointer<Calendar> calendarWithDateAndTimeZone(const Locale& locale, const Calendar* calendar, const Formattable& date, const TimeZone* timeZone, UErrorCode& status) {
    LocalPointer<Calendar> localCalendar;
    if (calendar) {
        localCalendar.adoptInstead(calendar->clone());
    } else if (timeZone) {
        localCalendar.adoptInstead(Calendar::createInstance(*timeZone, locale, status));
    } else {
        localCalendar.adoptInstead(Calendar::createInstance(TimeZone::createDefault(), locale, status));
    }
    UDate udate;
    formattableToUDate(date, udate, status);
    localCalendar->setTime(udate, status);
    if (timeZone) {
        localCalendar->setTimeZone(*timeZone);
    }
    return localCalendar;
}

class DateTimeFormatProviderImpl : public DateTimeFormatProvider {
 public:
    DateTimeFormatProviderImpl() = default;
    DateTimeFormatProviderImpl &operator=(DateTimeFormatProviderImpl&&) = default;
    DateTimeFormatProviderImpl(DateTimeFormatProviderImpl&&) = default;
    DateTimeFormatProviderImpl &operator=(const DateTimeFormatProviderImpl&) = delete;
    DateTimeFormatProviderImpl(const DateTimeFormatProviderImpl&) = delete;

    void formatDateTime(const Formattable& date, DateTimeStyle dateStyle, DateTimeStyle timeStyle, const Locale& locale, const TimeZone* timeZone, UnicodeString& appendTo, UErrorCode& status) const;
    void formatDateTimeWithSkeleton(const Formattable& date, const UnicodeString& skeleton, const Locale& locale, const TimeZone* timeZone, UnicodeString& appendTo, UErrorCode& status) const;
};

void DateTimeFormatProviderImpl::formatDateTime(const Formattable& date, DateTimeStyle dateStyle, DateTimeStyle timeStyle, const Locale& locale, const TimeZone* timeZone, UnicodeString& appendTo, UErrorCode& status) const {
    const UnifiedCache* cache = UnifiedCache::getInstance(status);
    if (U_FAILURE(status)) {
        return;
    }
    DateFormat::EStyle dateFormatStyle = dateTimeStyleToDateFormatStyle(dateStyle, status);
    DateFormat::EStyle timeFormatStyle = dateTimeStyleToDateFormatStyle(timeStyle, status);
    if (U_FAILURE(status)) {
      return;
    }
    const SharedDateFormat* shared = nullptr;
    cache->get(DateFormatWithStylesKey(locale, dateFormatStyle, timeFormatStyle), shared, status);
    if (U_FAILURE(status)) {
        return;
    }
    LocalPointer<Calendar> localCalendar(calendarWithDateAndTimeZone(locale, (*shared)->getCalendar(), date, timeZone, status));
    (*shared)->format(*localCalendar, appendTo, /*posIter=*/nullptr, status);
    shared->removeRef();
}

void DateTimeFormatProviderImpl::formatDateTimeWithSkeleton(const Formattable& date, const UnicodeString& skeleton, const Locale& locale, const TimeZone* timeZone, UnicodeString& appendTo, UErrorCode& status) const {
    const UnifiedCache* cache = UnifiedCache::getInstance(status);
    if (U_FAILURE(status)) {
        return;
    }
    const SharedDateFormat* shared = nullptr;
    cache->get(DateFormatWithSkeletonKey(locale, skeleton), shared, status);
    if (U_FAILURE(status)) {
        return;
    }
    LocalPointer<Calendar> localCalendar(calendarWithDateAndTimeZone(locale, (*shared)->getCalendar(), date, timeZone, status));
    (*shared)->format(*localCalendar, appendTo, /*posIter=*/nullptr, status);
    shared->removeRef();
}

}  // namespace

LocalPointer<const DateTimeFormatProvider> DateTimeFormatProviderNano::createInstance(UErrorCode& success) {
    if (U_FAILURE(success)) {
        return LocalPointer<const DateTimeFormatProvider>();
    }
    return LocalPointer<const DateTimeFormatProvider>(new DateTimeFormatProviderImpl());
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
