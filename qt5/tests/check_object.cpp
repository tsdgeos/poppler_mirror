#include <QtCore/QScopedPointer>
#include <QtTest/QtTest>

#include "poppler/Object.h"

class TestObject : public QObject
{
    Q_OBJECT
private slots:
    void benchDefaultConstructor();
    void benchMoveConstructor();
    void benchSetToNull();
};

void TestObject::benchDefaultConstructor() {
  QBENCHMARK {
    Object obj;
  }
}

void TestObject::benchMoveConstructor() {
  Object src;
  QBENCHMARK {
    Object dst{std::move(src)};
  }
}

void TestObject::benchSetToNull() {
  Object obj;
  QBENCHMARK {
    obj.setToNull();
  }
}

QTEST_GUILESS_MAIN(TestObject)
#include "check_object.moc"
