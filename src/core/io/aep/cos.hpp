/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <unordered_map>
#include <vector>
#include <variant>
#include <memory>

#include <QByteArray>
#include <QString>

#include "string_decoder.hpp"

namespace glaxnimate::io::aep {

enum class CosTokenType
{
    // /foo
    Identifier,
    // 123
    Number,
    // (foo)
    String,
    // <f000>
    HexString,
    // true
    Boolean,
    // <<
    ObjectStart,
    // >>
    ObjectEnd,
    // [
    ArrayStart,
    // ]
    ArrayEnd,
    // null
    Null,
    // end of file
    Eof,
};

class CosError : public std::runtime_error
{
public:
    CosError(QString message) : runtime_error(message.toStdString()), message(std::move(message)) {}

    QString message;
};


struct CosValue
{
    enum class Index
    {
        Null,
        Number,
        String,
        Boolean,
        Bytes,
        Object,
        Array
    };

    using Object = std::unique_ptr<std::unordered_map<QString, CosValue>>;
    using Array = std::unique_ptr<std::vector<CosValue>>;

    template<class T>
    CosValue(T&& v) : value(std::forward<T>(v)) {}
    CosValue() = default;
    CosValue(CosValue& v) = delete;
    CosValue(const CosValue& v) = delete;
    CosValue(CosValue&& v) = default;
    CosValue& operator=(CosValue&& v) = default;


    template<Index Ind>
    const auto& get() const
    {
        if ( Ind != type() )
            throw CosError("Invalid COS value type");
        return std::get<int(Ind)>(value);
    }

    Index type() const { return Index(value.index()); }

    std::variant<
        std::nullptr_t, double, QString, bool, QByteArray, Object, Array
    > value = nullptr;
};

using CosObject = CosValue::Object;
using CosArray = CosValue::Array;

struct CosToken
{
    CosTokenType type = CosTokenType::Eof;
    CosValue value = {};

    CosToken() = default;
    CosToken(CosToken&&) = default;
    CosToken& operator=(CosToken&&) = default;
};

class CosLexer
{
public:
    CosLexer(QByteArray data) : data(std::move(data)) {}

    CosToken next_token()
    {
        int ch;

        while ( true )
        {
            ch = get_char();
            if ( ch == -1 )
                return CosToken();
            else if ( ch == '%' )
                lex_comment();
            else if ( !std::isspace(ch) )
                break;
        }

        // <<
        if ( ch == '<' )
        {
            ch = get_char();
            if ( ch == '<' )
                return {CosTokenType::ObjectStart};
            else if ( ch == -1 )
                throw_lex("<");
            else if ( std::isxdigit(ch) )
                return lex_hex_string(ch);
            else
                throw_lex(QString("<") + QChar(ch));
        }

        // >>
        if ( ch == '>' )
        {
            auto d = get_char();
            if ( d != '>' )
            {
                QString tok(ch);
                if ( d != -1 )
                    tok += QChar(d);
                throw_lex(tok, ">>");
            }
            return {CosTokenType::ObjectEnd};
        }

        // [
        if ( ch == '[' )
            return {CosTokenType::ArrayStart};

        // ]
        if ( ch == ']' )
            return {CosTokenType::ArrayEnd};

        // /foo
        if ( ch == '/' ) {
            return lex_identifier();
        }

        // (foo)
        if ( ch == '(' ) {
            return lex_string();
        }

        // Keyword
        if ( std::isalpha(ch) )
            return lex_keyword(ch);

        // Number
        if ( std::isdigit(ch) || ch == '-' || ch == '+' || ch == '.' )
            return lex_number(ch);

        throw_lex(QString() + QChar(ch));
    }

    [[noreturn]] void throw_lex(const QString& token, const QString& exp = {})
    {
        QString msg = "Unknown COS token %1";
        msg = msg.arg(token);
        if ( !exp.isEmpty() )
        {
            msg += ", expected ";
            msg += exp;
        }

        throw CosError(msg);
    }

    int get_char()
    {
        if ( offset >= data.size() )
            return -1;

        int ch = std::uint8_t(data[offset]);
        offset += 1;
        return ch;
    }

    void unget()
    {
        offset -= 1;
        if ( offset < 0 )
            throw CosError("Buffer underflow");
    }

    void lex_comment()
    {
        while ( true )
        {
            auto ch = get_char();
            if ( ch == -1 || ch == '\n' )
                break;
        }
    }

    CosToken lex_number(int ch)
    {
        if ( ch == '.' )
            return lex_number_fract(QString(ch));
        else if ( ch == '+' || ch == '-' )
            return lex_number_int(get_char(), ch);
        else
            return lex_number_int(ch, '+');
    }

    CosToken lex_number_int(int ch, QChar sign)
    {
        QString head;
        head += sign;

        while ( true )
        {
            if ( ch == '.' )
            {
                return lex_number_fract(head + ch);
            }
            else if ( ch == -1 )
            {
                break;
            }
            else if ( std::isdigit(ch) )
            {
                head += ch;
                ch = get_char();
            }
            else
            {
                unget();
                break;
            }
        }

        return {CosTokenType::Number, head.toDouble()};
    }

    CosToken lex_number_fract(QString num)
    {
        while ( true )
        {
            int ch = get_char();

            if ( ch == -1 )
            {
                break;
            }
            else if ( std::isdigit(ch) )
            {
                num += ch;
            }
            else
            {
                unget();
                break;
            }
        }
        return {CosTokenType::Number, num.toDouble()};
    }

    CosToken lex_keyword(char start)
    {
        QString kw(start);

        while ( true )
        {
            auto ch = get_char();
            if ( ch == -1 )
            {
                break;
            }
            else if ( std::isalpha(ch) )
            {
                kw += ch;
            }
            else
            {
                unget();
                break;
            }
        }

        if ( kw == "true" )
                return {CosTokenType::Boolean, true};
        if ( kw == "false" )
                return {CosTokenType::Boolean, false};
        if ( kw == "null")
                return {CosTokenType::Null};

        throw CosError("Unknown keyword " + kw);
    }

    CosToken lex_string()
    {
        QByteArray string;

        while ( true )
        {
            auto ch = lex_string_char();
            if ( ch == -1 )
                break;

            string.push_back(ch);
        }

        return {CosTokenType::String, decode_string(string)};
    }

    int lex_string_char()
    {
        auto ch = get_char();
        if ( ch == -1 )
            throw CosError("Unterminated String");

        if ( ch == ')' )
            return -1;

        if ( ch == '\\' )
            return lex_string_escape();

        if ( ch == '\r' )
        {
            if ( get_char() != '\n' )
                unget();
            return '\n';
        }
        else if ( ch == '\n' )
        {
            if ( get_char() != '\r' )
                unget();
            return '\n';
        }

        return ch;
    }

    bool is_octal(char ch)
    {
        return '0' <= ch && ch <= '7';
    }

    char lex_string_escape()
    {
        auto ch = get_char();
        if ( ch == -1 )
            throw CosError("Unterminated string");

        switch ( ch )
        {
            case 'b':
                return '\b';
            case 'n':
                return '\n';
            case 'f':
                return '\f';
            case 'r':
                return '\r';
            case '(':
            case ')':
            case '\\':
                return ch;
        }

        if ( is_octal(ch) )
        {
            QString octal(ch);
            for ( auto i = 0; i < 2; i++ )
            {
                ch = get_char();
                if ( ch == -1 )
                    break;

                if ( !is_octal(ch) )
                {
                    unget();
                    break;
                }

                octal += ch;
            }

            return octal.toInt(nullptr, 8);
        }

        throw CosError("Invalid escape sequence");
    }

    CosToken lex_hex_string(char head)
    {
        QByteArray data;
        data.push_back(head);
        while ( true )
        {
            auto ch = get_char();
            if ( ch == -1 )
            {
                throw CosError("Unterminated hex string");
            }
            else if ( std::isxdigit(ch) )
            {
                data.push_back(ch);
            }
            else if ( ch == '>' )
            {
                if ( data.size() % 2 != 0 )
                    data.push_back('0');
                break;
            }
            else if ( !std::isspace(ch) )
            {
                throw CosError(QString("Invalid character in hex string: ") + ch);
            }
        }

        return {CosTokenType::HexString, QByteArray::fromHex(data)};
    }

    CosToken lex_identifier()
    {
        QString ident = "";
        const QString special = "()[]<>/%";
        while ( true )
        {
            auto ch = get_char();
            if ( ch == -1 )
                break;
            if ( ch < 0x21 || ch > 0x7e )
            {
                unget();
                break;
            }

            if ( ch == '#' )
            {
                QByteArray hexstr;
                for ( auto i = 0; i < 2; i++ )
                {
                    ch = get_char();
                    if ( ch == -1 || !std::isxdigit(ch) )
                        throw CosError("Invalid Identifier");
                    hexstr += ch;
                }
                ident += hexstr.toInt(nullptr, 16);
            }
            else if ( special.indexOf(ch) != -1 )
            {
                unget();
                break;
            }
            else
            {
                ident += ch;
            }
        }

        return {CosTokenType::Identifier, ident};
    }

private:
    QByteArray data;
    int offset = 0;
};

class CosParser
{
public:
    CosParser(QByteArray data) : lexer(std::move(data)) {}

    CosValue parse()
    {
        lex();
        if ( lookahead.type == CosTokenType::Identifier )
            return parse_object_content();

        auto val = parse_value();
        if ( lookahead.type == CosTokenType::Eof )
            return val;

        CosArray arr = parse_array_content();
        arr->emplace(arr->begin(), std::move(val));
        return arr;
    }


private:
    CosToken lookahead;
    CosLexer lexer;

    void lex()
    {
        lookahead = lexer.next_token();
    }

    CosObject parse_object_content()
    {
        CosObject value = std::make_unique<CosObject::element_type>();

        while ( true )
        {
            if ( lookahead.type == CosTokenType::Eof || lookahead.type == CosTokenType::ObjectEnd )
                break;

            expect(CosTokenType::Identifier);
            auto key = lookahead.value.get<CosValue::Index::String>();
            lex();
            auto val = parse_value();
            value->emplace(key, std::move(val));
        }

        return value;
    }

    void expect(CosTokenType token_type)
    {
        if ( lookahead.type != token_type )
            throw CosError(QString("Expected token %1, got %2").arg(int(token_type)).arg(int(lookahead.type)));
    }

    CosArray parse_array_content()
    {
        CosArray value = std::make_unique<CosArray::element_type>();

        while ( true )
        {
            if ( lookahead.type == CosTokenType::Eof || lookahead.type == CosTokenType::ArrayEnd )
                break;

            value->push_back(parse_value());
        }

        return value;
    }

    CosValue parse_value()
    {
        CosValue val;
        switch ( lookahead.type )
        {
            case CosTokenType::String:
            case CosTokenType::HexString:
            case CosTokenType::Null:
            case CosTokenType::Boolean:
            case CosTokenType::Identifier:
            case CosTokenType::Number:
                val = std::move(lookahead.value);
                lex();
                return val;
            case CosTokenType::ObjectStart:
                lex();
                val = parse_object_content();
                expect(CosTokenType::ObjectEnd);
                lex();
                return val;
            case CosTokenType::ArrayStart:
                lex();
                val = parse_array_content();
                expect(CosTokenType::ArrayEnd);
                lex();
                return val;
            default:
                throw CosError(QString("Expected token COS value, got %1").arg(int(lookahead.type)));
        }
    }
};

} // namespace glaxnimate::io::aep
