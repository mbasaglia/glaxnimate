/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "detail.hpp"

#include <algorithm>

#include <QDomDocument>

#include "app/utils/qstring_literal.hpp"

namespace glaxnimate::io::svg::detail {

class CssSelector
{
public:
    void set_tag(const QString& tag)
    {
        if ( tag != '*'_qc && this->tag.isEmpty() )
            specificity += 1;
        this->tag = tag;
    }

    void set_id(const QString& id)
    {
        if ( this->id.isEmpty() )
            specificity += 100;
        this->id = id;
    }

    void add_class(const QString& class_name)
    {
        classes.push_back(class_name);
        specificity += 10;
    }

    bool match(const QDomElement& element, const std::unordered_set<QString>& class_names) const
    {
        if ( !tag.isEmpty() && tag != "*"_qs && tag != element.tagName() )
            return false;

        if ( !id.isEmpty() && id != element.attribute("id"_qs) )
            return false;

        for ( const auto& class_name : classes )
        {
            if ( class_names.count(class_name) == 0 )
                return false;
        }

        if ( !rule.isEmpty() )
            return false;

        return true;
    }

    bool empty() const
    {
        return tag.isEmpty() && id.isEmpty() && classes.empty() && rule.isEmpty();
    }

    void set_at_rule(const QString& rule)
    {
        this->rule = rule;
    }

    const QString& at_rule() const
    {
        return this->rule;
    }

private:
    int specificity = 0;
    QString tag;
    QString id;
    QStringList classes;
    QString rule;
    friend struct CssStyleBlock;
};

struct CssStyleBlock
{
    CssSelector selector;
    Style::Map style;

    void merge_into(Style& output) const
    {
        for ( const auto& p : style )
            output[p.first] = p.second;
    }

    bool operator<(const CssStyleBlock& other) const
    {
        return selector.specificity < other.selector.specificity;
    }
};


class CssParser
{
public:
    CssParser(std::vector<CssStyleBlock>& blocks)
        : blocks(blocks)
    {
    }

    void parse(const QString& css)
    {
        data = css;
        index = -1;

        parse_selector();
    }


private:
    enum class TokenType
    {
        SelectorTag,
        SelectorClass,
        SelectorId,
        SelectorOther,
        SelectorComma,
        SelectorAt,

        BlockBegin,
        BlockEnd,

        RuleName,
        RuleColon,
        RuleArg,
        RuleSemicolon,

        Eof,
    };

    using Token = std::pair<TokenType, QString>;

    QChar next_ch_raw()
    {
        ++index;

        if ( eof() )
            return {};

        return data[index];
    }

    bool eof() const
    {
        return index >= data.size();
    }

    void back()
    {
        if ( !eof() )
            --index;
    }

    QChar next_ch()
    {
        QChar c = next_ch_raw();

        // Skip comments
        if ( c == '/'_qc )
        {
            QChar d = next_ch_raw();
            if ( d == '*'_qc )
            {
                while ( true )
                {
                    d = next_ch_raw();

                    if ( eof() )
                        return {};

                    if ( d == '*'_qc )
                    {
                        d = next_ch_raw();
                        // Treat comments as spaces
                        if ( d == '/'_qc )
                            return ' '_qc;
                        back();
                    }
                }
            }
            else
            {
                back();
            }
        }

        return c;
    }


    static bool is_identifier_start(const QChar& ch)
    {
        return ch.isLetter() || ch == '_'_qc || ch == '-'_qc;
    }

    static bool is_identifier(const QChar& ch)
    {
        return is_identifier_start(ch) || ch.isNumber();
    }

    QString lex_identifier()
    {
        QString id;
        QChar ch;

        while ( true )
        {
            ch = next_ch();
            if ( is_identifier(ch) )
                id += ch;
            else
                break;
        }

        back();

        return id;
    }

    QString lex_at_selector()
    {
        QString id = "@"_qs;
        QChar ch;

        while ( true )
        {
            ch = next_ch();
            if ( ch == '{'_qc || ch == ','_qc )
                break;
            else
                id += ch;
        }

        back();

        return id.trimmed();
    }

    Token lex_selector()
    {
        QChar ch = next_ch();
        if ( eof() )
            return {TokenType::Eof, {}};

        if ( is_identifier_start(ch) )
            return {TokenType::SelectorTag, ch + lex_identifier()};
        else if ( ch == '#'_qc )
            return {TokenType::SelectorId, lex_identifier()};
        else if ( ch == '.'_qc )
            return {TokenType::SelectorClass, lex_identifier()};
        else if ( ch == ','_qc )
            return {TokenType::SelectorComma, {}};
        else if ( ch == '{'_qc )
            return {TokenType::BlockBegin, {}};
        else if ( ch == '*'_qc )
            return {TokenType::SelectorTag, ch};
        else if ( ch == '@'_qc )
            return {TokenType::SelectorAt, lex_at_selector()};

        if ( ch.isSpace() )
        {
            skip_space();
            ch = next_ch();
            if ( ch == ','_qc )
                return {TokenType::SelectorComma, {}};
            if ( ch == '{'_qc )
                return {TokenType::BlockBegin, {}};
            back();
        }

        return {TokenType::SelectorOther, {}};
    }

    Token ignore_selector()
    {
        Token token = {TokenType::Eof, {}};

        do
        {
            token = lex_selector();
            if ( token.first == TokenType::SelectorComma )
                return lex_selector();
        }
        while ( token.first != TokenType::Eof && token.first != TokenType::BlockBegin );

        return token;
    }

    void skip_space()
    {
        QChar c;
        do
        {
            c = next_ch();
        }
        while ( !eof() && c.isSpace() );

        back();
    }

    void ignore_block()
    {
        Token token = {TokenType::Eof, {}};

        do
        {
            token = lex_selector();
        }
        while ( token.first != TokenType::Eof && token.first != TokenType::BlockEnd );
    }

    bool parse_selector_step(const Token& token)
    {
        if ( token.first == TokenType::SelectorClass )
            selectors.back().add_class(token.second);
        else if ( token.first == TokenType::SelectorId )
            selectors.back().set_id(token.second);
        else if ( token.first == TokenType::SelectorTag )
            selectors.back().set_tag(token.second);
        else if ( token.first == TokenType::SelectorAt )
            selectors.back().set_at_rule(token.second);
        else
            return false;

        return true;
    }

    void parse_selector()
    {
        while ( true )
        {
            skip_space();
            selectors.clear();
            Token token = lex_selector();

            if ( token.first == TokenType::BlockBegin )
            {
                ignore_block();
                token = lex_selector();
            }

            while ( true )
            {
                selectors.push_back({});

                while ( parse_selector_step(token) )
                    token = lex_selector();

                if ( eof() )
                    return;

                if ( token.first == TokenType::BlockBegin )
                {
                    if ( selectors.back().empty() )
                        selectors.pop_back();
                    break;
                }

                if ( token.first != TokenType::SelectorComma )
                {
                    token = ignore_selector();
                    selectors.pop_back();
                }
                else
                {
                    skip_space();
                    token = lex_selector();
                }
            }

            if ( selectors.empty() )
                ignore_block();
            else
                parse_block();
        }
    }

    Token lex_rule()
    {
        skip_space();

        QChar ch = next_ch();
        if ( eof() )
            return {TokenType::Eof, {}};

        if ( is_identifier_start(ch) )
            return {TokenType::RuleName, ch + lex_identifier()};
        else if ( ch == ':'_qc )
            return {TokenType::RuleColon, {}};
        else if ( ch == ';'_qc )
            return {TokenType::RuleSemicolon, {}};
        else if ( ch == '}'_qc )
            return {TokenType::BlockEnd, {}};

        return {TokenType::RuleArg, ch};

    }

    Token ignore_rule()
    {
        Token token = lex_rule();
        while ( token.first != TokenType::Eof && token.first != TokenType::RuleSemicolon && token.first != TokenType::BlockEnd )
            token = lex_rule();
        return token;
    }

    void lex_quoted_string(QString& value, QChar terminator)
    {
        while ( true )
        {
            QChar ch = next_ch();

            if ( eof() )
                break;

            value += ch;

            if ( ch == terminator )
                break;

            if ( ch == '\\'_qc )
            {
                ch = next_ch();
                if ( eof() )
                    break;
                value += ch;
            }
        }
    }

    Token lex_rule_value(QString& value)
    {
        if ( value == "\""_qs || value == "'"_qs )
            lex_quoted_string(value, value[0]);

        while ( true )
        {
            QChar ch = next_ch();
            if ( eof() )
                return {TokenType::Eof, {}};
            else if ( ch == ';'_qc )
                return {TokenType::RuleSemicolon, {}};
            else if ( ch == '}'_qc )
                return {TokenType::BlockEnd, {}};

            value += ch;
            if ( ch == '"'_qc || ch == '\''_qc )
            {
                lex_quoted_string(value, ch);
            }
        }
    }

    void parse_block()
    {
        rules.clear();

        while ( true )
        {
            Token token = lex_rule();
            if ( eof() || token.first == TokenType::BlockEnd )
                break;

            if ( token.first != TokenType::RuleName )
            {
                ignore_rule();
                continue;
            }

            QString name = token.second;

            if ( lex_rule().first != TokenType::RuleColon )
            {
                ignore_rule();
                continue;
            }

            token = lex_rule();
            if ( eof() || token.first == TokenType::BlockEnd )
                break;
            if ( token.first == TokenType::RuleSemicolon )
                continue;

            QString value = token.second;

            token = lex_rule_value(value);
            if ( !value.isEmpty() )
                rules[name] = value.trimmed();

            if ( eof() || token.first == TokenType::BlockEnd )
                break;
        }

        for ( const auto& selector : selectors )
            blocks.push_back({selector, rules});

        rules.clear();
        selectors.clear();
    }

    QString data;
    int index = 0;
    std::vector<CssStyleBlock>& blocks;
    std::vector<CssSelector> selectors;
    Style::Map rules;
};

} // namespace glaxnimate::io::svg::detail
