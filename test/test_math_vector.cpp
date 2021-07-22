#include <QtTest/QtTest>

#include <vector>
#include "math/vector.hpp"

using namespace glaxnimate;


class TestCase: public QObject
{
    Q_OBJECT

private slots:
    void test_default_constructor()
    {
        math::VecN<int, 5> v;
    }

    void test_arg_ctor()
    {
        math::VecN<int, 5> v{10, 20, 30, 40, 50};
    }

    void test_get_const()
    {
        const math::VecN<int, 5> v{10, 20, 30, 40, 50};
        QCOMPARE(v.get<0>(), 10);
        QCOMPARE(v.get<1>(), 20);
        QCOMPARE(v.get<2>(), 30);
        QCOMPARE(v.get<3>(), 40);
        QCOMPARE(v.get<4>(), 50);

        const math::VecN<int, 5> v0;
        QCOMPARE(v0.get<0>(), 0);
        QCOMPARE(v0.get<1>(), 0);
        QCOMPARE(v0.get<2>(), 0);
        QCOMPARE(v0.get<3>(), 0);
        QCOMPARE(v0.get<4>(), 0);
    }


    void test_get_nonconst()
    {
        math::VecN<int, 5> v{10, 20, 30, 40, 50};
        QCOMPARE(v.get<0>(), 10);
        QCOMPARE(v.get<1>(), 20);
        QCOMPARE(v.get<2>(), 30);
        QCOMPARE(v.get<3>(), 40);
        QCOMPARE(v.get<4>(), 50);

        v.get<0>() = 60;
        v.get<1>() = 61;
        v.get<2>() = 62;
        v.get<3>() = 63;
        v.get<4>() = 64;

        QCOMPARE(v.get<0>(), 60);
        QCOMPARE(v.get<1>(), 61);
        QCOMPARE(v.get<2>(), 62);
        QCOMPARE(v.get<3>(), 63);
        QCOMPARE(v.get<4>(), 64);
    }

    void test_data_const()
    {
        const math::VecN<int, 5> v{10, 20, 30, 40, 50};
        const int* data = v.data();
        QCOMPARE(data[0], 10);
        QCOMPARE(data[1], 20);
        QCOMPARE(data[2], 30);
        QCOMPARE(data[3], 40);
        QCOMPARE(data[4], 50);
    }

    void test_data_nonconst()
    {
        math::VecN<int, 5> v{10, 20, 30, 40, 50};
        int* data = v.data();
        QCOMPARE(data[0], 10);
        QCOMPARE(data[1], 20);
        QCOMPARE(data[2], 30);
        QCOMPARE(data[3], 40);
        QCOMPARE(data[4], 50);

        data[0] = 60;
        data[1] = 61;
        data[2] = 62;
        data[3] = 63;
        data[4] = 64;

        QCOMPARE(data[0], 60);
        QCOMPARE(data[1], 61);
        QCOMPARE(data[2], 62);
        QCOMPARE(data[3], 63);
        QCOMPARE(data[4], 64);
    }

    void test_subscript_const()
    {
        const math::VecN<int, 5> data{10, 20, 30, 40, 50};
        QCOMPARE(data[0], 10);
        QCOMPARE(data[1], 20);
        QCOMPARE(data[2], 30);
        QCOMPARE(data[3], 40);
        QCOMPARE(data[4], 50);
    }

    void test_subscript_nonconst()
    {
        math::VecN<int, 5> data{10, 20, 30, 40, 50};

        QCOMPARE(data[0], 10);
        QCOMPARE(data[1], 20);
        QCOMPARE(data[2], 30);
        QCOMPARE(data[3], 40);
        QCOMPARE(data[4], 50);

        data[0] = 60;
        data[1] = 61;
        data[2] = 62;
        data[3] = 63;
        data[4] = 64;

        QCOMPARE(data[0], 60);
        QCOMPARE(data[1], 61);
        QCOMPARE(data[2], 62);
        QCOMPARE(data[3], 63);
        QCOMPARE(data[4], 64);
    }

    void test_range()
    {
        math::VecN<int, 5> v{10, 20, 30, 40, 50};
        std::vector<int> data(v.begin(), v.end());
        QCOMPARE(data[0], 10);
        QCOMPARE(data[1], 20);
        QCOMPARE(data[2], 30);
        QCOMPARE(data[3], 40);
        QCOMPARE(data[4], 50);
    }

    void test_sign_neg()
    {
        math::VecN<int, 5> a{10, 20, 30, 40, 50};
        math::VecN<int, 5> v = -a;
        QCOMPARE(v.get<0>(), -10);
        QCOMPARE(v.get<1>(), -20);
        QCOMPARE(v.get<2>(), -30);
        QCOMPARE(v.get<3>(), -40);
        QCOMPARE(v.get<4>(), -50);
    }

    void test_plus_equal()
    {
        math::VecN<int, 5> a{10, 20, 30, 40, 50};
        math::VecN<int, 5> b{6, 7, 8, 9, 10};
        math::VecN<int, 5> c = a;
        c += b;

        QCOMPARE(c.get<0>(), 16);
        QCOMPARE(c.get<1>(), 27);
        QCOMPARE(c.get<2>(), 38);
        QCOMPARE(c.get<3>(), 49);
        QCOMPARE(c.get<4>(), 60);
    }

    void test_plus()
    {
        math::VecN<int, 5> a{10, 20, 30, 40, 50};
        math::VecN<int, 5> b{6, 7, 8, 9, 10};
        math::VecN<int, 5> c = a + b;

        QCOMPARE(c.get<0>(), 16);
        QCOMPARE(c.get<1>(), 27);
        QCOMPARE(c.get<2>(), 38);
        QCOMPARE(c.get<3>(), 49);
        QCOMPARE(c.get<4>(), 60);

        QCOMPARE(a[0], 10);
        QCOMPARE(a[1], 20);
        QCOMPARE(a[2], 30);
        QCOMPARE(a[3], 40);
        QCOMPARE(a[4], 50);
    }

    void test_minus_equal()
    {
        math::VecN<int, 5> a{10, 20, 30, 40, 50};
        math::VecN<int, 5> b{6, 7, 8, 9, 10};
        math::VecN<int, 5> c = a;
        c -= b;

        QCOMPARE(c.get<0>(), 4);
        QCOMPARE(c.get<1>(), 13);
        QCOMPARE(c.get<2>(), 22);
        QCOMPARE(c.get<3>(), 31);
        QCOMPARE(c.get<4>(), 40);
    }

    void test_minus()
    {
        math::VecN<int, 5> a{10, 20, 30, 40, 50};
        math::VecN<int, 5> b{6, 7, 8, 9, 10};
        math::VecN<int, 5> c = a - b;

        QCOMPARE(c.get<0>(), 4);
        QCOMPARE(c.get<1>(), 13);
        QCOMPARE(c.get<2>(), 22);
        QCOMPARE(c.get<3>(), 31);
        QCOMPARE(c.get<4>(), 40);
    }

    void test_times_equal()
    {
        math::VecN<int, 5> a{10, 20, 30, 40, 50};
        math::VecN<int, 5> c = a;
        c *= 2;

        QCOMPARE(c.get<0>(), 20);
        QCOMPARE(c.get<1>(), 40);
        QCOMPARE(c.get<2>(), 60);
        QCOMPARE(c.get<3>(), 80);
        QCOMPARE(c.get<4>(), 100);
    }

    void test_times()
    {
        math::VecN<int, 5> a{10, 20, 30, 40, 50};
        math::VecN<int, 5> c = a * 2;

        QCOMPARE(c.get<0>(), 20);
        QCOMPARE(c.get<1>(), 40);
        QCOMPARE(c.get<2>(), 60);
        QCOMPARE(c.get<3>(), 80);
        QCOMPARE(c.get<4>(), 100);
    }

    void test_over_equal()
    {
        math::VecN<int, 5> a{10, 20, 30, 40, 50};
        math::VecN<int, 5> c = a;
        c /= 2;

        QCOMPARE(c.get<0>(), 5);
        QCOMPARE(c.get<1>(), 10);
        QCOMPARE(c.get<2>(), 15);
        QCOMPARE(c.get<3>(), 20);
        QCOMPARE(c.get<4>(), 25);
    }

    void test_over()
    {
        math::VecN<int, 5> a{10, 20, 30, 40, 50};
        math::VecN<int, 5> c = a / 2;

        QCOMPARE(c.get<0>(), 5);
        QCOMPARE(c.get<1>(), 10);
        QCOMPARE(c.get<2>(), 15);
        QCOMPARE(c.get<3>(), 20);
        QCOMPARE(c.get<4>(), 25);
    }

    void test_cmp()
    {
        math::VecN<int, 5> a{10, 20, 30, 40, 50};
        math::VecN<int, 5> b{10, 20, 31, 40, 50};
        math::VecN<int, 5> c{10, 20, 30, 40, 50};
        QVERIFY(!(a == b));
        QVERIFY((a != b));
        QVERIFY((a == c));
        QVERIFY(!(a != c));
    }

    void test_length()
    {
        math::VecN<int, 2> a{3, 4};
        QCOMPARE(a.length_squared(), 25);
        QCOMPARE(a.length(), 5);
    }


    void test_normalize()
    {
        math::VecN<float, 2> a{3, 4};
        a.normalize();
        QCOMPARE(a.length(), 1);
        QCOMPARE(a * 5, (math::VecN<float, 2>{3, 4}));
    }


    void test_normalized()
    {
        math::VecN<float, 2> a{3, 4};
        math::VecN<float, 2> b = a.normalized();
        QCOMPARE(a.length(), 5);
        QCOMPARE(b.length(), 1);
        QCOMPARE(b * 5, a);
    }

    void test_lerp()
    {
        math::VecN<float, 2> a{20, 100};
        math::VecN<float, 2> b{40, 0};
        QCOMPARE(a.lerp(b, 0), a);
        QCOMPARE(a.lerp(b, .25), (math::VecN<float, 2>{25, 75}));
        QCOMPARE(a.lerp(b, .5), (math::VecN<float, 2>{30, 50}));
        QCOMPARE(a.lerp(b, .75), (math::VecN<float, 2>{35, 25}));
        QCOMPARE(a.lerp(b, 1), b);
    }

};


QTEST_GUILESS_MAIN(TestCase)
#include "test_math_vector.moc"
