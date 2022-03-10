#include <QtTest/QtTest>
#include <filesystem>

#include "trace/quantize.hpp"

using namespace glaxnimate::trace;


class TestTrace: public QObject
{
    Q_OBJECT

private:

private slots:

    void benchmark_eem()
    {
        auto path = std::filesystem::path(__FILE__).parent_path().parent_path() / "data" / "trace" / "images" / "flat.png";
        QImage image(QString::fromStdString(path.u8string()));
        auto segmented = segment(image);

        QBENCHMARK
        {
            auto seg = segmented;
            edge_exclusion_modes(seg, 256);
        }
    }

    void benchmark_octree()
    {
        auto path = std::filesystem::path(__FILE__).parent_path().parent_path() / "data" / "trace" / "images" / "flat.png";
        QImage image(QString::fromStdString(path.u8string()));
        auto segmented = segment(image);

        QBENCHMARK
        {
            auto seg = segmented;
            octree(seg, 16);
        }
    }

    void benchmark_kmeans()
    {
        auto path = std::filesystem::path(__FILE__).parent_path().parent_path() / "data" / "trace" / "images" / "flat.png";
        QImage image(QString::fromStdString(path.u8string()));
        auto segmented = segment(image);

        QBENCHMARK
        {
            auto seg = segmented;
            k_means(seg, 16, 100, KMeansMatch::Closest);
        }
    }
};

QTEST_GUILESS_MAIN(TestTrace)
#include "test_trace.moc"


