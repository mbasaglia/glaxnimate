#include <QtTest/QtTest>

#include <vector>
#include "math/bezier_solver.hpp"

namespace QTest {
    template<>
    char* toString(const math::Vec2& v)
    {
        return QTest::toString(
            "Vec2(" +
            QByteArray::number(v.x()) + ", " +
            QByteArray::number(v.y()) + ')'
        );
    }
}

class TestCase: public QObject
{
    Q_OBJECT

private slots:
    void test_order()
    {
        math::BezierSolver<math::Vec2> bs{
            math::Vec2{1, 2},
            math::Vec2{3, 4}
        };
        QCOMPARE(bs.order(), 1);

        bs = math::BezierSolver<math::Vec2> {
            math::Vec2{1, 2},
            math::Vec2{3, 4},
            math::Vec2{5, 6}
        };
        QCOMPARE(bs.order(), 2);

        bs = math::BezierSolver<math::Vec2> {
            math::Vec2{1, 2},
            math::Vec2{3, 4},
            math::Vec2{5, 6},
            math::Vec2{7, 8}
        };
        QCOMPARE(bs.order(), 3);
    }

    void test_solve_linear()
    {
        math::BezierSolver<math::Vec2> bs{
            math::Vec2{20, 100},
            math::Vec2{40, 0}
        };

        QCOMPARE(bs.solve(0.0), math::Vec2(20, 100));
        QCOMPARE(bs.solve(.25), math::Vec2(25, 75));
        QCOMPARE(bs.solve(.50), math::Vec2(30, 50));
        QCOMPARE(bs.solve(.75), math::Vec2(35, 25));
        QCOMPARE(bs.solve(1.0), math::Vec2(40, 0));
    }


    void test_solve_quadratic()
    {
        math::Vec2 a{20, 100};
        math::Vec2 b{35, 50};
        math::Vec2 c{40, 0};

        math::BezierSolver<math::Vec2> bs{a, b, c};

        auto explicit_solve = [a,b,c](double t) {
            auto p1 = a.lerp(b, t);
            auto p2 = b.lerp(c, t);
            return p1.lerp(p2, t);
        };

        QCOMPARE(bs.solve(0.0), a);
        QCOMPARE(bs.solve(.25), explicit_solve(0.25));
        QCOMPARE(bs.solve(.50), explicit_solve(0.50));
        QCOMPARE(bs.solve(.75), explicit_solve(0.75));
        QCOMPARE(bs.solve(1.0), c);
    }

#define FUZZY_COMPARE(a, b) QCOMPARE(a.x(), b.x()); QCOMPARE(a.y(), b.y());

    void test_solve_cubic()
    {
        math::Vec2 a{20, 100};
        math::Vec2 b{35, 50};
        math::Vec2 c{50, -20};
        math::Vec2 d{40, 0};

        math::BezierSolver<math::Vec2> bs{a, b, c, d};

        auto explicit_solve = [a,b,c,d](double t) {
            auto p1 = a.lerp(b, t);
            auto p2 = b.lerp(c, t);
            auto p3 = c.lerp(d, t);
            auto q1 = p1.lerp(p2, t);
            auto q2 = p2.lerp(p3, t);
            return q1.lerp(q2, t);
        };

        FUZZY_COMPARE(bs.solve(0.0), a);
        FUZZY_COMPARE(bs.solve(.25), explicit_solve(0.25));
        FUZZY_COMPARE(bs.solve(.50), explicit_solve(0.50));
        FUZZY_COMPARE(bs.solve(.75), explicit_solve(0.75));
        FUZZY_COMPARE(bs.solve(1.0), d);
    }

    void test_angle_linear()
    {
        math::Vec2::scalar angle = 1;
        math::Vec2 a{20, 30};
        math::Vec2 b = a + math::Vec2::from_polar(10, angle);

        math::BezierSolver<math::Vec2> bs{a, b};
        QCOMPARE(bs.tangent_angle(0.00), angle);
        QCOMPARE(bs.tangent_angle(0.25), angle);
        QCOMPARE(bs.tangent_angle(0.50), angle);
        QCOMPARE(bs.tangent_angle(0.75), angle);
        QCOMPARE(bs.tangent_angle(1.00), angle);

        b = a + math::Vec2::from_polar(10, 2 * M_PI -1);
        angle = - 1;

        bs = math::BezierSolver<math::Vec2>{a, b};
        QCOMPARE(bs.tangent_angle(0.00), angle);
        QCOMPARE(bs.tangent_angle(0.25), angle);
        QCOMPARE(bs.tangent_angle(0.50), angle);
        QCOMPARE(bs.tangent_angle(0.75), angle);
        QCOMPARE(bs.tangent_angle(1.00), angle);
    }

    void test_angle_quadratic()
    {
        math::Vec2 sp{20, 30};
        math::Vec2 h{30, 40};
        math::Vec2 ep{40, 30};

        math::BezierSolver<math::Vec2> bs{sp, h, ep};
        QCOMPARE(bs.tangent_angle(0.00), M_PI/4);
        QVERIFY(bs.tangent_angle(0.1) > 0);
        QCOMPARE(bs.tangent_angle(0.50), 0);
        QVERIFY(bs.tangent_angle(0.9) < 0);
        QCOMPARE(bs.tangent_angle(1.00), -M_PI/4);
    }

    void test_angle_cubic()
    {
        math::Vec2 sp{20, 30};
        math::Vec2 h1{20, 40};
        math::Vec2 h2{40, 20};
        math::Vec2 ep{40, 30};

        math::BezierSolver<math::Vec2> bs{sp, h1, h2, ep};
        QCOMPARE(bs.tangent_angle(0.00), M_PI/2);
        QVERIFY(bs.tangent_angle(0.1) > 0);
        QVERIFY(bs.tangent_angle(0.50) < 0);
        QVERIFY(bs.tangent_angle(0.9) > 0);
        QCOMPARE(bs.tangent_angle(1.00), M_PI/2);
    }

    void test_solve_step()
    {
        math::Vec2 a{20, 30};
        math::Vec2 b{15, 40};
        math::Vec2 c{30, 10};
        math::Vec2 d{40, 15};

        math::BezierSolver<math::Vec2> bs{a, b, c, d};
        double fac = 0.33;
        auto quad = bs.solve_step(fac);
        QCOMPARE(quad.size(), 3);
        auto q0 = a.lerp(b, fac);
        FUZZY_COMPARE(quad[0], q0);
        auto q1 = b.lerp(c, fac);
        FUZZY_COMPARE(quad[1], q1);
        auto q2 = c.lerp(d, fac);
        FUZZY_COMPARE(quad[2], q2);

        bs = math::BezierSolver<math::Vec2>{q0, q1, q2};
        auto lin = bs.solve_step(fac);
        QCOMPARE(lin.size(), 2);
        auto l0 = q0.lerp(q1, fac);
        auto l1 = q1.lerp(q2, fac);
        FUZZY_COMPARE(lin[0], l0);
        FUZZY_COMPARE(lin[1], l1);

        bs = math::BezierSolver<math::Vec2>{l0, l1};
        auto res = bs.solve_step(fac);
        QCOMPARE(res.size(), 1);
        FUZZY_COMPARE(res[0], l0.lerp(l1, fac));
        FUZZY_COMPARE(res[0], (math::BezierSolver<math::Vec2>{a, b, c, d}.solve(fac)));
    }

    void test_split_cubic()
    {
        math::Vec2 sp{20, 30};
        math::Vec2 ep{80, 30};
        math::Vec2 h1 = sp + math::Vec2{10, 20};
        math::Vec2 h2 = ep + math::Vec2{-10, 20};
        double mid_x = 50;

        math::BezierSolver<math::Vec2> bs{sp, h1, h2, ep};
        auto split = bs.split_cubic(0.5);

        // Fisrt split
        // Begins with the starting point
        QCOMPARE(split.first[0], sp);
        // First tangent pointing up and right
        QVERIFY(split.first[1].x() > sp.x());
        QVERIFY(split.first[1].x() < mid_x);
        QVERIFY(split.first[1].y() > sp.y());
        // End point should be in the middle of the original
        QCOMPARE(split.first[3].x(), mid_x);
        QVERIFY(split.first[3].y() > sp.y());
        // Second tangent points straight out of the mid point
        QVERIFY(split.first[2].x() > split.first[1].x());
        QVERIFY(split.first[2].x() < split.first[3].x());
        QCOMPARE(split.first[2].y(), split.first[3].y());

        // Second Split
        // Starts where the other left off
        QCOMPARE(split.second[0], split.first[3]);
        // First tangent points straight out of the mid point
        QVERIFY(split.second[1].x() > split.second[0].x());
        QVERIFY(split.second[1].x() < split.second[2].x());
        QCOMPARE(split.second[1].y(), split.second[0].y());
        // Ends with the original end point
        QCOMPARE(split.second[3], ep);
        // Second tangent points down
        QVERIFY(split.second[2].x() < ep.x());
        QVERIFY(split.second[2].x() > mid_x);
        QVERIFY(split.second[2].y() > ep.y());
    }


    void test_split_quadratic()
    {
        math::Vec2 sp{20, 30};
        math::Vec2 ep{80, 30};
        double mid_x = 50;
        math::Vec2 h{mid_x, 50};

        math::BezierSolver<math::Vec2> bs{sp, h, ep};
        auto split = bs.split_cubic(0.5);

        // Fisrt split
        // Begins with the starting point
        QCOMPARE(split.first[0], sp);
        QCOMPARE(split.first[1], sp);
        // End point should be in the middle of the original
        QCOMPARE(split.first[3].x(), mid_x);
        QVERIFY(split.first[3].y() > sp.y());
        // Second tangent points straight out of the mid point
        QVERIFY(split.first[2].x() > split.first[1].x());
        QVERIFY(split.first[2].x() < split.first[3].x());
        QCOMPARE(split.first[2].y(), split.first[3].y());

        // Second Split
        // Starts where the other left off
        QCOMPARE(split.second[0], split.first[3]);
        // First tangent points straight out of the mid point
        QVERIFY(split.second[1].x() > split.second[0].x());
        QVERIFY(split.second[1].x() < split.second[2].x());
        QCOMPARE(split.second[1].y(), split.second[0].y());
        // Ends with the original end point
        QCOMPARE(split.second[3], ep);
        QCOMPARE(split.second[2], ep);
    }


    void test_split_linear()
    {
        math::Vec2 sp{20, 30};
        math::Vec2 ep{80, 130};
        auto midp = (sp+ep)/2;

        math::BezierSolver<math::Vec2> bs{sp, ep};
        auto split = bs.split_cubic(0.5);

        // Fisrt split
        // Begins with the starting point
        QCOMPARE(split.first[0], sp);
        QCOMPARE(split.first[1], sp);
        // End point should be in the middle of the original
        QCOMPARE(split.first[3], midp);
        QCOMPARE(split.first[2], midp);
        // Second Split
        // Starts where the other left off
        QCOMPARE(split.second[0], midp);
        QCOMPARE(split.second[1], midp);
        // Ends with the original end point
        QCOMPARE(split.second[3], ep);
        QCOMPARE(split.second[2], ep);
    }

};


QTEST_GUILESS_MAIN(TestCase)
#include "test_bezier_solver.moc"

