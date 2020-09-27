Glaxnimate
=======================================

About
---------------------------------------

A simple vector graphics animation program.


Dependencies
---------------------------------------

* C++17 compliant compiler
* Qt5 (qtbase)
* CMake
* Python3
* ZLib


Getting the Latest Code
---------------------------------------

You can find the code on [GitLab](https://gitlab.com/mattia.basaglia/glaxnimate).

To clone with git:

    git clone --recursive https://gitlab.com/mattia.basaglia/glaxnimate.git


Building
---------------------------------------

### Generic Overview

If you are building from git, ensure your submodules are up to date

    git submodule update --init --recursive

Standard CMake build commands work

    mkdir build
    cd build
    cmake ..
    make -j 4 # This uses 4 cores to compile

It will produce the executable `bin/glaxnimate` relative to the build directory


### Deb-based systems (Ubuntu, Debian, etc)

Install the dependencies:

    apt-get install -y g++ cmake qtbase5-dev libpython3-dev zlib1g-dev

The generic `cmake` commands listed above should work.


### MacOS

Install the dependencies with homebrew:

    brew install cmake qt python gcc@10

Ensure you are using GCC (it seems clang has issues with C++17).

    export CC=gcc-10
    export CXX=g++-10


Build with `cmake`, specifying the Qt installation path:

    mkdir build
    cd build
    cmake .. -DQt5_DIR="$(brew --prefix qt)/lib/cmake/Qt5"
    make


### Windows

* Install [CMake](https://cmake.org/download/)
* Install Python, you can do so from the [store](https://www.microsoft.com/store/productId/9MSSZTT1N39L)
* Install [Qt](https://www.qt.io/download-qt-installer), instructions below assume it's in `C:/Qt/`
* Install [MinGW64](http://mingw-w64.yaxm.org/doku.php/download/mingw-builds),
instructions below assume it's in `C:/WinGW64/`
* Install [ZLIB](https://sourceforge.net/projects/gnuwin32/files/zlib/),
instructions below assume it's in `C:/Program Files (x86)/GnuWin32`

    # Prepare the environment, Change these to fit your installation directories
    $PYTHON_DIR=$(echo "import os; print(os.path.dirname(os.path.dirname(os.__file__)));" | python)
    $MINGW_DIR="C:/MinGW64/mingw64"
    $ZLIB_DIR="C:/Program Files (x86)/GnuWin32"
    $QT_DIR="C:/Qt/5.14.1"
    $ENV:PATH="$ENV:PATH;$MINGW_DIR/bin"

    # Build
    mkdir build
    cd build
    cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="$QT_DIR/mingw73_64/" -DZLIB_INCLUDE_DIR="$ZLIB_DIR/include" -DZLIB_LIBRARY="$ZLIB_DIR/lib/libz.a" -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=OFF -DCMAKE_CXX_FLAGS=-Wno-attributes
    mingw32-make.exe

    # Copy library files because windows is weird like that
    . "$QT_DIR/mingw73_64/bin/qtenv2.bat"
    . "$QT_DIR/mingw73_64/bin/windeployqt.exe" ./bin/glaxnimate.exe
    cp ./external/Qt-Color-Widgets/libQtColorWidgets.dll bin
    cp $MINGW_DIR/bin/*.dll bin
    cp "$PYTHON_DIR/python38.dll" bin


Contacts
---------------------------------------

* [Telegram (Chat)](https://t.me/Glaxnimate)
* [GitLab (Code, Issues)](https://gitlab.com/mattia.basaglia/glaxnimate)
* [User Manual](https://glaxnimate.mattbas.org)


License
---------------------------------------

GPLv3 or later, see COPYING.

Copyright (C) 2020 Mattia Basaglia

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
