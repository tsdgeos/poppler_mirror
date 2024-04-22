/*
 * Copyright (C) 2010, 2011, Pino Toscano <pino@kde.org>
 * Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, <info@kdab.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <QtTest/QTest>

#include <poppler-qt6.h>
#include <poppler-private.h>

#include <GlobalParams.h>
#include "UTF.h"

Q_DECLARE_METATYPE(GooString *)
Q_DECLARE_METATYPE(Unicode *)

class TestStrings : public QObject
{
    Q_OBJECT

public:
    explicit TestStrings(QObject *parent = nullptr) : QObject(parent) { }

private slots:
    void initTestCase();
    void cleanupTestCase();
    void check_unicodeToQString_data();
    void check_unicodeToQString();
    void check_UnicodeParsedString_data();
    void check_UnicodeParsedString();
    void check_QStringToUnicodeGooString_data();
    void check_QStringToUnicodeGooString();
    void check_QStringToGooString_data();
    void check_QStringToGooString();

private:
    GooString *newGooString(const char *s);
    GooString *newGooString(const char *s, int l);

    QVector<GooString *> m_gooStrings;
};

void TestStrings::initTestCase()
{
    qRegisterMetaType<GooString *>("GooString*");
    qRegisterMetaType<Unicode *>("Unicode*");

    globalParams = std::make_unique<GlobalParams>();
}

void TestStrings::cleanupTestCase()
{
    qDeleteAll(m_gooStrings);

    globalParams.reset();
}

void TestStrings::check_unicodeToQString_data()
{
    QTest::addColumn<Unicode *>("data");
    QTest::addColumn<int>("length");
    QTest::addColumn<QString>("result");

    {
        const int l = 1;
        Unicode *u = new Unicode[l];
        u[0] = int('a');
        QTest::newRow("a") << u << l << QStringLiteral("a");
    }
    {
        const int l = 1;
        Unicode *u = new Unicode[l];
        u[0] = 0x0161;
        QTest::newRow("\u0161") << u << l << QStringLiteral("\u0161");
    }
    {
        const int l = 2;
        Unicode *u = new Unicode[l];
        u[0] = int('a');
        u[1] = int('b');
        QTest::newRow("ab") << u << l << QStringLiteral("ab");
    }
    {
        const int l = 2;
        Unicode *u = new Unicode[l];
        u[0] = int('a');
        u[1] = 0x0161;
        QTest::newRow("a\u0161") << u << l << QStringLiteral("a\u0161");
    }
    {
        const int l = 2;
        Unicode *u = new Unicode[l];
        u[0] = 0x5c01;
        u[1] = 0x9762;
        QTest::newRow("\xe5\xb0\x81\xe9\x9d\xa2") << u << l << QStringLiteral("封面");
    }
    {
        const int l = 3;
        Unicode *u = new Unicode[l];
        u[0] = 0x5c01;
        u[1] = 0x9762;
        u[2] = 0x0;
        QTest::newRow("\xe5\xb0\x81\xe9\x9d\xa2 + 0") << u << l << QStringLiteral("封面");
    }
    {
        const int l = 4;
        Unicode *u = new Unicode[l];
        u[0] = 0x5c01;
        u[1] = 0x9762;
        u[2] = 0x0;
        u[3] = 0x0;
        QTest::newRow("\xe5\xb0\x81\xe9\x9d\xa2 + two 0") << u << l << QStringLiteral("封面");
    }
}

void TestStrings::check_unicodeToQString()
{
    QFETCH(Unicode *, data);
    QFETCH(int, length);
    QFETCH(QString, result);

    QCOMPARE(Poppler::unicodeToQString(data, length), result);

    delete[] data;
}

void TestStrings::check_UnicodeParsedString_data()
{
    QTest::addColumn<GooString *>("string");
    QTest::addColumn<QString>("result");

    // non-unicode strings
    QTest::newRow("<empty>") << newGooString("") << QString();
    QTest::newRow("a") << newGooString("a") << QStringLiteral("a");
    QTest::newRow("ab") << newGooString("ab") << QStringLiteral("ab");
    QTest::newRow("~") << newGooString("~") << QStringLiteral("~");
    QTest::newRow("test string") << newGooString("test string") << QStringLiteral("test string");

    // unicode strings
    QTest::newRow("<unicode marks>") << newGooString("\xFE\xFF") << QString();
    QTest::newRow("U a") << newGooString("\xFE\xFF\0a", 4) << QStringLiteral("a");
    QTest::newRow("U ~") << newGooString("\xFE\xFF\0~", 4) << QStringLiteral("~");
    QTest::newRow("U aa") << newGooString("\xFE\xFF\0a\0a", 6) << QStringLiteral("aa");
    QTest::newRow("U \xC3\x9F") << newGooString("\xFE\xFF\0\xDF", 4) << QStringLiteral("ß");
    QTest::newRow("U \xC3\x9F\x61") << newGooString("\xFE\xFF\0\xDF\0\x61", 6) << QStringLiteral("ßa");
    QTest::newRow("U \xC5\xA1") << newGooString("\xFE\xFF\x01\x61", 4) << QStringLiteral("š");
    QTest::newRow("U \xC5\xA1\x61") << newGooString("\xFE\xFF\x01\x61\0\x61", 6) << QStringLiteral("ša");
    QTest::newRow("test string") << newGooString("\xFE\xFF\0t\0e\0s\0t\0 \0s\0t\0r\0i\0n\0g", 24) << QStringLiteral("test string");
    QTest::newRow("UTF16-LE") << newGooString("\xFF\xFE\xDA\x00\x6E\x00\xEE\x00\x63\x00\xF6\x00\x64\x00\xE9\x00\x51\x75", 18) << QStringLiteral("Únîcödé畑");
}

void TestStrings::check_UnicodeParsedString()
{
    QFETCH(GooString *, string);
    QFETCH(QString, result);

    QCOMPARE(Poppler::UnicodeParsedString(string), result);
}

void TestStrings::check_QStringToUnicodeGooString_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<QByteArray>("result");

    QTest::newRow("<null>") << QString() << QByteArray("");
    QTest::newRow("<empty>") << QString(QLatin1String("")) << QByteArray("");
    QTest::newRow("a") << QStringLiteral("a") << QByteArray("\0a", 2);
    QTest::newRow("ab") << QStringLiteral("ab") << QByteArray("\0a\0b", 4);
    QTest::newRow("test string") << QStringLiteral("test string") << QByteArray("\0t\0e\0s\0t\0 \0s\0t\0r\0i\0n\0g", 22);
    QTest::newRow("\xC3\x9F") << QStringLiteral("ß") << QByteArray("\0\xDF", 2);
    QTest::newRow("\xC3\x9F\x61") << QStringLiteral("ßa") << QByteArray("\0\xDF\0\x61", 4);
}

void TestStrings::check_QStringToUnicodeGooString()
{
    QFETCH(QString, string);
    QFETCH(QByteArray, result);

    GooString *goo = Poppler::QStringToUnicodeGooString(string);
    if (string.isEmpty()) {
        QVERIFY(goo->toStr().empty());
        QCOMPARE(goo->getLength(), 0);
    } else {
        QVERIFY(hasUnicodeByteOrderMark(goo->toStr()));
        QCOMPARE(goo->getLength(), string.length() * 2 + 2);
        QCOMPARE(result, QByteArray::fromRawData(goo->c_str() + 2, goo->getLength() - 2));
    }

    delete goo;
}

void TestStrings::check_QStringToGooString_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<GooString *>("result");

    QTest::newRow("<null>") << QString() << newGooString("");
    QTest::newRow("<empty>") << QString(QLatin1String("")) << newGooString("");
    QTest::newRow("a") << QStringLiteral("a") << newGooString("a");
    QTest::newRow("ab") << QStringLiteral("ab") << newGooString("ab");
}

void TestStrings::check_QStringToGooString()
{
    QFETCH(QString, string);
    QFETCH(GooString *, result);

    GooString *goo = Poppler::QStringToGooString(string);
    QCOMPARE(goo->c_str(), result->c_str());

    delete goo;
}

GooString *TestStrings::newGooString(const char *s)
{
    GooString *goo = new GooString(s);
    m_gooStrings.append(goo);
    return goo;
}

GooString *TestStrings::newGooString(const char *s, int l)
{
    GooString *goo = new GooString(s, l);
    m_gooStrings.append(goo);
    return goo;
}

QTEST_GUILESS_MAIN(TestStrings)

#include "check_strings.moc"
