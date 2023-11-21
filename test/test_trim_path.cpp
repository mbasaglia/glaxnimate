/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "bezier_test.hpp"

#include "model/document.hpp"
#include "model/shapes/trim.hpp"


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
class TestTrimPath: public QObject, public BezierTestBase
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

private Q_SLOTS:

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

