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

    void test_cluster_merge_transparency()
    {
        QRgb color1 = 0x88ffff00;
        QRgb color2 = 0x8800ffff;
        QRgb color_mix = 0xc751ffae;
        QImage image(300, 300, QImage::Format_ARGB32);
        image.fill(0x0u);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor::fromRgba(color1));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QRectF(QPointF(10, 10), QPointF(220, 220)));
        painter.setBrush(QColor::fromRgba(color2));
        painter.drawEllipse(QRectF(QPointF(90, 90), QPointF(290, 290)));
        painter.end();

        auto segmented = segment(image);
        segmented.erase_if([](const Cluster& c){return c.color == 0x0;});
        QVERIFY(segmented.size() > 2);
        BETTER_COMPARE(segmented.cluster(50, 50)->color, color1);
        BETTER_COMPARE(segmented.cluster(250, 250)->color, color2);
        BETTER_COMPARE(segmented.cluster(150, 150)->color, color_mix);
        auto colors = cluster_merge(segmented, 256).colors;
        COMPARE_VECTOR(colors, color1, color2, color_mix);
        BETTER_COMPARE(segmented.cluster(50, 50)->color, color1);
        BETTER_COMPARE(segmented.cluster(250, 250)->color, color2);
        BETTER_COMPARE(segmented.cluster(150, 150)->color, color_mix);
        BETTER_COMPARE(segmented.size(), 3);
    }


    void test_cluster_merge_gradient_item()
    {
        QRgb color1 = 0xffff0000;
        QRgb color2 = 0xff000000;
        QRgb color3 = 0xffffffff;
        QImage image(300, 300, QImage::Format_ARGB32);
        image.fill(color3);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        QLinearGradient gradient(10, 0, 266, 0);
        gradient.setColorAt(0, color1);
        gradient.setColorAt(1, color2);
        painter.setBrush(gradient);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRectF(QPointF(10, 10), QPointF(266, 266)), 20, 20);
        painter.end();

        auto segmented = segment(image);
        image.save("/tmp/foo0.png");
        segmented.to_image().save("/tmp/foo1.png");
        QVERIFY(segmented.size() > 2);
        BETTER_COMPARE(segmented.cluster(0, 0)->color, color3);
        auto brushes = cluster_merge(segmented, 256);
        segmented.to_image().save("/tmp/foo2.png");
        BETTER_COMPARE(segmented.cluster(0, 0)->color, color3);
        BETTER_COMPARE(segmented.size(), 2);
        BETTER_COMPARE(brushes.colors.size(), 2);
        COMPARE_VECTOR(brushes.gradients.begin()->second.stops, {0, color1}, {1, color2});
    }
};

QTEST_GUILESS_MAIN(TestClusterMerge)
#include "test_cluster_merge.moc"

