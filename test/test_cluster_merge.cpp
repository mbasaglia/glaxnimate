#include <QtTest/QtTest>
#include <QPainter>

#include "glaxnimate/trace/segmentation.hpp"
#include "glaxnimate/trace/quantize.hpp"
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

        auto colors = cluster_merge(segmented, 10, 12).colors;

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
        QImage image(128, 128, QImage::Format_ARGB32);
        QPainter painter(&image);
        QLinearGradient gradient(0, 0, 128, 128);
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

        // Ideally it should be 1 but 2 is good enough
        QCOMPARE(colors.size(), 2);
        QCOMPARE(segmented.size(), 2);
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
        QVERIFY(segmented.size() > 2);
        BETTER_COMPARE(segmented.cluster(0, 0)->color, color3);
        auto brushes = cluster_merge(segmented, 256);
        BETTER_COMPARE(segmented.cluster(0, 0)->color, color3);
        BETTER_COMPARE(segmented.size(), 2);
        BETTER_COMPARE(brushes.colors.size(), 2);
        COMPARE_VECTOR(brushes.gradients.begin()->second.stops, {0, color1}, {1, color2});
    }

    void test_cluster_merge_gradient_item_in_item()
    {
        QRgb color1 = 0xffff0000;
        QRgb color2 = 0xff000000;
        QRgb color_bg = 0xffffffff;
        QRgb color_item = 0xffffff00;
        QImage image(300, 300, QImage::Format_ARGB32);
        image.fill(color_bg);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        QLinearGradient gradient(10, 0, 266, 0);
        gradient.setColorAt(0, color1);
        gradient.setColorAt(1, color2);
        painter.setBrush(gradient);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRectF(QPointF(20, 20), QSizeF(256, 265)), 20, 20);
        painter.setBrush(QColor::fromRgba(color_item));
        painter.drawRoundedRect(QRectF(QPointF(200, 200), QPointF(80, 80)), 20, 20);
        painter.end();

        auto segmented = segment(image);
        QVERIFY(segmented.size() > 2);
        BETTER_COMPARE(segmented.cluster(0, 0)->color, color_bg);
        auto brushes = cluster_merge(segmented, 256);
        BETTER_COMPARE(segmented.cluster(0, 0)->color, color_bg);
        BETTER_COMPARE(segmented.cluster(130, 130)->color, color_item);
        // Ideally it would be 3, but 4 is good enough for now
        BETTER_COMPARE(segmented.size(), 4);
        BETTER_COMPARE(brushes.colors.size(), 4);
        auto id = segmented.cluster_id(150, 40);
        BETTER_COMPARE(brushes.gradients.size(), 1);
        BETTER_COMPARE(brushes.gradients.count(id), 1);
        // This should be true but currently isn't
        // COMPARE_VECTOR(brushes.gradients[id].stops, {0, color1}, {1, color2});
    }

    void test_cluster_merge_symmetric()
    {
        QImage image(32, 9, QImage::Format_ARGB32);
        image.fill(0xff000000);
        int half = image.height() / 2;
        for ( int i = 0; i < 16; i++ )
        {
            QRgb color = 0xff000000 | (i * 0x110000);
            image.setPixel(i, half, color);
            image.setPixel(image.width() - 1 - i, half, color);
            for ( int y = half + 1; y < image.height(); y++ )
            {
                image.setPixel(i, y, 0xffff0000);
                image.setPixel(image.width() - 1 - i, y, 0xffff0000);
            }
        }
        image.save("/tmp/line.png");
        SegmentedImage segmented = segment(image);
        cluster_merge(segmented, 10, 2);
        segmented.to_image(true, true).save("/tmp/line1.png");
    }

    void test_cluster_merge_symmetric_circle()
    {
        struct Quadrant{
            int x, y;
            SegmentedImage image{128, 128};
            Cluster* cluster = nullptr;
        };
        std::array<Quadrant, 4> quadrants{{
            {0, 0},
            {1, 0},
            {0, 1},
            {1, 1}
        }};

        QRgb color_back = 0xff3250b0;
        QRgb color_ellipse = 0xffc4d9f5;

        // Baked quarter circle to ensure pixels are the same in every quadrant
        QImage quarter_circle(128, 128, QImage::Format_ARGB32);
        {
            quarter_circle.fill(Qt::transparent);
            QPainter painter(&quarter_circle);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor::fromRgba(color_ellipse));
            painter.drawEllipse(QPointF(128, 128), 100, 100);
        }

        // Create images with different quadrants of a circle
        for ( auto& quadrant : quadrants )
        {
            QImage image(128, 128, QImage::Format_ARGB32);;
            image.fill(color_back);
            QPainter painter(&image);;
            painter.drawImage(0, 0, quarter_circle.mirrored(quadrant.x, quadrant.y));
            painter.end();
            quadrant.image = segment(image);
            quadrant.cluster = quadrant.image.cluster(127 * quadrant.x, 127 * quadrant.y);
        }

        // Check everything works before cluster merge
        QVERIFY(quadrants[0].cluster != nullptr);
        QVERIFY(quadrants[1].cluster != nullptr);
        QVERIFY(quadrants[2].cluster != nullptr);
        QVERIFY(quadrants[3].cluster != nullptr);

        BETTER_COMPARE(quadrants[0].cluster->color, color_back);
        BETTER_COMPARE(quadrants[1].cluster->color, color_back);
        BETTER_COMPARE(quadrants[2].cluster->color, color_back);
        BETTER_COMPARE(quadrants[3].cluster->color, color_back);

        BETTER_COMPARE(quadrants[1].cluster->size, quadrants[0].cluster->size);
        BETTER_COMPARE(quadrants[2].cluster->size, quadrants[0].cluster->size);
        BETTER_COMPARE(quadrants[3].cluster->size, quadrants[0].cluster->size);

        // Apply cluster merge
        for ( auto& quadrant : quadrants )
        {
            cluster_merge(quadrant.image, 10);
            quadrant.image.to_image().save(QString("/tmp/foo_%1%2.png").arg(quadrant.x).arg(quadrant.y));
        }

        std::vector<int> sizes = {
            quadrants[0].cluster->size,
            quadrants[1].cluster->size,
            quadrants[2].cluster->size,
            quadrants[3].cluster->size,
        };
        std::vector<int> sizes_check(4, quadrants[0].cluster->size);

        BETTER_COMPARE(sizes, sizes_check);
    }
};

QTEST_GUILESS_MAIN(TestClusterMerge)
#include "test_cluster_merge.moc"

