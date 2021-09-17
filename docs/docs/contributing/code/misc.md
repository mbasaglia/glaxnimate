Authors: Mattia Basaglia

# Miscellaneous Tips

## Screen recording

To click on the window to record and start recording straight away:

    # No sound
    recordmydesktop --no-sound -o /tmp/out.ogv --windowid `xwininfo -display :0 | grep 'id: 0x' | grep -oE "0x\w+"`
    # Yes sound
    recordmydesktop -o /tmp/out.ogv --windowid `xwininfo -display :0 | grep 'id: 0x' | grep -oE "0x\w+"`
    # Convert
    ffmpeg -i /tmp/out.ogv /tmp/out.mp4

## Test GitLab CI jobs

Something along these lines:

    docker run -it -v $PWD:/glaxnimate ubuntu:16.04 bash


## Release Checklist

1. Update the version number on the root `CMakeLists.txt`
2. Ensure `CHANGELOG.md` is up to date and changes are under the heading for the scheduled release
3. Build the `release_0` target
4. Wait for CI to complete <https://gitlab.com/mattbas/glaxnimate/-/pipelines>
5. If there are erros
    * fix the errors
    * `./deploy/tag-branch.sh`
    * go back to 4.
6. build the `release_1` target, this builds the `release` branch
7. Wait for CI to complete
8. In the tag pipeline, manually run the `release` job
9. Wait for CI to complete
10. New release should be ready at <https://gitlab.com/mattbas/glaxnimate/-/releases>
11. Run the release validation jobs
    * https://gitlab.com/mattbas/glaxnimate/-/pipelines `release:check`
    * https://github.com/mbasaglia/glaxnimate/actions/workflows/verify_release.yml
12. Merge `release` / `pre-release` back into master if there have been any new commits


## Creating/Editing AUR packages with docker

    # This enables using GUI as well for testing
    docker run -it --rm --net=host -e DISPLAY -v /tmp/.X11-unix archlinux bash

    # System Setup
    pacman -Sy
    # You might need to install upgrades too
    pacman -Su --noconfirm
    # (Add required build packages as needed)
    pacman -S --noconfirm git base-devel fakeroot vim openssh namcap xorg-xauth

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
