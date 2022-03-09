#include <QtTest/QtTest>

#include "trace/segmentation.hpp"
#include <QDebug>
#include <iostream>

extern int debug_segmentation;
using namespace glaxnimate::trace;

bool operator==(const Cluster& a, const Cluster& b)
{
    return a.id == b.id && a.color == b.color && a.size == b.size && a.merge_target == b.merge_target;
}

QString to_qstring(int a)
{
    return QString::number(a);
}

QString to_qstring(const Cluster& a)
{
    return QString("Cluster { id = %1 color = %2 size = %3 merge_target = %4 }")
        .arg(a.id)
        .arg("0x" + QString::number(a.color, 16).rightJustified(8, '0'))
        .arg(a.size)
        .arg(a.merge_target)
    ;
}


template<class T>
QString to_string(const std::vector<T>& a)
{
    QString str = QString("vector[%1] = { ").arg(a.size());
    for ( const auto & cluster : a )
    {
        str += to_qstring(cluster) += ", ";
    }
    str += "}";
    return str;
}

namespace QTest {

template <> char* toString(const Cluster& a)
{
    return QTest::toString(to_qstring(a));
}

} // namespace QTest

template<class T>
bool operator==(const std::vector<T>& a, const std::vector<T>& b)
{
    if ( a.size() != b.size() )
        return false;

    for ( int i = 0, e = a.size(); i != e; ++i )
        if ( !(a[i] == b[i]) )
            return false;

    return true;
}



template<class T1, class T2>
bool better_compare(const T1& val1, const T2& val2, const QString& name1, const QString& name2, const char* file, int line)
{
    using namespace QTest;

    if ( val1 == val2 )
        return true;

    auto len = qMax(name1.size(), name2.size());
    QTest::qFail("Compared values are not the same", file, line);

    QString message = QString("   Actual   (%1): %2\n   Expected (%3): %4\n")
        .arg(name1.leftJustified(len))
        .arg(to_string(val1))
        .arg(name2.leftJustified(len))
        .arg(to_string(val2))
    ;
    // Using C++ streams because it's really hard to avoid QTest messing up the output
    std::cerr << message.toStdString();

    return false;
}

#define COMPARE_VECTOR(actual, ...) \
    do {\
        if (!better_compare(actual, std::decay_t<decltype(actual)>{__VA_ARGS__}, #actual, "", __FILE__, __LINE__))\
            return;\
    } while (false)

class TestSegmentation: public QObject
{
    Q_OBJECT

private:
    QImage make_image(const std::vector<std::vector<QRgb>>& pixels)
    {
        QImage image(pixels[0].size(), pixels.size(), QImage::Format_RGBA8888);
        for ( int y = 0; y < image.height(); y++ )
            for ( int x = 0; x < image.width(); x++ )
                image.setPixel(x, y, pixels[y][x]);
        return image;
    }

private slots:
    void test_add_cluster()
    {
        SegmentedImage img(1, 1);
        img.add_cluster(0xffff0000);
        COMPARE_VECTOR(img.clusters(), Cluster{1, 0xffff0000, 1, 0});
        img.add_cluster(0xff00ff00, 3);
        COMPARE_VECTOR(img.clusters(),
            Cluster{1, 0xffff0000, 1, 0},
            Cluster{2, 0xff00ff00, 3, 0}
        );
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
        COMPARE_VECTOR(img.clusters(),
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
        img.cluster(1)->merge_target = 3;
        img.cluster(3)->merge_sources = {1};
        img.merge(img.cluster(1), img.cluster(2));
        COMPARE_VECTOR(img.clusters(),
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
        img.clusters()[1].merge_target = 3;
        img.clusters()[2].merge_sources = {2};
        img.merge(img.cluster(1), img.cluster(2));
        COMPARE_VECTOR(img.clusters(),
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
        img.clusters()[0].merge_target = 4;
        img.clusters()[3].merge_sources = {1};
        img.clusters()[1].merge_target = 3;
        img.clusters()[2].merge_sources = {2};
        img.merge(img.cluster(1), img.cluster(2));
        COMPARE_VECTOR(img.clusters(),
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
        COMPARE_VECTOR(img.clusters(),
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
        img.clusters()[0].merge_target = 2;
        img.clusters()[2].merge_sources = {1};
        img.merge(img.cluster(1), img.cluster(2));
        COMPARE_VECTOR(img.clusters(),
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
        QCOMPARE(img.cluster(0, 0), &img.clusters()[1-1]);
        QCOMPARE(img.cluster(1, 0), &img.clusters()[1-1]);
        QCOMPARE(img.cluster(2, 0), &img.clusters()[1-1]);
        QCOMPARE(img.cluster(0, 1), &img.clusters()[2-1]);
        QCOMPARE(img.cluster(1, 1), nullptr);
        QCOMPARE(img.cluster(2, 1), &img.clusters()[4-1]);
        QCOMPARE(img.cluster(0, 2), &img.clusters()[3-1]);
        QCOMPARE(img.cluster(1, 2), &img.clusters()[4-1]);
        QCOMPARE(img.cluster(2, 2), &img.clusters()[2-1]);
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
        COMPARE_VECTOR(img.clusters(),
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
        img.cluster(2)->merge_target = 4;
        img.cluster(3)->merge_target = 1;
        img.normalize();
        COMPARE_VECTOR(img.bitmap(),
            1, 1, 1,
            1, 0, 2,
            1, 2, 2
        );
        COMPARE_VECTOR(img.clusters(),
            Cluster{1, 0x1, 5, 0},
            Cluster{2, 0x4, 3, 0},
        );
    }

    void test_segment_all_unique()
    {
        QImage image(3, 3, QImage::Format_RGBA8888);
        image.setPixel(0, 0, 0xff000000);
        image.setPixel(1, 0, 0xff000001);
        image.setPixel(2, 0, 0xff000002);
        image.setPixel(0, 1, 0xff000010);
        image.setPixel(1, 1, 0xff000011);
        image.setPixel(2, 1, 0xff000012);
        image.setPixel(0, 2, 0xff000020);
        image.setPixel(1, 2, 0xff000021);
        image.setPixel(2, 2, 0xff000022);
        SegmentedImage segmented = segment(image);
        COMPARE_VECTOR(segmented.clusters(),
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

    void test_segment_alpha_threshold()
    {
        QImage image(3, 3, QImage::Format_RGBA8888);
        image.setPixel(0, 0, 0x10000000);
        image.setPixel(1, 0, 0x30000001);
        image.setPixel(2, 0, 0x50000002);
        image.setPixel(0, 1, 0x70000010);
        image.setPixel(1, 1, 0x90000011);
        image.setPixel(2, 1, 0xb0000012);
        image.setPixel(0, 2, 0xd0000020);
        image.setPixel(1, 2, 0xf0000021);
        image.setPixel(2, 2, 0xff000022);
        SegmentedImage segmented = segment(image, 128);
        COMPARE_VECTOR(segmented.clusters(),
            Cluster{1, 0x90000011, 1, 0},
            Cluster{2, 0xb0000012, 1, 0},
            Cluster{3, 0xd0000020, 1, 0},
            Cluster{4, 0xf0000021, 1, 0},
            Cluster{5, 0xff000022, 1, 0},
        );
    }

    void test_segment_join_left()
    {
        QImage image(3, 3, QImage::Format_RGBA8888);
        image.setPixel(0, 0, 0xff000001);
        image.setPixel(1, 0, 0xff000001);
        image.setPixel(2, 0, 0xff000001);
        image.setPixel(0, 1, 0x0);
        image.setPixel(1, 1, 0x0);
        image.setPixel(2, 1, 0x0);
        image.setPixel(0, 2, 0xff000001);
        image.setPixel(1, 2, 0xff000001);
        image.setPixel(2, 2, 0x0);
        SegmentedImage segmented = segment(image);
        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 1,
            0, 0, 0,
            2, 2, 0
        );
        COMPARE_VECTOR(segmented.clusters(),
            Cluster{1, 0xff000001, 3, 0},
            Cluster{2, 0xff000001, 2, 0},
        );
    }

    void test_segment_join_up()
    {
        QImage image(3, 3, QImage::Format_RGBA8888);
        image.setPixel(0, 0, 0xff000001);
        image.setPixel(1, 0, 0x0);
        image.setPixel(2, 0, 0xff000001);
        image.setPixel(0, 1, 0xff000001);
        image.setPixel(1, 1, 0x0);
        image.setPixel(2, 1, 0xff000001);
        image.setPixel(0, 2, 0xff000001);
        image.setPixel(1, 2, 0x0);
        image.setPixel(2, 2, 0x0);
        SegmentedImage segmented = segment(image);
        COMPARE_VECTOR(segmented.bitmap(),
            1, 0, 2,
            1, 0, 2,
            1, 0, 0
        );
        COMPARE_VECTOR(segmented.clusters(),
            Cluster{1, 0xff000001, 3, 0},
            Cluster{2, 0xff000001, 2, 0},
        );
    }

    void test_segment_join_diag_1_corner()
    {
        QImage image(3, 3, QImage::Format_RGBA8888);
        image.setPixel(0, 0, 0x0);
        image.setPixel(1, 0, 0x0);
        image.setPixel(2, 0, 0xff000001);
        image.setPixel(0, 1, 0x0);
        image.setPixel(1, 1, 0xff000001);
        image.setPixel(2, 1, 0xff000001);
        image.setPixel(0, 2, 0xff000001);
        image.setPixel(1, 2, 0xff000001);
        image.setPixel(2, 2, 0x0);
        SegmentedImage segmented = segment(image);
        COMPARE_VECTOR(segmented.bitmap(),
            0, 0, 1,
            0, 1, 1,
            1, 1, 0
        );
        COMPARE_VECTOR(segmented.clusters(),
            Cluster{1, 0xff000001, 5, 0},
        );
    }

    void test_segment_join_diag_1_no_corner()
    {
        QImage image(3, 3, QImage::Format_RGBA8888);
        image.setPixel(0, 0, 0x0);
        image.setPixel(1, 0, 0x0);
        image.setPixel(2, 0, 0xff000001);
        image.setPixel(0, 1, 0x0);
        image.setPixel(1, 1, 0xff000001);
        image.setPixel(2, 1, 0x0);
        image.setPixel(0, 2, 0xff000001);
        image.setPixel(1, 2, 0x0);
        image.setPixel(2, 2, 0x0);
        SegmentedImage segmented = segment(image);
        COMPARE_VECTOR(segmented.bitmap(),
            0, 0, 1,
            0, 1, 0,
            1, 0, 0
        );
        COMPARE_VECTOR(segmented.clusters(),
            Cluster{1, 0xff000001, 3, 0},
        );
    }

    void test_segment_join_diag_2()
    {
        QImage image(3, 3, QImage::Format_RGBA8888);
        image.setPixel(0, 0, 0xff000001);
        image.setPixel(1, 0, 0x0);
        image.setPixel(2, 0, 0x0);
        image.setPixel(0, 1, 0x0);
        image.setPixel(1, 1, 0xff000001);
        image.setPixel(2, 1, 0x0);
        image.setPixel(0, 2, 0x0);
        image.setPixel(1, 2, 0x0);
        image.setPixel(2, 2, 0xff000001);
        SegmentedImage segmented = segment(image);
        COMPARE_VECTOR(segmented.bitmap(),
            1, 0, 0,
            0, 1, 0,
            0, 0, 1
        );
        COMPARE_VECTOR(segmented.clusters(),
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
            1, 1, 3,
            2, 3, 4,
            5, 6, 7
        );
        segmented.unique_colors();
        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 3,
            2, 3, 4,
            4, 5, 1
        );
        COMPARE_VECTOR(segmented.clusters(),
            Cluster{1, 0xff000001, 3, 0},
            Cluster{2, 0xff000002, 1, 0},
            Cluster{3, 0xff000003, 2, 0},
            Cluster{4, 0xff000004, 2, 0},
            Cluster{5, 0xff000005, 1, 0},
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
            1, 1, 3,
            2, 3, 4,
            5, 6, 7
        );
        segmented.unique_colors();
        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 3,
            2, 3, 4,
            4, 5, 1
        );
        COMPARE_VECTOR(segmented.clusters(),
            Cluster{1, 0x10000001, 3, 0},
            Cluster{2, 0x20000002, 1, 0},
            Cluster{3, 0x30000002, 2, 0},
            Cluster{4, 0x40000001, 2, 0},
            Cluster{5, 0x50000002, 1, 0},
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
            1, 1, 3,
            2, 3, 4,
            5, 6, 7
        );
        segmented.unique_colors(true);
        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 2,
            2, 2, 1,
            1, 2, 1
        );
        COMPARE_VECTOR(segmented.clusters(),
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
            1, 1, 3,
            2, 3, 4,
            5, 6, 7
        );
        auto hist = segmented.histogram();
        QCOMPARE(hist.size(), 5);
        QCOMPARE(hist[0xff000001], 3);
        QCOMPARE(hist[0xff000002], 1);
        QCOMPARE(hist[0xff000003], 2);
        QCOMPARE(hist[0xff000004], 2);
        QCOMPARE(hist[0xff000005], 1);
    }

    void test_fix_cluster_ids()
    {
        QImage image = make_image({
            {0xff000001, 0xff000001, 0xff000003},
            {0xff000002, 0xff000003, 0xff000004},
            {0xff000004, 0xff000005, 0xff000001},
        });

        SegmentedImage segmented = segment(image);
        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 3,
            2, 3, 4,
            5, 6, 7
        );
        COMPARE_VECTOR(segmented.clusters(),
            Cluster{1, 0xff000001, 2, 0},
            Cluster{2, 0xff000002, 1, 0},
            Cluster{3, 0xff000003, 2, 0},
            Cluster{4, 0xff000004, 1, 0},
            Cluster{5, 0xff000004, 1, 0},
            Cluster{6, 0xff000005, 1, 0},
            Cluster{7, 0xff000001, 1, 0},
        );

        std::reverse(segmented.clusters().begin(), segmented.clusters().end());
        segmented.fix_cluster_ids();

        COMPARE_VECTOR(segmented.bitmap(),
            7, 7, 5,
            6, 5, 4,
            3, 2, 1
        );
        COMPARE_VECTOR(segmented.clusters(),
            Cluster{1, 0xff000001, 1, 0},
            Cluster{2, 0xff000005, 1, 0},
            Cluster{3, 0xff000004, 1, 0},
            Cluster{4, 0xff000004, 1, 0},
            Cluster{5, 0xff000003, 2, 0},
            Cluster{6, 0xff000002, 1, 0},
            Cluster{7, 0xff000001, 2, 0},
        );
    }

    void test_sort()
    {
        QImage image = make_image({
            {0xff000001, 0xff000001, 0xff000003},
            {0xff000002, 0xff000003, 0xff000004},
            {0xff000004, 0xff000005, 0xff000001},
        });

        SegmentedImage segmented = segment(image);
        segmented.unique_colors();

        COMPARE_VECTOR(segmented.bitmap(),
            1, 1, 3,
            2, 3, 4,
            4, 5, 1
        );
        COMPARE_VECTOR(segmented.clusters(),
            Cluster{1, 0xff000001, 3, 0},
            Cluster{2, 0xff000002, 1, 0},
            Cluster{3, 0xff000003, 2, 0},
            Cluster{4, 0xff000004, 2, 0},
            Cluster{5, 0xff000005, 1, 0},
        );

        segmented.sort_clusters([](const Cluster& a, const Cluster& b){
            return a.size < b.size;
        });

        COMPARE_VECTOR(segmented.bitmap(),
            5, 5, 3,
            1, 3, 4,
            4, 2, 5
        );
        COMPARE_VECTOR(segmented.clusters(),
            Cluster{1, 0xff000002, 1, 0},
            Cluster{2, 0xff000005, 1, 0},
            Cluster{3, 0xff000003, 2, 0},
            Cluster{4, 0xff000004, 2, 0},
            Cluster{5, 0xff000001, 3, 0},
        );
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

        COMPARE_VECTOR(segmented.clusters(),
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

        COMPARE_VECTOR(quantized.clusters(),
            Cluster{1, 0xffff0000, 7, 0},
            Cluster{2, 0xffffff00, 6, 0},
            Cluster{3, 0xff00ff00, 8, 0},
            Cluster{4, 0xff0000aa, 4, 0},
        );

        COMPARE_VECTOR(quantized.bitmap(),
            1, 1, 2, 2, 2,
            1, 2, 3, 3, 3,
            1, 2, 3, 3, 3,
            1, 2, 4, 3, 3,
            1, 1, 4, 4, 4,
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
        segmented.clusters() = {
            Cluster{1, 0x01, 28, 0},
            Cluster{2, 0x02, 18, 0},
        };
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
        COMPARE_VECTOR(segmented.clusters(),
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
        segmented.clusters() = {
            Cluster{1, 0x01, 22, 0},
            Cluster{2, 0x02, 18, 0},
            Cluster{3, 0x03, 3, 0},
            Cluster{4, 0x04, 3, 0},
        };

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
        COMPARE_VECTOR(segmented.clusters(),
            Cluster{1, 0x01, 22, 0},
            Cluster{2, 0x02, 22, 0},
            Cluster{3, 0x03, 0, 0},
            Cluster{4, 0x04, 2, 0},
        );
    }
};

QTEST_GUILESS_MAIN(TestSegmentation)
#include "test_segmentation.moc"

