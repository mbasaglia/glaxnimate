Authors: Mattia Basaglia

# Git Branches

## `release`

This branch is for stable releases only.

Each time you merge other branches in here you must ensure they already pass CI
and you must add a version tag.

CI will automatically create a release.

## `pre-release`

This branch is to prepare the code for merging into `release`.

This is so new features can be worked on and merged into `master` without affecting
the next release.

## `master`

This is the main development branch, CI will use it to create the development snapshot builds.

## Feature branches

Most things should be developed in feature branches,
which can be tested separately before being merged into `master`.
