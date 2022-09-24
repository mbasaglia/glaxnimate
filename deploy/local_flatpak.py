#!/usr/bin/env python3
import sys
import yaml
import pathlib

root = pathlib.Path(__file__).parent.parent
original = root / "deploy" / "org.mattbas.Glaxnimate.yml"


with open(original) as f:
    data = yaml.load(f, yaml.BaseLoader)



data["modules"][0]["sources"] = {
    "type": "dir",
    "path": str(root),
    "skip": ["build"]
}

yaml.dump(data, sys.stdout)
