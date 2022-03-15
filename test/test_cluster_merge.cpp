#include <QtTest/QtTest>
#include <QPainter>

#include "trace/segmentation.hpp"
#include "trace/quantize.hpp"
#include "test_trace.hpp"

class TestClusterMerge: public QObject
{
    Q_OBJECT

private slots:
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
};

QTEST_GUILESS_MAIN(TestClusterMerge)
#include "test_cluster_merge.moc"

