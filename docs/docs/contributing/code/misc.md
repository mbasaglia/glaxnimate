Authors: Mattia Basaglia

# Miscellaneous Tips

## Screen recording

To click on the window to record and start recording straight away:

    recordmydesktop -o /tmp/out.ogv --windowid `xwininfo -display :0 | grep 'id: 0x' | grep -oE "0x\w+"`
    ffmpeg -i /tmp/out.ogv /tmp/out.mp4

## Test GitLab CI jobs

Something along these lines:

    docker run -it -v $PWD:/glaxnimate ubuntu:16.04 bash


## Release Checklist

1. Update the version number on the root `CMakeLists.txt`
2. Update the `pre-release` branch

    ./deploy/tag-branch.sh

3. Push `pre-release`
4. Wait for CI to complete <https://gitlab.com/mattia.basaglia/glaxnimate/-/pipelines>
5. No errors: go to 6. else fix the errors and go back to 3.
5. `git tag` the new release
7. Update the `release` branch

    ./deploy/tag_branch.sh release

8. Push `release` and tags
9. Wait for CI to complete
10. New release should be ready at <https://gitlab.com/mattia.basaglia/glaxnimate/-/releases>
