#!/bin/bash

branch="${1:-pre-release}"

git branch -d ${branch} && git branch ${branch} && git push origin ${branch}:${branch}
