#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: GPL-3.0-or-later

import re
from xml.etree.ElementTree import parse
from pathlib import Path


def identity(x):
    return x


def process(basename, callback, force):
    filename = Path(__file__).parent / basename
    with open(filename) as file:
        dom = parse(file)

    for message in dom.findall(".//message"):
        translation = message.find("translation")
        type = translation.attrib.get("type")
        if force or type == "unfinished":
            translation.text = callback(message.find("source").text)
            translation.attrib.pop("type", None)

    with open(filename, "w") as file:
        dom.write(filename, "utf-8")


word_re = re.compile("\w+")
british_map = [
    ("color", "colour"),
    ("license", "licence"),
    ("center", "centre"),
    ("dialog", "dialogue"),
]


def british_word(match):
    word = match.group(0)
    lower = word.lower()
    for us, gb in british_map:
        if lower.startswith(us):
            lower = gb + lower[len(us):]
            break

    if word[0].isupper():
        if word[-1].isupper():
            return lower.upper()
        return lower[0].upper() + lower[1:]
    return lower


def british(x):
    return word_re.sub(british_word, x)


process("glaxnimate_en_US.ts", identity, False)
process("glaxnimate_en_GB.ts", british, False)
