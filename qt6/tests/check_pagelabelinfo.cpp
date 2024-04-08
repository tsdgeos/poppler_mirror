#include <QtTest/QTest>

#include <poppler-private.h>

#include "PageLabelInfo_p.h"

#include "config.h"

class TestPageLabelInfo : public QObject
{
    Q_OBJECT
public:
    explicit TestPageLabelInfo(QObject *parent = nullptr) : QObject(parent) { }
private slots:
    void testFromDecimal();
    void testFromDecimalUnicode();
    void testToRoman();
    void testFromRoman();
    void testToLatin();
    void testFromLatin();
};

void TestPageLabelInfo::testFromDecimal()
{
    std::string str { "2342" };
    const auto res = fromDecimal(str, false);
    QCOMPARE(res.first, 2342);
    QCOMPARE(res.second, true);
}

void TestPageLabelInfo::testFromDecimalUnicode()
{
    std::unique_ptr<GooString> str(Poppler::QStringToUnicodeGooString(QString::fromLocal8Bit("2342")));
    const auto res = fromDecimal(str->toStr(), hasUnicodeByteOrderMark(str->toStr()));
    QCOMPARE(res.first, 2342);
    QCOMPARE(res.second, true);
}

void TestPageLabelInfo::testToRoman()
{
    GooString str;
    toRoman(177, &str, false);
    QCOMPARE(str.c_str(), "clxxvii");
}

void TestPageLabelInfo::testFromRoman()
{
    GooString roman("clxxvii");
    QCOMPARE(fromRoman(roman.c_str()), 177);
}

void TestPageLabelInfo::testToLatin()
{
    GooString str;
    toLatin(54, &str, false);
    QCOMPARE(str.c_str(), "bbb");
}

void TestPageLabelInfo::testFromLatin()
{
    GooString latin("ddd");
    QCOMPARE(fromLatin(latin.c_str()), 56);
}

QTEST_GUILESS_MAIN(TestPageLabelInfo)
#include "check_pagelabelinfo.moc"
