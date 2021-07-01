#!/usr/bin/env python3
import re
import sys
import json
import enum
import argparse
from urllib.request import urlopen


class EmojiObject:
    def to_dict(self):
        return {
            k: EmojiObject._to_dict(v)
            for k, v in vars(self).items()
        }

    @staticmethod
    def _to_dict(v):
        if isinstance(v, EmojiObject):
            return v.to_dict()
        elif isinstance(v, enum.Enum):
            return v.name()
        elif isinstance(v, list):
            return list(map(EmojiObject._to_dict, v))
        return v


class EmojiGroup(EmojiObject):
    def __init__(self, name):
        self.name = name
        self.children = []


class Emoji(EmojiObject):
    class Status(enum.Enum):
        component = enum.auto()
        fully_qualified = enum.auto()
        minimally_qualified = enum.auto()
        unqualified = enum.auto()

    def __init__(self, emoji, name, status, since_version, flag_gender=False, flag_skin_tone=False, flag_hair_style=False):
        self.emoji = emoji
        self.name = name
        self.status = status
        self.flag_gender = flag_gender
        self.flag_skin_tone = flag_skin_tone
        self.flag_hair_style = flag_hair_style
        self.since_version = since_version

    @staticmethod
    def hexord(char):
        return "%x" % ord(char)

    @property
    def hexes(self):
        return map(self.hexord, self.emoji)

    @property
    def slug(self):
        return "-".join(self.hexes)

    @property
    def hex_title(self):
        return " ".join(self.hexes)

    def __str__(self):
        return "%s - %s" % (self.emoji, self.hex_title)

    @staticmethod
    def slug2emoji(slug):
        try:
            return "".join(chr(int(p, 16)) for p in slug.split("-"))
        except Exception:
            return ""

    def is_variant(self):
        return self.flag_gender or self.flag_hair_style or self.flag_skin_tone


def pull_emoji(uri, max_version, ignore_variants):
    qualifiers = {
        "skin_tone": ["1F3FB", "1F3FC", "1F3FD", "1F3FE", "1F3FF"],
        "gender": ["200D 2640", "200D 2642"],
        "hair_style": ["1F9B0", "1F9B1" "1F9B3", "1F9B2"],
    }
    for name, quallist in qualifiers.items():
        qualifiers[name] = [" %s " % i for i in quallist]

    current_group = None
    current_subgroup = None

    reparser = re.compile(
        r'(?P<group>^\s*# (?P<grp_sub>sub)?group:\s*(?P<grp_name>.*))|' +
        r'(?P<emoji>^(?P<seq>[0-9A-Fa-f ]+); (?P<status>[-a-z ]+)# (?P<unicode>[^ ]+) (?:E(?P<version>[0-9]+\.[0-9]+) )?(?P<name>.+)$)'
    )

    if uri.startswith("http"):
        r = urlopen(uri)
        if r.code != 200:
            raise Exception("Could not fetch file: %s" % r.code)

        encoding = r.headers.get_content_charset()
    else:
        r = open(uri, "rb")
        encoding = "utf8"

    table = EmojiGroup("emoji")

    for line in r:
        decoded = line.decode(encoding).strip()
        match = reparser.match(decoded)
        if not match:
            continue
        if match.group("group"):
            model = EmojiGroup(match.group("grp_name"))

            if match.group("grp_sub"):
                current_subgroup = model
                current_group.children.append(model)
            else:
                current_group = model
                table.children.append(model)
        elif match.group("emoji"):
            version = version_tuple(match.group("version"))
            if max_version and version > max_version:
                continue
            status = Emoji.Status[match.group("status").strip().replace("-", "_")].value
            emoji = Emoji(match.group("unicode"), match.group("name"), status, version)

            seq = match.group("seq")
            for name, quallist in qualifiers.items():
                for qualifier in quallist:
                    if qualifier in seq:
                        setattr(emoji, "flag_" + name, True)
                        break
                else:
                    setattr(emoji, "flag_" + name, False)

            if not ignore_variants or not emoji.is_variant():
                current_subgroup.children.append(emoji)

    return table


def version_tuple(version_string):
    if not version_string:
        return tuple()
    return tuple(map(int, version_string.split(".")))


def _group_to_cxx_model(grp, indent):
    if isinstance(grp, Emoji) and grp.status == Emoji.Status.component:
        return
    if isinstance(grp, EmojiGroup) and grp.name == "Component":
        return

    sys.stdout.write(indent * ' ')
    sys.stdout.write('{\n')

    def write_line(x, comma=True):
        sys.stdout.write((indent + 4) * ' ')
        sys.stdout.write(x)
        if comma:
            sys.stdout.write(",")
        sys.stdout.write("\n")

    if isinstance(grp, EmojiGroup):
        write_line(json.dumps(grp.name))
        write_line('{', False)
        for child in grp.children:
            _group_to_cxx_model(child, indent + 8)
        write_line('}', False)
    elif isinstance(grp, Emoji):
        write_line(json.dumps(grp.name))
        write_line('"%s"' % "".join("\\x%02x" % c for c in grp.emoji.encode("utf8")))

    sys.stdout.write(indent * ' ')
    sys.stdout.write('},\n')


def to_cxx_model(table):
    print('#include "emoji_data.hpp"\n\n')
    print("//File generated by update_emoji.py\n\n")
    print("const std::vector<glaxnimate::android::EmojiGroup> glaxnimate::android::EmojiGroup::table = {")
    for child in table.children:
        _group_to_cxx_model(child, 4)
    print("};")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--emoji-version",
        "-e",
        default="latest",
        help="Load a specific version of the Emoji standard"
    )
    parser.add_argument(
        "--file",
        "-f",
        default="https://unicode.org/Public/emoji/%s/emoji-test.txt",
        help="Input file"
    )
    parser.add_argument(
        "--max-version",
        default=None,
        type=version_tuple,
        help="Max emoji version"
    )
    parser.add_argument(
        "--ignore-variants",
        action="store_true",
        help="Ignore skin tone / gender / hair variants"
    )
    parser.add_argument(
        "action",
        choices=["download", "json", "model"]
    )

    ns = parser.parse_args()

    filename = ns.file
    if "%s" in filename:
        filename = filename % ns.emoji_version

    if ns.action == "download":
        r = urlopen(filename)
        if r.code != 200:
            raise Exception("Could not fetch file: %s" % r.code)

        encoding = r.headers.get_content_charset()
        for line in r:
            print(line.decode(encoding).strip())
    else:
        table = pull_emoji(filename, ns.max_version, ns.ignore_variants)
        if ns.action == "json":
            json.dump(table.to_dict(), sys.stdout, indent=4)
        elif ns.action == "model":
            to_cxx_model(table)



