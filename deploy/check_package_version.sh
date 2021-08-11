#!/bin/bash
set -ex
ROOT="$(dirname "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")")"
CMAKE_FILE="$ROOT/CMakeLists.txt"
VERSION_EXPECTED="$(sed -rn 's/.*project\(.* VERSION (\S+).*/\1/p' "$CMAKE_FILE")"
VERSION_PACKAGE="$("$@" --version | sed -Er 's/.* ([0-9.]+).*/\1/')"
echo "Expected version: $VERSION_EXPECTED"
echo "Packaged version: $VERSION_PACKAGE"
if [ "$VERSION_EXPECTED" != "$VERSION_PACKAGE" ]
then
    exit 1
fi
