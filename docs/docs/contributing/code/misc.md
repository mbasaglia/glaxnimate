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
3. Update the `pre-release` branch

    ./deploy/tag-branch.sh

4. Push `pre-release`
5. Wait for CI to complete <https://gitlab.com/mattia.basaglia/glaxnimate/-/pipelines>
6. No errors: go to 7. else fix the errors and go back to 4.
7. `git tag` the new release
8. Update the `release` branch

    ./deploy/tag-branch.sh release

9. Push `release` and tags
10. Wait for CI to complete
11. New release should be ready at <https://gitlab.com/mattia.basaglia/glaxnimate/-/releases>
12. Merge `release` / `pre-release` back into master if there have been any new commits
