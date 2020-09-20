Authors: Mattia Basaglia

# Miscellaneous Tips

## Screen recording

To click on the window to record and start recording straight away:

    recordmydesktop --windowid `xwininfo -display :0 | grep 'id: 0x' | grep -oE "0x\w+"`

## Test GitLab CI jobs

Something along these lines:

    docker run -it -v $PWD:/glaxnimate ubuntu:16.04 bash
