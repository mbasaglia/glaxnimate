#!/bin/bash


ROOT="$(dirname "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")")"
ACTION="${1:-build}"

set -ex

case "$ACTION" in
    deps)
        pacman --noconfirm -S \
            git zip unzip \
            mingw-w64-x86_64-toolchain \
            mingw-w64-x86_64-qt5 \
            mingw-w64-x86_64-zlib \
            mingw-w64-x86_64-cmake \
            mingw-w64-x86_64-python \
            mingw-w64-x86_64-potrace \
            mingw-w64-x86_64-ffmpeg \
            mingw-w64-x86_64-libimagequant
        ;;

    build)
        SUFFIX="$2"
        mkdir -p "$ROOT/build"
        cd "$ROOT/build"

        (cd ../data/icons/breeze-icons/ && git config core.symlinks true && git reset --hard)

        # Build
        cmake.exe .. \
            -DQt5_DIR=/mingw64/lib/cmake/Qt5 \
            -DZLIB_LIBRARY=/mingw64/lib/libz.a \
            -DCMAKE_PREFIX_PATH='/mingw64/lib/' \
            -DZLIB_INCLUDE_DIR=/mingw64/include \
            -DPython3_PREFIX=/mingw64/ \
            -DPython3_LIBRARIES=/mingw64/bin/libpython$PY_VERSION.dll \
            -DPython3_EXECUTABLE=/mingw64/bin/python3 \
            -G 'MSYS Makefiles' \
            -DCMAKE_INSTALL_PREFIX='' \
            -DCMAKE_BUILD_TYPE=Release \
            -DVERSION_SUFFIX="$SUFFIX"
        mingw32-make.exe -j2

        # Setup package
        PACKDIR=glaxnimate
        mingw32-make.exe translations
        mingw32-make.exe install DESTDIR=$PACKDIR
        windeployqt.exe $PACKDIR/bin/glaxnimate.exe
        cp /mingw64/bin/*.dll $PACKDIR/bin
        cp ./external/Qt-Color-Widgets/libQtColorWidgets.dll $PACKDIR/bin
        cp ../deploy/glaxnimate.vbs $PACKDIR
        mkdir -p $PACKDIR/share/glaxnimate/glaxnimate/pythonhome/lib/python
        cp -r /mingw64/lib/python$PY_VERSION/*.py /mingw64/lib/python$PY_VERSION/{json,collections,encodings} $PACKDIR/share/glaxnimate/glaxnimate/pythonhome/lib/python

        ;;

    deploy)
        BRANCH="${2:-master}"
        SSH_ARGS="$3"

        if [ "$BRANCH" = master -o "$BRANCH" = github ]
        then
            path=master
            channel="windows-beta"
        else
            path="$BRANCH"
            channel="windows-stable"
        fi

        cd "$ROOT/build"
        mkdir -p "artifacts/$path/Win"
        mv glaxnimate-x86_64.zip "artifacts/$path/Win"
        mv checksum.txt "artifacts/$path/Win"
        cd artifacts


        # Upload SourceForge
        rsync -a "$path" mbasaglia@frs.sourceforge.net:/home/frs/project/glaxnimate/ -e "ssh -o StrictHostKeyChecking=no $SSH_ARGS"

        # Upload itch.io
        version="$(../deploy/get_version.sh CMakeCache.txt)"
        wget https://broth.itch.ovh/butler/windows-amd64/LATEST/archive/default
        unzip default
        ./butler.exe push "$path/Win/glaxnimate-x86_64.zip" "MattBas/glaxnimate:$channel" --userversion "$version"

        ;;

    pypi)
        pacman --noconfirm -S mingw-w64-x86_64-python-pip
        pip.exe install wheel twine
        cmake.exe .. -DVERSION_SUFFIX=""
        mingw32-make.exe glaxnimate_python_depends_install
        mingw32-make.exe glaxnimate_python
        (cd py_module && ./setup.py build --compiler=unix bdist_wheel && cd ..)
        ;;

    *)
        echo " # Install dependencies"
        echo "win_build.sh deps"
        echo
        echo " # Compile / package"
        echo "win_build.sh build [VERSION_SUFFIX]"
        echo
        echo " # Add package to artifacts"
        echo "win_build.sh deploy [BRANCH=master [SSH_ARGS]]"
        echo
        echo " # Build python package"
        echo "win_build.sh pypi"
        ;;
esac


# set -xe
# pacman --noconfirm -S \
#     git zip unzip \
#     mingw-w64-x86_64-toolchain \
#     mingw-w64-x86_64-qt5 \
#     mingw-w64-x86_64-zlib \
#     mingw-w64-x86_64-cmake \
#     mingw-w64-x86_64-python \
#     mingw-w64-x86_64-potrace \
#     mingw-w64-x86_64-ffmpeg \
#     mingw-w64-x86_64-libimagequant \
#     mingw-w64-x86_64-make
#
# (cd ../data/icons/breeze-icons/ && git config core.symlinks true && git reset --hard)
# # find data/icons/breeze-icons/ -name '*@?x' -exec rm -rf {} \;
#
# env
#
# # Build
# cmake.exe .. \
#     -DQt5_DIR=/mingw64/lib/cmake/Qt5 \
#     -DZLIB_LIBRARY=/mingw64/lib/libz.a \
#     -DCMAKE_PREFIX_PATH='/mingw64/lib/' \
#     -DZLIB_INCLUDE_DIR=/mingw64/include \
#     -DPython3_PREFIX=/mingw64/ \
#     -DPython3_LIBRARIES=/mingw64/bin/libpython3.8.dll \
#     -DPython3_EXECUTABLE=/mingw64/bin/python3 \
#     -G 'MSYS Makefiles' \
#     -DCMAKE_INSTALL_PREFIX='' \
#     -DCMAKE_MAKE_PROGRAM='mingw32-make.exe'
# mingw32-make.exe -j4
#
# # Setup package
# mingw32-make.exe translations
# mingw32-make.exe install DESTDIR=glaxnimate
# windeployqt.exe glaxnimate/bin/glaxnimate.exe
# cp /mingw64/bin/*.dll glaxnimate/bin
# cp ./external/Qt-Color-Widgets/libQtColorWidgets.dll glaxnimate/bin
# cp ../deploy/glaxnimate.vbs glaxnimate
#
# # Create Artifacts
# zip -r glaxnimate-x86_64.zip glaxnimate
# sha1sum glaxnimate-x86_64.zip >checksum.txt
