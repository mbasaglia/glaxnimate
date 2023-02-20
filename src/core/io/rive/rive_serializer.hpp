/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "io/binary_stream.hpp"

#include "type_system.hpp"

namespace glaxnimate::io::rive {



class RiveSerializer
{
public:
    explicit RiveSerializer(QIODevice* file);

    void write_header(int vmaj, int vmin, Identifier file_id);

    void write_property_table(const PropertyTable& properties);

    void write_object(const Object& output);

    void write_property_value(PropertyType id, const QVariant& value);

private:
    BinaryOutputStream stream;
};

} // namespace glaxnimate::io::rive
