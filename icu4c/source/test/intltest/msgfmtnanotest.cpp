// © 2020 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License

#include "unicode/unistr.h"
#include "unicode/utypes.h"
#include "unicode/msgfmtnano.h"
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
    TESTCASE_AUTO_END;
}

void MessageFormatNanoTest::testEmpty() {
    IcuTestErrorCode errorCode(*this, "testEmpty");
    UParseError parseError;
    MessageFormatNano format("", Locale::getUS(), parseError, errorCode);
    UnicodeString result;
    assertEquals("format empty", UnicodeString(), format.format(/*argumentNames=*/{}, /*arguments=*/{}, /*count=*/0, result, errorCode));
}

void MessageFormatNanoTest::testBasics() {
    IcuTestErrorCode errorCode(*this, "testBasics");
    UParseError parseError;
    MessageFormatNano format("{0} says {1}", Locale::getUS(), parseError, errorCode);
    UnicodeString result;
    const UnicodeString argumentNames[] = {UnicodeString(u"0"), UnicodeString(u"1")};
    const Formattable arguments[] = {UnicodeString(u"Alice"), UnicodeString(u"hi")};
    assertEquals(
        "format basic",
        UnicodeString("Alice says hi"),
        format.format(argumentNames, arguments, UPRV_LENGTHOF(argumentNames), result, errorCode));
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
        Locale::getUS(),
        NumberFormatProviderNano::createInstance(errorCode),
        LocalPointer<const DateTimeFormatProvider>(new DateTimeFormatProvider()),
        LocalPointer<const RuleBasedNumberFormatProvider>(new RuleBasedNumberFormatProvider()),
        PluralFormatProviderNano::createInstance(errorCode),
        parseError,
        errorCode);
    UnicodeString result;
    const UnicodeString argumentNames[] = {UnicodeString(u"itemCount")};
    const Formattable argumentZero[] = {0};
    assertEquals(
        "format plurals zero",
        UnicodeString("You have no messages"),
        format.format(argumentNames, argumentZero, UPRV_LENGTHOF(argumentNames), result, errorCode));
    result.remove();
    const Formattable argumentOne[] = {1};
    assertEquals(
        "format plurals one",
        UnicodeString("You have one message"),
        format.format(argumentNames, argumentOne, UPRV_LENGTHOF(argumentNames), result, errorCode));
    result.remove();
    const Formattable argumentOther[] = {42};
    assertEquals(
        "format plurals other",
        UnicodeString("You have 42 messages"),
        format.format(argumentNames, argumentOther, UPRV_LENGTHOF(argumentNames), result, errorCode));
}
