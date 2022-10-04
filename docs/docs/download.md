Authors: Mattia Basaglia
nav_icon: fa fa-download

# Download

## Stable Releases

You can also see the [list of previous releases](https://gitlab.com/mattbas/glaxnimate/-/releases).

{download_table:release}

If you enjoy using Glaxnimate consider [donating](donate.md).

## Package Managers
<table>
<tr><th>Package</th><th>Notes</th></tr>
<tr>
<td><img src="/img/misc/arch-icon.svg" style="height:1.2rem;" /> <a href="https://aur.archlinux.org/packages/glaxnimate/">AUR</a></td>
<td><a href="#aur-package">Notes</a></td>
</tr>
<tr>
<td><img src="/img/misc/snap.svg" style="height:1.2rem;" /> <a href="https://snapcraft.io/glaxnimate">Snap</a></td>
<td><a href="#snap">Notes</a></td>
</tr>
<tr>
<td><i class="fab fa-python"></i> <a href="https://pypi.org/project/glaxnimate/">PyPI</a></td>
<td><a href="#pypi">Notes</a></td>
</tr>
<tr>
<td><i class="fab fa-freebsd"></i> <a href="https://www.freshports.org/graphics/glaxnimate/">FreeBSD</a></td>
<td><a href="#freebsd">Notes</a></td>
</tr>
<tr>
<td><img src="/img/misc/Flatpak_Logo.svg" style="height:1.2rem;" /> <a href="https://flathub.org/apps/details/org.mattbas.Glaxnimate">FlatPak</a></td>
<td><a href="#flatpak">Notes</a></td>
</tr>
</table>


## Development Snapshots

[![](https://gitlab.com/mattbas/glaxnimate/badges/master/pipeline.svg)](https://gitlab.com/mattbas/glaxnimate/-/pipelines)

These packages are built continuously as new changes are made.
They contain all the latest features but might also include bugs and broken features.


All packages provided here are for the x86_64 architecture.

{download_table:master:git}

If you enjoy using Glaxnimate consider [donating](donate.md).


## Building from Source

See the [build instructions](contributing/read_me.md).

# Installation Notes

## Linux AppImage

Make sure the AppImage is executable

    chmod a+x glaxnimate-x86_64.AppImage

Running the AppImage should start Glaxnimate.

If your system doesn't support mounting user level filesystems use the following commands instead:

    ./glaxnimate-x86_64.AppImage --appimage-extract
    ./squashfs-root/AppRun

## Deb Package

Open the deb with your package manager and follow the instructions to install it.

Or from the command line:

    sudo dpkg -i  glaxnimate.deb

## Windows Zip

Extract the zip and execute the file at the top level `glaxnimate.vbs`.

Note that the built-in zip extraction that comes with Windows is excruciatingly slow,
so you are better off using 7zip or something like that.

### Limitations

When using Python scripting `sys.stderr`, `sys.stdout`, and `print()` won't work.

### Antivirus

Sometimes the antivirus will delete random files, which will prevent glaxnimate from running
(This has been observed with Norton).

If this happens, you must tell your antivirus to restore the files.

## Mac dmg

You need to install the dependencies with [Homebrew](https://brew.sh/):

    brew install python qt gcc potrace ffmpeg

Open (mount) the dmg file, then either open `glaxnimate.app` to run it or drag it
to Applications to install it.

### Developer cannot be verified

If you get an error message saying the developer isn't verified:

* Go to System Preferences > Security & Privacy > General
* Click on the lock to make changes
* Under *Allow Apps download from*, select glaxnimate

### Incompatible Library Version

If you get an error saying "Incompatible Library Version" you might need to
upgrade some of the dependencies:

    brew upgrade qt

## Aur Package


For the stable package:

    git clone https://aur.archlinux.org/glaxnimate.git
    cd glaxnimate
    makepkg -rsi

For the development snapshot package

    git clone https://aur.archlinux.org/glaxnimate-git.git
    cd glaxnimate-git
    makepkg -rsi

## Snap

For the stable version:

    snap install glaxnimate

For the testing version:

    snap install --beta glaxnimate

## PyPI

This package is for the [python module](contributing/scripting/index.md) only, not for the full program.

## FreeBSD

    pkg install glaxnimate

## FlatPak

    flatpak install flathub org.mattbas.Glaxnimate
