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
    

### Android

Note that these instructions have been tested on an Ubuntu host, on different
systems things might vary.

#### Setting up Java

Ensure you have the JVM and JDK installed on your system.

    sudo apt install default-jdk

#### Installing Qt

See also <https://doc.qt.io/qt-5/android-getting-started.html>.

Download the Qt Installer for the open source edition 
<https://www.qt.io/download-open-source>.

For some reson Qt wants you to have an account even if you install 
the open source version so you might have to set that up;

Install Qt Creator and Qt 5 Android. 
Any Qt version above 5.12 should work, it hasn't been tested with Qt 6 
so stick with 5 for now. 

Select "Custom Installation", then on the following page check
Qt > Qt 5.15.2 (or whatever version you prefer) > Android.
You can also install additional components such as Qt for Desktop and such.

Accept the licenses and proceed to install.

#### Setting up Qt for Android

Once Qt is installed, you can download and install the Android development tools
from Qt Creator.

You can follow the instructions from here:
<https://doc.qt.io/qtcreator/creator-developing-android.html#specifying-android-device-settings>.

From Qt Creator:

* Tools > Options... > Devices > Android
* Click "Setup SDK"
* Install additional packages if prompted (you might need to accept some licenses)
* Wait for everything to be downloaded and installed
* Under "SDK Manager" (which is at the bottom of the dialog) 
    * click the "Install" checkbox for one of the available images
        (eg: Android 11 > Google API Atom x86 System Image)
    * click "Apply" on the side (not the one at the bottom that closes the dialog)
    * after it's downloaded, you can click OK to close the dialog

#### Building

* Open `glaxnimate-android.pro` as a project in Qt Creator (Welcome > Projects > Open).
* Enable "Android Qt Clang".
* Click "Configure Project"
* If multiple toolkits were enabled, ensure you have the android one active,
    you can click the icon above the run button on the bottom-left and
    select the "Android Qt" option.
    The icon on this button should look like a phone (rather than a computer screen).
* If using a "Multi ABI" toolkit enable the ABIs you need: 
    Projects > Qt Android > Build > Build Steps > qmake > Details > ABIs
    Usually x86 is what you can use on emulators
* Click Run
* "Create Virtual Device" if needed or select an existing device 
    (your physical phone would show up here if connected to the computer and USB debugging is enabled).


#### Troubleshooting

**Could not determine java version**

Qt downloads an old version of `gradle`, so if you get an error that your
Java version cannot be recognize find the file `gradle-wrapper.properties`
and update it to version 5.1 or later.

**Incompatible target / Undefined reference / Redefinition**

Sometimes `qmake` messes up the build, the best option is to remove the build
directory created by Qt Creator and rebuild.

**Invalid MinSdkVersion**

Depending on which version of the Android SDK you have, you might have to select 
a different value in `src/android/android/AndroidManifest.xml`.


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
