#!/usr/bin/env bash

APPDIR="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"

PYTHONHOME="$APPDIR/usr/" "$APPDIR/usr/bin/glaxnimate" "$@"
