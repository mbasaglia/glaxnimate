#pragma once
#include <iostream>
#include "trace/gradient.hpp"

using namespace glaxnimate::trace;
using namespace glaxnimate;

bool operator==(const Cluster& a, const Cluster& b)
{
    return a.id == b.id && a.color == b.color && a.size == b.size && a.merge_target == b.merge_target;
}

template<class T>
QString to_qstring(const T& a)
{
    using namespace QTest;
    auto text = toString(a);
    QString string(text);
    delete[] text;
    return string;
}

QString to_qstring(int a)
{
    return QString::number(a);
}

QString to_qstring(const Cluster& a)
{
    return a.to_string();
}

QString to_qstring(const StructuredColor& a)
{
    return "#" + QString::number(a.rgb(), 16).leftJustified(6, '0');
}

QString to_qstring(const ImageCoord& a)
{
    return QString("(%1, %2)").arg(a.x).arg(a.y);
}

template<class T1, class T2>
QString to_qstring(const std::pair<T1, T2>& p)
{
    return QString("(%1, %2)").arg(to_qstring(p.first)).arg(to_qstring(p.second));
}

template<class T>
QString to_qstring(const std::vector<T>& a)
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
        .arg(to_qstring(val1))
        .arg(name2.leftJustified(len))
        .arg(to_qstring(val2))
    ;
    // Using C++ streams because it's really hard to avoid QTest messing up the output
    std::cerr << message.toStdString();

    return false;
}

template<>
bool better_compare(const SegmentedImage& val1, const std::vector<Cluster>& val2, const QString& name1, const QString& name2, const char* file, int line)
{
    std::vector<Cluster> v(val1.begin(), val1.end());
    std::sort(v.begin(), v.end(), [](const Cluster& a, const Cluster& b) { return a.id < b.id; });
    return better_compare(v, val2, name1, name2, file, line);
}

#define COMPARE_VECTOR(actual, ...) \
    do {\
        using v_type = std::vector<std::decay_t<decltype(actual)>::value_type>; \
        if (!better_compare(actual, (v_type{{__VA_ARGS__}}), #actual, "", __FILE__, __LINE__))\
            return;\
    } while (false)

#define BETTER_COMPARE(actual, ...) \
    do {\
        if (!better_compare(actual, (std::decay_t<decltype(actual)>{__VA_ARGS__}), #actual, "", __FILE__, __LINE__))\
            return;\
    } while (false)


QImage make_image(const std::vector<std::vector<QRgb>>& pixels)
{
    QImage image(pixels[0].size(), pixels.size(), QImage::Format_ARGB32);
    for ( int y = 0; y < image.height(); y++ )
        for ( int x = 0; x < image.width(); x++ )
            image.setPixel(x, y, pixels[y][x]);
    return image;
}



void benchmark_data()
{
    auto root = QFileInfo(__FILE__).dir();
    root.cdUp();
    QTest::addColumn<QString>("image_path");
    QTest::newRow("flat") << root.filePath("data/images/trace/flat.png");
    QTest::newRow("pixel") << root.filePath("data/images/trace/pixel.png");
    QTest::newRow("small complex") << root.filePath("data/images/trace/complex.jpg");
    QTest::newRow("main window") << root.filePath("docs/docs/img/screenshots/main_window/main_window.png");
}
