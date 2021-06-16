Glaxnimate
=======================================

About
---------------------------------------

A simple vector graphics animation program.


Dependencies
---------------------------------------

* C++17 compliant compiler
* Qt5 (qtbase) >= 5.12
* CMake >= 3.5
* Python3
* ZLib
* Potrace
* libav (libavformat, libswscale, libavcodec, libavutil) >= 59


Getting the Latest Code
---------------------------------------

You can find the code on [GitLab](https://gitlab.com/mattbas/glaxnimate).

To clone with git:

    git clone --recursive https://gitlab.com/mattbas/glaxnimate.git


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

    apt-get install -y g++ cmake qtbase5-dev qttools5-dev libpython3-dev zlib1g-dev libpotrace-dev libavformat-dev libswscale-dev libavcodec-dev libavutil-dev

The generic `cmake` commands listed above should work.


### MacOS

Install the dependencies with homebrew:

    brew install cmake qt python potrace ffmpeg

Build with `cmake`, specifying the Qt installation path:

    mkdir build
    cd build
    cmake .. -DQt5_DIR="$(brew --prefix qt@5)/lib/cmake/Qt5" -DCMAKE_PREFIX_PATH="$(brew --prefix qt)/lib/cmake/Qt5Designer"
    make


### Windows

Install [MSYS2](https://www.msys2.org/), select "Mingw-w64 64 bit" when asked, and run these commands on a it:

    # Set up dependencies
    pacman --noconfirm -Sy
    pacman --noconfirm -Su
    # Restart msys2 before continuing
    pacman  --noconfirm  -S \
        mingw-w64-x86_64-qt5 \
        mingw-w64-x86_64-zlib \
        mingw-w64-x86_64-cmake \
        mingw-w64-x86_64-python \
        mingw-w64-x86_64-ffmpeg  \
        mingw-w64-x86_64-potrace  \
        mingw-w64-x86_64-toolchain \
        make

    # Build
    # cd to where the code is
    mkdir build
    cd build
    cmake.exe .. \
        -DQt5_DIR=/mingw64/lib/cmake/Qt5 \
        -DZLIB_LIBRARY=/mingw64/lib/libz.a \
        -DCMAKE_PREFIX_PATH="/mingw64/lib/" \
        -DZLIB_INCLUDE_DIR=/mingw64/include \
        -G "MSYS Makefiles"
    make

    # Copy library files because windows is weird like that
    windeployqt.exe bin/glaxnimate.exe
    cp /mingw64/bin/*.dll bin
    cp ./external/Qt-Color-Widgets/libQtColorWidgets.dll bin


Contacts
---------------------------------------

* [Telegram (Chat)](https://t.me/Glaxnimate)
* [GitLab (Code, Issues)](https://gitlab.com/mattbas/glaxnimate)
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
