Authors: Mattia Basaglia
nav_icon: fa fa-download

# Download

## Stable Releases

You can also see the [list of previous releases](https://gitlab.com/mattbas/glaxnimate/-/releases).

<table>
<tr><th>Package</th><th>Checksum</th><th>Notes</th></tr>
<tr>
<td><i class="fab fa-linux"></i> <a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/release/raw/build/glaxnimate-x86_64.AppImage?job=linux%3Aappimage">Linux AppImage</a></td>
<td><a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/release/raw/build/checksum.txt?job=linux%3Aappimage">SHA1</a></td>
<td><a href="#linux-appimage">Notes</a></td>
</tr>
<tr>
<td><i class="fab fa-ubuntu"></i> <a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/release/raw/build/glaxnimate.deb?job=linux%3Adeb">Deb Package</a></td>
<td><a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/release/raw/build/checksum.txt?job=linux%3Adeb">SHA1</a></td>
<td><a href="#deb-package">Notes</a></td>
</tr>
<tr>
<td><i class="fab fa-windows"></i> <a href="https://dl.bintray.com/mattbas/Glaxnimate/release/Win/glaxnimate-x86_64.zip">Windows Zip</a></td>
<td><a href="https://dl.bintray.com/mattbas/Glaxnimate/release/Win/checksum.txt">SHA1</a></td>
<td><a href="#windows-zip">Notes</a></td>
</tr>
<tr>
<td><i class="fab fa-apple"></i> <a href="https://dl.bintray.com/mattbas/Glaxnimate/release/MacOs/glaxnimate.dmg">Mac dmg</a></td>
<td><a href="https://dl.bintray.com/mattbas/Glaxnimate/release/MacOs/checksum.txt">SHA1</a></td>
<td><a href="#mac-dmg">Notes</a></td>
</tr>
<tr>
<td><i class="fas fa-wrench"></i> <a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/release/raw/glaxnimate-src.tar.gz?job=tarball">Source Tarball</a></td>
<td><a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/release/raw/sha256.txt?job=tarball">SHA256</a></td>
<td><a href="/contributing/read_me/">Notes</a></td>
</tr>
</table>

If you enjoy using Glaxnimate consider [donating](donate.md).

## Package Managers
<table>
<tr><th>Package</th><th>Notes</th></tr>
<tr>
<td><img src="/img/misc/arch-icon.svg" style="height:1.2rem;" /> <a href="https://aur.archlinux.org/packages/glaxnimate-bin/">AUR</a></td>
<td><a href="#aur-package">Notes</a></td>
</tr>
<tr>
<td><img src="/img/misc/snap.svg" style="height:1.2rem;" /> <a href="https://snapcraft.io/glaxnimate">Snap</a></td>
<td><a href="#snap">Notes</a></td>
</tr>
</table>


## Development Snapshots

[![](https://gitlab.com/mattbas/glaxnimate/badges/master/pipeline.svg)](https://gitlab.com/mattbas/glaxnimate/-/pipelines)

These packages are built continuosly as new changes are made.
They contain all the latest features but might also include bugs and broken features.


All packages provided here are for the x86_64 architecture.

<table>
<tr><th>Package</th><th>Checksum</th><th>Notes</th></tr>
<tr>
<td><i class="fab fa-linux"></i> <a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/master/raw/build/glaxnimate-x86_64.AppImage?job=linux%3Aappimage">Linux AppImage</a></td>
<td><a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/master/raw/build/checksum.txt?job=linux%3Aappimage">SHA1</a></td>
<td><a href="#linux-appimage">Notes</a></td>
</tr>
<tr>
<td><i class="fab fa-ubuntu"></i> <a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/master/raw/build/glaxnimate.deb?job=linux%3Adeb">Deb Package</a></td>
<td><a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/master/raw/build/checksum.txt?job=linux%3Adeb">SHA1</a></td>
<td><a href="#deb-package">Notes</a></td>
</tr>
<tr>
<td><i class="fab fa-windows"></i> <a href="https://dl.bintray.com/mattbas/Glaxnimate/master/Win/glaxnimate-x86_64.zip">Windows Zip</a></td>
<td><a href="https://dl.bintray.com/mattbas/Glaxnimate/master/Win/checksum.txt">SHA1</a></td>
<td><a href="#windows-zip">Notes</a></td>
</tr>
<tr>
<td><i class="fab fa-apple"></i> <a href="https://dl.bintray.com/mattbas/Glaxnimate/master/MacOs/glaxnimate.dmg">Mac dmg</a></td>
<td><a href="https://dl.bintray.com/mattbas/Glaxnimate/master/MacOs/checksum.txt">SHA1</a></td>
<td><a href="#mac-dmg">Notes</a></td>
</tr>
<tr>
<td><i class="fas fa-wrench"></i> <a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/master/raw/glaxnimate-src.tar.gz?job=tarball">Source Tarball</a></td>
<td><a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/master/raw/sha256.txt?job=tarball">SHA256</a></td>
<td><a href="/contributing/read_me/">Notes</a></td>
</tr>
<tr>
<td><i class="fas fa-code-branch"></i> <a href="https://gitlab.com/mattbas/glaxnimate.git">Git Repo</a></td>
<td></td>
<td><a href="/contributing/read_me/">Notes</a></td>
</tr>
</table>

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

    brew install python qt gcc potrace

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

    git clone https://aur.archlinux.org/glaxnimate-bin.git
    cd glaxnimate-bin
    makepkg -rsi

## Snap

For the stable version:

    snap install glaxnimate

For the testing version:

    snap install --beta glaxnimate
