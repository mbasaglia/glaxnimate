#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2021 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: GPL-3.0-or-later

import os
import shutil
import pathlib
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("source")
parser.add_argument("dest")
ns = parser.parse_args()

ascii_A = 0x41
emoji_A = 0x1F1E6
emoji_flag = 0x1F3F4
emoji_tag_A = 0xE0061
emoji_tag_cancel = 0xE007F

source = pathlib.Path(ns.source)
dest = pathlib.Path(ns.dest)

for file in source.iterdir():
    in_code = file.stem
    if "-" in in_code:
        out_code = [emoji_flag] + [emoji_tag_A + ord(c) - ascii_A for c in in_code.replace("-", "")] + [emoji_tag_cancel]
    else:
        out_code = [emoji_A + ord(c) - ascii_A for c in in_code]

    out_filename = "emoji_u" + "-".join("%x" % x for x in out_code) + file.suffix
    out_path = str(dest / out_filename)

    shutil.copyfile(str(file), out_path)
