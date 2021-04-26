#!/bin/bash

ROOT="$(dirname "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")")"
ACTION="${1:-build}"

if ! (which readlink | grep -q gnu)
then
    echo "Install GNU utils"
    echo "brew install coreutils ed findutils gawk gnu-sed gnu-tar grep make"
    echo 'export PATH="$(echo /usr/local/opt/*/libexec/gnubin | tr ' ' :):$PATH";'
    exit 1
fi


set -x

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
        mkdir -p glaxnimate.iconset
        cp ../data/images/glaxnimate.png glaxnimate.iconset/icon_512x512.png
        iconutil -c icns glaxnimate.iconset -o glaxnimate.icns
        cpack -G Bundle

        mv Glaxnimate-*.dmg glaxnimate.dmg
        shasum -a 1 glaxnimate.dmg >checksum.txt
        ;;

    deploy)
        BRANCH="${2:-master}"
        "$ROOT/deploy/gitlab_upload.py" "$ROOT/build/glaxnimate.dmg" "$BRANCH/MacOs/glaxnimate.dmg"
        "$ROOT/deploy/gitlab_upload.py" "$ROOT/build/checksum.txt" "$BRANCH/MacOs/checksum.txt"
        ;;

    pypi)
        cd "$ROOT/build"
        make glaxnimate_python_depends_install
        make glaxnimate_python
        libpath="$(echo 'from distutils.util import get_platform; import sys; print("lib.%s-%d.%d" % (get_platform(), *sys.version_info[:2]))' | python3)"
        ln -sf lib py_module/build/$libpath
        make glaxnimate_python_wheel
        make glaxnimate_python_upload
        ;;

    *)
        echo " # Install dependencies"
        echo "mac_build.sh deps"
        echo " # Compile / package"
        echo "mac_build.sh build"
        echo " # Add package to artifacts"
        echo "GITLAB_ACCESS_TOKEN='' mac_build.sh deploy [branch]"
        echo " # Build and upload python package"
        echo "mac_build.sh pypi"
        ;;
esac
