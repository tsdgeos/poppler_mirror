#include <QtTest/QtTest>

#include "GooString.h"

class TestGooString : public QObject
{
    Q_OBJECT
private slots:
    void testInsert();
};

void TestGooString::testInsert()
{
    GooString goo;
    goo.insert(0, ".");
    goo.insert(0, "This is a very long long test string");
    QCOMPARE(goo.getCString(), "This is a very long long test string.");
}

QTEST_MAIN(TestGooString)
#include "check_goostring.moc"

