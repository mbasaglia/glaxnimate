#!/bin/bash

branch="${1:-pre-release}"

git branch -D ${branch} ; git branch ${branch} && git push origin ${branch}:${branch}
