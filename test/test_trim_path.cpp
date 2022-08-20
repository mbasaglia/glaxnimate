#include <QtTest/QtTest>

#include "model/document.hpp"
#include "model/shapes/trim.hpp"


#define COMPARE_MULTIBEZIER(actual, expected) \
    compare_multibez(actual, expected, #actual, #expected, __FILE__, __LINE__)

using namespace glaxnimate;

/*
 * Note:
 * This test class defines a lot of machinery to ensure we can properly compare
 * bezier curves (using fuzzy float comparison) and to produce nice readable output.
 *
 * The process() function creates a model::Trim object, initializes its properties
 * as per arguments and returns the processed bezier.
 *
 * To compare beziers in a test, use COMPARE_MULTIBEZIER.
 *
 * Most test case have comments that shows a visual representation of the input
 * and output because the number of tests is kinda overwhelming.
 */
class TestTrimPath: public QObject
{
    Q_OBJECT

private:
    math::bezier::MultiBezier process(float start, float end, float offset, const math::bezier::MultiBezier& input)
    {
        model::Document doc("foo");
        model::Trim trim(&doc);
        trim.start.set(start);
        trim.end.set(end);
        trim.offset.set(offset);
        return trim.process(0, input);
    }

    bool compare_multibez(const math::bezier::MultiBezier& actual, const math::bezier::MultiBezier& expected,
                          const QString& actual_name, const QString& expected_name, const char *file, int line)
    {
        std::string actual_s = build_name(actual_name, "size()");
        std::string expected_s = build_name(expected_name, "size()");

        if ( !QTest::qCompare(actual.size(), expected.size(), actual_s.c_str(), expected_s.c_str(), file, line) )
        {
            QString warning = "\n";
            build_warning_multi(warning, actual_name, actual);
            build_warning_multi(warning, expected_name, expected);
            QWARN(warning.toStdString().c_str());
            return false;
        }

        for ( int i = 0; i < actual.size(); i++ )
        {
            QString name_suf = QString("[%1]").arg(i);
            if ( !compare_bez(actual.beziers()[i], expected.beziers()[i], actual_name + name_suf, expected_name + name_suf, file, line) )
                return false;
        }

        return true;
    }

    using WarningCollectData = std::pair<int, std::vector<QString>>;

    void collect_warning(const QPointF& p, WarningCollectData& out)
    {
        QString text = QTest::toString(p);
        if ( text.size() > out.first )
            out.first = text.size();
        out.second.push_back(text);
    }

    void build_warning(QString& warning, const QString& name, const math::bezier::Bezier& bez)
    {
        warning += name + "\n";
        WarningCollectData pos = {0, {}};
        WarningCollectData tan_in = {0, {}};
        WarningCollectData tan_out = {0, {}};

        for ( const auto& p : bez )
        {
            collect_warning(p.pos, pos);
            collect_warning(p.tan_in, tan_in);
            collect_warning(p.tan_out, tan_out);
        }

        for ( std::size_t i = 0; i < pos.second.size(); i++ )
        {
            warning += "   ";
            warning += pos.second[i].leftJustified(pos.first);
            warning += " in: ";
            warning += tan_in.second[i].leftJustified(tan_in.first);
            warning += " out: ";
            warning += tan_out.second[i].leftJustified(tan_out.first);
            warning += "\n";
        }
    }

    void build_warning_multi(QString& warning, const QString& name, const math::bezier::MultiBezier& bez)
    {
        warning += name + ":\n";

        for ( int i = 0; i < bez.size(); i++ )
            build_warning(warning, name + QString("[%1]").arg(i), bez.beziers()[i]);
    }

    bool compare_bez(const math::bezier::Bezier& actual, const math::bezier::Bezier& expected,
                     const QString& actual_name, const QString& expected_name,
                     const char *file, int line)
    {
        std::string actual_s = build_name(actual_name, "size()");
        std::string expected_s = build_name(expected_name, "size()");

        bool ret = true;

        if ( !QTest::qCompare(actual.size(), expected.size(), actual_s.c_str(), expected_s.c_str(), file, line) )
        {
            ret = false;
        }
        else
        {
            actual_s = build_name(actual_name, "closed()");
            expected_s = build_name(expected_name, "closed()");
            if ( !QTest::qCompare(actual.closed(), expected.closed(), actual_s.c_str(), expected_s.c_str(), file, line) )
            {
                ret = false;
            }

            for ( int i = 0; i < actual.size(); i++ )
            {
                QString name_suf = QString("[%1]").arg(i);
                if ( !compare_point(actual[i], expected[i], actual_name + name_suf, expected_name + name_suf, file, line) )
                {
                    ret = false;
                    break;
                }
            }
        }

        if ( !ret )
        {
            QString warning = "\n";
            build_warning(warning, actual_name, actual);
            build_warning(warning, expected_name, expected);
            QWARN(warning.toStdString().c_str());
        }

        return ret;
    }

    std::string build_name(const QString& base, const char* member)
    {
        return (base + "." + member).toStdString();
    }

    bool compare_point(const math::bezier::Point& actual, const math::bezier::Point& expected,
                       const QString& actual_name, const QString& expected_name,
                       const char *file, int line)
    {
        bool ret = true;
        ret = compare_qpointf(actual.pos, expected.pos, actual_name, expected_name, "pos", file, line) && ret;
        ret = compare_qpointf(actual.tan_in, expected.tan_in, actual_name, expected_name, "tan_in", file, line) && ret;
        ret = compare_qpointf(actual.tan_out, expected.tan_out, actual_name, expected_name, "tan_out", file, line) && ret;
        return ret;
    }


    bool compare_qpointf(const QPointF& actual, const QPointF& expected,
                       const QString& actual_name, const QString& expected_name,  const char* point_name,
                       const char *file, int line)
    {
        // reduce accuracy requirements by casting to float
        if ( !qFuzzyCompare(float(actual.x()), float(expected.x())) || !qFuzzyCompare(float(actual.y()), float(expected.y())) )
        {
            // actual test comparisons for output

            std::string actual_s = build_name(actual_name, point_name);
            std::string expected_s = build_name(expected_name, point_name);
            QTest::qCompare(actual, expected, actual_s.c_str(), expected_s.c_str(), file, line);

            QString suffix = QString(".") + point_name;
            compare_qpointf_component(actual.x(), expected.x(), actual_name+suffix, expected_name+suffix, "x()", file, line);
            compare_qpointf_component(actual.y(), expected.y(), actual_name+suffix, expected_name+suffix, "y()", file, line);
            return false;
        }

        return true;
    }

    bool compare_qpointf_component(qreal actual, qreal expected,
                       const QString& actual_name, const QString& expected_name,  const char* component,
                       const char *file, int line)
    {
        std::string actual_s = build_name(actual_name, component);
        std::string expected_s = build_name(expected_name, component);
        return QTest::qCompare(actual, expected, actual_s.c_str(), expected_s.c_str(), file, line);
    }

private slots:

    void test_process_empty()
    {
        math::bezier::MultiBezier input;
        math::bezier::MultiBezier output = process(0, 0.5, 0, input);
        QVERIFY(output.empty());
    }

// Line

    /*
     * ======================
     * ^--------------------^
     */
    void test_process_line_full()
    {
        math::bezier::MultiBezier input;
        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));
        math::bezier::MultiBezier output = process(0, 1, 0, input);
        math::bezier::MultiBezier expected;
        expected.move_to(QPointF(0, 0));
        expected.line_to(QPointF(100, 0));
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*
     * ======================
     * ^----------^
     */
    void test_process_line_start()
    {
        math::bezier::MultiBezier input;
        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));
        math::bezier::MultiBezier output = process(0, 0.5, 0, input);
        math::bezier::MultiBezier expected;
        expected.move_to(QPointF(0, 0));
        expected.line_to(QPointF(50, 0));
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*
     * ======================
     *      ^----------^
     */
    void test_process_line_mid()
    {
        math::bezier::MultiBezier input;
        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));
        math::bezier::MultiBezier output = process(0, 0.5, 0.2, input);
        math::bezier::MultiBezier expected;
        expected.move_to(QPointF(20, 0));
        expected.line_to(QPointF(70, 0));
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*
     * ======================
     *           ^----------^
     */
    void test_process_line_end()
    {
        math::bezier::MultiBezier input;
        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));
        math::bezier::MultiBezier output = process(0, 0.5, 0.5, input);
        math::bezier::MultiBezier expected;
        expected.move_to(QPointF(50, 0));
        expected.line_to(QPointF(100, 0));
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*
     * ======================
     * -----^          ^-----
     */
    void test_process_line_wrap()
    {
        math::bezier::MultiBezier input;
        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));
        math::bezier::MultiBezier output = process(0, 0.5, 0.7, input);
        math::bezier::MultiBezier expected;
        expected.move_to(QPointF(70, 0));
        expected.line_to(QPointF(100, 0));
        expected.move_to(QPointF(0, 0));
        expected.line_to(QPointF(20, 0));
        COMPARE_MULTIBEZIER(output, expected);
    }

// Square

    /*  ____
     * |    |
     * |____|
     *  ____
     * |    |
     * |____|
     */
    void test_process_square_full()
    {
        math::bezier::MultiBezier input;
        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(0, 100));
        input.close();
        math::bezier::MultiBezier output = process(0, 1, 0, input);
        COMPARE_MULTIBEZIER(output, input);
    }

    /*  ____
     * |    |
     * |____|
     *  ____
     *      |
     *      |
     */
    void test_process_square_start_corner()
    {
        math::bezier::MultiBezier input;
        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(0, 100));
        input.close();
        math::bezier::MultiBezier output = process(0, 0.5, 0, input);
        math::bezier::MultiBezier expected;
        expected.move_to(QPointF(0, 0));
        expected.line_to(QPointF(100, 0));
        expected.line_to(QPointF(100, 100));
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*  ____
     * |    |
     * |____|
     *  ____
     *      |
     *
     */
    void test_process_square_start_edge()
    {
        math::bezier::MultiBezier input;
        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(0, 100));
        input.close();
        math::bezier::MultiBezier output = process(0, 0.375, 0, input);
        math::bezier::MultiBezier expected;
        expected.move_to(QPointF(0, 0));
        expected.line_to(QPointF(100, 0));
        expected.line_to(QPointF(100, 50));
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*  ____
     * |    |
     * |____|
     *
     *      |
     *  ____|
     */
    void test_process_square_mid_corner()
    {
        math::bezier::MultiBezier input;
        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(0, 100));
        input.close();
        math::bezier::MultiBezier output = process(0, 0.5, 0.25, input);
        math::bezier::MultiBezier expected;
        expected.move_to(QPointF(100, 0));
        expected.line_to(QPointF(100, 100));
        expected.line_to(QPointF(0, 100));
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*  ____
     * |    |
     * |____|
     *    __
     *      |
     *    __|
     */
    void test_process_square_mid_edge()
    {
        math::bezier::MultiBezier input;
        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(0, 100));
        input.close();
        math::bezier::MultiBezier output = process(0, 0.5, 0.125, input);
        math::bezier::MultiBezier expected;
        expected.move_to(QPointF(50, 0));
        expected.line_to(QPointF(100, 0));
        expected.line_to(QPointF(100, 100));
        expected.line_to(QPointF(50, 100));
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*  ____
     * |    |
     * |____|
     *
     * |
     * |____
     */
    void test_process_square_end_corner()
    {
        math::bezier::MultiBezier input;
        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(0, 100));
        input.close();
        math::bezier::MultiBezier output = process(0, 0.5, 0.5, input);
        math::bezier::MultiBezier expected;
        expected.move_to(QPointF(100, 100));
        expected.line_to(QPointF(0, 100));
        expected.line_to(QPointF(0, 0));
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*  ____
     * |    |
     * |____|
     *
     * |
     * |__
     */
    void test_process_square_end_edge()
    {
        math::bezier::MultiBezier input;
        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(0, 100));
        input.close();
        math::bezier::MultiBezier output = process(0.625, 1, 0, input);
        math::bezier::MultiBezier expected;
        expected.move_to(QPointF(50, 100));
        expected.line_to(QPointF(0, 100));
        expected.line_to(QPointF(0, 0));
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*  ____
     * |    |
     * |____|
     *  ____
     * |
     * |
     */
    void test_process_square_wrap_corner()
    {
        math::bezier::MultiBezier input;
        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(0, 100));
        input.close();
        math::bezier::MultiBezier output = process(0, 0.5, 0.75, input);
        math::bezier::MultiBezier expected;
        expected.move_to(QPointF(0, 100));
        expected.line_to(QPointF(0, 0));
        expected.line_to(QPointF(100, 0));
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*  ____
     * |    |
     * |____|
     *  __
     * |
     * |__
     */
    void test_process_square_wrap_edge()
    {
        math::bezier::MultiBezier input;
        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(0, 100));
        input.close();
        math::bezier::MultiBezier output = process(0, 0.5, 0.625, input);
        math::bezier::MultiBezier expected;
        expected.move_to(QPointF(50, 100));
        expected.line_to(QPointF(0, 100));
        expected.line_to(QPointF(0, 0));
        expected.line_to(QPointF(50, 0));
        COMPARE_MULTIBEZIER(output, expected);
    }

// Multi Line

    /*
     * ==== ==== ==== ====
     */
    void test_process_multi_line_full()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(0, 0));
        expected.line_to(QPointF(100, 0));

        expected.move_to(QPointF(100, 0));
        expected.line_to(QPointF(200, 0));

        expected.move_to(QPointF(200, 0));
        expected.line_to(QPointF(300, 0));

        expected.move_to(QPointF(300, 0));
        expected.line_to(QPointF(400, 0));

        math::bezier::MultiBezier output = process(0, 1, 0, input);
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*
     * ==== ==== ---- ----
     */
    void test_process_multi_line_start_corner()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(0, 0));
        expected.line_to(QPointF(100, 0));

        expected.move_to(QPointF(100, 0));
        expected.line_to(QPointF(200, 0));

        math::bezier::MultiBezier output = process(0, 0.5, 0, input);
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*
     * ==== ==== ==-- ----
     */
    void test_process_multi_line_start_edge()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(0, 0));
        expected.line_to(QPointF(100, 0));

        expected.move_to(QPointF(100, 0));
        expected.line_to(QPointF(200, 0));

        expected.move_to(QPointF(200, 0));
        expected.line_to(QPointF(250, 0));

        math::bezier::MultiBezier output = process(0, 0.625, 0, input);
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*
     * ---- ---- ==== ====
     */
    void test_process_multi_line_end_corner()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(200, 0));
        expected.line_to(QPointF(300, 0));

        expected.move_to(QPointF(300, 0));
        expected.line_to(QPointF(400, 0));

        math::bezier::MultiBezier output = process(0, 0.5, 0.5, input);
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*
     * ---- ---- --== ====
     */
    void test_process_multi_line_end_edge()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(250, 0));
        expected.line_to(QPointF(300, 0));

        expected.move_to(QPointF(300, 0));
        expected.line_to(QPointF(400, 0));

        math::bezier::MultiBezier output = process(0.625, 1, 0, input);
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*
     * ---- ==== ==== ----
     */
    void test_process_multi_line_mid_corner()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(100, 0));
        expected.line_to(QPointF(200, 0));

        expected.move_to(QPointF(200, 0));
        expected.line_to(QPointF(300, 0));

        math::bezier::MultiBezier output = process(0, 0.5, 0.25, input);
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*
     * --== ==== ==-- ----
     */
    void test_process_multi_line_mid_edge()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(50, 0));
        expected.line_to(QPointF(100, 0));

        expected.move_to(QPointF(100, 0));
        expected.line_to(QPointF(200, 0));

        expected.move_to(QPointF(200, 0));
        expected.line_to(QPointF(250, 0));

        math::bezier::MultiBezier output = process(0.125, 0.625, 0, input);
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*
     * ==== ---- ---- ====
     */
    void test_process_multi_line_wrap_corner()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(300, 0));
        expected.line_to(QPointF(400, 0));

        expected.move_to(QPointF(0, 0));
        expected.line_to(QPointF(100, 0));

        math::bezier::MultiBezier output = process(0, 0.5, 0.75, input);
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*
     * ==-- ---- --== ====
     */
    void test_process_multi_line_wrap_edge()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(250, 0));
        expected.line_to(QPointF(300, 0));

        expected.move_to(QPointF(300, 0));
        expected.line_to(QPointF(400, 0));

        expected.move_to(QPointF(0, 0));
        expected.line_to(QPointF(50, 0));

        math::bezier::MultiBezier output = process(0, 0.5, 0.625, input);
        COMPARE_MULTIBEZIER(output, expected);
    }


    /*
     * ==== ==== ---- ====
     */
    void test_process_multi_line_wrap_corner_same()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(300, 0));
        expected.line_to(QPointF(400, 0));

        expected.move_to(QPointF(0, 0));
        expected.line_to(QPointF(100, 0));

        expected.move_to(QPointF(100, 0));
        expected.line_to(QPointF(200, 0));

        math::bezier::MultiBezier output = process(0, 0.75, 0.75, input);
        COMPARE_MULTIBEZIER(output, expected);
    }

    /*
     * =--= ==== ==== ====
     */
    void test_process_multi_line_wrap_edge_same()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(0, 0));
        input.line_to(QPointF(100, 0));

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(75, 0));
        expected.line_to(QPointF(100, 0));

        expected.move_to(QPointF(100, 0));
        expected.line_to(QPointF(200, 0));

        expected.move_to(QPointF(200, 0));
        expected.line_to(QPointF(300, 0));

        expected.move_to(QPointF(300, 0));
        expected.line_to(QPointF(400, 0));

        expected.move_to(QPointF(0, 0));
        expected.line_to(QPointF(25, 0));

        math::bezier::MultiBezier output = process(0, 14./16., 3./16., input);
        COMPARE_MULTIBEZIER(output, expected);
    }

// Multi Square

    /*  ____   ____   ____   ____
     * |    | |    | |    | |    |
     * |____| |____| |____| |____|
     *  ____   ____   ____   ____
     * |    | |    | |    | |    |
     * |____| |____| |____| |____|
     */
    void test_process_multi_square_full()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(000, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(000, 100));
        input.close();

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));
        input.line_to(QPointF(200, 100));
        input.line_to(QPointF(100, 100));
        input.close();

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));
        input.line_to(QPointF(300, 100));
        input.line_to(QPointF(200, 100));
        input.close();

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));
        input.line_to(QPointF(400, 100));
        input.line_to(QPointF(300, 100));
        input.close();

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(000, 000));
        expected.line_to(QPointF(100, 000));
        expected.line_to(QPointF(100, 100));
        expected.line_to(QPointF(000, 100));
        expected.close();

        expected.move_to(QPointF(100, 0));
        expected.line_to(QPointF(200, 0));
        expected.line_to(QPointF(200, 100));
        expected.line_to(QPointF(100, 100));
        expected.close();

        expected.move_to(QPointF(200, 0));
        expected.line_to(QPointF(300, 0));
        expected.line_to(QPointF(300, 100));
        expected.line_to(QPointF(200, 100));
        expected.close();

        expected.move_to(QPointF(300, 0));
        expected.line_to(QPointF(400, 0));
        expected.line_to(QPointF(400, 100));
        expected.line_to(QPointF(300, 100));
        expected.close();

        math::bezier::MultiBezier output = process(0, 1, 0, input);

        COMPARE_MULTIBEZIER(output, expected);
    }

    /*  ____   ____   ____   ____
     * |    | |    | |    | |    |
     * |____| |____| |____| |____|
     *  ____   ____
     * |    | |    |
     * |____| |____|
     */
    void test_process_multi_square_start_corner()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(000, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(000, 100));
        input.close();

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));
        input.line_to(QPointF(200, 100));
        input.line_to(QPointF(100, 100));
        input.close();

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));
        input.line_to(QPointF(300, 100));
        input.line_to(QPointF(200, 100));
        input.close();

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));
        input.line_to(QPointF(400, 100));
        input.line_to(QPointF(300, 100));
        input.close();

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(000, 0));
        expected.line_to(QPointF(100, 0));
        expected.line_to(QPointF(100, 100));
        expected.line_to(QPointF(000, 100));
        expected.close();

        expected.move_to(QPointF(100, 0));
        expected.line_to(QPointF(200, 0));
        expected.line_to(QPointF(200, 100));
        expected.line_to(QPointF(100, 100));
        expected.close();

        math::bezier::MultiBezier output = process(0, 0.5, 0, input);

        COMPARE_MULTIBEZIER(output, expected);
    }

    /*  ____   ____   ____   ____
     * |    | |    | |    | |    |
     * |____| |____| |____| |____|
     *                ____   ____
     *               |    | |    |
     *               |____| |____|
     */
    void test_process_multi_square_end_corner()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(000, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(000, 100));
        input.close();

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));
        input.line_to(QPointF(200, 100));
        input.line_to(QPointF(100, 100));
        input.close();

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));
        input.line_to(QPointF(300, 100));
        input.line_to(QPointF(200, 100));
        input.close();

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));
        input.line_to(QPointF(400, 100));
        input.line_to(QPointF(300, 100));
        input.close();

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(200, 0));
        expected.line_to(QPointF(300, 0));
        expected.line_to(QPointF(300, 100));
        expected.line_to(QPointF(200, 100));
        expected.close();

        expected.move_to(QPointF(300, 0));
        expected.line_to(QPointF(400, 0));
        expected.line_to(QPointF(400, 100));
        expected.line_to(QPointF(300, 100));
        expected.close();

        math::bezier::MultiBezier output = process(0, 0.5, 0.5, input);

        COMPARE_MULTIBEZIER(output, expected);
    }

    /*  ____   ____   ____   ____
     * |    | |    | |    | |    |
     * |____| |____| |____| |____|
     *         ____   ____
     *        |    | |    |
     *        |____| |____|
     */
    void test_process_multi_square_mid_corner()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(000, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(000, 100));
        input.close();

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));
        input.line_to(QPointF(200, 100));
        input.line_to(QPointF(100, 100));
        input.close();

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));
        input.line_to(QPointF(300, 100));
        input.line_to(QPointF(200, 100));
        input.close();

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));
        input.line_to(QPointF(400, 100));
        input.line_to(QPointF(300, 100));
        input.close();

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(100, 0));
        expected.line_to(QPointF(200, 0));
        expected.line_to(QPointF(200, 100));
        expected.line_to(QPointF(100, 100));
        expected.close();

        expected.move_to(QPointF(200, 0));
        expected.line_to(QPointF(300, 0));
        expected.line_to(QPointF(300, 100));
        expected.line_to(QPointF(200, 100));
        expected.close();

        math::bezier::MultiBezier output = process(0, 0.5, 0.25, input);

        COMPARE_MULTIBEZIER(output, expected);
    }

    /*  ____   ____   ____   ____
     * |    | |    | |    | |    |
     * |____| |____| |____| |____|
     *  ____                 ____
     * |    |               |    |
     * |____|               |____|
     */
    void test_process_multi_square_wrap_corner()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(000, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(000, 100));
        input.close();

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));
        input.line_to(QPointF(200, 100));
        input.line_to(QPointF(100, 100));
        input.close();

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));
        input.line_to(QPointF(300, 100));
        input.line_to(QPointF(200, 100));
        input.close();

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));
        input.line_to(QPointF(400, 100));
        input.line_to(QPointF(300, 100));
        input.close();

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(300, 0));
        expected.line_to(QPointF(400, 0));
        expected.line_to(QPointF(400, 100));
        expected.line_to(QPointF(300, 100));
        expected.close();

        expected.move_to(QPointF(000, 0));
        expected.line_to(QPointF(100, 0));
        expected.line_to(QPointF(100, 100));
        expected.line_to(QPointF(000, 100));
        expected.close();

        math::bezier::MultiBezier output = process(0, 0.5, 0.75, input);

        COMPARE_MULTIBEZIER(output, expected);
    }

    /*  ____   ____   ____   ____
     * |    | |    | |    | |    |
     * |____| |____| |____| |____|
     *  ____   ____   ____
     * |    | |    |      |
     * |____| |____|      |
     */
    void test_process_multi_square_start_edge()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(000, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(000, 100));
        input.close();

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));
        input.line_to(QPointF(200, 100));
        input.line_to(QPointF(100, 100));
        input.close();

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));
        input.line_to(QPointF(300, 100));
        input.line_to(QPointF(200, 100));
        input.close();

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));
        input.line_to(QPointF(400, 100));
        input.line_to(QPointF(300, 100));
        input.close();

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(000, 0));
        expected.line_to(QPointF(100, 0));
        expected.line_to(QPointF(100, 100));
        expected.line_to(QPointF(000, 100));
        expected.close();

        expected.move_to(QPointF(100, 0));
        expected.line_to(QPointF(200, 0));
        expected.line_to(QPointF(200, 100));
        expected.line_to(QPointF(100, 100));
        expected.close();

        expected.move_to(QPointF(200, 0));
        expected.line_to(QPointF(300, 0));
        expected.line_to(QPointF(300, 100));

        math::bezier::MultiBezier output = process(0, 0.625, 0, input);

        COMPARE_MULTIBEZIER(output, expected);
    }

    /*  ____   ____   ____   ____
     * |    | |    | |    | |    |
     * |____| |____| |____| |____|
     *                       ____
     *               |      |    |
     *               |____  |____|
     */
    void test_process_multi_square_end_edge()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(000, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(000, 100));
        input.close();

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));
        input.line_to(QPointF(200, 100));
        input.line_to(QPointF(100, 100));
        input.close();

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));
        input.line_to(QPointF(300, 100));
        input.line_to(QPointF(200, 100));
        input.close();

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));
        input.line_to(QPointF(400, 100));
        input.line_to(QPointF(300, 100));
        input.close();

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(300, 100));
        expected.line_to(QPointF(200, 100));
        expected.line_to(QPointF(200, 0));

        expected.move_to(QPointF(300, 0));
        expected.line_to(QPointF(400, 0));
        expected.line_to(QPointF(400, 100));
        expected.line_to(QPointF(300, 100));
        expected.close();

        math::bezier::MultiBezier output = process(0.625, 1, 0, input);

        COMPARE_MULTIBEZIER(output, expected);
    }

    /*  ____   ____   ____   ____
     * |    | |    | |    | |    |
     * |____| |____| |____| |____|
     *  ____
     *      |               |
     *      |               |____
     */
    void test_process_multi_square_wrap_edge()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(000, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(000, 100));
        input.close();

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));
        input.line_to(QPointF(200, 100));
        input.line_to(QPointF(100, 100));
        input.close();

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));
        input.line_to(QPointF(300, 100));
        input.line_to(QPointF(200, 100));
        input.close();

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));
        input.line_to(QPointF(400, 100));
        input.line_to(QPointF(300, 100));
        input.close();

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(400, 100));
        expected.line_to(QPointF(300, 100));
        expected.line_to(QPointF(300, 0));

        expected.move_to(QPointF(000, 0));
        expected.line_to(QPointF(100, 0));
        expected.line_to(QPointF(100, 100));

        math::bezier::MultiBezier output = process(0, 2./8., 7./8., input);

        COMPARE_MULTIBEZIER(output, expected);
    }

    /*  ____   ____   ____   ____
     * |    | |    | |    | |    |
     * |____| |____| |____| |____|
     *                ____
     *        |           |
     *        |____       |
     */
    void test_process_multi_square_mid_edge()
    {
        math::bezier::MultiBezier input;

        input.move_to(QPointF(000, 0));
        input.line_to(QPointF(100, 0));
        input.line_to(QPointF(100, 100));
        input.line_to(QPointF(000, 100));
        input.close();

        input.move_to(QPointF(100, 0));
        input.line_to(QPointF(200, 0));
        input.line_to(QPointF(200, 100));
        input.line_to(QPointF(100, 100));
        input.close();

        input.move_to(QPointF(200, 0));
        input.line_to(QPointF(300, 0));
        input.line_to(QPointF(300, 100));
        input.line_to(QPointF(200, 100));
        input.close();

        input.move_to(QPointF(300, 0));
        input.line_to(QPointF(400, 0));
        input.line_to(QPointF(400, 100));
        input.line_to(QPointF(300, 100));
        input.close();

        math::bezier::MultiBezier expected;

        expected.move_to(QPointF(200, 100));
        expected.line_to(QPointF(100, 100));
        expected.line_to(QPointF(100, 0));

        expected.move_to(QPointF(200, 0));
        expected.line_to(QPointF(300, 0));
        expected.line_to(QPointF(300, 100));

        math::bezier::MultiBezier output = process(3./8., 5./8., 0, input);

        COMPARE_MULTIBEZIER(output, expected);
    }
};

QTEST_GUILESS_MAIN(TestTrimPath)
#include "test_trim_path.moc"

