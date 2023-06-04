//========================================================================
//
// check_cidfontswidthsbuilder.cpp
//
// This file is licensed under the GPLv2 or later
//
// Copyright 2023 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//========================================================================

#include "CIDFontsWidthsBuilder.h"

#include <QtTest/QtTest>

class TestCIDFontsWidthsBuilder : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
private Q_SLOTS:
    void testEmpty();
    void testSingle();
    void testSimpleSequence();
};

void TestCIDFontsWidthsBuilder::testEmpty()
{
    CIDFontsWidthsBuilder b;
    auto segments = b.takeSegments();
    QCOMPARE(segments.size(), 0);
}

static bool compare(const CIDFontsWidthsBuilder::Segment &segment1, const CIDFontsWidthsBuilder::Segment &segment2)
{
    return std::visit(
            [](const auto &s1, const auto &s2) {
                using T1 = std::decay_t<decltype(s1)>;
                using T2 = std::decay_t<decltype(s2)>;
                if constexpr (!std::is_same_v<T1, T2>) {
                    return false;
                } else if constexpr (std::is_same_v<T1, CIDFontsWidthsBuilder::ListSegment>) {
                    return s1.first == s2.first && s1.widths == s2.widths;
                } else if constexpr (std::is_same_v<T1, CIDFontsWidthsBuilder::RangeSegment>) {
                    return s1.first == s2.first && s1.last == s2.last && s1.width == s2.width;
                } else {
                    return false;
                }
            },
            segment1, segment2);
}

void TestCIDFontsWidthsBuilder::testSingle()
{
    CIDFontsWidthsBuilder b;
    b.addWidth(0, 10);
    auto segments = b.takeSegments();
    QCOMPARE(segments.size(), 1);
    auto segment0 = CIDFontsWidthsBuilder::ListSegment { 0, { 10 } };
    QVERIFY(compare(segments[0], segment0));
}

void TestCIDFontsWidthsBuilder::testSimpleSequence()
{
    CIDFontsWidthsBuilder b;
    for (int i = 0; i < 2; i++) { // repeat to verify that takeSegments resets
        b.addWidth(0, 10);
        b.addWidth(1, 10);
        b.addWidth(2, 10);
        b.addWidth(3, 10);
        b.addWidth(4, 10);
        b.addWidth(5, 20);
        b.addWidth(6, 21);
        b.addWidth(7, 21);
        b.addWidth(8, 20);
        b.addWidth(9, 10);
        b.addWidth(10, 10);
        b.addWidth(11, 10);
        b.addWidth(12, 10);
        b.addWidth(13, 10);
        b.addWidth(14, 20);
        b.addWidth(15, 21);
        b.addWidth(16, 21);
        b.addWidth(17, 20);
        b.addWidth(19, 20);
        auto segments = b.takeSegments();
        QCOMPARE(segments.size(), 5);
        auto segment0 = CIDFontsWidthsBuilder::RangeSegment { 0, 4, 10 };
        QVERIFY(compare(segments[0], segment0));
        auto segment1 = CIDFontsWidthsBuilder::ListSegment { 5, { 20, 21, 21, 20 } };
        QVERIFY(compare(segments[1], segment1));
        auto segment2 = CIDFontsWidthsBuilder::RangeSegment { 9, 13, 10 };
        QVERIFY(compare(segments[2], segment2));
        auto segment3 = CIDFontsWidthsBuilder::ListSegment { 14, { 20, 21, 21, 20 } };
        QVERIFY(compare(segments[3], segment3));
        auto segment4 = CIDFontsWidthsBuilder::ListSegment { 19, { 20 } };
        QVERIFY(compare(segments[4], segment4));
    }
}

QTEST_GUILESS_MAIN(TestCIDFontsWidthsBuilder);
#include "check_cidfontswidthsbuilder.moc"
