/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "riff.hpp"

namespace glaxnimate::io::aep {

class AepRiff : public RiffReader
{
protected:
    void on_chunk(BinaryReader & reader, RiffChunk & chunk) override
    {
        if ( chunk.header == "tdsn" || chunk.header == "fnam" || chunk.header == "pdnm" )
            chunk.children = read_chunks(reader);
        else
            chunk.data = reader.sub_reader(chunk.length);
    }

    void on_list(BinaryReader& reader, RiffChunk& chunk) override
    {
        chunk.subheader = reader.read(4);
        if ( chunk.subheader == "btdk" )
            chunk.data = reader.sub_reader(chunk.length);
        else
            chunk.children = read_chunks(reader);
    }
};

} // namespace glaxnimate::io::aep

