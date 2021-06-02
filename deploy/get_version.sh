#!/bin/bash
CMAKECACHE="$1"
grep PROJECT_VERSION: "$CMAKECACHE" | cut -d "=" -f2
