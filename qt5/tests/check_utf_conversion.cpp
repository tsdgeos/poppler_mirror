#include <QtCore/QScopedPointer>
#include <QtTest/QtTest>

#include <cstring>
#include "UTF.h"

class TestUTFConversion : public QObject
{
    Q_OBJECT
private slots:
    void testUTF_data();
    void testUTF();
};

static bool compare(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

static bool compare(const uint16_t *a, const uint16_t *b)
{
    while (*a && *b) {
	if (*a++ != *b++)
	    return false;
    }
    return *a == *b;
}

void TestUTFConversion::testUTF_data()
{
   QTest::addColumn<QString>("s");

   QTest::newRow("<empty>") << QString::fromUtf8("");
   QTest::newRow("a") << QString::fromUtf8("a");
   QTest::newRow("abc") << QString::fromUtf8("abc");
   QTest::newRow("Latin") << QString::fromUtf8("Vitrum edere possum; mihi non nocet");
   QTest::newRow("Greek") << QString::fromUtf8("Μπορώ να φάω σπασμένα γυαλιά χωρίς να πάθω τίποτα");
   QTest::newRow("Icelandic") << QString::fromUtf8("Ég get etið gler án þess að meiða mig");
   QTest::newRow("Russian") << QString::fromUtf8("Я могу есть стекло, оно мне не вредит.");
   QTest::newRow("Sanskrit") << QString::fromUtf8("काचं शक्नोम्यत्तुम् । नोपहिनस्ति माम् ॥");
   QTest::newRow("Arabic") << QString::fromUtf8("أنا قادر على أكل الزجاج و هذا لا يؤلمني");
   QTest::newRow("Chinese") << QString::fromUtf8("我能吞下玻璃而不伤身体。");
   QTest::newRow("Thai") << QString::fromUtf8("ฉันกินกระจกได้ แต่มันไม่ทำให้ฉันเจ็บ");
   QTest::newRow("non BMP") << QString::fromUtf8("𝓹𝓸𝓹𝓹𝓵𝓮𝓻");
 }

void TestUTFConversion::testUTF()
{
    char utf8Buf[100];
    char *utf8String;
    uint16_t utf16Buf[100];
    uint16_t *utf16String;
    int len;

    QFETCH(QString, s);
    char *str = strdup(s.toUtf8().constData());

    // UTF-8 to UTF-16

    // QString size() returns number of code units, not code points
    QCOMPARE( utf8CountUtf16CodeUnits(str), s.size() );

    len = utf8ToUtf16(str, utf16Buf);
    QVERIFY( compare(utf16Buf, s.utf16()) );
    QCOMPARE( len, s.size() );

    utf16String = utf8ToUtf16(str);
    QVERIFY( compare(utf16String, s.utf16()) );
    free (utf16String);

    // UTF-16 to UTF-8

    QCOMPARE( utf16CountUtf8Bytes(s.utf16()), (int)strlen(str) );

    len = utf16ToUtf8(s.utf16(), utf8Buf);
    QVERIFY( compare(utf8Buf, str) );
    QCOMPARE( len, (int)strlen(str) );

    utf8String = utf16ToUtf8(s.utf16() );
    QVERIFY( compare(utf8String, str) );
    free (utf8String);

    free(str);
}

QTEST_GUILESS_MAIN(TestUTFConversion)
#include "check_utf_conversion.moc"
