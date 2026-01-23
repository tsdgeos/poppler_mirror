#include <QtTest/QTest>

#include "Object.h"
#include "Lexer.h"

class TestLexer : public QObject
{
    Q_OBJECT
public:
    explicit TestLexer(QObject *parent = nullptr) : QObject(parent) { }
private Q_SLOTS:
    static void testNumbers();
};

void TestLexer::testNumbers()
{
    char data[] = "0 1 -1 2147483647 -2147483647 2147483648 -2147483648 4294967297 -2147483649 0.1 1.1 -1.1 2147483647.1 -2147483647.1 2147483648.1 -2147483648.1 4294967297.1 -2147483649.1 9223372036854775807 18446744073709551615";
    auto stream = std::make_unique<MemStream>(data, 0, strlen(data), Object::null());
    auto *lexer = new Lexer(nullptr, std::move(stream));
    QVERIFY(lexer);

    Object obj;

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objInt);
    QCOMPARE(obj.getInt(), 0);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objInt);
    QCOMPARE(obj.getInt(), 1);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objInt);
    QCOMPARE(obj.getInt(), -1);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objInt);
    QCOMPARE(obj.getInt(), 2147483647);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objInt);
    QCOMPARE(obj.getInt(), -2147483647);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objInt64);
    QCOMPARE(obj.getInt64(), 2147483648LL);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objInt);
    QCOMPARE(obj.getInt(), -2147483647 - 1);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objInt64);
    QCOMPARE(obj.getInt64(), 4294967297LL);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objInt64);
    QCOMPARE(obj.getInt64(), -2147483649LL);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), 0.1);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), 1.1);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), -1.1);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), 2147483647.1);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), -2147483647.1);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), 2147483648.1);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), -2147483648.1);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), 4294967297.1);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), -2147483649.1);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objInt64);
    QCOMPARE(obj.getInt64(), 9223372036854775807LL);

    obj = lexer->getObj();
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), 18446744073709551616.);

    delete lexer;
}

QTEST_GUILESS_MAIN(TestLexer)
#include "check_lexer.moc"
