#!/usr/bin/env python3
import json
import pathlib
import argparse


type_map = {
    "double":   "Float",
    "Id":       "VarUint",
    "String":   "String",
    "uint":     "VarUint",
    "Bytes":    "Bytes",
    "bool":     "Bool",
    "Color":    "Color"
}

def load_dir(path, data, types, props, relpath = pathlib.Path()):
    for item in path.iterdir():
        if item.is_dir():
            load_dir(item, data, types, props, relpath / item.name)
        elif item.suffix == ".json":
            load_file(item, data, types, props, relpath / item.name)


def load_prop(name, pdef, props):
    id = pdef["key"]["int"]
    if props.get(id, name) != name:
        raise Exception("Mismatching prop %s %s %s" % (id, name, props[name]))

    props[id] = name
    return {
        "id": id,
        "name": name,
        "type": type_map[pdef["type"]]
    }

def load_file(path, data, types, props, relpath):
    with open(path) as f:
        definition = json.load(f)

    id = definition["key"]["int"]
    data.append({
        "id": id,
        "extends": definition.get("extends"),
        "name": definition["name"],
        "properties": [
            load_prop(name, pdef, props)
            for name, pdef in definition.get("properties", {}).items()
            if pdef.get("runtime", True)
        ]
    })
    types[str(relpath)] = definition["name"]

def fix_extends(data, types):
    for item in data:
        if item["extends"] is not None:
            item["extends"] = types[item["extends"]]
        else:
            item["extends"] = "NoType"


def format_props(props):
    indent = " " * 16
    return "".join(
        "\n%(indent)s{%(id)d, {%(name)r, PropertyType::%(type)s}}," % dict(
            indent=indent,
            **prop
        )
        for prop in props
    )

def disclaimer(cmd):
    print("""/**
 * NOTE: This file is generated automatically, do not edit manually
 * To generate this file run
 *       %s
 */
""" % cmd)

parser = argparse.ArgumentParser()
parser.add_argument("--defs", type=pathlib.Path, default=pathlib.Path.home() / "src/rive-cpp/dev/defs/")
parser.add_argument("--type", "-t", choices=["source", "ids"], default="source")
args = parser.parse_args()


data = []
types = {}
props = {}
rive_def_root = args.defs
load_dir(rive_def_root, data, types, props)
data = sorted(data, key=lambda o: o["id"])

if args.type == "source":
    fix_extends(data, types)
    disclaimer("./external/rive_typedef.py -t source >src/core/io/rive/type_def.cpp")
    print("""
#include "type_def.hpp"

using namespace glaxnimate::io::rive;

std::unordered_map<TypeId, ObjectDefinition> glaxnimate::io::rive::defined_objects = {""")
    for obj in data:
        print(("""    {
        TypeId::%(name)s, {
            %(name)r, TypeId::%(name)s,
            TypeId::%(extends)s, {%(property_str)s
            }
        }
    },""" % dict(property_str=format_props(obj["properties"]), **obj)
        ).replace("'", '"'))
    print("};")
elif args.type == "ids":
    print("#pragma once")
    disclaimer("./external/rive_typedef.py -t ids >src/core/io/rive/type_ids.hpp")
    print("namespace glaxnimate::io::rive {\nenum class TypeId {")
    print("    NoType = 0,")
    for obj in data:
        print("    %(name)s = %(id)d," % obj)
    #print("};\n")
    #print("enum class PropId {")
    #for id, name in sorted(props.items()):
        #print("    %(name)s = %(id)d," % {"id": id, "name": name})
    print("};\n} // namespace glaxnimate::io::rive")
