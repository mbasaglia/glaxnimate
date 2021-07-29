#!/bin/bash


ROOT="$(dirname "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")")"
ACTION="${1:-build}"
PY_VERSION=3.9

set -ex

case "$ACTION" in
    deps)
        pacman --noconfirm -S \
            git zip base-devel \
            unzip               \
            mingw-w64-x86_64-qt5 \
            mingw-w64-x86_64-zlib \
            mingw-w64-x86_64-cmake \
            mingw-w64-x86_64-python \
            mingw-w64-x86_64-potrace \
            mingw-w64-x86_64-ffmpeg   \
            mingw-w64-x86_64-toolchain \
            mingw-w64-x86_64-python-pip \
            mingw-w64-x86_64-libarchive  \
            mingw-w64-x86_64-libimagequant
        ;;

    configure)
        SUFFIX="$2"
        mkdir -p "$ROOT/build"
        cd "$ROOT/build"

        (cd ../data/icons/breeze-icons/ && git config core.symlinks true && git reset --hard &>/dev/null)

        cmake.exe .. \
            -DQt5_DIR=/mingw64/lib/cmake/Qt5 \
            -DZLIB_LIBRARY=/mingw64/lib/libz.a \
            -DCMAKE_PREFIX_PATH='/mingw64/lib/' \
            -DZLIB_INCLUDE_DIR=/mingw64/include \
            -DPython3_PREFIX=/mingw64/ \
            -DPython3_LIBRARIES=/mingw64/bin/libpython$PY_VERSION.dll \
            -DPython3_EXECUTABLE=/mingw64/bin/python3 \
            -G 'MSYS Makefiles' \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX='' \
            -DVERSION_SUFFIX="$SUFFIX"
        ;;

    build)
        JOBS="${2:-4}"
        cd "$ROOT/build"
        mingw32-make.exe -j$JOBS

        # Setup package
        PACKDIR=glaxnimate
        mingw32-make.exe translations
        mingw32-make.exe install DESTDIR=$PACKDIR >/dev/null
        windeployqt.exe $PACKDIR/bin/glaxnimate.exe

        # Copy dependencies, needs to run a couple times to pick everything up *shrugs*
#         echo =====
#         ldd.exe --version
#         ldd.exe $PACKDIR/bin/glaxnimate.exe
#         for i in {0..3}
#         do
#             echo "Adding"
#             ldd.exe $PACKDIR/bin/glaxnimate.exe | sed -rn 's/.* => (.*\/mingw64\/bin\/\S+).*/\1/p'
#             ldd.exe $PACKDIR/bin/glaxnimate.exe | sed -rn 's/.* => (.*\/mingw64\/bin\/\S+).*/\1/p' | xargs -i cp {} $PACKDIR/bin
#         done
#         echo "."
        # Dunno why but the above doesn't work on CI (it does work locally *shrugs*)
        cp /mingw64/bin/*.dll $PACKDIR/bin

        cp ./external/Qt-Color-Widgets/libQtColorWidgets.dll $PACKDIR/bin
        cp ../deploy/glaxnimate.vbs $PACKDIR
        mkdir -p $PACKDIR/share/glaxnimate/glaxnimate/pythonhome/lib/python
        cp -r /mingw64/lib/python$PY_VERSION/*.py /mingw64/lib/python$PY_VERSION/{json,collections,encodings} $PACKDIR/share/glaxnimate/glaxnimate/pythonhome/lib/python

        # Create Artifacts
        zip -r glaxnimate-x86_64.zip glaxnimate >/dev/null
        sha1sum glaxnimate-x86_64.zip >checksum.txt

        ;;

    deploy)
        BRANCH="${2:-master}"
        SSH_ARGS="$3"

        pacman --noconfirm -S rsync

        if [ "$BRANCH" = master -o "$BRANCH" = github ]
        then
            path=master
            channel="windows-beta"
        else
            path="$BRANCH"
            channel="windows-stable"
        fi

        cd "$ROOT/build"
        version="$(../deploy/get_version.sh CMakeCache.txt)"

        mkdir -p "artifacts/$path/Win"
        mv glaxnimate-x86_64.zip "artifacts/$path/Win"
        mv checksum.txt "artifacts/$path/Win"
        cd artifacts


        # Upload SourceForge
        rsync -a "$path" mbasaglia@frs.sourceforge.net:/home/frs/project/glaxnimate/ -e "ssh -o StrictHostKeyChecking=no $SSH_ARGS"

        # Upload itch.io
        wget -nv https://broth.itch.ovh/butler/windows-amd64/LATEST/archive/default
        unzip default
        ./butler.exe push "$path/Win/glaxnimate-x86_64.zip" "MattBas/glaxnimate:$channel" --userversion "$version"

        ;;

    pypi)
        cd "$ROOT/build"
        pip.exe install wheel twine
        mingw32-make.exe glaxnimate_python_depends_install
        mingw32-make.exe glaxnimate_python
        mingw32-make.exe glaxnimate_python_wheel
        ;;

    *)
        echo " # Install dependencies"
        echo "win_build.sh deps"
        echo
        echo " # Configure CMake"
        echo "win_build.sh configure [VERSION_SUFFIX]"
        echo
        echo " # Compile / package"
        echo "win_build.sh build [JOBS=4]"
        echo
        echo " # Add package to artifacts"
        echo "win_build.sh deploy [BRANCH=master [SSH_ARGS]]"
        echo
        echo " # Build python package"
        echo "win_build.sh pypi"
        ;;
esac
