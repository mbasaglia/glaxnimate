#pragma once

#include <QDomDocument>
#include "aep_riff.hpp"
#include "io/svg/detail.hpp"

namespace glaxnimate::io::aep {

class AepxConverter
{
public:

    RiffChunk aepx_to_chunk(const QDomElement& element)
    {
        QString header = element.tagName();

        if ( header == "ProjectXMPMetadata" )
        {
            return chunk("XMPM", text(element.text()));
        }
        else if ( header == "string" )
        {
            return chunk("Utf8", text(element.text()));
        }
        else if ( header == "numS" )
        {
            std::uint32_t val = element.firstChildElement().text().toUInt();
            auto data = buffer(Endianness::Big().write_uint(val));
            return chunk(header, data);
        }
        else if ( header == "ppSn" )
        {
            std::uint32_t val = element.firstChildElement().text().toDouble();
            auto data = buffer(Endianness::Big().write_float64(val));
            return chunk(header, data);
        }
        else if ( element.hasAttribute("bdata") )
        {
            return chunk(header, hex(element.attribute("bdata")));
        }

        ChunkId riff_header = header.toLatin1();
        ChunkId subheader = {""};
        if ( header == "AfterEffectsProject" )
        {
            riff_header = {"RIFX"};
        }
        else if ( !AepRiff::is_fake_list(riff_header) )
        {
            subheader = riff_header;
            riff_header = {"LIST"};
        }

        return {riff_header, 0, subheader, {}, read_chunk_list(svg::detail::ElementRange(element))};
    }

    std::vector<std::unique_ptr<RiffChunk>> read_chunk_list(const svg::detail::ElementRange& range)
    {
        std::vector<std::unique_ptr<RiffChunk>> out;
        out.reserve(range.size());
        for ( const auto& el : range )
            out.push_back(std::make_unique<RiffChunk>(aepx_to_chunk(el)));
        return out;
    }

private:
    struct BinaryData
    {
        QByteArray data;
        QBuffer file;
        std::uint32_t length;
    };

    RiffChunk chunk(const QString& header, BinaryData* data, const QString& subheader = {})
    {
        return {
            header.toLatin1(), data->length, subheader.toLatin1(),
            {Endianness::Big(), &data->file, data->length, 0}
        };
    }

    BinaryData* buffer(QByteArray&& content)
    {
        data.push_back(std::make_unique<BinaryData>());
        data.back()->length = content.size();
        data.back()->data = std::move(content);
        data.back()->file.setBuffer(&data.back()->data);
        data.back()->file.open(QIODevice::ReadOnly);
        return data.back().get();
    }

    BinaryData* hex(const QString& hex)
    {
        return buffer(QByteArray::fromHex(hex.toLatin1()));
    }

    BinaryData* text(const QString& string)
    {
        return buffer(string.toUtf8());
    }

    std::vector<std::unique_ptr<BinaryData>> data;
};

} // namespace glaxnimate::io::aep
