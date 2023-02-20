/*
 * SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <vector>
#include <unordered_map>
#include <QString>
#include <QVariant>

#include "type_ids.hpp"
#include "io/binary_types.hpp"

namespace glaxnimate::io::rive {

using Identifier = VarUint;

enum class PropertyType
{
    VarUint = 0, // LEB128 Uint
    Bool    = 1, // Byte
    String  = 2, // length(LEB128 Uint) + Utf8
    Bytes   = 3, // length(LEB128 Uint) + Data
    Float   = 4, // Float32
    Color   = 5, // Uint32
};

using PropertyTable = std::unordered_map<VarUint, PropertyType>;


struct Property
{
    QString name;
    Identifier id = 0;
    PropertyType type = PropertyType::VarUint;
};

struct ObjectDefinition
{
    QString name;
    TypeId type_id = TypeId::NoType;
    TypeId extends = TypeId::NoType;
    std::vector<Property> properties;
};


extern std::unordered_map<TypeId, ObjectDefinition> defined_objects;


} // namespace glaxnimate::io::rive
