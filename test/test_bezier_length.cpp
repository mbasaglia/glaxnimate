#include <QtTest/QtTest>

#include <vector>
#include "math/bezier/bezier_length.hpp"

using namespace glaxnimate::math::bezier;

#define CLOSE_ENOUGH(a, b, ret) \
do {\
    if (!close_enough(a, b, 0.5, #a, #b, __FILE__, __LINE__) && ret)\
        return;\
} while (false)


class TestCase: public QObject
{
    Q_OBJECT

private:
    bool close_enough(qreal actual, qreal expected, qreal tolerance, const char* actual_name, const char* expected_name, const char *file, int line)
    {
        auto delta = qAbs(actual - expected);
        if ( delta > tolerance )
        {
            int max = qMax(strlen(actual_name), strlen(expected_name));
            std::string msg = "Compared values are too different\n";
            msg += "    Actual   ";
            msg += actual_name;
            msg += std::string(max - strlen(actual_name), ' ');
            msg += ": ";
            msg += std::to_string(actual);
            msg += "\n    Expected ";
            msg += expected_name;
            msg += std::string(max - strlen(expected_name), ' ');
            msg += ": ";
            msg += std::to_string(expected);
            msg += "\n    Delta    ";
            msg += std::string(max, ' ');
            msg += ": ";
            msg += std::to_string(delta);
            QTest::qFail(msg.c_str(), file, line);
            return false;
        }

        return true;
    }
private slots:
    void test_basics()
    {
        for ( qreal off = 0; off <= 100; off += 50 )
        {
            for ( qreal len = 50; len <= 150; len += 50 )
            {
                Solver seg{{off, 0}, {off + len/3., 0}, {off + len*2./3., 0}, {off + len, 0}};
                LengthData data(seg, 10);
                QCOMPARE(data.length(), len);
            }
        }
    }

    void test_sloped()
    {
        Solver seg{{0, 0}, {0, 0}, {30, 40}, {30, 40}};
        LengthData data(seg, 10);
        QCOMPARE(data.length(), 50);
    }

    void test_segment_line_uniform()
    {
        Solver seg{{0, 0}, {33.333, 0}, {66.667, 0}, {100, 0}};
        LengthData data(seg, 10);
        QCOMPARE(data.length(), 100);

        for ( int i = 0; i <= 100; i += 10 )
        {
            CLOSE_ENOUGH(seg.solve(data.at_length(i).ratio).x(), i, false);
        }
    }

    void test_segment_line_bunched_up()
    {
        Solver seg{{0, 0}, {0, 0}, {100, 0}, {100, 0}};
        LengthData data(seg, 10);
        QCOMPARE(data.length(), 100);

        for ( int i = 0; i <= 100; i += 10 )
        {
            CLOSE_ENOUGH(seg.solve(data.at_length(i).ratio).x(), i, false);
        }
    }

    void test_bez_line_uniform()
    {
        Bezier bez;
        bez.add_point({0, 0}, {0, 0}, {50./3., 0});
        bez.add_point({50, 0}, {-50./3., 0}, {150./3., 0});
        bez.add_point({200, 0}, {-150./3., 0});

        LengthData data(bez, 10);
        QCOMPARE(data.length(), 200);

        for ( int i = 0; i <= 200; i += 10 )
        {
            auto split = data.at_length(i);
            Solver seg(bez.segment(split.index));
            CLOSE_ENOUGH(seg.solve(split.descend().ratio).x(), i, false);
        }
    }

    void test_bez_line_bunched_up()
    {
        Bezier bez;
        bez.add_point({0, 0});
        bez.add_point({50, 0});
        bez.add_point({200, 0});

        LengthData data(bez, 16);
        QCOMPARE(data.length(), 200);

        for ( int i = 0; i <= 200; i += 10 )
        {
            auto split = data.at_length(i);
            Solver seg(bez.segment(split.index));
            CLOSE_ENOUGH(seg.solve(split.descend().ratio).x(), i, false);
        }
    }

    void test_multibez_line_bunched_up()
    {
        MultiBezier mbez;
        mbez.move_to({0, 0});
        mbez.line_to({50, 0});
        mbez.line_to({200, 0});
        mbez.move_to({0, 100});
        mbez.line_to({50, 100});
        mbez.line_to({200, 100});

        LengthData data(mbez, 16);
        QCOMPARE(data.length(), 400);

        for ( int i = 0; i < 200; i += 10 )
        {
            auto split = data.at_length(i);
            QCOMPARE(split.index, 0);
            QCOMPARE(split.length, i);
            auto child_split = split.descend();
            if ( i < 50 )
                QCOMPARE(child_split.index, 0);
            else
                QCOMPARE(child_split.index, 1);
            Solver seg(mbez.beziers()[split.index].segment(child_split.index));
            CLOSE_ENOUGH(seg.solve(child_split.descend().ratio).x(), i, false);
            CLOSE_ENOUGH(seg.solve(child_split.descend().ratio).y(), 0, false);
        }

        for ( int i = 200; i <= 400; i += 10 )
        {
            auto split = data.at_length(i);
            QCOMPARE(split.index, 1);
            QCOMPARE(split.length, i - 200);
            auto child_split = split.descend();
            if ( i < 250 )
                QCOMPARE(child_split.index, 0);
            else
                QCOMPARE(child_split.index, 1);
            Solver seg(mbez.beziers()[split.index].segment(child_split.index));
            CLOSE_ENOUGH(seg.solve(child_split.descend().ratio).x(), i - 200, false);
            CLOSE_ENOUGH(seg.solve(child_split.descend().ratio).y(), 100, false);
        }
    }
};

QTEST_GUILESS_MAIN(TestCase)
#include "test_bezier_length.moc"
