Authors: Mattia Basaglia

# Git Branches

## `release`

This branch is for stable releases only.

Each time you merge other branches in here you must ensure they already pass CI
and you must add a version tag.

CI will automatically create a release.

## `pre-release`

This branch is to prepare the code for merging into `release`.

This is so new features can be worked on and merged into `master` without affecting
the next release.

### `pre-release` Downloads
<table>
<tr><th>Package</th><th>Checksum</th><th>Notes</th></tr>
<tr>
<td><i class="fab fa-linux"></i> <a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/pre-release/raw/build/glaxnimate-x86_64.AppImage?job=linux%3Aappimage">Linux AppImage</a></td>
<td><a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/pre-release/raw/build/checksum.txt?job=linux%3Aappimage">SHA1</a></td>
<td><a href="#linux-appimage">Notes</a></td>
</tr>
<tr>
<td><i class="fab fa-ubuntu"></i> <a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/pre-release/raw/build/glaxnimate.deb?job=linux%3Adeb">Deb Package</a></td>
<td><a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/pre-release/raw/build/checksum.txt?job=linux%3Adeb">SHA1</a></td>
<td><a href="#deb-package">Notes</a></td>
</tr>
<tr>
<td><i class="fab fa-windows"></i> <a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/pre-release/raw/build/glaxnimate-x86_64.zip?job=mxe%3Abuild">Windows Zip</a></td>
<td><a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/pre-release/raw/build/checksum.txt?job=mxe%3Abuild">SHA1</a></td>
<td><a href="#windows-zip">Notes</a></td>
</tr>
<tr>
<td><i class="fab fa-apple"></i> <a href="https://dl.bintray.com/mattbas/Glaxnimate/pre-release/MacOs/glaxnimate.dmg">Mac dmg</a></td>
<td><a href="https://dl.bintray.com/mattbas/Glaxnimate/pre-release/MacOs/checksum.txt">SHA1</a></td>
<td><a href="#mac-dmg">Notes</a></td>
</tr>
</table>

## `master`

This is the main development branch, CI will use it to create the development snapshot builds.

### `master` Downloads
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
<td><i class="fab fa-windows"></i> <a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/master/raw/build/glaxnimate-x86_64.zip?job=mxe%3Abuild">Windows Zip</a></td>
<td><a href="https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/master/raw/build/checksum.txt?job=mxe%3Abuild">SHA1</a></td>
<td><a href="#windows-zip">Notes</a></td>
</tr>
<tr>
<td><i class="fab fa-apple"></i> <a href="https://dl.bintray.com/mattbas/Glaxnimate/master/MacOs/glaxnimate.dmg">Mac dmg</a></td>
<td><a href="https://dl.bintray.com/mattbas/Glaxnimate/master/MacOs/checksum.txt">SHA1</a></td>
<td><a href="#mac-dmg">Notes</a></td>
</tr>
</table>

## Feature branches

Most things should be developed in feature branches,
which can be tested sparately before being merged into `master`.
