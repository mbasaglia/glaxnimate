/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QtTest/QtTest>

#include "model/animation/keyframe_transition.hpp"

using namespace glaxnimate;

class TestCase: public QObject
{
    Q_OBJECT

private slots:

    void test_set_handle()
    {
        model::KeyframeTransition kft;
        kft.set_before(QPointF(.46, .94));
        kft.set_after(QPointF(.89, .34));
        QCOMPARE(kft.bezier().points()[0], QPointF(0, 0));
        QCOMPARE(kft.bezier().points()[1], QPointF(.46, .94));
        QCOMPARE(kft.bezier().points()[2], QPointF(.89, .34));
        QCOMPARE(kft.bezier().points()[3], QPointF(1, 1));

//         double step = 0.1;
//         int count = 10;
//         for ( int i = 0; i <= count; i++ )
//         {
//             double x = i * step;
//             qDebug() << i * 10 << kft.lerp_factor(x);
//         }
// QDEBUG : TestCase::test_lerp_factor() 0 0
// QDEBUG : TestCase::test_lerp_factor() 10 0.182092
// QDEBUG : TestCase::test_lerp_factor() 20 0.323608
// QDEBUG : TestCase::test_lerp_factor() 30 0.430119
// QDEBUG : TestCase::test_lerp_factor() 40 0.507039
// QDEBUG : TestCase::test_lerp_factor() 50 0.560079
// QDEBUG : TestCase::test_lerp_factor() 60 0.595965
// QDEBUG : TestCase::test_lerp_factor() 70 0.62387
// QDEBUG : TestCase::test_lerp_factor() 80 0.659026
// QDEBUG : TestCase::test_lerp_factor() 90 0.735291


    }

    void test_lerp_factor_linear()
    {
        model::KeyframeTransition kft;

        QCOMPARE(kft.lerp_factor(0), 0);
        QCOMPARE(kft.lerp_factor(0.1), 0.1);
        QCOMPARE(kft.lerp_factor(0.2), 0.2);
        QCOMPARE(kft.lerp_factor(0.3), 0.3);
        QCOMPARE(kft.lerp_factor(0.4), 0.4);
        QCOMPARE(kft.lerp_factor(0.5), 0.5);
        QCOMPARE(kft.lerp_factor(0.6), 0.6);
        QCOMPARE(kft.lerp_factor(0.7), 0.7);
        QCOMPARE(kft.lerp_factor(0.8), 0.8);
        QCOMPARE(kft.lerp_factor(0.9), 0.9);
        QCOMPARE(kft.lerp_factor(1), 1);
    }

    void benchmark_lerp_factor()
    {
        model::KeyframeTransition kft;
        kft.set_before(QPointF(.46, .94));
        kft.set_after(QPointF(.89, .34));
        QCOMPARE(kft.bezier().points()[0], QPointF(0, 0));
        QCOMPARE(kft.bezier().points()[1], QPointF(.46, .94));
        QCOMPARE(kft.bezier().points()[2], QPointF(.89, .34));
        QCOMPARE(kft.bezier().points()[3], QPointF(1, 1));

        double step = 0.1;
        int count = 10;
        QBENCHMARK{
            for ( int i = 0; i <= count; i++ )
                kft.lerp_factor(i * step);
        }

    }

    void test_lerp_factor()
    {
        model::KeyframeTransition kft;

        kft.set_before(QPointF(.46, .94));
        kft.set_after(QPointF(.89, .34));
        QCOMPARE(qRound(kft.lerp_factor(0.1)*100), 18);
    }

    void test_lerp_factor_cache_flush()
    {
        model::KeyframeTransition kft;
        QCOMPARE(kft.lerp_factor(0.1), 0.1);

        kft.set_before(QPointF(.46, .94));
        kft.set_after(QPointF(.89, .34));
        QCOMPARE(qRound(kft.lerp_factor(0.1)*100), 18);
    }

    void test_split_hold()
    {
        model::KeyframeTransition kft({0, 0}, {1, 1}, true);
        auto split = kft.split(0.2);
        QCOMPARE(split.first.hold(), true);
        QCOMPARE(split.second.hold(), true);
    }

    void test_split_linear()
    {
        model::KeyframeTransition kft({1./3., 1./3.}, {2./3., 2./3.}, false);
        auto split = kft.split(0.2);

        QCOMPARE(split.first.hold(), false);
        QCOMPARE(split.first.before().x(), 1./3.);
        QCOMPARE(split.first.before().y(), 1./3.);
        QCOMPARE(split.first.after().x(), 2./3.);
        QCOMPARE(split.first.after().y(), 2./3.);

        QCOMPARE(split.second.hold(), false);
        QCOMPARE(split.second.before().x(), 1./3.);
        QCOMPARE(split.second.before().y(), 1./3.);
        QCOMPARE(split.second.after().x(), 2./3.);
        QCOMPARE(split.second.after().y(), 2./3.);
    }

    void test_split_ease()
    {
        model::KeyframeTransition kft({1./3., 0}, {2./3., 1}, false);
        auto split = kft.split(0.5);

        QCOMPARE(split.first.hold(), false);
        QCOMPARE(split.first.before().x(), 1./3.);
        QCOMPARE(split.first.before().y(), 0);
        QCOMPARE(split.first.after().x(), 2./3.);
        QCOMPARE(split.first.after().y(), 0.5);

        QCOMPARE(split.second.hold(), false);
        QCOMPARE(split.second.before().x(), 1./3.);
        QCOMPARE(split.second.before().y(), 0.5);
        QCOMPARE(split.second.after().x(), 2./3.);
        QCOMPARE(split.second.after().y(), 1.);
    }

    void test_split_left()
    {
        model::KeyframeTransition kft({1, -1}, {0, 2});
        float t = 0.146446609407;
        auto split = kft.split_t(t).first;

        QCOMPARE(split.hold(), false);
        QCOMPARE(float(split.before().x()), 0.453082f);
        QCOMPARE(float(split.before().y()), 0.707107f);
        QCOMPARE(float(split.after().x()), 0.773459f);
        QCOMPARE(float(split.after().y()), 1.f);
    }

    void test_split_right()
    {
        model::KeyframeTransition kft({1, -1}, {0, 2});
        float t = 0.146446609407;
        auto split = kft.split_t(1-t).second;

        QCOMPARE(split.hold(), false);
        QCOMPARE(float(split.before().x()), 1-0.773459f);
        QCOMPARE(float(split.before().y()), 0.f);
        QCOMPARE(float(split.after().x()), 1-0.453082f);
        QCOMPARE(float(split.after().y()), 1-0.707107f);
    }
};


QTEST_GUILESS_MAIN(TestCase)
#include "test_keyframe_transition.moc"
