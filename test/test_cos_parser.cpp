/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <QtTest/QtTest>

#include <vector>
#include <cstring>
#include "io/aep/cos.hpp"

using namespace glaxnimate::io::aep;


inline QByteArray operator "" _b(const char* c, std::size_t sz)
{
    return QByteArray(c, sz);
}

#define COS_VALUE(val, type_index, expected) \
    QCOMPARE(val.type(), CosValue::Index::type_index); \
    QCOMPARE(val.get<CosValue::Index::type_index>(), (expected));

#define COS_TOKEN_TYPE(token, tok_type) \
    QCOMPARE(int(token.type), int(CosTokenType::tok_type));

#define COS_TOKEN(token, tok_type, type_index, expected) \
    COS_TOKEN_TYPE(token, tok_type) \
    COS_VALUE(token.value, type_index, expected)


class TestCase: public QObject
{
    Q_OBJECT

    CosValue parse(QByteArray ba)
    {
        CosParser parser(std::move(ba));
        return parser.parse();
    }

    CosToken lex(QByteArray ba)
    {
        CosLexer lexer(std::move(ba));
        return lexer.next_token();
    }

private slots:
    void test_lex_object_start()
    {
        CosLexer l("<<"_b);
        auto token = l.next_token();
        QCOMPARE(token.type, CosTokenType::ObjectStart);
        QVERIFY_EXCEPTION_THROWN(lex("<"), CosError);
        QVERIFY_EXCEPTION_THROWN(lex("<a"), CosError);
    }

    void test_lex_identidfier()
    {
        CosLexer l("/foo/bar<<"_b);
        auto token = l.next_token();
        COS_TOKEN(token, Identifier, String, "foo");
        token = l.next_token();
        COS_TOKEN(token, Identifier, String, "bar");
        token = l.next_token();
        QCOMPARE(token.type, CosTokenType::ObjectStart);
        COS_TOKEN(lex("/foo#62ar"_b), Identifier, String, "foobar");
        QVERIFY_EXCEPTION_THROWN(lex("/foo#6"), CosError);
        QVERIFY_EXCEPTION_THROWN(lex("/foo#6r"), CosError);
    }

    void test_lex_spaces()
    {
        CosLexer l("  \t\n/1\n /2\n% /3 /4\n /5"_b);
        auto token = l.next_token();
        COS_TOKEN(token, Identifier, String, "1");
        token = l.next_token();
        COS_TOKEN(token, Identifier, String, "2");
        token = l.next_token();
        COS_TOKEN(token, Identifier, String, "5");
        token = l.next_token();
        COS_TOKEN_TYPE(token, Eof);
    }

    void test_lex_xstring()
    {
        CosLexer l("<F00d><a><13 37>"_b);
        auto token = l.next_token();
        COS_TOKEN(token, HexString, Bytes, "\xf0\x0d"_b);
        token = l.next_token();
        COS_TOKEN(token, HexString, Bytes, "\xa0"_b);
        token = l.next_token();
        COS_TOKEN(token, HexString, Bytes, "\x13\x37"_b);
        QVERIFY_EXCEPTION_THROWN(lex("<F00"), CosError);
        QVERIFY_EXCEPTION_THROWN(lex("<G>"), CosError);
        QVERIFY_EXCEPTION_THROWN(lex("<FG>"), CosError);
    }

    void test_lex_object_end()
    {
        CosLexer l(">>"_b);
        auto token = l.next_token();
        COS_TOKEN_TYPE(token, ObjectEnd);
        QVERIFY_EXCEPTION_THROWN(lex(">"), CosError);
        QVERIFY_EXCEPTION_THROWN(lex(">a"), CosError);
    }

    void test_lex_array()
    {
        CosLexer l("[]"_b);
        auto token = l.next_token();
        COS_TOKEN_TYPE(token, ArrayStart);
        token = l.next_token();
        COS_TOKEN_TYPE(token, ArrayEnd);
    }

    void test_lex_string_ascii()
    {
        CosLexer l("(Hello)"_b);
        auto token = l.next_token();
        COS_TOKEN(token, String, String, "Hello");
    }

    void test_lex_string_utf8()
    {
        CosLexer l("(H\xe2\x82\xacllo)"_b);
        auto token = l.next_token();
        COS_TOKEN(token, String, String, "H€llo");
    }

    void test_lex_string_utf16le()
    {
        CosLexer l("(\xff\xfeH\0\xac l\0l\0o\0)"_b);
        auto token = l.next_token();
        COS_TOKEN(token, String, String, "H€llo");
    }

    void test_lex_string_utf16be()
    {
        CosLexer l("(\xfe\xff\0H \xac\0l\0l\0o)"_b);
        auto token = l.next_token();
        COS_TOKEN(token, String, String, "H€llo");
    }

    void test_lex_string_escapes()
    {
        CosLexer l(R"((\b\n\f\r\(\)\\\100\41a\41))"_b);
        auto token = l.next_token();
        COS_TOKEN(token, String, String, QString("\b\n\f\r()\\@!a!"));
    }

    void test_lex_string_newlines()
    {
        CosLexer l("(1\n2\n\r3\r4\n\r5)"_b);
        auto token = l.next_token();
        COS_TOKEN(token, String, String, QString("1\n2\n3\n4\n5"));
    }

    void test_lex_string_errors()
    {
        QVERIFY_EXCEPTION_THROWN(lex("(Foo"), CosError);
        QVERIFY_EXCEPTION_THROWN(lex("(\\@)"), CosError);
        QVERIFY_EXCEPTION_THROWN(lex("(\\"), CosError);
        QVERIFY_EXCEPTION_THROWN(lex("(\\1"), CosError);
    }

    void test_lex_keywords()
    {
        CosLexer l("true false null foo"_b);
        auto token = l.next_token();
        COS_TOKEN(token, Boolean, Boolean, true);
        token = l.next_token();
        COS_TOKEN(token, Boolean, Boolean, false);
        token = l.next_token();
        COS_TOKEN(token, Null, Null, nullptr);
        QVERIFY_EXCEPTION_THROWN(l.next_token(), CosError);
    }

    void test_lex_number()
    {
        COS_TOKEN(lex("1234"_b), Number, Number, 1234);
        COS_TOKEN(lex("+1234"_b), Number, Number, 1234);
        COS_TOKEN(lex("-1234"_b), Number, Number, -1234);
        COS_TOKEN(lex(".25"_b), Number, Number, 0.25);
        COS_TOKEN(lex("0.25"_b), Number, Number, 0.25);
        COS_TOKEN(lex("16.25"_b), Number, Number, 16.25);
        COS_TOKEN(lex("-.25"_b), Number, Number, -0.25);
        COS_TOKEN(lex("+.25"_b), Number, Number, 0.25);
        COS_TOKEN(lex("-0.25"_b), Number, Number, -0.25);
        COS_TOKEN(lex("+0.25"_b), Number, Number, 0.25);
        CosLexer l("12true0.5false"_b);
        auto token = l.next_token();
        COS_TOKEN(token, Number, Number, 12);
        token = l.next_token();
        COS_TOKEN(token, Boolean, Boolean, true);
        token = l.next_token();
        COS_TOKEN(token, Number, Number, 0.5);
        token = l.next_token();
        COS_TOKEN(token, Boolean, Boolean, false);
    }

    void test_unkown_token()
    {
        QVERIFY_EXCEPTION_THROWN(lex("@"), CosError);
    }
};

QTEST_GUILESS_MAIN(TestCase)
#include "test_cos_parser.moc"


