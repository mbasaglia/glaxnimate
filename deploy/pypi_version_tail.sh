#!/bin/bash
# Just change it whevever we need to make a new release, seems the easiest way
# We could use something like this:
# curl -s https://pypi.org/simple/glaxnimate/ | grep -Eo "glaxnimate-[0-9.]+" | sort | tail -n 1 | cut -d . -f 4
# Then increase it but it might cause different CI jobs to end up with different versions
echo .1
