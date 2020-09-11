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


### Installing Dependencies on Deb systems

    apt-get install -y g++ cmake qtbase5-dev libpython3-dev zlib1g-dev


Getting the Latest Code
---------------------------------------

You can find the code on [GitLab](https://gitlab.com/mattia.basaglia/glaxnimate).

To clone with git:

    git clone --recursive https://gitlab.com/mattia.basaglia/glaxnimate.git


Building
---------------------------------------

If you are building from git, ensure your submodules are up to date

    git submodule update --init --recursive

Standard CMake build commands work

    mkdir build
    cd build
    cmake ..
    make -j 4 # This uses 4 cores to compile

It will produce the executable `bin/glaxnimate` relative to the build directory


Contacts
---------------------------------------

* [Telegram (Chat)](https://t.me/Glaxnimate)
* [GitLab (Code, Issues)](https://gitlab.com/mattia.basaglia/glaxnimate)
* [Documentation](https://glaxnimate.mattbas.org)


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
