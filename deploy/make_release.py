#!/usr/bin/env python3

import os
import re
import sys
from argparse import ArgumentParser
from pathlib import Path
from gitlab_api import GitlabApi, environ, fail


parser = ArgumentParser()
parser.add_argument("--fake-env", action="store_true")
parser.add_argument("--download-tag", default=None)
parser.add_argument("--changelog-tag", default=None)

ns = parser.parse_args()
root_dir = Path(__file__).absolute().parent.parent

if ns.fake_env:
    fake_ver = re.search("project\(.* VERSION (\S+)", open(root_dir / "CMakeLists.txt").read()).group(1)
    os.environ.setdefault("CI_COMMIT_TAG", fake_ver)
    GitlabApi.fake_env()


api = GitlabApi()
commit_tag = environ("CI_COMMIT_TAG", "You must run this on a tag build")
download_tag = ns.download_tag or commit_tag
changelog_tag = ns.changelog_tag or commit_tag


with open(root_dir / "CHANGELOG.md") as changelog:
    lines = list(changelog)
    for i in range(len(lines)):
        if lines[i].startswith("## ") and changelog_tag in lines[i]:
            break
    else:
        fail("No release notes in the Changelog")

    chlog = ""
    for j in range(i+1, len(lines)):
        if lines[j].startswith("## "):
            break
        chlog += lines[j]

    if not chlog:
        fail("No release notes in the Changelog")

notes = """
# Glaxnimate {version}

## Download

<table>
<tr><th>Package</th><th>Checksum</th><th>Installation Instructions</th></tr>
<tr>
<td><a href="{artifacts_url}/build/glaxnimate-x86_64.AppImage?job=linux%3Aappimage">Linux AppImage</a></td>
<td><a href="{artifacts_url}/build/checksum.txt?job=linux%3Aappimage">SHA1</a></td>
<td><a href="{install_notes_url}#linux-appimage">Installation Instructions</a></td>
</tr>
<tr>
<td><a href="{artifacts_url}/build/glaxnimate.deb?job=linux%3Adeb">Deb Package</a></td>
<td><a href="{artifacts_url}/build/checksum.txt?job=linux%3Adeb">SHA1</a></td>
<td><a href="{install_notes_url}#deb-package">Installation Instructions</a></td>
</tr>
<tr>
<td><a href="{extra_artifacts}/glaxnimate-x86_64.zip">Windows Zip</a></td>
<td><a href="{extra_artifacts}/checksum-win.txt">SHA1</a></td>
<td><a href="{install_notes_url}#windows-zip">Installation Instructions</a></td>
</tr>
<tr>
<td><a href="{extra_artifacts}/glaxnimate.dmg">Mac dmg</a></td>
<td><a href="{extra_artifacts}/checksum-mac.txt">SHA1</a></td>
<td><a href="{install_notes_url}#mac-dmg">Installation Instructions</a></td>
</tr>
<tr>
<td><a href="{artifacts_url}/glaxnimate-src.tar.gz?job=tarball">Source Tarball</a></td>
<td><a href="{artifacts_url}/sha256.txt?job=tarball">SHA256</a></td>
<td><a href="{install_notes_url}#building-from-source">Notes</a></td>
</tr>
</table>

## User Manual

See the [Documentation](https://glaxnimate.mattbas.org/manual/) page.

## Changes
{chlog}

""".format(
    version=commit_tag,
    chlog=chlog,
    project_url=api.project_url,
    artifacts_url=api.project_url+"/-/jobs/artifacts/" + download_tag + "/raw",
    install_notes_url="https://glaxnimate.mattbas.org/download/",
    extra_artifacts="https://github.com/mbasaglia/glaxnimate/releases/download/%s/" % download_tag,
    extra_tail="?viasf=1",
)


old_release = api.project_request("get", ["repository", "tags", commit_tag])["release"]

release = {
    "description": notes,
    "tag_name": commit_tag,
}

if old_release:
    api.project_request("put", ["releases", commit_tag], json=release)
else:
    api.project_request("post", ["releases"], json=release)

# Milestone might not exist for point releases
api.project_request("put", ["releases", commit_tag], can_fail=True, json={
    "milestones": [commit_tag]
})
