/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include <variant>
#include <cstdint>
#include <vector>
#include <cstring>
#include <memory>

#include <QByteArray>
#include <QSysInfo>
#include <QBuffer>

namespace glaxnimate::io::aep {

template<int size> struct IntSize;
template<> struct IntSize<1> { using uint = std::uint8_t;  using sint = std::int8_t; };
template<> struct IntSize<2> { using uint = std::uint16_t; using sint = std::int16_t; };
template<> struct IntSize<3> { using uint = std::uint32_t; using sint = std::int32_t; };
template<> struct IntSize<4> { using uint = std::uint32_t; using sint = std::int32_t; };
template<> struct IntSize<8> { using uint = std::uint64_t; using sint = std::int64_t; };

class Endianness
{
public:
    template<class T>
    constexpr T read_uint(const QByteArray& arr) const noexcept
    {
        if constexpr ( sizeof(T) == 1 )
        {
            return arr[0];
        }
        else
        {
            T v = 0;

            for ( int i = 0; i < arr.size(); i++ )
            {
                int j = swap ? arr.size() - i - 1 : i;
                v <<= 8;
                v |= std::uint8_t(arr[j]);
            }

            return v;
        }
    }

    template<int size>
    constexpr typename IntSize<size>::uint read_uint(const QByteArray& arr) const noexcept
    {
        return read_uint<typename IntSize<size>::uint>(arr);
    }

    template<int size>
    constexpr typename IntSize<size>::sint read_sint(const QByteArray& arr) const noexcept
    {
        using uint_t = typename IntSize<size>::uint;
        using sint_t = typename IntSize<size>::uint;
        uint_t uint = read_uint<size>(arr);
        constexpr const uint_t sbit = 1ull << (size * 8 - 1);

        if ( !(uint & sbit) )
            return uint;

        return -sint_t(~uint + 1);
    }


    template<class T>
    constexpr T read_sint(const QByteArray& arr) const noexcept
    {
        return read_uint<sizeof(T)>(arr);
    }

    /**
     * \note Expects IEEE 754 floats
     */
    constexpr float read_float32(const QByteArray& arr) const noexcept
    {
        union {
            std::uint32_t vali;
            float valf;
        } x {read_uint<std::uint32_t>(arr)};
        return x.valf;
    }

    /**
     * \note Expects IEEE 754 floats
     */
    constexpr double read_float64(const QByteArray& arr) const noexcept
    {
        union {
            std::uint64_t vali;
            double valf;
        } x {read_uint<std::uint64_t>(arr)};
        return x.valf;
    }

    static constexpr const Endianness Big() noexcept
    {
        return {QSysInfo::ByteOrder == QSysInfo::BigEndian};
    }

    static constexpr const Endianness Little() noexcept
    {
        return {QSysInfo::ByteOrder == QSysInfo::LittleEndian};
    }

private:
    constexpr Endianness(bool swap) noexcept : swap(swap) {}
    bool swap;
};

class RiffError : public std::runtime_error
{
public:
    RiffError(QString message) : runtime_error(message.toStdString()), message(std::move(message)) {}


    QString message;
};

class Flags
{
public:
    constexpr Flags(std::uint32_t data) noexcept
    : data(data) {}

    constexpr bool get(int byte, int bit) const noexcept
    {
        return (data >> (8*byte)) & (1 << bit);
    }

private:
    std::uint32_t data;
};


class BinaryReader
{
public:
    BinaryReader()
        : endian(Endianness::Big()),
        file(nullptr),
        file_pos(0),
        length_left(0)
    {}

    BinaryReader(Endianness endian, QIODevice* file, std::uint32_t length, qint64 pos)
        : endian(endian),
        file(file),
        file_pos(pos),
        length_left(length)
    {}

    BinaryReader(Endianness endian, QIODevice* file, std::uint32_t length)
        : endian(endian),
        file(file),
        file_pos(file->pos()),
        length_left(length)
    {}
/*
    BinaryReader(Endianness endian, QByteArray& data, std::uint32_t length)
        : endian(endian), buffer(std::make_unique<QBuffer>(&data)), file(buffer.get()), length_left(length)
    {}
*/
    BinaryReader sub_reader(std::uint32_t length)
    {
        if ( length > length_left )
            throw RiffError(QObject::tr("Not enough data"));
        length_left -= length;
        return sub_reader(length, 0);
    }

    /**
     * \brief Creates a sub-reader without affecting the current reader
     */
    BinaryReader sub_reader(std::uint32_t length, std::uint32_t offset) const
    {
        if ( length + offset > length_left )
            throw RiffError(QObject::tr("Not enough data"));

        return {endian, file, length, file_pos + offset};
    }

    void set_endianness(const Endianness& endian)
    {
        this->endian = endian;
    }

    QByteArray read()
    {
        return read(length_left);
    }

    QByteArray read(std::uint32_t length)
    {
        length_left -= length;
        file_pos += length;
        auto data = file->read(length);
        if ( std::uint32_t(data.size()) < length )
            throw RiffError(QObject::tr("Not enough data"));
        return data;
    }

    template<int size>
    typename IntSize<size>::uint read_uint()
    {
        return endian.read_uint<size>(read(size));
    }

    template<int size>
    typename IntSize<size>::sint read_sint()
    {
        return endian.read_sint<size>(read(size));
    }

    float read_float32()
    {
        return endian.read_float32(read(4));
    }

    double read_float64()
    {
        return endian.read_float64(read(8));
    }

    void skip(std::uint32_t length)
    {
        length_left -= length;
        file_pos += length;
        if ( file->skip(length) < length )
            throw RiffError(QObject::tr("Not enough data"));
    }

    std::int64_t available() const
    {
        return length_left;
    }

    QString read_utf8(std::uint32_t length)
    {
        return QString::fromUtf8(read(length));
    }

    /**
     * \brief Read a NUL-terminated UTF-8 string
     */
    QString read_utf8_nul(std::uint32_t length)
    {
        auto data = read(length);
        int str_len = data.indexOf('\0');
        return QString::fromUtf8(data.data(), str_len == -1 ? str_len : length);
    }

    QString read_utf8_nul()
    {
        return read_utf8_nul(length_left);
    }

    std::uint32_t size() const
    {
        return length_left;
    }

    void prepare()
    {
        file->seek(file_pos);
    }

    /**
     * \brief Defer data reading to a later point
     */
    void defer()
    {
        file->skip(length_left);
    }

private:
    Endianness endian;
//     std::unique_ptr<QBuffer> buffer;
    QIODevice* file;
    qint64 file_pos;
    std::int64_t length_left;
};

struct ChunkId
{
    char name[4] = "";

    ChunkId(const QByteArray& arr)
    {
        std::memcpy(name, (void*)arr.data(), std::min(4, arr.size()));
    }

    bool operator==(const char* ch) const {
        return std::strncmp(name, ch, 4) == 0;
    }

    bool operator!=(const char* ch) const {
        return std::strncmp(name, ch, 4) != 0;
    }

    QString to_string() const
    {
        return QString::fromUtf8(QByteArray(chunk->name().name, 4));
    }
};

struct RiffChunk
{
    ChunkId header;
    std::uint32_t length = 0;
    ChunkId subheader = {""};
    BinaryReader reader = {};
    std::vector<std::unique_ptr<RiffChunk>> children = {};

    using iterator = std::vector<std::unique_ptr<RiffChunk>>::const_iterator;

    struct RangeIterator
    {
    public:
        constexpr RangeIterator(const iterator& internal, const char* name, const RiffChunk* chunk)
            : internal(internal), name(name), chunk(chunk)
        {}

        RangeIterator& operator++()
        {
            internal = chunk->find(name, internal);
            return *this;
        }

        const RiffChunk& operator*() const
        {
            return **internal;
        }

        const RiffChunk* operator->() const
        {
            return internal->get();
        }

        bool operator==(const RangeIterator& other) const
        {
            return other.internal == internal;
        }

        bool operator!=(const RangeIterator& other) const
        {
            return other.internal != internal;
        }

    private:
        iterator internal;
        const char* name;
        const RiffChunk* chunk;
    };

    struct FindRange
    {
        RangeIterator a, b;
        RangeIterator begin() const { return a; }
        RangeIterator end() const { return b; }
    };

    bool operator==(const char* name) const
    {
        if ( header == name )
            return true;

        if ( header == "LIST" )
            return subheader == name;

        return false;
    }

    bool operator!=(const char* name) const
    {
        return !(*this == name);
    }

    BinaryReader data() const
    {
        BinaryReader data = reader;
        data.prepare();
        return data;
    }

    iterator find(const char* name) const
    {
        return find(name, children.begin());
    }

    iterator find(const char* name, iterator from) const
    {
        return std::find_if(from, children.end(), [name](const std::unique_ptr<RiffChunk>& c){ return *c == name; });
    }

    const RiffChunk* child(const char* name) const
    {
        auto it = find(name);
        if ( it == children.end() )
            return nullptr;
        return it->get();
    }

    FindRange find_all(const char* name) const
    {
        return {{find(name), name, this}, {children.end(), name, this}};
    }

    void find_multiple(
        const std::vector<const RiffChunk**>& out,
        const std::vector<const char*> names
    ) const
    {
        std::size_t found = 0;
        for ( const auto& child: children )
        {
            for ( std::size_t i = 0; i < names.size(); i++ )
            {
                if ( !*out[i] && *child == names[i] )
                {
                    *out[i] = child.get();
                    found++;
                    if ( found == names.size() )
                        return;
                }
            }
        }
    }

    const ChunkId& name() const
    {
        if ( header == "LIST" )
            return subheader;
        return header;
    }
};

class RiffReader
{
public:
    virtual ~RiffReader() = default;

    RiffChunk parse(QIODevice* file)
    {
        auto headerraw = file->read(4);
        ChunkId header = headerraw;
        Endianness endian = Endianness::Big();
        if ( header == "RIFF" )
            endian = Endianness::Little();
        else if ( header != "RIFX" )
            throw RiffError(QObject::tr("Unknown format %1").arg(QString(headerraw)));

        auto length = endian.read_uint<4>(file->read(4));

        BinaryReader reader = BinaryReader(endian, file, length);
        ChunkId format = reader.read(4);
        RiffChunk chunk{header, length, format};
        chunk.reader = reader;
        on_root(chunk);
        return chunk;
    }

protected:
    RiffChunk read_chunk(BinaryReader& reader)
    {
        ChunkId header = reader.read(4);
        auto length = reader.read_uint<4>();
        RiffChunk chunk{header, length};

        chunk.reader = reader.sub_reader(length);

        on_chunk(chunk);

        if ( length % 2 )
            reader.skip(1);

        return chunk;
    }

    std::vector<std::unique_ptr<RiffChunk>> read_chunks(BinaryReader& reader)
    {
        std::vector<std::unique_ptr<RiffChunk>> chunks;
        while ( reader.available() )
            chunks.push_back(std::make_unique<RiffChunk>(read_chunk(reader)));
        return chunks;
    }

    virtual void on_root(RiffChunk& chunk)
    {
        chunk.children = read_chunks(chunk.reader);
    }

    virtual void on_chunk(RiffChunk& chunk)
    {
        if ( chunk.header == "LIST" )
        {
            chunk.subheader = chunk.reader.read(4);
            chunk.children = read_chunks(chunk.reader);
        }
        else
        {
            chunk.reader.defer();
        }
    }
};

} // namespace glaxnimate::io::aep
