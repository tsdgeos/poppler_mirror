#include <QtCore/QScopedPointer>
#include <QtTest/QTest>

#include "poppler/Object.h"

class TestObject : public QObject
{
    Q_OBJECT
public:
    explicit TestObject(QObject *parent = nullptr) : QObject(parent) { }
private Q_SLOTS:
    static void benchDefaultConstructor();
    static void benchMoveConstructor();
    static void benchSetToNull();
};

void TestObject::benchDefaultConstructor()
{
    QBENCHMARK {
        Object obj;
    }
}

void TestObject::benchMoveConstructor()
{
    QBENCHMARK {
        Object src;
        Object dst { std::move(src) };
    }
}

void TestObject::benchSetToNull()
{
    Object obj;
    QBENCHMARK {
        obj.setToNull();
    }
}

QTEST_GUILESS_MAIN(TestObject)
#include "check_object.moc"
