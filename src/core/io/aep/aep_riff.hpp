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
    void on_chunk(RiffChunk & chunk) override
    {
        if ( chunk.header == "tdsn" || chunk.header == "fnam" || chunk.header == "pdnm" )
        {
            chunk.children = read_chunks(chunk.reader);
        }
        else if ( chunk.header == "LIST" )
        {
            chunk.subheader = chunk.reader.read(4);
            if ( chunk.subheader != "btdk" )
                chunk.children = read_chunks(chunk.reader);
            else
                chunk.reader.defer();
        }
        else
        {
            chunk.reader.defer();
        }
    }
};

} // namespace glaxnimate::io::aep

