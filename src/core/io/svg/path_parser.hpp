#pragma once

#include <variant>

#include <QString>
#include <QPointF>
#include <QVector>

#include "utils/regexp.hpp"
#include "math/bezier/bezier.hpp"
#include "math/ellipse_solver.hpp"

namespace io::svg::detail {

class PathDParser
{
public:
    enum TokenType
    {
        Command,
        Parameter,
    };
    using Token = std::variant<ushort, qreal>;

private:
    struct Lexer
    {
        Lexer(const QString& d, std::vector<Token>& tokens)
            : d(d), tokens(tokens)
        {}

        QString d;
        int off = 0;
        std::vector<Token>& tokens;
        QString lexed;
        QChar ch;

        bool eof() const
        {
            return off >= d.size();
        }

        bool next()
        {
            ++off;
            if ( !eof() )
            {
                ch = d[off];
                return true;
            }

            ch = {};
            return false;
        }

        void lex()
        {
            static QString cmds = "MLHVCSQTAZ";

            ch = d[off];

            while ( off < d.size() )
            {
                if ( cmds.contains(ch.toUpper()) )
                {
                    tokens.emplace_back(ch.unicode());
                    next();
                }
                else if ( ch.isSpace() || ch == ',')
                {
                    next();
                }
                else
                {
                    lex_value();
                }
            }
        }

        void lex_value()
        {
            lexed.clear();

            if ( ch == '+' || ch == '-' )
            {
                lexed += ch;
                if ( !next() )
                    return;
            }

            if ( ch.isDigit() )
                lex_value_int();

            if ( ch == '.' )
            {
                lexed += ch;
                if ( !next() )
                    return;
                lex_value_decimal();
            }
            else if ( ch.toUpper() == 'E' )
            {
                lexed += ch;
                if ( !next() )
                    return;
                lex_value_exponent();
            }

            if ( lexed.isEmpty() )
            {
                next();
                return;
            }

            tokens.emplace_back(lexed.toDouble());
            lexed.clear();
        }

        void lex_value_int()
        {
            while ( off < d.size() && ch.isDigit() )
            {
                lexed += ch;
                next();
            }
        }

        void lex_value_decimal()
        {
            lex_value_int();

            if ( ch.toUpper() == 'E' )
            {
                lexed += ch;
                if ( !next() )
                    return;
                lex_value_exponent();
            }
        }

        void lex_value_exponent()
        {
            if ( ch == '+' || ch == '-' )
            {
                lexed += ch;
                if ( !next() )
                    return;
            }

            lex_value_int();
        }

    };

public:
    PathDParser(const QString& d)
    {
        tokenize(d);
    }

    const math::bezier::MultiBezier& parse()
    {
        while ( !eof() )
        {
            if ( la_type() == Command )
            {
                ushort cmd = std::get<Command>(la());
                next_token();
                parse_command(cmd);
            }
            else
            {
                parse_command(implicit);
            }
        }

        return bez;
    }

private:
    const Token& la() const { return tokens[index]; }
    void next_token() { ++index; }
    bool eof() const { return index >= int(tokens.size()); }
    TokenType la_type() const { return TokenType(la().index()); }

    void tokenize(const QString& d)
    {
        if ( d.isEmpty() )
            return;

        Lexer(d, tokens).lex();
    }

    qreal read_param()
    {
        if ( la_type() == Parameter )
        {
            qreal v = std::get<Parameter>(la());
            next_token();
            return v;
        }
        return 0;
    }

    QPointF read_vector()
    {
        return {read_param(), read_param()};
    }

    void parse_M()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }

        p = read_vector();
        bez.move_to(p);
        implicit = 'L';
    }

    void parse_m()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }

        p += read_vector();
        bez.move_to(p);
        implicit = 'l';
    }

    void parse_L()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }

        p = read_vector();
        bez.line_to(p);
        implicit = 'L';
    }

    void parse_l()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }

        p += read_vector();
        bez.line_to(p);
        implicit = 'l';
    }

    void parse_H()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }

        p.setX(read_param());
        bez.line_to(p);
        implicit = 'H';
    }

    void parse_h()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }

        p.setX(p.x() + read_param());
        bez.line_to(p);
        implicit = 'h';
    }

    void parse_V()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }

        p.setY(read_param());
        bez.line_to(p);
        implicit = 'V';
    }

    void parse_v()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }

        p.setY(p.y() + read_param());
        bez.line_to(p);
        implicit = 'v';
    }

    void parse_C()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }
        QPointF tan_out = read_vector();
        QPointF tan_in = read_vector();
        p = read_vector();
        bez.cubic_to(tan_out, tan_in, p);
        implicit = 'C';
    }

    void parse_c()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }
        QPointF tan_out = p + read_vector();
        QPointF tan_in = p + read_vector();
        p += read_vector();
        bez.cubic_to(tan_out, tan_in, p);
        implicit = 'c';
    }

    void parse_S()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }

        QPointF old_p = p;
        QPointF tan_in = read_vector();
        p = read_vector();

        if ( bez.beziers().empty() || bez.beziers().back().empty() )
        {
            bez.cubic_to(old_p, tan_in, p);
        }
        else
        {
            auto& prev = bez.beziers().back().points().back();
            QPointF tan_out = prev.pos - prev.relative_tan_in();
            prev.type = math::bezier::Symmetrical;
            bez.cubic_to(tan_out, tan_in, p);
        }

        implicit = 'S';
    }

    void parse_s()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }

        QPointF old_p = p;
        QPointF tan_in = p+read_vector();
        p += read_vector();

        if ( bez.beziers().empty() || bez.beziers().back().empty() )
        {
            bez.cubic_to(old_p, tan_in, p);
        }
        else
        {
            auto& prev = bez.beziers().back().points().back();
            QPointF tan_out = prev.pos - prev.relative_tan_in();
            prev.type = math::bezier::Symmetrical;
            bez.cubic_to(tan_out, tan_in, p);
        }

        implicit = 's';
    }

    void parse_Q()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }
        QPointF tan = read_vector();
        p = read_vector();
        bez.quadratic_to(tan, p);
        implicit = 'Q';
    }

    void parse_q()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }
        QPointF tan = p+read_vector();
        p += read_vector();
        bez.quadratic_to(tan, p);
        implicit = 'q';
    }

    void parse_T()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }

        QPointF old_p = p;
        p = read_vector();

        if ( bez.beziers().empty() || bez.beziers().back().empty() )
        {
            bez.quadratic_to(old_p, p);
        }
        else
        {
            auto& prev = bez.beziers().back().points().back();
            QPointF tan_out = prev.pos - prev.relative_tan_in();
            prev.type = math::bezier::Symmetrical;
            bez.quadratic_to(tan_out, p);
        }

        implicit = 'T';
    }

    void parse_t()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }

        QPointF old_p = p;
        p += read_vector();

        if ( bez.beziers().empty() || bez.beziers().back().empty() )
        {
            bez.quadratic_to(old_p, p);
        }
        else
        {
            auto& prev = bez.beziers().back().points().back();
            QPointF tan_out = prev.pos - prev.relative_tan_in();
            prev.type = math::bezier::Symmetrical;
            bez.quadratic_to(tan_out, p);
        }

        implicit = 't';
    }

    void do_arc(qreal rx, qreal ry, qreal xrot, bool large, bool sweep, const QPointF& dest)
    {
        if ( p == dest )
            return;

        // straight line
        if ( rx == 0 || ry == 0 )
        {
            p = dest;
            bez.line_to(p);
            return;
        }

        if ( bez.beziers().empty() || bez.beziers().back().empty() )
            return;

        math::bezier::Bezier points = math::EllipseSolver::from_svg_arc(
            p, rx, ry, xrot, large, sweep, dest
        );

        auto& target_points = bez.beziers().back().points();
        target_points.back().tan_out = points[0].tan_out;
        target_points.insert(target_points.end(), points.begin()+1, points.end());
        p = dest;
    }

    void parse_A()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }

        QPointF r = read_vector();
        qreal xrot = read_param();
        qreal large = read_param();
        qreal sweep = read_param();
        QPointF dest = read_vector();

        do_arc(r.x(), r.y(), xrot, large, sweep, dest);

        implicit = 'A';
    }

    void parse_a()
    {
        if ( la_type() != Parameter )
        {
            next_token();
            return;
        }

        QPointF r = read_vector();
        qreal xrot = read_param();
        qreal large = read_param();
        qreal sweep = read_param();
        QPointF dest = p + read_vector();

        do_arc(r.x(), r.y(), xrot, large, sweep, dest);

        implicit = 'a';
    }

    void parse_command(ushort c)
    {
        switch ( c )
        {
            case 'M': return parse_M();
            case 'm': return parse_m();
            case 'L': return parse_L();
            case 'l': return parse_l();
            case 'H': return parse_H();
            case 'h': return parse_h();
            case 'V': return parse_V();
            case 'v': return parse_v();
            case 'C': return parse_C();
            case 'c': return parse_c();
            case 'S': return parse_S();
            case 's': return parse_s();
            case 'Q': return parse_Q();
            case 'q': return parse_q();
            case 'T': return parse_T();
            case 't': return parse_t();
            case 'A': return parse_A();
            case 'a': return parse_a();
            case 'Z':
            case 'z':
                bez.close();
                break;
            default:
                next_token();

        }
    }

    std::vector<Token> tokens;
    int index = 0;
    ushort implicit = 'M';
    QPointF p{0, 0};
    math::bezier::MultiBezier bez;
};

} // namespace io::svg::detail
