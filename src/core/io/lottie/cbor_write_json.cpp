#include "cbor_write_json.hpp"
#include <QCborValue>
#include <QCborMap>
#include <QCborArray>
#include <cstdint>

/******************************************************************************
 * These function are mostly taken from Qt code
 * See
 * https://github.com/qt/qtbase/blob/dev/src/corelib/serialization/qjsonwriter.cpp
 * https://github.com/qt/qtbase/blob/dev/src/corelib/text/qstringconverter_p.h
 ******************************************************************************/

#ifndef __cpp_char8_t
enum char8_t : uchar {};
#endif


struct QUtf8BaseTraits
{
    static const bool isTrusted = false;
    static const bool allowNonCharacters = true;
    static const bool skipAsciiHandling = false;
    static const int Error = -1;
    static const int EndOfString = -2;

    static bool isValidCharacter(uint u)
    { return int(u) >= 0; }

    static void appendByte(uchar *&ptr, uchar b)
    { *ptr++ = b; }

    static void appendByte(char8_t *&ptr, char8_t b)
    { *ptr++ = b; }

    static uchar peekByte(const uchar *ptr, qsizetype n = 0)
    { return ptr[n]; }

    static uchar peekByte(const char8_t *ptr, int n = 0)
    { return ptr[n]; }

    static qptrdiff availableBytes(const uchar *ptr, const uchar *end)
    { return end - ptr; }

    static qptrdiff availableBytes(const char8_t *ptr, const char8_t *end)
    { return end - ptr; }

    static void advanceByte(const uchar *&ptr, qsizetype n = 1)
    { ptr += n; }

    static void advanceByte(const char8_t *&ptr, int n = 1)
    { ptr += n; }

    static void appendUtf16(ushort *&ptr, ushort uc)
    { *ptr++ = uc; }

    static void appendUtf16(char16_t *&ptr, ushort uc)
    { *ptr++ = char16_t(uc); }

    static void appendUcs4(ushort *&ptr, uint uc)
    {
        appendUtf16(ptr, QChar::highSurrogate(uc));
        appendUtf16(ptr, QChar::lowSurrogate(uc));
    }

    static void appendUcs4(char16_t *&ptr, char32_t uc)
    {
        appendUtf16(ptr, QChar::highSurrogate(uc));
        appendUtf16(ptr, QChar::lowSurrogate(uc));
    }

    static ushort peekUtf16(const ushort *ptr, qsizetype n = 0)
    { return ptr[n]; }

    static ushort peekUtf16(const char16_t *ptr, int n = 0)
    { return ptr[n]; }

    static qptrdiff availableUtf16(const ushort *ptr, const ushort *end)
    { return end - ptr; }

    static qptrdiff availableUtf16(const char16_t *ptr, const char16_t *end)
    { return end - ptr; }

    static void advanceUtf16(const ushort *&ptr, qsizetype n = 1)
    { ptr += n; }

    static void advanceUtf16(const char16_t *&ptr, int n = 1)
    { ptr += n; }

    // it's possible to output to UCS-4 too
    static void appendUtf16(uint *&ptr, ushort uc)
    { *ptr++ = uc; }

    static void appendUtf16(char32_t *&ptr, ushort uc)
    { *ptr++ = char32_t(uc); }

    static void appendUcs4(uint *&ptr, uint uc)
    { *ptr++ = uc; }

    static void appendUcs4(char32_t *&ptr, uint uc)
    { *ptr++ = char32_t(uc); }
};


namespace QUtf8Functions
{
    /// returns 0 on success; errors can only happen if \a u is a surrogate:
    /// Error if \a u is a low surrogate;
    /// if \a u is a high surrogate, Error if the next isn't a low one,
    /// EndOfString if we run into the end of the string.
    template <typename Traits, typename OutputPtr, typename InputPtr> inline
    int toUtf8(ushort u, OutputPtr &dst, InputPtr &src, InputPtr end)
    {
        if (!Traits::skipAsciiHandling && u < 0x80) {
            // U+0000 to U+007F (US-ASCII) - one byte
            Traits::appendByte(dst, uchar(u));
            return 0;
        } else if (u < 0x0800) {
            // U+0080 to U+07FF - two bytes
            // first of two bytes
            Traits::appendByte(dst, 0xc0 | uchar(u >> 6));
        } else {
            if (!QChar::isSurrogate(u)) {
                // U+0800 to U+FFFF (except U+D800-U+DFFF) - three bytes
                if (!Traits::allowNonCharacters && QChar::isNonCharacter(u))
                    return Traits::Error;

                // first of three bytes
                Traits::appendByte(dst, 0xe0 | uchar(u >> 12));
            } else {
                // U+10000 to U+10FFFF - four bytes
                // need to get one extra codepoint
                if (Traits::availableUtf16(src, end) == 0)
                    return Traits::EndOfString;

                ushort low = Traits::peekUtf16(src);
                if (!QChar::isHighSurrogate(u))
                    return Traits::Error;
                if (!QChar::isLowSurrogate(low))
                    return Traits::Error;

                Traits::advanceUtf16(src);
                uint ucs4 = QChar::surrogateToUcs4(u, low);

                if (!Traits::allowNonCharacters && QChar::isNonCharacter(ucs4))
                    return Traits::Error;

                // first byte
                Traits::appendByte(dst, 0xf0 | (uchar(ucs4 >> 18) & 0xf));

                // second of four bytes
                Traits::appendByte(dst, 0x80 | (uchar(ucs4 >> 12) & 0x3f));

                // for the rest of the bytes
                u = ushort(ucs4);
            }

            // second to last byte
            Traits::appendByte(dst, 0x80 | (uchar(u >> 6) & 0x3f));
        }

        // last byte
        Traits::appendByte(dst, 0x80 | (u & 0x3f));
        return 0;
    }
}


static void objectContentToJson(const QCborMap& p, QByteArray &json, int indent, bool compact);
static void arrayContentToJson(const QCborArray& a, QByteArray &json, int indent, bool compact);

static inline uchar hexdig(uint u)
{
    return (u < 0xa ? '0' + u : 'a' + u - 0xa);
}

static QByteArray escapedString(const QString &s)
{
    // give it a minimum size to ensure the resize() below always adds enough space
    QByteArray ba(qMax(s.length(), 16), Qt::Uninitialized);

    uchar *cursor = reinterpret_cast<uchar *>(const_cast<char *>(ba.constData()));
    const uchar *ba_end = cursor + ba.length();
    const ushort *src = reinterpret_cast<const ushort *>(s.constBegin());
    const ushort *const end = reinterpret_cast<const ushort *>(s.constEnd());

    while (src != end) {
        if (cursor >= ba_end - 6) {
            // ensure we have enough space
            int pos = cursor - (const uchar *)ba.constData();
            ba.resize(ba.size()*2);
            cursor = (uchar *)ba.data() + pos;
            ba_end = (const uchar *)ba.constData() + ba.length();
        }

        uint u = *src++;
        if (u < 0x80) {
            if (u < 0x20 || u == 0x22 || u == 0x5c) {
                *cursor++ = '\\';
                switch (u) {
                case 0x22:
                    *cursor++ = '"';
                    break;
                case 0x5c:
                    *cursor++ = '\\';
                    break;
                case 0x8:
                    *cursor++ = 'b';
                    break;
                case 0xc:
                    *cursor++ = 'f';
                    break;
                case 0xa:
                    *cursor++ = 'n';
                    break;
                case 0xd:
                    *cursor++ = 'r';
                    break;
                case 0x9:
                    *cursor++ = 't';
                    break;
                default:
                    *cursor++ = 'u';
                    *cursor++ = '0';
                    *cursor++ = '0';
                    *cursor++ = hexdig(u>>4);
                    *cursor++ = hexdig(u & 0xf);
               }
            } else {
                *cursor++ = (uchar)u;
            }
        } else if (QUtf8Functions::toUtf8<QUtf8BaseTraits>(u, cursor, src, end) < 0) {
            // failed to get valid utf8 use JSON escape sequence
            *cursor++ = '\\';
            *cursor++ = 'u';
            *cursor++ = hexdig(u>>12 & 0x0f);
            *cursor++ = hexdig(u>>8 & 0x0f);
            *cursor++ = hexdig(u>>4 & 0x0f);
            *cursor++ = hexdig(u & 0x0f);
        }
    }

    ba.resize(cursor - (const uchar *)ba.constData());
    return ba;
}

static void valueToJson(const QCborValue &v, QByteArray &json, int indent, bool compact)
{
    QCborValue::Type type = v.type();
    switch (type) {
    case QCborValue::True:
        json += "true";
        break;
    case QCborValue::False:
        json += "false";
        break;
    case QCborValue::Integer:
        json += QByteArray::number(v.toInteger());
        break;
    case QCborValue::Double: {
        const double d = v.toDouble();
        if (qIsFinite(d))
            json += QByteArray::number(d, 'g', QLocale::FloatingPointShortest);
        else
            json += "null"; // +INF || -INF || NaN (see RFC4627#section2.4)
        break;
    }
    case QCborValue::String:
        json += '"';
        json += escapedString(v.toString());
        json += '"';
        break;
    case QCborValue::Array:
        json += compact ? "[" : "[\n";
        arrayContentToJson(v.toArray(), json, indent + (compact ? 0 : 1), compact);
        json += QByteArray(4*indent, ' ');
        json += ']';
        break;
    case QCborValue::Map:
        json += compact ? "{" : "{\n";
        objectContentToJson(v.toMap(), json, indent + (compact ? 0 : 1), compact);
        json += QByteArray(4*indent, ' ');
        json += '}';
        break;
    case QCborValue::Null:
    default:
        json += "null";
    }
}

static void arrayContentToJson(const QCborArray& a, QByteArray &json, int indent, bool compact)
{
    if ( a.empty() )
        return;

    QByteArray indentString(4*indent, ' ');

    qsizetype i = 0;
    while (true) {
        json += indentString;
        valueToJson(a.at(i), json, indent, compact);

        if (++i == a.size()) {
            if (!compact)
                json += '\n';
            break;
        }

        json += compact ? "," : ",\n";
    }
}


static void objectContentToJson(const QCborMap& o, QByteArray &json, int indent, bool compact)
{
    if ( o.empty() )
        return;

    QByteArray indentString(4*indent, ' ');

    auto it = o.begin();
    auto end = o.end();

    while (true) {
        json += indentString;
        json += '"';
        json += escapedString(it.key().toString());
        json += compact ? "\":" : "\": ";
        valueToJson(it.value(), json, indent, compact);

        ++it;
        if ( it == end ) {
            if (!compact)
                json += '\n';
            break;
        }

        json += compact ? "," : ",\n";
    }
}

QByteArray io::lottie::cbor_write_json(const QCborMap &o, bool compact)
{
    QByteArray json;
    json += compact ? "{" : "{\n";
    objectContentToJson(o, json, 0, compact);
    json += compact ? "}" : "}\n";
    return json;
}
