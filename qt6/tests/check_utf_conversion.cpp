#include <QtCore/QScopedPointer>
#include <QtTest/QtTest>

#include <poppler-private.h>

#include <cstring>

#include "GlobalParams.h"
#include "UnicodeTypeTable.h"
#include "UTF.h"

class TestUTFConversion : public QObject
{
    Q_OBJECT
public:
    explicit TestUTFConversion(QObject *parent = nullptr) : QObject(parent) { }
private slots:
    void testUTF_data();
    void testUTF();
    void testUnicodeToAscii7();
    void testUnicodeLittleEndian();
};

static bool compare(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

static bool compare(const uint16_t *a, const uint16_t *b)
{
    while (*a && *b) {
        if (*a++ != *b++) {
            return false;
        }
    }
    return *a == *b;
}

static bool compare(const Unicode *a, const char *b, int len)
{
    for (int i = 0; i < len; i++) {
        if (a[i] != (Unicode)b[i]) {
            return false;
        }
    }

    return true;
}

static bool compare(const Unicode *a, const uint16_t *b, int len)
{
    for (int i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}
void TestUTFConversion::testUTF_data()
{
    QTest::addColumn<QString>("s");

    QTest::newRow("<empty>") << QString(QLatin1String(""));
    QTest::newRow("a") << QStringLiteral("a");
    QTest::newRow("abc") << QStringLiteral("abc");
    QTest::newRow("Latin") << QStringLiteral("Vitrum edere possum; mihi non nocet");
    QTest::newRow("Greek") << QStringLiteral("Μπορώ να φάω σπασμένα γυαλιά χωρίς να πάθω τίποτα");
    QTest::newRow("Icelandic") << QStringLiteral("Ég get etið gler án þess að meiða mig");
    QTest::newRow("Russian") << QStringLiteral("Я могу есть стекло, оно мне не вредит.");
    QTest::newRow("Sanskrit") << QStringLiteral("काचं शक्नोम्यत्तुम् । नोपहिनस्ति माम् ॥");
    QTest::newRow("Arabic") << QStringLiteral("أنا قادر على أكل الزجاج و هذا لا يؤلمني");
    QTest::newRow("Chinese") << QStringLiteral("我能吞下玻璃而不伤身体。");
    QTest::newRow("Thai") << QStringLiteral("ฉันกินกระจกได้ แต่มันไม่ทำให้ฉันเจ็บ");
    QTest::newRow("non BMP") << QStringLiteral("𝓹𝓸𝓹𝓹𝓵𝓮𝓻");
}

void TestUTFConversion::testUTF()
{
    char utf8Buf[1000];
    char *utf8String;
    uint16_t utf16Buf[1000];
    uint16_t *utf16String;
    int len;

    QFETCH(QString, s);
    char *str = strdup(s.toUtf8().constData());

    // UTF-8 to UTF-16

    len = utf8CountUtf16CodeUnits(str);
    QCOMPARE(len, s.size()); // QString size() returns number of code units, not code points
    Q_ASSERT(len < (int)sizeof(utf16Buf)); // if this fails, make utf16Buf larger

    len = utf8ToUtf16(str, utf16Buf, sizeof(utf16Buf), INT_MAX);
    QVERIFY(compare(utf16Buf, s.utf16()));
    QCOMPARE(len, s.size());

    utf16String = utf8ToUtf16(str);
    QVERIFY(compare(utf16String, s.utf16()));
    free(utf16String);

    std::string sUtf8(str);
    std::string gsUtf16_a(utf8ToUtf16WithBom(sUtf8));
    std::unique_ptr<GooString> gsUtf16_b(Poppler::QStringToUnicodeGooString(s));
    QCOMPARE(gsUtf16_b->cmp(gsUtf16_a), 0);

    // UTF-16 to UTF-8

    len = utf16CountUtf8Bytes(s.utf16());
    QCOMPARE(len, (int)strlen(str));
    Q_ASSERT(len < (int)sizeof(utf8Buf)); // if this fails, make utf8Buf larger

    len = utf16ToUtf8(s.utf16(), utf8Buf);
    QVERIFY(compare(utf8Buf, str));
    QCOMPARE(len, (int)strlen(str));

    utf8String = utf16ToUtf8(s.utf16());
    QVERIFY(compare(utf8String, str));
    free(utf8String);

    free(str);
}

void TestUTFConversion::testUnicodeToAscii7()
{
    globalParams = std::make_unique<GlobalParams>();

    // Test string is one 'Registered' and twenty 'Copyright' chars
    // so it's long enough to reproduce the bug given that glibc
    // malloc() always returns 8-byte aligned memory addresses.
    GooString *goo = Poppler::QStringToUnicodeGooString(QString::fromUtf8("®©©©©©©©©©©©©©©©©©©©©")); // clazy:exclude=qstring-allocations

    Unicode *in;
    const int in_len = TextStringToUCS4(goo->toStr(), &in);

    delete goo;

    int in_norm_len;
    int *in_norm_idx;
    Unicode *in_norm = unicodeNormalizeNFKC(in, in_len, &in_norm_len, &in_norm_idx, true);

    free(in);

    Unicode *out;
    int out_len;
    int *out_ascii_idx;

    unicodeToAscii7(in_norm, in_norm_len, &out, &out_len, in_norm_idx, &out_ascii_idx);

    free(in_norm);
    free(in_norm_idx);

    // ascii7 conversion: ® -> (R)   © -> (c)
    const char *expected_ascii = (char *)"(R)(c)(c)(c)(c)(c)(c)(c)(c)(c)(c)(c)(c)(c)(c)(c)(c)(c)(c)(c)(c)";

    QCOMPARE(out_len, (int)strlen(expected_ascii));
    QVERIFY(compare(out, expected_ascii, out_len));

    free(out);
    free(out_ascii_idx);
}

void TestUTFConversion::testUnicodeLittleEndian()
{
    uint16_t UTF16LE_hi[5] { 0xFFFE, 0x4800, 0x4900, 0x2100, 0x1126 }; // UTF16-LE "HI!☑"
    std::string GooUTF16LE(reinterpret_cast<const char *>(UTF16LE_hi), sizeof(UTF16LE_hi));

    uint16_t UTF16BE_hi[5] { 0xFEFF, 0x0048, 0x0049, 0x0021, 0x2611 }; // UTF16-BE "HI!☑"
    std::string GooUTF16BE(reinterpret_cast<const char *>(UTF16BE_hi), sizeof(UTF16BE_hi));

    // Let's assert both GooString's are different
    QVERIFY(GooUTF16LE != GooUTF16BE);

    Unicode *UCS4fromLE, *UCS4fromBE;
    const int len1 = TextStringToUCS4(GooUTF16LE, &UCS4fromLE);
    const int len2 = TextStringToUCS4(GooUTF16BE, &UCS4fromBE);

    // len is 4 because TextStringToUCS4() removes the two leading Byte Order Mark (BOM) code points
    QCOMPARE(len1, len2);
    QCOMPARE(len1, 4);

    // Check that now after conversion, UCS4fromLE and UCS4fromBE are now the same
    for (int i = 0; i < len1; i++) {
        QCOMPARE(UCS4fromLE[i], UCS4fromBE[i]);
    }

    const QString expected = QStringLiteral("HI!☑");

    // Do some final verifications, checking the strings to be "HI!"
    QVERIFY(*UCS4fromLE == *UCS4fromBE);
    QVERIFY(compare(UCS4fromLE, expected.utf16(), len1));
    QVERIFY(compare(UCS4fromBE, expected.utf16(), len1));
}

QTEST_GUILESS_MAIN(TestUTFConversion)
#include "check_utf_conversion.moc"
