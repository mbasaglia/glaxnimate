#!/bin/bash

CMAKECACHE="$1"
TAG_NAME="$(grep PROJECT_VERSION: "$CMAKECACHE" | cut -d "=" -f2)"
HERE="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"

echo "Pushing release for version $TAG_NAME"

cd "$HERE/.."

"$HERE/tag-branch.sh" release

git tag -f "$TAG_NAME"
git push --tags -f
