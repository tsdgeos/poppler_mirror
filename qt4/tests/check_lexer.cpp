#include <QtTest/QtTest>

#include "Object.h"
#include "Lexer.h"

class TestLexer : public QObject
{
    Q_OBJECT
private slots:
    void testNumbers();
};

void TestLexer::testNumbers()
{
    char *data = "0 1 -1 2147483647 -2147483647 2147483648 -2147483648 4294967297 -2147483649 0.1 1.1 -1.1 2147483647.1 -2147483647.1 2147483648.1 -2147483648.1 4294967297.1 -2147483649.1";
    Object dummy;
    MemStream *stream = new MemStream(data, 0, strlen(data), &dummy);
    Lexer *lexer = new Lexer(NULL, stream);
    QVERIFY( lexer );
    
    Object obj;
    
    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objInt);
    QCOMPARE(obj.getInt(), 0);
    obj.free();
    
    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objInt);
    QCOMPARE(obj.getInt(), 1);
    obj.free();
    
    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objInt);
    QCOMPARE(obj.getInt(), -1);
    obj.free();
    
    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objInt);
    QCOMPARE(obj.getInt(), 2147483647);
    obj.free();
    
    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objInt);
    QCOMPARE(obj.getInt(), -2147483647);
    obj.free();
    
    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objUint);
    QCOMPARE(obj.getUint(), 2147483648u);
    obj.free();
      
    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objInt);
    QCOMPARE(obj.getInt(), -2147483647-1);
    obj.free();
    
    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), 4294967297.);
    obj.free();
    
    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), -2147483649.);
    obj.free();

    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), 0.1);
    obj.free();
    
    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), 1.1);
    obj.free();
    
    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), -1.1);
    obj.free();
    
    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), 2147483647.1);
    obj.free();
    
    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), -2147483647.1);
    obj.free();
    
    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), 2147483648.1);
    obj.free();
      
    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), -2147483648.1);
    obj.free();
    
    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), 4294967297.1);
    obj.free();
    
    lexer->getObj(&obj);
    QCOMPARE(obj.getType(), objReal);
    QCOMPARE(obj.getReal(), -2147483649.1);
    obj.free();

    delete lexer;
}

QTEST_MAIN(TestLexer)
#include "check_lexer.moc"

