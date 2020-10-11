#include <QtTest/QtTest>

#include <vector>
#include "math/bezier/solver.hpp"
#include "print_vec.hpp"

class TestCase: public QObject
{
    Q_OBJECT

private slots:
#if 0
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
            auto p1 = math::lerp(a, b, t);
            auto p2 = math::lerp(b, c, t);
            return math::lerp(p1, p2, t);
        };

        QCOMPARE(bs.solve(0.0), a);
        QCOMPARE(bs.solve(.25), explicit_solve(0.25));
        QCOMPARE(bs.solve(.50), explicit_solve(0.50));
        QCOMPARE(bs.solve(.75), explicit_solve(0.75));
        QCOMPARE(bs.solve(1.0), c);
    }
#endif

#define FUZZY_COMPARE(a, b) QCOMPARE(a.x(), b.x()); QCOMPARE(a.y(), b.y());

    void test_solve_cubic()
    {
        math::Vec2 a{20, 100};
        math::Vec2 b{35, 50};
        math::Vec2 c{50, -20};
        math::Vec2 d{40, 0};

        math::bezier::CubicBezierSolver<math::Vec2> bs{a, b, c, d};

        auto explicit_solve = [a,b,c,d](double t) {
            auto p1 = math::lerp(a, b, t);
            auto p2 = math::lerp(b, c, t);
            auto p3 = math::lerp(c, d, t);
            auto q1 = math::lerp(p1, p2, t);
            auto q2 = math::lerp(p2, p3, t);
            return math::lerp(q1, q2, t);
        };

        FUZZY_COMPARE(bs.solve(0.0), a);
        FUZZY_COMPARE(bs.solve(.25), explicit_solve(0.25));
        FUZZY_COMPARE(bs.solve(.50), explicit_solve(0.50));
        FUZZY_COMPARE(bs.solve(.75), explicit_solve(0.75));
        FUZZY_COMPARE(bs.solve(1.0), d);
    }

#if 0
    void test_angle_linear()
    {
        math::scalar_type<math::Vec2> angle = 1;
        math::Vec2 a{20, 30};
        math::Vec2 b = a + math::from_polar(10, angle);

        math::BezierSolver<math::Vec2> bs{a, b};
        QCOMPARE(bs.tangent_angle(0.00), angle);
        QCOMPARE(bs.tangent_angle(0.25), angle);
        QCOMPARE(bs.tangent_angle(0.50), angle);
        QCOMPARE(bs.tangent_angle(0.75), angle);
        QCOMPARE(bs.tangent_angle(1.00), angle);

        b = a + math::from_polar(10, 2 * M_PI -1);
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
#endif

    void test_angle_cubic()
    {
        math::Vec2 sp{20, 30};
        math::Vec2 h1{20, 40};
        math::Vec2 h2{40, 20};
        math::Vec2 ep{40, 30};

        math::bezier::CubicBezierSolver<math::Vec2> bs{sp, h1, h2, ep};
        QCOMPARE(bs.tangent_angle(0.00), M_PI/2);
        QVERIFY(bs.tangent_angle(0.1) > 0);
        QVERIFY(bs.tangent_angle(0.50) < 0);
        QVERIFY(bs.tangent_angle(0.9) > 0);
        QCOMPARE(bs.tangent_angle(1.00), M_PI/2);
    }

#if 0
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
        auto q0 = math::lerp(a, b, fac);
        FUZZY_COMPARE(quad[0], q0);
        auto q1 = math::lerp(b, c, fac);
        FUZZY_COMPARE(quad[1], q1);
        auto q2 = math::lerp(c, d, fac);
        FUZZY_COMPARE(quad[2], q2);

        bs = math::BezierSolver<math::Vec2>{q0, q1, q2};
        auto lin = bs.solve_step(fac);
        QCOMPARE(lin.size(), 2);
        auto l0 = math::lerp(q0, q1, fac);
        auto l1 = math::lerp(q1, q2, fac);
        FUZZY_COMPARE(lin[0], l0);
        FUZZY_COMPARE(lin[1], l1);

        bs = math::BezierSolver<math::Vec2>{l0, l1};
        auto res = bs.solve_step(fac);
        QCOMPARE(res.size(), 1);
        FUZZY_COMPARE(res[0], math::lerp(l0, l1, fac));
        FUZZY_COMPARE(res[0], (math::BezierSolver<math::Vec2>{a, b, c, d}.solve(fac)));
    }
#endif

    void test_split_cubic()
    {
        math::Vec2 sp{20, 30};
        math::Vec2 ep{80, 30};
        math::Vec2 h1 = sp + math::Vec2{10, 20};
        math::Vec2 h2 = ep + math::Vec2{-10, 20};
        double mid_x = 50;

        math::bezier::CubicBezierSolver<math::Vec2> bs{sp, h1, h2, ep};
        auto split = bs.split(0.5);

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

#if 0
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
#endif

    void benchmark_solve()
    {
        using VecT = math::Vec2;
        VecT a{20, 30};
        VecT b{15, 40};
        VecT c{30, 10};
        VecT d{40, 15};
        math::bezier::CubicBezierSolver<VecT> bs{a, b, c, d};
        QBENCHMARK{
            for ( double t = 0; t <= 1; t += 0.01 )
                bs.solve(t);
        }
        // Without cubic optimization
        //      0.17 msecs per iteration (total: 91, iterations: 512)
        // With the optimization
        //      0.017 msecs per iteration (total: 71, iterations: 4096)
        // 1 order of magnitude difference
    }

    void benchmark_solve_qvec()
    {
        using VecT = QVector2D;
        VecT a{20, 30};
        VecT b{15, 40};
        VecT c{30, 10};
        VecT d{40, 15};
        math::bezier::CubicBezierSolver<VecT> bs{a, b, c, d};
        QBENCHMARK{
            for ( double t = 0; t <= 1; t += 0.01 )
                bs.solve(t);
        }
        // Slightly faster but calculating tangents loses significant precision
        //      0.013 msecs per iteration (total: 57, iterations: 4096)
    }

    void benchmark_solve_qpointf()
    {
        using VecT = QPointF;
        VecT a{20, 30};
        VecT b{15, 40};
        VecT c{30, 10};
        VecT d{40, 15};
        math::bezier::CubicBezierSolver<VecT> bs{a, b, c, d};
        QBENCHMARK{
            for ( double t = 0; t <= 1; t += 0.01 )
                bs.solve(t);
        }
        // Slightly faster but calculating tangents loses significant precision
        //      0.013 msecs per iteration (total: 57, iterations: 4096)
    }
#if 0
    void benchmark_solve_quadratic()
    {
        using VecT = math::Vec2;
        VecT a{20, 30};
        VecT b{15, 40};
        VecT c{30, 10};
        math::BezierSolver<VecT> bs{a, b, c};
        QBENCHMARK{
            for ( double t = 0; t <= 1; t += 0.01 )
                bs.solve(t);
        }
        // Without optimization
        //     0.12 msecs per iteration (total: 65, iterations: 512)
        // With the optimization
        //     0.013 msecs per iteration (total: 57, iterations: 4096)
    }
#endif

    void test_box_line_simple()
    {
        using VecT = QPointF;
        VecT a{20, 30};
        VecT d{130, 250};
        math::bezier::CubicBezierSolver<VecT> bs{a, a, d, d};
        auto bbox = bs.bounds();
        QCOMPARE(bbox.first, a);
        QCOMPARE(bbox.second, d);
    }

    void test_box_line_mix()
    {
        using VecT = QPointF;
        VecT a{130, 30};
        VecT d{20, 250};
        math::bezier::CubicBezierSolver<VecT> bs{a, a, d, d};
        auto bbox = bs.bounds();
        QCOMPARE(bbox.first, VecT(20, 30));
        QCOMPARE(bbox.second, VecT(130, 250));
    }

    void test_box_simple()
    {
        using VecT = QPointF;
        VecT a{20, 30};
        VecT b{20, 200};
        VecT c{130, 100};
        VecT d{130, 250};
        math::bezier::CubicBezierSolver<VecT> bs{a, b, c, d};
        auto bbox = bs.bounds();
        QCOMPARE(bbox.first, a);
        QCOMPARE(bbox.second, d);
    }

    void test_box_simple_transposed()
    {
        using VecT = QPointF;
        VecT a{30, 20};
        VecT b{200, 20};
        VecT c{100, 130};
        VecT d{250, 130};
        math::bezier::CubicBezierSolver<VecT> bs{a, b, c, d};
        auto bbox = bs.bounds();
        QCOMPARE(bbox.first, a);
        QCOMPARE(bbox.second, d);
    }

    void test_box()
    {
        using VecT = QPointF;
        VecT a{30, 20};
        VecT b{-40, 160};
        VecT c{330, 370};
        VecT d{250, 130};
        math::bezier::CubicBezierSolver<VecT> bs{a, b, c, d};
        auto bbox = bs.bounds();
        FUZZY_COMPARE(bbox.first, VecT(21.1349479424, 20));
        FUZZY_COMPARE(bbox.second, VecT(261.392510712, 239.272612647));
    }
};

QTEST_GUILESS_MAIN(TestCase)
#include "test_bezier_solver.moc"
