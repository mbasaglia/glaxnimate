#!/usr/bin/env python3
import sys
import glaxnimate

expected_version = sys.argv[1]
if expected_version.count(".") == 3:
    expected_version = expected_version.rsplit(".", 1)[0]

glaxnimate_version = glaxnimate.__version__
if glaxnimate_version.count(".") == 3:
    glaxnimate_version = glaxnimate_version.rsplit(".", 1)[0]

print("Expected version")
print(expected_version)
print("Glaxnimate version")
print(glaxnimate_version)
sys.exit(glaxnimate_version != expected_version)
