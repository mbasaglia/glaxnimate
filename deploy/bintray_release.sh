#!/bin/bash

if [ -z "$BINTRAY_USER" -o -z "$BINTRAY_KEY" ]
then
    echo "Missing BINTRAY_USER or BINTRAY_KEY. Skipping deployment."
    exit 1
fi

HERE="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"
RELEASE="$1"
FILE="$2"
TMP_FILE="/tmp/$(basename "$2")"

curl -L "https://dl.bintray.com/mattbas/Glaxnimate/release/$FILE" -o "$TMP_FILE"

"$HERE/bintray_upload.sh" Dev "$TMP_FILE" "$RELEASE/$FILE"
