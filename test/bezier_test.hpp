/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once
#include <QtTest/QtTest>

#include"math/bezier/bezier.hpp"

#define COMPARE_MULTIBEZIER(actual, expected) \
    compare_multibez(actual, expected, #actual, #expected, __FILE__, __LINE__)

using namespace glaxnimate;

class BezierTestBase
{
public:

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
            qWarning() << warning;
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
            qWarning() << warning;
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
};
