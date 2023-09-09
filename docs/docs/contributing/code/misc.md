Authors: Mattia Basaglia

# Miscellaneous Tips

## Screen recording

To click on the window to record and start recording straight away:

    # No sound
    recordmydesktop --overwrite --no-sound -o /tmp/out.ogv --windowid `xwininfo -display :0 | grep 'id: 0x' | grep -oE "0x\w+"`
    # Yes sound
    recordmydesktop --overwrite -o /tmp/out.ogv --windowid `xwininfo -display :0 | grep 'id: 0x' | grep -oE "0x\w+"`
    # Convert
    ffmpeg -i /tmp/out.ogv /tmp/out.mp4


## Docker

Something along these lines:

    docker run --rm -it -v $PWD:/glaxnimate ubuntu:16.04 bash

To have GUI stuff, remember thse options:

    --net=host -e DISPLAY -v /tmp/.X11-unix

You can use the script that adds all that

    ./deploy/docker-run.sh image:tag


## Release Checklist

1. Update the version number on the root `CMakeLists.txt`
2. Ensure `CHANGELOG.md` is up to date and changes are under the heading for the scheduled release
3. Add release notes page to the docs
4. Build the `release_0` target
5. Wait for CI to complete <https://gitlab.com/mattbas/glaxnimate/-/pipelines>
6. If there are errors
    * fix the errors
    * `./deploy/tag-branch.sh`
    * go back to 4.
7. build the `release_1` target, this builds the `release` branch
8. Wait for CI to complete
9. In the tag pipeline, manually run the `release` job
10. Wait for CI to complete
11. New release should be ready at <https://gitlab.com/mattbas/glaxnimate/-/releases>
12. Run the release validation jobs
    * https://gitlab.com/mattbas/glaxnimate/-/pipelines `release:check`
    * https://github.com/mbasaglia/glaxnimate/actions/workflows/verify_release.yml
13. Merge `release` / `pre-release` back into master if there have been any new commits


## Creating/Editing AUR packages with docker

    # This enables using GUI as well for testing
    docker run -it --rm --net=host -e DISPLAY -v /tmp/.X11-unix archlinux bash

    # System Setup
    pacman -Sy
    # You might need to install upgrades too
    pacman -Su --noconfirm
    # (Add required build packages as needed) check .gitlab-ci.yml for what CI uses
    pacman -S --noconfirm git base-devel fakeroot vim openssh namcap xorg-xauth cmake qt5-base python zlib hicolor-icon-theme potrace ffmpeg qt5-tools sudo qt5-svg qt5-imageformats
    # Make jobs
    echo 'MAKEFLAGS="-j4"' >>/etc/makepkg.conf

    # Set up a user if using docker
    useradd -m foo
    su foo
    cd
    cat <<HERE >key
    # (KEY)
    chmod 600 key
    AUR_SSH_KEY="$PWD/key"

    # Clone AUR package
    PACK_NAME=glaxnimate-git
    GIT_SSH_COMMAND="ssh -o StrictHostKeyChecking=no -i $AUR_SSH_KEY" git clone ssh://aur@aur.archlinux.org/$PACK_NAME.git
    cd $PACK_NAME

    # Edit script see https://wiki.archlinux.org/index.php/Creating_packages
    vim PKGBUILD
    makepkg -f

    # Test
    pacman -U *.pkg.tar.zst
    namcap PKGBUILD
    namcap *.pkg.tar.zst

    # Set up GUI for docker
    touch ~/.Xauthority
    # Run `xauth list` on the host and paster the output as argument to the following
    xauth add

    # Submit to AUR
    makepkg --printsrcinfo > .SRCINFO
    git config --global user.name
    git config --global user.email
    git add PKGBUILD .SRCINFO
    git commit
    GIT_SSH_COMMAND="ssh -o StrictHostKeyChecking=no -i $AUR_SSH_KEY" git push origin master

## Run a VM from an ISO file to test in a full DE

    # Append the path to the iso file
    qemu-system-x86_64 -m 4G -boot d -enable-kvm -smp 3 -net nic -net user -cdrom

## Various Repositories

* [FlatHub](https://github.com/flathub/org.mattbas.Glaxnimate)
* Aur [Stable](https://aur.archlinux.org/packages/glaxnimate) [Dev](https://aur.archlinux.org/packages/glaxnimate-git)
* [PyPI](https://pypi.org/project/glaxnimate/)
* [Snapcraft](https://snapcraft.io/glaxnimate)
