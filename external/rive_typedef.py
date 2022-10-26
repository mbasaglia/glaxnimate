#!/usr/bin/env python3
import pathlib
import json


type_map = {
    "double":   "Float",
    "Id":       "VarUint",
    "String":   "String",
    "uint":     "VarUint",
    "Bytes":    "Bytes",
    "bool":     "Bool",
    "Color":    "Color"
}

def load_dir(path, data, types, relpath = pathlib.Path()):
    for item in path.iterdir():
        if item.is_dir():
            load_dir(item, data, types, relpath / item.name)
        elif item.suffix == ".json":
            load_file(item, data, types, relpath / item.name)


def load_file(path, data, types, relpath):
    with open(path) as f:
        definition = json.load(f)

    id = definition["key"]["int"]
    data.append({
        "id": id,
        "extends": definition.get("extends"),
        "name": definition["name"],
        "properties": [
            {
                "id": pdef["key"]["int"],
                "name": name,
                "type": type_map[pdef["type"]]
            }
            for name, pdef in definition.get("properties", {}).items()
            if pdef.get("runtime", True)
        ]
    })
    types[str(relpath)] = id

def fix_extends(data, types):
    for item in data:
        if item["extends"] is not None:
            item["extends"] = types[item["extends"]]
        else:
            item["extends"] = 0


def format_props(props):
    indent = " " * 16
    return "".join(
        "\n%(indent)s{%(id)d, {%(name)r, PropertyType::%(type)s}}," % dict(
            indent=indent,
            **prop
        )
        for prop in props
    )

data = []
types = {}
rive_def_root = pathlib.Path.home() / "src/rive-cpp/dev/defs/"
load_dir(rive_def_root, data, types)
fix_extends(data, types)

print("""/**
 * NOTE: This file is generated automatically, do not edit manually
 */

#include "type_def.hpp"

using namespace glaxnimate::io::rive;

std::unordered_map<Identifier, ObjectDefinition> glaxnimate::io::rive::defined_objects = {""")
for obj in data:
    print(("""    {
        %(id)d, {
            %(name)r,
            %(extends)s, {%(property_str)s
            }
        }
    },""" % dict(property_str=format_props(obj["properties"]), **obj)
    ).replace("'", '"'))

print("};")
