/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QtTest/QtTest>
#include <filesystem>

#include "utils/quantize.hpp"

using namespace glaxnimate::utils::quantize;


class TestTrace: public QObject
{
    Q_OBJECT

private:

private slots:

    void benchmark_eem()
    {
        auto path = std::filesystem::path(__FILE__).parent_path().parent_path() / "data" / "trace" / "images" / "flat.png";
        QImage image(QString::fromStdString(path.u8string()));

        QBENCHMARK
        {
            edge_exclusion_modes(image, 256);
        }
    }

    void benchmark_octree()
    {
        auto path = std::filesystem::path(__FILE__).parent_path().parent_path() / "data" / "trace" / "images" / "flat.png";
        QImage image(QString::fromStdString(path.u8string()));

        QBENCHMARK
        {
            octree(image, 16);
        }
    }

    void benchmark_kmeans()
    {
        auto path = std::filesystem::path(__FILE__).parent_path().parent_path() / "data" / "trace" / "images" / "flat.png";
        QImage image(QString::fromStdString(path.u8string()));

        QBENCHMARK
        {
            k_means(image, 16, 100, KMeansMatch::Closest);
        }
    }
};

QTEST_GUILESS_MAIN(TestTrace)
#include "test_trace.moc"


