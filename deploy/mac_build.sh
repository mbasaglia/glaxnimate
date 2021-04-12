#!/bin/bash

ROOT="$(dirname "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")")"
ACTION="${1:-build}"

case "$ACTION" in
    deps)
        brew list cmake || brew install cmake
        brew list qt@5 || brew install qt@5
        brew upgrade python@3.8 || true
        brew list potrace || brew install potrace
        brew list ffmpeg || brew install ffmpeg
        ;;

    build)
        mkdir -p "$ROOT/build"
        cd "$ROOT/build"
        cmake .. -DQt5_DIR="$(brew --prefix qt)/lib/cmake/Qt5" -DCMAKE_PREFIX_PATH="$(brew --prefix qt)/lib/cmake/Qt5Designer"
        make -j4
        make translations
        mkdir glaxnimate.iconset
        cp ../data/images/glaxnimate.png glaxnimate.iconset/icon_512x512.png
        iconutil -c icns glaxnimate.iconset -o glaxnimate.icns
        cpack -G Bundle

        mv Glaxnimate-*.dmg glaxnimate.dmg
        shasum -a 1 glaxnimate.dmg >checksum.txt
        ;;

    deploy)
        BRANCH="${2:-master}"
        set -x
        "$ROOT/deploy/gitlab_upload.py" glaxnimate.dmg "$BRANCH/MacOs/glaxnimate.dmg"
        "$ROOT/deploy/gitlab_upload.py" checksum.txt "$BRANCH/MacOs/checksum.txt"
        ;;

    pypi)
        make glaxnimate_python_depends_install
        make glaxnimate_python
        libpath="$(echo 'from distutils.util import get_platform; import sys; print("lib.%s-%d.%d" % (get_platform(), *sys.version_info[:2]))' | python3)"
        ln -s lib py_module/build/$libpath
        make glaxnimate_python_wheel
        make glaxnimate_python_upload
        ;;

    *)
        echo "mac_build.sh deps"
        echo "mac_build.sh build"
        echo "GITLAB_ACCESS_TOKEN='' mac_build.sh deploy [branch]"
        ;;
esac
