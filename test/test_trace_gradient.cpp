#include <QtTest/QtTest>

#include "trace/gradient.hpp"
#include "math/math.hpp"

#include "test_trace.hpp"

class TestTraceGradient: public QObject
{
    Q_OBJECT

private:
    std::vector<StructuredColor> make_gradient_colors(
        const GradientStops& gradient,
        int n_colors = 256
    )
    {
        std::vector<StructuredColor> colors;
        int start = 0;
        for ( int i = 0; i <= n_colors; i++ )
        {
            float factor = i / float(n_colors);
            if ( factor > gradient[start+1].first )
                start++;

            float t = (factor - gradient[start].first) / (gradient[start+1].first - gradient[start].first);
            colors.push_back(gradient[start].second.lerp(gradient[start+1].second, t));
        }
        return colors;
    }

private slots:
    void test_gradient_stops_2()
    {
        GradientStops gradient{
            {0.00, 0xffff0000},
            {1.00, 0xff000000},
        };
        std::vector<StructuredColor> colors = make_gradient_colors(gradient);

        auto stops = gradient_stops(colors);
        COMPARE_VECTOR(stops, gradient);

    }

    void test_gradient_stops_3()
    {
        GradientStops gradient{
            {0.00, 0xffff0000},
            {0.75, 0xffffff00},
            {1.00, 0xff000000},
        };
        std::vector<StructuredColor> colors = make_gradient_colors(gradient);

        auto stops = gradient_stops(colors);
        COMPARE_VECTOR(stops, gradient);

    }

    void test_gradient_stops_too_many_pixels()
    {
        GradientStops gradient{
            {0.00, 0xffff0000},
            {0.75, 0xffffff00},
            {1.00, 0xff000000},
        };
        std::vector<StructuredColor> colors = make_gradient_colors(gradient, 2048);

        auto stops = gradient_stops(colors);
        COMPARE_VECTOR(stops, gradient);
    }

    void test_gradient_stops_not_enough_pixels()
    {
        GradientStops gradient{
            {0.00, 0xffff0000},
            {0.75, 0xffffff00},
            {1.00, 0xff000000},
        };
        std::vector<StructuredColor> colors = make_gradient_colors(gradient, 32);

        auto stops = gradient_stops(colors);
        COMPARE_VECTOR(stops, gradient);
    }

    void test_gradient_stops_subtle()
    {
        GradientStops gradient{
            {0.00, 0xffff8800},
            {0.75, 0xffcccc00},
            {1.00, 0xff88ff00},
        };
        std::vector<StructuredColor> colors = make_gradient_colors(gradient);

        auto stops = gradient_stops(colors);
        COMPARE_VECTOR(stops, gradient);
    }

    void test_cluster_angle_horiz()
    {
        SegmentedImage img(7, 7);
        img.bitmap() = {
            0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0,
            1, 1, 1, 1, 1, 1, 1,
            0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0,
        };
        auto cluster = img.add_cluster(1, 0x01);
        cluster->index_start = 0;
        cluster->index_end = 48;

        auto angle = cluster_angle(img, 1, {2, 2}) * 180 / math::pi;
        QCOMPARE(qRound(angle), 0);
    }
    void test_cluster_angle_vert()
    {
        SegmentedImage img(7, 7);
        img.bitmap() = {
            0, 1, 0, 0, 0, 0, 0,
            0, 1, 0, 0, 0, 0, 0,
            0, 1, 0, 0, 0, 0, 0,
            0, 1, 0, 0, 0, 0, 0,
            0, 1, 0, 0, 0, 0, 0,
            0, 1, 0, 0, 0, 0, 0,
            0, 1, 0, 0, 0, 0, 0,
        };
        auto cluster = img.add_cluster(1, 0x01);
        cluster->index_start = 0;
        cluster->index_end = 48;

        auto angle = cluster_angle(img, 1, {1, 3}) * 180 / math::pi;
        QCOMPARE(qRound(angle), 90);
    }

    void test_cluster_angle_diag_1()
    {
        SegmentedImage img(7, 7);
        img.bitmap() = {
            1, 1, 0, 0, 0, 0, 0,
            0, 1, 1, 0, 0, 0, 0,
            0, 0, 1, 1, 0, 0, 0,
            0, 0, 0, 1, 1, 0, 0,
            0, 0, 0, 0, 1, 1, 0,
            0, 0, 0, 0, 0, 1, 1,
            0, 0, 0, 0, 0, 0, 1,
        };
        auto cluster = img.add_cluster(1, 0x01);
        cluster->index_start = 0;
        cluster->index_end = 48;

        auto angle = cluster_angle(img, 1, {3, 3}) * 180 / math::pi;
        QCOMPARE(qRound(angle), 45);
    }

    void test_cluster_angle_diag_2()
    {
        SegmentedImage img(7, 7);
        img.bitmap() = {
            0, 0, 0, 0, 0, 0, 1,
            0, 0, 0, 0, 0, 1, 1,
            0, 0, 0, 0, 1, 1, 0,
            0, 0, 0, 1, 1, 0, 0,
            0, 0, 1, 1, 0, 0, 0,
            0, 1, 1, 0, 0, 0, 0,
            1, 1, 0, 0, 0, 0, 0,
        };
        auto cluster = img.add_cluster(1, 0x01);
        cluster->index_start = 0;
        cluster->index_end = 48;

        auto angle = cluster_angle(img, 1, {3, 3}) * 180 / math::pi;
        QCOMPARE(qRound(angle), 135);
    }

    void test_line_rect_0()
    {
        ImageRect rect{{0, 0}, {6, 6}};
        auto o = rect.center();
        BETTER_COMPARE(
            line_rect_intersection(o, 0, rect),
            ImageCoord(0, 3),
            ImageCoord(6, 3)
        );
    }

    void test_line_rect_pi()
    {
        ImageRect rect{{0, 0}, {6, 6}};
        auto o = rect.center();
        BETTER_COMPARE(
            line_rect_intersection(o, math::pi, rect),
            ImageCoord(0, 3),
            ImageCoord(6, 3),
        );
    }

    void test_line_rect_pi2()
    {
        ImageRect rect{{0, 0}, {6, 6}};
        auto o = rect.center();
        BETTER_COMPARE(
            line_rect_intersection(o, math::pi/2, rect),
            ImageCoord(3, 0),
            ImageCoord(3, 6),
        );
    }

    void test_line_rect_pi4()
    {
        ImageRect rect{{0, 0}, {6, 6}};
        auto o = rect.center();
        BETTER_COMPARE(
            line_rect_intersection(o, math::pi/4, rect),
            ImageCoord(0, 0),
            ImageCoord(6, 6),
        );
    }

    void test_line_rect_3pi4()
    {
        ImageRect rect{{0, 0}, {6, 6}};
        auto o = rect.center();
        BETTER_COMPARE(
            line_rect_intersection(o, math::pi*3/4, rect),
            ImageCoord(0, 6),
            ImageCoord(6, 0),
        );
    }

    void test_line_rect_m2()
    {
        ImageRect rect{{0, 0}, {6, 6}};
        BETTER_COMPARE(
            line_rect_intersection({0, 1}, math::atan2(1,2), rect),
            ImageCoord(0, 1),
            ImageCoord(6, 3),
        );
    }

    void test_line_pixels_0()
    {
        ImageRect rect{{0, 0}, {6, 6}};
        auto o = rect.center();
        COMPARE_VECTOR(
            line_pixels({0,3}, {6,3}),
            ImageCoord(0, 3),
            ImageCoord(1, 3),
            ImageCoord(2, 3),
            ImageCoord(3, 3),
            ImageCoord(4, 3),
            ImageCoord(5, 3),
            ImageCoord(6, 3),
        );
    }

    void test_line_pixels_pi2()
    {
        ImageRect rect{{0, 0}, {6, 6}};
        auto o = rect.center();
        COMPARE_VECTOR(
            line_pixels({3,0}, {3,6}),
            ImageCoord(3, 0),
            ImageCoord(3, 1),
            ImageCoord(3, 2),
            ImageCoord(3, 3),
            ImageCoord(3, 4),
            ImageCoord(3, 5),
            ImageCoord(3, 6),
        );
    }

    void test_line_pixels_pi4()
    {
        ImageRect rect{{0, 0}, {6, 6}};
        auto o = rect.center();
        COMPARE_VECTOR(
            line_pixels({0,0}, {6,6}),
            ImageCoord(0, 0),
            ImageCoord(1, 1),
            ImageCoord(2, 2),
            ImageCoord(3, 3),
            ImageCoord(4, 4),
            ImageCoord(5, 5),
            ImageCoord(6, 6),
        );
    }

    void test_line_pixels_3pi4()
    {
        ImageRect rect{{0, 0}, {6, 6}};
        auto o = rect.center();
        COMPARE_VECTOR(
            line_pixels({0,6}, {6, 0}),
            ImageCoord(0, 6),
            ImageCoord(1, 5),
            ImageCoord(2, 4),
            ImageCoord(3, 3),
            ImageCoord(4, 2),
            ImageCoord(5, 1),
            ImageCoord(6, 0),
        );
    }

    void test_line_pixels_m2()
    {
        ImageRect rect{{0, 0}, {6, 6}};
        COMPARE_VECTOR(
            line_pixels({0, 1}, {6,3}),
            ImageCoord(0, 1),
            ImageCoord(1, 1),
            ImageCoord(2, 2),
            ImageCoord(3, 2),
            ImageCoord(4, 2),
            ImageCoord(5, 3),
            ImageCoord(6, 3),
        );
    }
};

QTEST_GUILESS_MAIN(TestTraceGradient)
#include "test_trace_gradient.moc"
