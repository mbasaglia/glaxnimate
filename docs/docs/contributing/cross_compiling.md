# Cross Compiling

This page shows how to cross compile from Linux to windows using [mxe](https://mxe.cc/).

## Download MXE

You can follow [the tutorial](https://mxe.cc/#tutorial) or just clone with git:

    git clone https://github.com/mxe/mxe.git

Also make sure you have the [mxe dependencies](https://mxe.cc/#requirements) installed.

## Compile Required Packages

From the `mxe` directory: (You can change `-j4 JOBS=4` to fit your machine)

    make MXE_TARGETS=x86_64-w64-mingw32.shared.posix MXE_PLUGIN_DIRS=plugins/gcc9 -j4 JOBS=4 qt5 gcc cmake
    MXE_PATH="$PWD" # Set a shell variable for later

(TODO: build CPython libraries)

## Compile Glaxnimate

Run these from the Glaxnimate source directory (or anywhere if you know how CMake works).
It's similar to the build instructions listed in the [README](read_me.md#building)
but we need to use the tools provided by mxe

    git submodule update --init --recursive # Make sure you have all submodules cloned
    export PATH="$MXE_PATH/usr/bin/:$PATH"
    mkdir build
    cd build
    x86_64-w64-mingw32.shared.posix-cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/
    make -j 4 # This uses 4 cores to compile

This will produce the binary as `./bin/glaxnimate.exe`.


## Packaging

### Install mxedeployqt

Somewhere in you system (for example `~/src/`):

    git clone https://github.com/saidinesh5/mxedeployqt.git

### Creating The Package

From the same build directory as when compiling:

    make translations
    make install DESTDIR=glaxnimate
    # Change the command below to the actual mxedeployqt path
    ~/src/mxedeployqt/mxedeployqt --mxepath "$MXE_PATH" --mxetarget x86_64-w64-mingw32.shared.posix glaxnimate/bin/
    zip -r glaxnimate.zip glaxnimate

You now should have `glaxnimate.zip` in your build directory.
