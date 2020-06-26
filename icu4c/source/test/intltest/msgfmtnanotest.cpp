// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License

#include "unicode/unistr.h"
#include "unicode/utypes.h"
#include "unicode/msgfmtnano.h"
#include "unicode/msgfmtnano_datetimeprovider.h"
#include "unicode/msgfmtnano_numberprovider.h"
#include "unicode/msgfmtnano_pluralprovider.h"
#include "unicode/plurfmt.h"
#include "cmemory.h"
#include "intltest.h"

class MessageFormatNanoTest : public IntlTest {
public:
    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par=NULL);
    void testEmpty();
    void testBasics();
    void testPlurals();
    void testDateTime();
    void testDateTimeLocale();
};

extern IntlTest *createMessageFormatNanoTest() {
    return new MessageFormatNanoTest();
}

void MessageFormatNanoTest::runIndexedTest(int32_t index, UBool exec, const char *&name, char * /*par*/) {
    if(exec) {
        logln("TestSuite MessageFormatNanoTest: ");
    }
    TESTCASE_AUTO_BEGIN;
    TESTCASE_AUTO(testEmpty);
    TESTCASE_AUTO(testBasics);
    TESTCASE_AUTO(testPlurals);
    TESTCASE_AUTO(testDateTime);
    TESTCASE_AUTO(testDateTimeLocale);
    TESTCASE_AUTO_END;
}

void MessageFormatNanoTest::testEmpty() {
    IcuTestErrorCode errorCode(*this, "testEmpty");
    UParseError parseError;
    MessageFormatNano format("", parseError, errorCode);
    UnicodeString result;
    MessageFormatNano::FormatParams params =
            MessageFormatNano::FormatParamsBuilder::withArguments(/*arguments=*/{}, /*count=*/0)
                    .setLocale(Locale::getUS())
                    .build();
    assertEquals("format empty", UnicodeString(), format.format(params, result, errorCode));
}

void MessageFormatNanoTest::testBasics() {
    IcuTestErrorCode errorCode(*this, "testBasics");
    UParseError parseError;
    MessageFormatNano format("{0} says {1}", parseError, errorCode);
    UnicodeString result;
    const UnicodeString argumentNames[] = {UnicodeString(u"0"), UnicodeString(u"1")};
    const Formattable arguments[] = {UnicodeString(u"Alice"), UnicodeString(u"hi")};
    MessageFormatNano::FormatParams params =
            MessageFormatNano::FormatParamsBuilder::withNamedArguments(argumentNames, arguments, UPRV_LENGTHOF(arguments))
                    .setLocale(Locale::getUS())
                    .build();
    assertEquals(
        "format basic",
        UnicodeString("Alice says hi"),
        format.format(params, result, errorCode));
}

void MessageFormatNanoTest::testPlurals() {
    IcuTestErrorCode errorCode(*this, "testPlurals");
    UParseError parseError;
    MessageFormatNano format(
        "You {itemCount, plural, "
        "  =0 {have no messages}"
        "  one {have one message}"
        "  other {have # messages}"
        "}",
        NumberFormatProviderNano::createInstance(errorCode),
        LocalPointer<const DateTimeFormatProvider>(new DateTimeFormatProvider()),
        LocalPointer<const RuleBasedNumberFormatProvider>(new RuleBasedNumberFormatProvider()),
        PluralFormatProviderNano::createInstance(errorCode),
        parseError,
        errorCode);
    UnicodeString result;
    const UnicodeString argumentNames[] = {UnicodeString(u"itemCount")};
    const Formattable argumentZero[] = {0};
    const MessageFormatNano::FormatParams paramsZero =
            MessageFormatNano::FormatParamsBuilder::withNamedArguments(argumentNames, argumentZero, UPRV_LENGTHOF(argumentZero))
                    .setLocale(Locale::getUS())
                    .build();
    assertEquals(
        "format plurals zero",
        UnicodeString("You have no messages"),
        format.format(paramsZero, result, errorCode));
    result.remove();
    const Formattable argumentOne[] = {1};
    const MessageFormatNano::FormatParams paramsOne =
            MessageFormatNano::FormatParamsBuilder::withNamedArguments(argumentNames, argumentOne, UPRV_LENGTHOF(argumentOne))
                    .setLocale(Locale::getUS())
                    .build();
    assertEquals(
        "format plurals one",
        UnicodeString("You have one message"),
        format.format(paramsOne, result, errorCode));
    result.remove();
    const Formattable argumentOther[] = {42};
    const MessageFormatNano::FormatParams paramsOther =
            MessageFormatNano::FormatParamsBuilder::withNamedArguments(argumentNames, argumentOther, UPRV_LENGTHOF(argumentOther))
                    .setLocale(Locale::getUS())
                    .build();
    assertEquals(
        "format plurals other",
        UnicodeString("You have 42 messages"),
        format.format(paramsOther, result, errorCode));
}

void MessageFormatNanoTest::testDateTime() {
    IcuTestErrorCode errorCode(*this, "testDateTime");
    UParseError parseError;
    MessageFormatNano format("Do not open before {0}",
        LocalPointer<const NumberFormatProvider>(new NumberFormatProvider()),
        DateTimeFormatProviderNano::createInstance(errorCode),
        LocalPointer<const RuleBasedNumberFormatProvider>(new RuleBasedNumberFormatProvider()),
        LocalPointer<const PluralFormatProvider>(new PluralFormatProvider()),
        parseError,
        errorCode);
    UnicodeString result;
    const Formattable arguments[] = { Formattable(1572901980000., Formattable::kIsDate) };
    const MessageFormatNano::FormatParams params =
            MessageFormatNano::FormatParamsBuilder::withArguments(arguments, UPRV_LENGTHOF(arguments))
                    .setLocale(Locale::getUS())
                    .setTimeZone(LocalPointer<const TimeZone>(TimeZone::createTimeZone("Europe/Berlin")))
                    .build();
    assertEquals(
        "format time and date",
        UnicodeString("Do not open before 11/4/19, 10:13 PM"),
        format.format(params, result, errorCode));
}

void MessageFormatNanoTest::testDateTimeLocale() {
    IcuTestErrorCode errorCode(*this, "testDateTime");
    UParseError parseError;
    MessageFormatNano format(UnicodeString(u"Nicht vor dem {0} Uhr \u00f6ffnen"),
        LocalPointer<const NumberFormatProvider>(new NumberFormatProvider()),
        DateTimeFormatProviderNano::createInstance(errorCode),
        LocalPointer<const RuleBasedNumberFormatProvider>(new RuleBasedNumberFormatProvider()),
        LocalPointer<const PluralFormatProvider>(new PluralFormatProvider()),
        parseError,
        errorCode);
    UnicodeString result;
    const Formattable arguments[] = { Formattable(1572901980000., Formattable::kIsDate) };
    const MessageFormatNano::FormatParams params =
            MessageFormatNano::FormatParamsBuilder::withArguments(arguments, UPRV_LENGTHOF(arguments))
                    .setLocale(Locale::forLanguageTag("de-DE", errorCode))
                    .setTimeZone(LocalPointer<const TimeZone>(TimeZone::createTimeZone("Europe/Berlin")))
                    .build();
    assertEquals(
        "format time and date with locale",
        UnicodeString(u"Nicht vor dem 04.11.19, 22:13 Uhr \u00f6ffnen"),
        format.format(params, result, errorCode));
}
