#!/usr/bin/env python3
import glaxnimate
import sys

version = sys.argv[1]
if version.count(".") == 3:
    version = version.rsplit(".", 1)[0]

print("Glaxnimate version")
print(glaxnimate.__version__)
print("Expected version")
print(version)
sys.exit(glaxnimate.__version__ != version)
