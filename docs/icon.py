#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2019-2023 Mattia Basaglia <dev@dragon.best>
# SPDX-License-Identifier: GPL-3.0-or-later
import os
import sys
import argparse
from pathlib import Path

here = Path(__file__).resolve().parent
icon_src = here.parent / 'data/icons/breeze-icons/icons'
icon_dst = here / "docs/img/ui/icons"


parser = argparse.ArgumentParser()
subparsers = parser.add_subparsers(dest="cmd")

md = subparsers.add_parser("md")
md.add_argument("icon")
md.add_argument("size", type=int, default="32", nargs="?")

add = subparsers.add_parser("add")
add.add_argument("icon")
add.add_argument("size", type=str, default="32", nargs="?")

ns = parser.parse_args()

if ns.cmd == "md":
    if not (icon_dst / (ns.icon+".svg")).exists():
        sys.stderr.write("\x1b[31mIcon not imported\x1b[m\n")
    print('<img src="/img/ui/icons/%s.svg" width="%s" />' % (ns.icon, ns.size))
elif ns.cmd == "add":
    def key(x):
        if "@" in x.name:
            return 1000
        try:
            return -int(x.name)
        except:
            return 0

    def find():
        for subp in icon_src.iterdir():
            if subp.is_dir():
                preferred = subp / ns.size / (ns.icon+".svg")
                if preferred.exists():
                    return preferred
                for subsub in sorted(subp.iterdir(), key=key):
                    match = subsub / (ns.icon+".svg")
                    if match.exists():
                        return match

    found = find()
    if not found:
        sys.stderr.write("\x1b[31mNot found\x1b[m\n")
        sys.exit(0)
    os.symlink(os.path.relpath(found, icon_dst), icon_dst / (ns.icon+".svg"))
