#!/usr/bin/env python3

from xml.etree.ElementTree import parse
from pathlib import Path

filename = Path(__file__).parent / "glaxnimate_en.ts"
with open(filename) as file:
    dom = parse(file)

for message in dom.findall(".//message"):
    translation = message.find("translation")
    type = translation.attrib.get("type")
    if type == "unfinished":
        translation.text = message.find("source").text
        del translation.attrib["type"]

with open(filename, "w") as file:
    dom.write(filename, "utf-8")
