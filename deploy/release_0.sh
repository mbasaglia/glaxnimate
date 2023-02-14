#!/bin/bash

CMAKECACHE="$1"
TAG_NAME="$(grep PROJECT_VERSION: "$CMAKECACHE" | cut -d "=" -f2)"
HERE="$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")"

function question()
{
    echo -n "$1 [Y|n] "
    local answer
    read answer
    if [ "$answer" = 'n' ]
    then
        echo "Quitting"
        exit 0
    fi
}

echo "Preparing release for version $TAG_NAME"

if ! grep -q "^## $TAG_NAME$" "$HERE/../CHANGELOG.md"
then
    echo "No release notes!"
    exit 1
else
    echo "Release notes OK"
fi

cd "$HERE/.."
if ! git tag "$TAG_NAME"
then
    echo "Tag already exists!"
    exit 1
else
    echo "Git tag OK"
fi


METAINFO_FILE="$HERE/org.mattbas.Glaxnimate.metainfo.xml"
metainfo_release_tag="version=\"$TAG_NAME\" date=\"`date +%Y-%m-%d`\""
if ! grep -q -F "$metainfo_release_tag" "$METAINFO_FILE"
then
    sed -ri "$METAINFO_FILE" -e "s/(<release )[^>]*(>.*)/\1$metainfo_release_tag\2/"
    git add "$METAINFO_FILE"
    git commit -m "Update metainfo"

fi


"$HERE/tag-branch.sh"
