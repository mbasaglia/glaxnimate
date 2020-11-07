git submodule update --init --recursive

# Build
cmake.exe .. \
    -DQt5_DIR=/mingw64/lib/cmake/Qt5 \
    -DZLIB_LIBRARY=/mingw64/lib/libz.a \
    -DCMAKE_PREFIX_PATH='/mingw64/lib/' \
    -DZLIB_INCLUDE_DIR=/mingw64/include \
    -G 'MSYS Makefiles' \
    -DCMAKE_INSTALL_PREFIX=''
mingw32-make.exe

# Setup package
mingw32-make.exe install DESTDIR=glaxnimate
windeployqt.exe glaxnimate/bin/glaxnimate.exe
cp /mingw64/bin/*.dll glaxnimate/bin
cp ./external/Qt-Color-Widgets/libQtColorWidgets.dll glaxnimate/bin
cp ../deploy/glaxnimate.vbs glaxnimate

# PyPI
if [ "$APPVEYOR_REPO_TAG" = true ]
then
    pacman --noconfirm -S mingw-w64-x86_64-python-pip
    cmingw32-make.exe .. -DVERSION_SUFFIX=""
    mingw32-make.exe glaxnimate_python_depends_install
    mingw32-make.exe glaxnimate_python
    (cd py_module && ./setup.py build --compiler=unix bdist_wheel && cd ..)
    mingw32-make.exe glaxnimate_python_upload
fi

# Create Artifacts
zip -r glaxnimate-x86_64.zip glaxnimate
mkdir -p ../$APPVEYOR_REPO_BRANCH/Win
sha1sum glaxnimate-x86_64.zip >checksum.txt
cp glaxnimate-x86_64.zip ../$APPVEYOR_REPO_BRANCH/Win
cp checksum.txt ../$APPVEYOR_REPO_BRANCH/Win
cd ..
zip -r bintray.zip $APPVEYOR_REPO_BRANCH
