#!/usr/bin/env python3
import sys
import glaxnimate

version = sys.argv[1]
if version.count(".") == 3:
    version = version.rsplit(".", 1)[0]

print("Expected version")
print(version)
print("Glaxnimate version")
print(glaxnimate.__version__)
sys.exit(glaxnimate.__version__ != version)
