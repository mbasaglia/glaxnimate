#!/bin/bash
ROOT="$(dirname "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")")"
IMAGE="$1"

if [ -z "$IMAGE" ]
then
    echo "Usage: $0 image"
else
    set -ex
    docker run --rm -it --net=host -e DISPLAY -v /tmp/.X11-unix -v "$ROOT:/glaxnimate" "$IMAGE" bash
fi
