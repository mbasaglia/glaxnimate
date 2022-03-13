#include <iostream>

#include <QtTest/QtTest>
#include <QPainter>

#include "trace/segmentation.hpp"
#include "trace/quantize.hpp"
#include "test_trace.hpp"

class TestSegmentation: public QObject
{
    Q_OBJECT


    std::vector<StructuredColor> make_gradient_colors(
        const std::vector<std::pair<float, StructuredColor>>& gradient,
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
    void test_add_cluster()
    {
        SegmentedImage img(1, 1);
        img.add_cluster(0xffff0000);
        COMPARE_VECTOR(img, Cluster{1, 0xffff0000, 1, 0});
        img.add_cluster(0xff00ff00, 3);
        COMPARE_VECTOR(img,
            Cluster{1, 0xffff0000, 1, 0},
            Cluster{2, 0xff00ff00, 3, 0}
        );
    }

    void test_size()
    {
        SegmentedImage img(1, 1);
        QCOMPARE(img.size(), 0);
        img.add_cluster(0xffaa0000);
        QCOMPARE(img.size(), 1);
        img.add_cluster(0xff00bb00);
        QCOMPARE(img.size(), 2);
        img.add_cluster(0xff0000cc);
        QCOMPARE(img.size(), 3);
        img.erase(img.begin());
        QCOMPARE(img.size(), 2);
        img.erase(img.begin());
        QCOMPARE(img.size(), 1);
        img.erase(img.begin());
        QCOMPARE(img.size(), 0);
    }

    void test_cluster_from_id()
    {
        SegmentedImage img(1, 1);
        img.add_cluster(0xffaa0000);
        img.add_cluster(0xff00bb00);
        img.add_cluster(0xff0000cc);
        QCOMPARE(img.cluster(Cluster::null_id), nullptr);
        QCOMPARE(*img.cluster(1), (Cluster{1, 0xffaa0000, 1, 0}));
        QCOMPARE(*img.cluster(2), (Cluster{2, 0xff00bb00, 1, 0}));
        QCOMPARE(*img.cluster(3), (Cluster{3, 0xff0000cc, 1, 0}));
    }

    void test_merge_direct()
    {
        SegmentedImage img(1, 1);
        img.add_cluster(0xffaa0000);
        img.add_cluster(0xff00bb00);
        img.add_cluster(0xff0000cc);
        img.add_cluster(0xff000000);
        img.merge(img.cluster(1), img.cluster(2));
        COMPARE_VECTOR(img,
            Cluster{1, 0xffaa0000, 1, 2},
            Cluster{2, 0xff00bb00, 1, 0},
            Cluster{3, 0xff0000cc, 1, 0},
            Cluster{4, 0xff000000, 1, 0}
        );
    }

    void test_merge_from_merged()
    {
        SegmentedImage img(1, 1);
        img.add_cluster(0xffaa0000);
        img.add_cluster(0xff00bb00);
        img.add_cluster(0xff0000cc);
        img.add_cluster(0xff000000);
        img.merge(img.cluster(1), img.cluster(3));
        img.merge(img.cluster(1), img.cluster(2));
        COMPARE_VECTOR(img,
            Cluster{1, 0xffaa0000, 1, 2},
            Cluster{2, 0xff00bb00, 1, 0},
            Cluster{3, 0xff0000cc, 1, 2},
            Cluster{4, 0xff000000, 1, 0}
        );
    }

    void test_merge_to_merged()
    {
        SegmentedImage img(1, 1);
        img.add_cluster(0xffaa0000);
        img.add_cluster(0xff00bb00);
        img.add_cluster(0xff0000cc);
        img.add_cluster(0xff000000);
        img.merge(img.cluster(2), img.cluster(3));
        img.merge(img.cluster(1), img.cluster(2));
        COMPARE_VECTOR(img,
            Cluster{1, 0xffaa0000, 1, 3},
            Cluster{2, 0xff00bb00, 1, 3},
            Cluster{3, 0xff0000cc, 1, 0},
            Cluster{4, 0xff000000, 1, 0}
        );
    }

    void test_merge_both_merged()
    {
        SegmentedImage img(1, 1);
        img.add_cluster(0xffaa0000);
        img.add_cluster(0xff00bb00);
        img.add_cluster(0xff0000cc);
        img.add_cluster(0xff000000);
        img.merge(img.cluster(1), img.cluster(4));
        img.merge(img.cluster(2), img.cluster(3));
        img.merge(img.cluster(1), img.cluster(2));
        COMPARE_VECTOR(img,
            Cluster{1, 0xffaa0000, 1, 3},
            Cluster{2, 0xff00bb00, 1, 3},
            Cluster{3, 0xff0000cc, 1, 0},
            Cluster{4, 0xff000000, 1, 3}
        );
    }

    void test_merge_self()
    {
        SegmentedImage img(1, 1);
        img.add_cluster(0xffaa0000);
        img.add_cluster(0xff00bb00);
        img.add_cluster(0xff0000cc);
        img.add_cluster(0xff000000);
        img.merge(img.cluster(1), img.cluster(1));
        COMPARE_VECTOR(img,
            Cluster{1, 0xffaa0000, 1, 0},
            Cluster{2, 0xff00bb00, 1, 0},
            Cluster{3, 0xff0000cc, 1, 0},
            Cluster{4, 0xff000000, 1, 0}
        );
    }

    void test_merge_noop()
    {
        SegmentedImage img(1, 1);
        img.add_cluster(0xffaa0000);
        img.add_cluster(0xff00bb00);
        img.add_cluster(0xff0000cc);
        img.add_cluster(0xff000000);
        img.merge(img.cluster(1), img.cluster(2));
        img.merge(img.cluster(1), img.cluster(2));
        COMPARE_VECTOR(img,
            Cluster{1, 0xffaa0000, 1, 2},
            Cluster{2, 0xff00bb00, 1, 0},
            Cluster{3, 0xff0000cc, 1, 0},
            Cluster{4, 0xff000000, 1, 0},
        );
    }

    void test_merge_cycle()
    {
        SegmentedImage img(1, 1);
        img.add_cluster(0xffaa0000);
        img.add_cluster(0xff00bb00);
        img.add_cluster(0xff0000cc);
        img.add_cluster(0xff000000);
        img.merge(img.cluster(1), img.cluster(2));
        img.merge(img.cluster(2), img.cluster(1));
        COMPARE_VECTOR(img,
            Cluster{1, 0xffaa0000, 1, 2},
            Cluster{2, 0xff00bb00, 1, 0},
            Cluster{3, 0xff0000cc, 1, 0},
            Cluster{4, 0xff000000, 1, 0},
        );
    }

    void test_cluster_id_from_xy()
    {
        SegmentedImage img(3, 3);
        img.bitmap() = {
            1, 1, 1,
            2, 0, 4,
            3, 4, 2,
        };
        img.add_cluster(0xffaa0000);
        img.add_cluster(0xff00bb00);
        img.add_cluster(0xff0000cc);
        img.add_cluster(0xff000000);
        QCOMPARE(img.cluster_id(-1, 0), Cluster::null_id);
        QCOMPARE(img.cluster_id(0, -1), Cluster::null_id);
        QCOMPARE(img.cluster_id(-1, -1), Cluster::null_id);
        QCOMPARE(img.cluster_id(0, 0), 1);
        QCOMPARE(img.cluster_id(1, 0), 1);
        QCOMPARE(img.cluster_id(2, 0), 1);
        QCOMPARE(img.cluster_id(0, 1), 2);
        QCOMPARE(img.cluster_id(1, 1), 0);
        QCOMPARE(img.cluster_id(2, 1), 4);
        QCOMPARE(img.cluster_id(0, 2), 3);
        QCOMPARE(img.cluster_id(1, 2), 4);
        QCOMPARE(img.cluster_id(2, 2), 2);
    }

    void test_cluster_from_xy()
    {
        SegmentedImage img(3, 3);
        img.bitmap() = {
            1, 1, 1,
            2, 0, 4,
            3, 4, 2,
        };
        img.add_cluster(0xffaa0000);
        img.add_cluster(0xff00bb00);
        img.add_cluster(0xff0000cc);
        img.add_cluster(0xff000000);
        QCOMPARE(img.cluster(-1, 0), nullptr);
        QCOMPARE(img.cluster(0, -1), nullptr);
        QCOMPARE(img.cluster(-1, -1), nullptr);
        QCOMPARE(img.cluster(0, 0), img.cluster(1));
        QCOMPARE(img.cluster(1, 0), img.cluster(1));
        QCOMPARE(img.cluster(2, 0), img.cluster(1));
        QCOMPARE(img.cluster(0, 1), img.cluster(2));
        QCOMPARE(img.cluster(1, 1), nullptr);
        QCOMPARE(img.cluster(2, 1), img.cluster(4));
        QCOMPARE(img.cluster(0, 2), img.cluster(3));
        QCOMPARE(img.cluster(1, 2), img.cluster(4));
        QCOMPARE(img.cluster(2, 2), img.cluster(2));
    }

    void test_normalize_noop()
    {
        SegmentedImage img(3, 3);
        img.bitmap() = {
            1, 1, 1,
            3, 0, 4,
            3, 4, 2,
        };
        img.add_cluster(0x1, 3);
        img.add_cluster(0x2, 1);
        img.add_cluster(0x3, 2);
        img.add_cluster(0x4, 2);
        img.normalize();
        COMPARE_VECTOR(img.bitmap(),
            1, 1, 1,
            3, 0, 4,
            3, 4, 2,
        );
        COMPARE_VECTOR(img,
            Cluster{1, 0x1, 3, 0},
            Cluster{2, 0x2, 1, 0},
            Cluster{3, 0x3, 2, 0},
            Cluster{4, 0x4, 2, 0},
        );
    }

    void test_normalize()
    {
        SegmentedImage img(3, 3);
        img.bitmap() = {
            1, 1, 1,
            3, 0, 4,
            3, 4, 2,
        };
        img.add_cluster(0x1, 3);
        img.add_cluster(0x2, 1);
        img.add_cluster(0x3, 2);
        img.add_cluster(0x4, 2);
        img.add_cluster(0x5, 0);
        img.merge(img.cluster(2), img.cluster(4));
        img.merge(img.cluster(3), img.cluster(1));
        img.normalize();
        COMPARE_VECTOR(img.bitmap(),
            1, 1, 1,
            1, 0, 4,
            1, 4, 4,
        );
        COMPARE_VECTOR(img,
            Cluster{1, 0x1, 5, 0},
            Cluster{4, 0x4, 3, 0},
        );
    }

    void test_segment_all_unique()
    {
        QImage image = make_image({
            {0xff000000, 0xff000001, 0xff000002},
            {0xff000010, 0xff000011, 0xff000012},
            {0xff000020, 0xff000021, 0xff000022},
        });
        SegmentedImage segmented = segment(image);
        COMPARE_VECTOR(segmented.bitmap(),
            1, 2, 3,
            4, 5, 6,
            7, 8, 9
        );
        COMPARE_VECTOR(segmented,
            Cluster{1, 0xff000000, 1, 0},
            Cluster{2, 0xff000001, 1, 0},
            Cluster{3, 0xff000002, 1, 0},

            Cluster{4, 0xff000010, 1, 0},
            Cluster{5, 0xff000011, 1, 0},
            Cluster{6, 0xff000012, 1, 0},

            Cluster{7, 0xff000020, 1, 0},
            Cluster{8, 0xff000021, 1, 0},
            Cluster{9, 0xff000022, 1, 0},
        );
    }

    void test_erase_alpha_threshold()
    {
        QImage image = make_image({
            {0x10000000, 0x30000001, 0x50000002},
            {0x70000010, 0x90000011, 0xb0000012},
            {0xd0000020, 0xf0000021, 0xff000022}
        });
        SegmentedImage segmented = segment(image);
        segmented.erase_if([](const Cluster& c) { return qAlpha(c.color) < 128; });
        COMPARE_VECTOR(segmented.bitmap(),
            0, 0, 0,
            0, 5, 6,
            7, 8, 9
        );
        COMPARE_VECTOR(segmented,
            Cluster{5, 0x90000011, 1, 0},
            Cluster{6, 0xb0000012, 1, 0},
            Cluster{7, 0xd0000020, 1, 0},
            Cluster{8, 0xf0000021, 1, 0},
            Cluster{9, 0xff000022, 1, 0},
        );
    }

    void test_segment_join_left()
    {
        QImage image = make_image({
            {0xff000001, 0xff000001, 0xff000001},
            {0x0,        0x0,        0x0},
            {0xff000001, 0xff000001, 0x0},
        });
        SegmentedImage segmented = segment(image);
        segmented.erase_if([](const Cluster& c) { return c.color == 0; });
        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 1,
            0, 0, 0,
            3, 3, 0
        );
        COMPARE_VECTOR(segmented,
            Cluster{1, 0xff000001, 3, 0},
            Cluster{3, 0xff000001, 2, 0},
        );
    }

    void test_segment_join_up()
    {
        QImage image = make_image({
            {0xff000001, 0x0, 0xff000001},
            {0xff000001, 0x0, 0xff000001},
            {0xff000001, 0x0, 0x0},
        });
        SegmentedImage segmented = segment(image);
        segmented.erase_if([](const Cluster& c) { return c.color == 0; });
        COMPARE_VECTOR(segmented.bitmap(),
            1, 0, 3,
            1, 0, 3,
            1, 0, 0
        );
        COMPARE_VECTOR(segmented,
            Cluster{1, 0xff000001, 3, 0},
            Cluster{3, 0xff000001, 2, 0},
        );
    }

    void test_segment_join_diag_1_corner()
    {
        QImage image = make_image({
            {0x0,        0x0,        0xff000001},
            {0x0,        0xff000001, 0xff000001},
            {0xff000001, 0xff000001, 0x0},
        });
        SegmentedImage segmented = segment(image);
        segmented.erase_if([](const Cluster& c) { return c.color == 0; });
        COMPARE_VECTOR(segmented.bitmap(),
            0, 0, 2,
            0, 2, 2,
            2, 2, 0
        );
        COMPARE_VECTOR(segmented,
            Cluster{2, 0xff000001, 5, 0},
        );
    }

    void test_segment_join_diag_1_no_corner()
    {
        QImage image = make_image({
            {0x0,        0x0,        0xff000001},
            {0x0,        0xff000001, 0x0},
            {0xff000001, 0x0,        0x0},
        });
        SegmentedImage segmented = segment(image);
        segmented.erase_if([](const Cluster& c) { return c.color == 0; });
        COMPARE_VECTOR(segmented.bitmap(),
            0, 0, 2,
            0, 2, 0,
            2, 0, 0
        );
        COMPARE_VECTOR(segmented,
            Cluster{2, 0xff000001, 3, 0},
        );
    }

    void test_segment_join_diag_2()
    {
        QImage image = make_image({
            {0xff000001, 0x0,        0x0},
            {0x0,        0xff000001, 0x0},
            {0x0,        0x0,        0xff000001},
        });
        SegmentedImage segmented = segment(image);
        segmented.erase_if([](const Cluster& c) { return c.color == 0; });
        COMPARE_VECTOR(segmented.bitmap(),
            1, 0, 0,
            0, 1, 0,
            0, 0, 1
        );
        COMPARE_VECTOR(segmented,
            Cluster{1, 0xff000001, 3, 0},
        );
    }

    void test_unique_colors()
    {
        QImage image = make_image({
            {0xff000001, 0xff000001, 0xff000003},
            {0xff000002, 0xff000003, 0xff000004},
            {0xff000004, 0xff000005, 0xff000001},
        });
        SegmentedImage segmented = segment(image);
        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 2,
            3, 2, 5,
            6, 7, 8
        );
        segmented.unique_colors();
        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 2,
            3, 2, 5,
            5, 7, 1
        );
        COMPARE_VECTOR(segmented,
            Cluster{1, 0xff000001, 3, 0},
            Cluster{2, 0xff000003, 2, 0},
            Cluster{3, 0xff000002, 1, 0},
            Cluster{5, 0xff000004, 2, 0},
            Cluster{7, 0xff000005, 1, 0},
        );
    }

    void test_unique_colors_alpha()
    {
        QImage image = make_image({
            {0x10000001, 0x10000001, 0x30000002},
            {0x20000002, 0x30000002, 0x40000001},
            {0x40000001, 0x50000002, 0x10000001},
        });
        SegmentedImage segmented = segment(image);
        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 2,
            3, 2, 5,
            6, 7, 8
        );
        segmented.unique_colors();
        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 2,
            3, 2, 5,
            5, 7, 1
        );
        COMPARE_VECTOR(segmented,
            Cluster{1, 0x10000001, 3, 0},
            Cluster{2, 0x30000002, 2, 0},
            Cluster{3, 0x20000002, 1, 0},
            Cluster{5, 0x40000001, 2, 0},
            Cluster{7, 0x50000002, 1, 0},
        );
    }

    void test_unique_colors_flatten_alpha()
    {
        QImage image = make_image({
            {0x10000001, 0x10000001, 0x30000002},
            {0x20000002, 0x30000002, 0x40000001},
            {0x40000001, 0x50000002, 0x10000001},
        });
        SegmentedImage segmented = segment(image);
        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 2,
            3, 2, 5,
            6, 7, 8
        );
        segmented.unique_colors(true);
        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 2,
            2, 2, 1,
            1, 2, 1
        );
        COMPARE_VECTOR(segmented,
            Cluster{1, 0xff000001, 5, 0},
            Cluster{2, 0xff000002, 4, 0},
        );
    }

    void test_histogram()
    {
        QImage image = make_image({
            {0xff000001, 0xff000001, 0xff000003},
            {0xff000002, 0xff000003, 0xff000004},
            {0xff000004, 0xff000005, 0xff000001},
        });

        SegmentedImage segmented = segment(image);
        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 2,
            3, 2, 5,
            6, 7, 8
        );
        auto hist = segmented.histogram();
        QCOMPARE(hist.size(), 5);
        QCOMPARE(hist[0xff000001], 3);
        QCOMPARE(hist[0xff000002], 1);
        QCOMPARE(hist[0xff000003], 2);
        QCOMPARE(hist[0xff000004], 2);
        QCOMPARE(hist[0xff000005], 1);
    }

    void test_quantize()
    {
        QImage image = make_image({
            {0xffff0000, 0xffff0000, 0xffffff00, 0xffffff00, 0xffffff00},
            {0xffff0000, 0xffaaff00, 0xff00ff00, 0xff00ff00, 0xff00ff00},
            {0xffaa0000, 0xffaaff00, 0xff00aa00, 0xff00aa00, 0xff00ff00},
            {0xffaa0000, 0xffaaff00, 0xff0000ff, 0xff00aa00, 0xff00aa00},
            {0xffff00aa, 0xffff00aa, 0xff0000ff, 0xff0000ff, 0xff0000ff},
        });
        SegmentedImage segmented = segment(image);

        COMPARE_VECTOR(segmented,
            Cluster{1, 0xffff0000, 3, 0},
            Cluster{2, 0xffffff00, 3, 0},
            Cluster{3, 0xffaaff00, 3, 0},
            Cluster{4, 0xff00ff00, 4, 0},
            Cluster{5, 0xffaa0000, 2, 0},
            Cluster{6, 0xff00aa00, 4, 0},
            Cluster{7, 0xff0000ff, 4, 0},
            Cluster{8, 0xffff00aa, 2, 0},
        );

        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 2, 2, 2,
            1, 3, 4, 4, 4,
            5, 3, 6, 6, 4,
            5, 3, 7, 6, 6,
            8, 8, 7, 7, 7,
        );

        auto quantized = segmented;
        quantized.quantize({0xffff0000, 0xffffff00, 0xff00ff00, 0xff0000aa});

        COMPARE_VECTOR(quantized,
            Cluster{9, 0xffff0000, 7, 0},
            Cluster{10, 0xffffff00, 6, 0},
            Cluster{11, 0xff00ff00, 8, 0},
            Cluster{12, 0xff0000aa, 4, 0},
        );

        COMPARE_VECTOR(quantized.bitmap(),
            9,  9, 10, 10, 10,
            9, 10, 11, 11, 11,
            9, 10, 11, 11, 11,
            9, 10, 12, 11, 11,
            9,  9, 12, 12, 12,
        );
    }

    void test_dilate()
    {
        SegmentedImage segmented(7, 7);
        segmented.bitmap() = {
            2, 2, 1, 1, 1, 1, 1,
            1, 2, 2, 1, 1, 1, 1,
            1, 1, 2, 1, 2, 2, 1,
            1, 1, 2, 1, 2, 2, 1,
            1, 1, 2, 1, 2, 1, 1,
            1, 0, 2, 2, 2, 1, 1,
            1, 0, 0, 1, 2, 2, 2,
        };
        segmented.add_cluster(0x01, 28);
        segmented.add_cluster(0x02, 18);
        segmented.dilate(2);

        COMPARE_VECTOR(segmented.bitmap(),
            2, 2, 2, 2, 1, 1, 1,
            2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2,
            1, 2, 2, 2, 2, 2, 2,
            1, 2, 2, 2, 2, 2, 2,
            1, 0, 2, 2, 2, 2, 2,
            1, 0, 0, 2, 2, 2, 2,
        );
        COMPARE_VECTOR(segmented,
            Cluster{1, 0x01, 7, 0},
            Cluster{2, 0x02, 39, 0},
        );
    }

    void test_dilate_protect()
    {
        SegmentedImage segmented(7, 7);
        segmented.bitmap() = {
            2, 2, 1, 1, 1, 1, 1,
            1, 2, 2, 1, 1, 1, 1,
            1, 1, 2, 1, 2, 2, 3,
            1, 1, 2, 1, 2, 2, 3,
            4, 4, 2, 1, 2, 1, 3,
            4, 0, 2, 2, 2, 1, 1,
            1, 0, 0, 1, 2, 2, 2,
        };
        segmented.add_cluster(0x01, 22);
        segmented.add_cluster(0x02, 18);
        segmented.add_cluster(0x03, 3);
        segmented.add_cluster(0x04, 3);

        segmented.dilate(2, 4);

        COMPARE_VECTOR(segmented.bitmap(),
            2, 2, 1, 1, 1, 1, 1,
            1, 2, 2, 1, 1, 1, 1,
            1, 1, 2, 1, 2, 2, 2,
            1, 1, 2, 1, 2, 2, 2,
            4, 2, 2, 1, 2, 1, 2,
            4, 0, 2, 2, 2, 1, 1,
            1, 0, 0, 1, 2, 2, 2,
        );
        COMPARE_VECTOR(segmented,
            Cluster{1, 0x01, 22, 0},
            Cluster{2, 0x02, 22, 0},
            Cluster{3, 0x03, 0, 0},
            Cluster{4, 0x04, 2, 0},
        );
    }

    void test_perimeter()
    {
        SegmentedImage segmented(5, 5);
        segmented.bitmap() = {
            0, 1, 1, 1, 1,
            0, 1, 2, 1, 1,
            0, 2, 2, 1, 0,
            0, 2, 1, 1, 1,
            0, 0, 1, 1, 1,
        };
        segmented.add_cluster(0x01);
        segmented.add_cluster(0x02);
        QCOMPARE(segmented.perimeter(1), 13);
        QCOMPARE(segmented.perimeter(2), 4);
        segmented.bitmap() = {
            0, 1, 1, 1, 1,
            0, 3, 3, 3, 3,
            0, 3, 3, 3, 3,
            2, 3, 3, 3, 3,
            2, 3, 3, 3, 3,
        };
        segmented.add_cluster(0x03);
        QCOMPARE(segmented.perimeter(3), 12);
    }

    void test_to_image()
    {
        QImage image = make_image({
            {0xff000001, 0xff000001, 0xff000003},
            {0xff000002, 0xff000003, 0xff000004},
            {0xff000004, 0x0, 0xff000001},
        });
        SegmentedImage segmented = segment(image);
        segmented.erase_if([](const Cluster& c) { return c.color == 0; });

        QImage output = segmented.to_image();
        QCOMPARE(output.size(), image.size());
        QCOMPARE(output.format(), QImage::Format_ARGB32);
        QCOMPARE(image.format(), QImage::Format_ARGB32);
        for ( int i = 0; i < output.width() * output.height() * 4; i++ )
            QCOMPARE(output.constBits()[i], image.constBits()[i]);
    }

    void benchmark_segment_data()
    {
        benchmark_data();
    }

    void benchmark_segment()
    {
        QFETCH(QString, image_path);
        QImage image(image_path);
        if ( image.isNull() )
        {
            auto msg = ("Wrong path: " + image_path).toStdString();
            QFAIL(msg.c_str());
        }

        QBENCHMARK
        {
            segment(image);
        }
    }

    void benchmark_unique_colors_data()
    {
        benchmark_data();
    }

    void benchmark_unique_colors()
    {
        QFETCH(QString, image_path);
        QImage image(image_path);
        if ( image.isNull() )
        {
            auto msg = ("Wrong path: " + image_path).toStdString();
            QFAIL(msg.c_str());
        }
        auto segmented = segment(image);

        QBENCHMARK
        {
            auto copy = segmented;
            copy.unique_colors();
        }
    }

    void test_neighbours_none()
    {
        SegmentedImage segmented(5, 5);
        segmented.bitmap() = {
            1, 1, 1, 1, 1,
            1, 1, 1, 1, 1,
            1, 1, 1, 1, 1,
            1, 1, 1, 1, 1,
            1, 1, 1, 1, 1,
        };
        segmented.add_cluster(0x1, 25);
        QCOMPARE(segmented.neighbours(1).size(), 0);
    }

    void test_neighbours_none_void()
    {
        SegmentedImage segmented(5, 5);
        segmented.bitmap() = {
            1, 1, 1, 1, 1,
            1, 1, 1, 1, 1,
            1, 1, 1, 0, 1,
            0, 0, 1, 0, 1,
            0, 1, 1, 1, 1,
        };
        segmented.add_cluster(0x1, 20);
        QCOMPARE(segmented.neighbours(1).size(), 0);
    }

    void test_neighbours()
    {
        SegmentedImage segmented(5, 5);
        segmented.bitmap() = {
            1, 3, 1, 1, 1,
            3, 3, 1, 1, 1,
            1, 3, 3, 2, 1,
            2, 2, 1, 2, 1,
            2, 1, 1, 1, 1,
        };
        segmented.add_cluster(0x1, 15);
        segmented.add_cluster(0x2, 5);
        segmented.add_cluster(0x2, 5);
        COMPARE_VECTOR(segmented.neighbours(1), 2, 3);
    }


    void test_direct_merge()
    {
        QImage image = make_image({
            {0xff000001, 0xff000001, 0xff000003},
            {0xff000002, 0xff000003, 0xff000004},
            {0xff000004, 0xff000005, 0xff000001},
        });
        SegmentedImage segmented = segment(image);
        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 2,
            3, 2, 5,
            6, 7, 8
        );
        segmented.direct_merge(1, 2);
        COMPARE_VECTOR(segmented.bitmap(),
            2, 2, 2,
            3, 2, 5,
            6, 7, 8
        );
        COMPARE_VECTOR(segmented,
            Cluster{2, 0xff000003, 4, 0},
            Cluster{3, 0xff000002, 1, 0},
            Cluster{5, 0xff000004, 1, 0},
            Cluster{6, 0xff000004, 1, 0},
            Cluster{7, 0xff000005, 1, 0},
            Cluster{8, 0xff000001, 1, 0},
        );
    }

    void test_cluster_merge_simple()
    {
        QImage image = make_image({
            {0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff},
            {0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff},
            {0xff0000ff, 0xff0000ff, 0xff00ff00, 0xff00ff00, 0xff00ff00},
            {0xff0000ff, 0xff00ffff, 0xff00ff00, 0xff00ff00, 0xff00ff00},
            {0xff0000ff, 0xff00ffff, 0xff00ff00, 0xff00ff00, 0xff00ff00},
        });
        SegmentedImage segmented = segment(image);
        COMPARE_VECTOR(segmented,
            Cluster{1, 0xff0000ff, 14, 0},
            Cluster{2, 0xff00ff00, 9, 0},
            Cluster{3, 0xff00ffff, 2, 0},
        );
        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 1, 1, 1,
            1, 1, 1, 1, 1,
            1, 1, 2, 2, 2,
            1, 3, 2, 2, 2,
            1, 3, 2, 2, 2,
        );
        auto colors = cluster_merge(segmented, 10).colors;
        COMPARE_VECTOR(colors, 0xff0000ff, 0xff00ff00);
        COMPARE_VECTOR(segmented,
            Cluster{1, 0xff0000ff, 16, 0},
            Cluster{2, 0xff00ff00, 9, 0},
        );
        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 1, 1, 1,
            1, 1, 1, 1, 1,
            1, 1, 2, 2, 2,
            1, 1, 2, 2, 2,
            1, 1, 2, 2, 2,
        );
    }

    void test_cluster_merge_antialias()
    {
        QRgb color_back = 0xff3250b0;
        QRgb color_ellipse = 0xffc4d9f5;
        QImage image(256, 256, QImage::Format_ARGB32);
        image.fill(color_back);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor::fromRgba(color_ellipse));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(255, 255), 100, 100);
        painter.end();

        auto segmented = segment(image);
        QVERIFY(segmented.size() > 2);
        QCOMPARE(segmented.cluster(0, 0)->color, color_back);
        QCOMPARE(segmented.cluster(255, 255)->color, color_ellipse);
        int id_bg = 1;
        int id_circle = segmented.cluster_id(255, 255);

        auto colors = cluster_merge(segmented, 10).colors;

        COMPARE_VECTOR(colors, color_back, color_ellipse);
        COMPARE_VECTOR(segmented,
            Cluster{id_bg, color_back, segmented.cluster(id_bg)->size},
            Cluster{id_circle, color_ellipse, segmented.cluster(id_circle)->size},
        );
        QCOMPARE(segmented.size(), 2);
        QCOMPARE(segmented.cluster(id_bg)->color, color_back);
        QCOMPARE(segmented.cluster(id_circle)->color, color_ellipse);
    }

    void test_cluster_merge_noise()
    {
        QRgb color_back = 0xff3250b0;
        QRgb color_ellipse = 0xffc4d9f5;
        QImage image(256, 256, QImage::Format_ARGB32);
        image.fill(color_back);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor::fromRgba(color_ellipse));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(255, 255), 100, 100);
        for ( int i = 0; i < 20; i ++ )
        {
            painter.fillRect(i*10, 10, 2, 6, Qt::white);
        }

        painter.end();

        auto segmented = segment(image);
        QVERIFY(segmented.size() > 2);
        QCOMPARE(segmented.cluster(0, 0)->color, color_back);
        QCOMPARE(segmented.cluster(255, 255)->color, color_ellipse);
        int id_bg = 1;
        int id_circle = segmented.cluster_id(255, 255);

        auto colors = cluster_merge(segmented, 10).colors;

        QCOMPARE(segmented.cluster_id(255, 255), id_circle);
        QCOMPARE(segmented.cluster_id(0, 0), id_bg);

        COMPARE_VECTOR(segmented,
            Cluster{id_bg, color_back, segmented.cluster(id_bg)->size},
            Cluster{id_circle, color_ellipse, segmented.cluster(id_circle)->size},
        );
        COMPARE_VECTOR(colors, color_back, color_ellipse);
        QCOMPARE(segmented.size(), 2);
        QCOMPARE(segmented.cluster(id_bg)->color, color_back);
        QCOMPARE(segmented.cluster(id_circle)->color, color_ellipse);
    }

    void test_cluster_merge_gradient()
    {
        QImage image(256, 256, QImage::Format_ARGB32);

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);

        QLinearGradient gradient(0, 0, 0, 256);
        gradient.setColorAt(0, 0xffff0000);
        gradient.setColorAt(1, 0xff000000);
        painter.setBrush(gradient);
        painter.setPen(Qt::NoPen);
        painter.drawRect(0, 0, 256, 256);
        painter.end();

        auto segmented = segment(image);
        QCOMPARE(segmented.size(), 256);
        QCOMPARE(segmented.histogram().size(), 256);
        auto colors = cluster_merge(segmented, 1024).colors;
        QCOMPARE(colors.size(), 1);
        QCOMPARE(segmented.size(), 1);
    }

    void test_cluster_merge_gradient_diagonal()
    {
        QImage image(256, 256, QImage::Format_ARGB32);

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);

        QLinearGradient gradient(0, 0, 256, 256);
        gradient.setColorAt(0, 0xffff0000);
        gradient.setColorAt(1, 0xff000000);
        painter.setBrush(gradient);
        painter.setPen(Qt::NoPen);
        painter.drawRect(0, 0, 256, 256);
        painter.end();

        auto segmented = segment(image);
        QVERIFY(segmented.size() > 2);
        QVERIFY(segmented.histogram().size() > 2);

        auto colors = cluster_merge(segmented, 1024).colors;

        // Ideally it should be 1 but 3 is good enough
        QCOMPARE(colors.size(), 3);
        QCOMPARE(segmented.size(), 3);
    }

    void test_gradient_stops()
    {
        std::vector<std::pair<float, StructuredColor>> gradient{
            {0.00, 0xffff0000},
            {0.75, 0xffffff00},
            {1.00, 0xff000000},
        };
        std::vector<StructuredColor> colors = make_gradient_colors(gradient);

        auto stops = gradient_stops(colors);
        std::vector<std::pair<float, StructuredColor>> out_gradient;
        for ( const auto& stop : stops )
            out_gradient.emplace_back(stop.first, stop.second.rgba());

        COMPARE_VECTOR(out_gradient, gradient);

    }

    void test_gradient_stops_too_many_pixels()
    {
        std::vector<std::pair<float, StructuredColor>> gradient{
            {0.00, 0xffff0000},
            {0.75, 0xffffff00},
            {1.00, 0xff000000},
        };
        std::vector<StructuredColor> colors = make_gradient_colors(gradient, 2048);

        auto stops = gradient_stops(colors);
        std::vector<std::pair<float, StructuredColor>> out_gradient;
        for ( const auto& stop : stops )
            out_gradient.emplace_back(stop.first, stop.second.rgba());

        COMPARE_VECTOR(out_gradient, gradient);
    }

    void test_gradient_stops_not_enough_pixels()
    {
        std::vector<std::pair<float, StructuredColor>> gradient{
            {0.00, 0xffff0000},
            {0.75, 0xffffff00},
            {1.00, 0xff000000},
        };
        std::vector<StructuredColor> colors = make_gradient_colors(gradient, 32);

        auto stops = gradient_stops(colors);
        std::vector<std::pair<float, StructuredColor>> out_gradient;
        for ( const auto& stop : stops )
            out_gradient.emplace_back(stop.first, stop.second.rgba());

        COMPARE_VECTOR(out_gradient, gradient);
    }

    void test_gradient_stops_subtle()
    {
        std::vector<std::pair<float, StructuredColor>> gradient{
            {0.00, 0xffff8800},
            {0.75, 0xffcccc00},
            {1.00, 0xff88ff00},
        };
        std::vector<StructuredColor> colors = make_gradient_colors(gradient);

        auto stops = gradient_stops(colors);
        std::vector<std::pair<float, StructuredColor>> out_gradient;
        for ( const auto& stop : stops )
            out_gradient.emplace_back(stop.first, stop.second.rgba());

        COMPARE_VECTOR(out_gradient, gradient);

    }

};

QTEST_GUILESS_MAIN(TestSegmentation)
#include "test_segmentation.moc"
