#!/bin/bash

echo "Bintray upload..."

if [ -z "$BINTRAY_USER" -o -z "$BINTRAY_KEY" ]
then
    echo "Missing BINTRAY_USER or BINTRAY_KEY. Skipping deployment."
    exit 0
fi

RELEASE="$1"
INPUT_FILE="$2"
OUTPUT_FILE="$3"

curl "https://api.bintray.com/content/mattbas/Glaxnimate/Glaxnimate/$RELEASE/$OUTPUT_FILE?publish=1&override=1" \
    -u"$BINTRAY_USER:$BINTRAY_KEY" \
    -X PUT \
    -H "Content-Type: application/octet-stream" \
    --data-binary "@$INPUT_FILE"
