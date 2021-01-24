set -xe
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

(cd ../data/icons/breeze-icons/ && git config core.symlinks true && git reset --hard)
# find data/icons/breeze-icons/ -name '*@?x' -exec rm -rf {} \;

env

# Build
cmake.exe .. \
    -DQt5_DIR=/mingw64/lib/cmake/Qt5 \
    -DZLIB_LIBRARY=/mingw64/lib/libz.a \
    -DCMAKE_PREFIX_PATH='/mingw64/lib/' \
    -DZLIB_INCLUDE_DIR=/mingw64/include \
    -DPython3_PREFIX=/mingw64/ \
    -DPython3_LIBRARIES=/mingw64/bin/libpython3.8.dll \
    -DPython3_EXECUTABLE=/mingw64/bin/python3 \
    -G 'MSYS Makefiles' \
    -DCMAKE_INSTALL_PREFIX=''
mingw32-make.exe -j2

# Setup package
mingw32-make.exe translations
mingw32-make.exe install DESTDIR=glaxnimate
windeployqt.exe glaxnimate/bin/glaxnimate.exe
cp /mingw64/bin/*.dll glaxnimate/bin
cp ./external/Qt-Color-Widgets/libQtColorWidgets.dll glaxnimate/bin
cp ../deploy/glaxnimate.vbs glaxnimate

# Create Artifacts
zip -r glaxnimate-x86_64.zip glaxnimate
sha1sum glaxnimate-x86_64.zip >checksum.txt
