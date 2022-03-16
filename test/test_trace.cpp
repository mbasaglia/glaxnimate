#include <QtTest/QtTest>

#include "trace/quantize.hpp"
#include "trace/trace.hpp"
#include "test_trace.hpp"

class TestTrace: public QObject
{
    Q_OBJECT

private:

private slots:
    void initTestCase_data()
    {
        benchmark_data();
    }


    void benchmark_eem()
    {
        QFETCH_GLOBAL(QString, image_path);
        QImage image(image_path);
        if ( image.isNull() )
        {
            auto msg = ("Wrong path: " + image_path).toStdString();
            QFAIL(msg.c_str());
        }
        auto segmented = segment(image);

        QBENCHMARK
        {
            auto seg = segmented;
            edge_exclusion_modes(seg, 256);
        }
    }

    void benchmark_octree()
    {
        QFETCH_GLOBAL(QString, image_path);
        QImage image(image_path);
        if ( image.isNull() )
        {
            auto msg = ("Wrong path: " + image_path).toStdString();
            QFAIL(msg.c_str());
        }
        auto segmented = segment(image);

        QBENCHMARK
        {
            auto seg = segmented;
            octree(seg, 16);
        }
    }

    void benchmark_kmeans()
    {
        QFETCH_GLOBAL(QString, image_path);
        QImage image(image_path);
        if ( image.isNull() )
        {
            auto msg = ("Wrong path: " + image_path).toStdString();
            QFAIL(msg.c_str());
        }
        auto segmented = segment(image);

        QBENCHMARK
        {
            auto seg = segmented;
            k_means(seg, 16, 100, KMeansMatch::Closest);
        }
    }

    void benchmark_cluster_merge()
    {
        QFETCH_GLOBAL(QString, image_path);
        QImage image(image_path);
        if ( image.isNull() )
        {
            auto msg = ("Wrong path: " + image_path).toStdString();
            QFAIL(msg.c_str());
        }
        auto segmented = segment(image);

        QBENCHMARK
        {
            auto seg = segmented;
            cluster_merge(seg, 256);
        }
    }

    void benchmark_segmentation_eem()
    {
        QFETCH_GLOBAL(QString, image_path);
        QImage image(image_path);
        if ( image.isNull() )
        {
            auto msg = ("Wrong path: " + image_path).toStdString();
            QFAIL(msg.c_str());
        }
        auto segmented = segment(image);

        QBENCHMARK
        {
            auto seg = segmented;
            auto colors = edge_exclusion_modes(seg, 256);
            seg.quantize(colors);
        }
    }

    void benchmark_segmentation_octree()
    {
        QFETCH_GLOBAL(QString, image_path);
        QImage image(image_path);
        if ( image.isNull() )
        {
            auto msg = ("Wrong path: " + image_path).toStdString();
            QFAIL(msg.c_str());
        }
        auto segmented = segment(image);

        QBENCHMARK
        {
            auto seg = segmented;
            auto colors = octree(seg, 16);
            seg.quantize(colors);
        }
    }

    void benchmark_segmentation_kmeans()
    {
        QFETCH_GLOBAL(QString, image_path);
        QImage image(image_path);
        if ( image.isNull() )
        {
            auto msg = ("Wrong path: " + image_path).toStdString();
            QFAIL(msg.c_str());
        }
        auto segmented = segment(image);

        QBENCHMARK
        {
            auto seg = segmented;
            auto colors = k_means(seg, 16, 100, KMeansMatch::Closest);
            seg.quantize(colors);
        }
    }

    void benchmark_segmentation_cluster_merge()
    {
        QFETCH_GLOBAL(QString, image_path);
        QImage image(image_path);
        if ( image.isNull() )
        {
            auto msg = ("Wrong path: " + image_path).toStdString();
            QFAIL(msg.c_str());
        }
        auto segmented = segment(image);

        QBENCHMARK
        {
            auto seg = segmented;
            cluster_merge(seg, 256);
        }
    }

    void benchmark_quantize()
    {
        QFETCH_GLOBAL(QString, image_path);
        QImage image(image_path);
        if ( image.isNull() )
        {
            auto msg = ("Wrong path: " + image_path).toStdString();
            QFAIL(msg.c_str());
        }
        auto segmented = segment(image);
        auto colors = edge_exclusion_modes(segmented, 256);

        QBENCHMARK
        {
            auto seg = segmented;
            seg.quantize(colors);
        }
    }

    void benchmark_trace()
    {
        QFETCH_GLOBAL(QString, image_path);
        QImage image(image_path);
        if ( image.isNull() )
        {
            auto msg = ("Wrong path: " + image_path).toStdString();
            QFAIL(msg.c_str());
        }
        auto segmented = segment(image);
        auto colors = octree(segmented, 16);
        segmented.quantize(colors);
        Tracer tracer(segmented, {});

        QBENCHMARK
        {
            auto seg = segmented;
            for ( const auto& cluster : segmented )
            {
                tracer.set_target_cluster(cluster.id);
                glaxnimate::math::bezier::MultiBezier bez;
                tracer.trace(bez);
            }
        }
    }
};

QTEST_GUILESS_MAIN(TestTrace)
#include "test_trace.moc"


