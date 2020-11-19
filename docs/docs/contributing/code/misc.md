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
11. Run `./deploy/release_check.py` or `make release_check` to check the release is ok
12. Merge `release` / `pre-release` back into master if there have been any new commits
