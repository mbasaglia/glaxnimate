#!/bin/bash
curl -s https://pypi.org/simple/glaxnimate/ | grep -Eo "glaxnimate-[0-9.]+" | sort | tail -n 1 | cut -d - -f 2
