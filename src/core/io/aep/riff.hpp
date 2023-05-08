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
};

struct RiffChunk
{
    ChunkId header;
    std::uint32_t length = 0;
    ChunkId subheader = {""};
    QByteArray data = {};
    std::vector<std::unique_ptr<RiffChunk>> children = {};

    using iterator = std::vector<std::unique_ptr<RiffChunk>>::const_iterator;

    bool operator==(const char* name) const
    {
        if ( header == name )
            return true;

        if ( header == "LIST" )
            return subheader == name;

        return false;
    }

    iterator find(const char* name) const
    {
        return find(name, children.begin());
    }

    iterator find(const char* name, iterator from) const
    {
        return std::find_if(from, children.end(), [name](const std::unique_ptr<RiffChunk>& c){ return *c == name; });
    }

    std::vector<const RiffChunk*> find_multiple(const std::vector<const char*> names) const
    {
        std::vector<const RiffChunk*> out;
        out.resize(names.size());
        std::size_t found = 0;
        for ( const auto& child: children )
        {
            for ( std::size_t i = 0; i < names.size(); i++ )
            {
                if ( !out[i] && *child == names[i] )
                {
                    out[i] = child.get();
                    found++;
                    if ( found == names.size() )
                        return out;
                }
            }
        }
        return out;
    }
};

template<int size> struct IntSize;
template<> struct IntSize<1> { using uint = std::uint8_t;  using sint = std::int8_t; };
template<> struct IntSize<2> { using uint = std::uint16_t; using sint = std::int16_t; };
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


class BinaryReader
{
public:
    BinaryReader()
        : endian(Endianness::Big()), file(nullptr), length_left(0)
    {}

    BinaryReader(Endianness endian, QIODevice* file, std::uint32_t length)
        : endian(endian), file(file), length_left(length)
    {}

    BinaryReader(Endianness endian, QByteArray& data, std::uint32_t length)
        : endian(endian), buffer(std::make_unique<QBuffer>(&data)), file(buffer.get()), length_left(length)
    {}

    BinaryReader sub_reader(std::uint32_t length)
    {
        if ( length > length_left )
            throw RiffError(QObject::tr("Not enough data"));
        this->length_left -= length;
        return {endian, file, length};
    }

    void set_endianness(const Endianness& endian)
    {
        this->endian = endian;
    }

    QByteArray read(std::uint32_t length)
    {
        this->length_left -= length;
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
        this->length_left -= length;
        if ( file->skip(length) < length )
            throw RiffError(QObject::tr("Not enough data"));
    }

    std::int64_t available() const
    {
        return length_left;
    }

private:
    Endianness endian;
    std::unique_ptr<QBuffer> buffer;
    QIODevice* file;
    std::int64_t length_left;
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
        on_root(reader, chunk);
        return chunk;
    }

protected:
    RiffChunk read_chunk(BinaryReader& reader)
    {
        ChunkId header = reader.read(4);
        auto length = reader.read_uint<4>();
        RiffChunk chunk{header, length};

        auto sub_reader = reader.sub_reader(length);

        if ( chunk.header == "LIST" )
            on_list(sub_reader, chunk);
        else
            on_chunk(sub_reader, chunk);

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

    virtual void on_root(BinaryReader& reader, RiffChunk& chunk)
    {
        chunk.children = read_chunks(reader);
    }

    virtual void on_list(BinaryReader& reader, RiffChunk& chunk)
    {
        chunk.subheader = reader.read(4);
        chunk.children = read_chunks(reader);
    }

    virtual void on_chunk(BinaryReader& reader, RiffChunk& chunk)
    {
        chunk.data = reader.read(chunk.length);
    }
};



} // namespace glaxnimate::io::aep
