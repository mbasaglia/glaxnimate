#!/usr/bin/env python3

import os
import re
import sys
from argparse import ArgumentParser
from urllib.parse import urljoin, quote_plus
from pathlib import Path
import requests


def fail(msg):
    sys.stderr.write(msg+"\n")
    sys.exit(1)


def environ(varname, failmsg=""):
    try:
        return os.environ[varname]
    except KeyError:
        fail("Missing environment variable %s: %s\n" % (varname, failmsg))


class GitlabApi:
    def __init__(self):
        self.tag = environ("CI_COMMIT_TAG", "You must run this on a tag build")
        self.project_url = environ("CI_PROJECT_URL")
        self.project_id = environ("CI_PROJECT_ID")
        self.api_version = "v4"
        self.api_url = urljoin(self.project_url, "/api/%s/projects/%s" % (self.api_version, self.project_id))
        self.api_key = environ("GITLAB_ACCESS_TOKEN", "You must specify an access token. See https://gitlab.com/profile/personal_access_tokens")

    def request(self, method, url, **kwargs):
        kwargs.setdefault("headers", {})
        kwargs["headers"]["PRIVATE-TOKEN"] = self.api_key
        if "json" in kwargs:
            kwargs["headers"]["Content-Type"] = "application/json"
        can_fail = not kwargs.pop("can_fail", False)
        res = requests.request(method, url, **kwargs)
        if can_fail:
            res.raise_for_status()
        return res.json()

    def project_request(self, method, url, **kwargs):
        return self.request(method, "/".join([self.api_url] + url), **kwargs)


parser = ArgumentParser()
parser.add_argument("--fake-env", action="store_true")
parser.add_argument("--download-tag", default=None)
parser.add_argument("--changelog-tag", default=None)

ns = parser.parse_args()
root_dir = Path(__file__).absolute().parent.parent

if ns.fake_env:
    fake_ver = re.search("project\(.* VERSION (\S+)", open(root_dir / "CMakeLists.txt").read()).group(1)
    os.environ.setdefault("CI_COMMIT_TAG", fake_ver)
    os.environ.setdefault("CI_PROJECT_URL", "https://gitlab.com/mattia.basaglia/glaxnimate")
    os.environ.setdefault("CI_PROJECT_ID", "19921167")


api = GitlabApi()
download_tag = ns.download_tag or api.tag
changelog_tag = ns.changelog_tag or api.tag


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
<td><a href="{artifacts_url}/glaxnimate-x86_64.AppImage?job=linux%3Aappimage">Linux AppImage</a></td>
<td><a href="{artifacts_url}/checksum.txt?job=linux%3Aappimage">SHA1</a></td>
<td><a href="{install_notes_url}#linux-appimage">Installation Instructions</a></td>
</tr>
<tr>
<td><a href="{artifacts_url}/glaxnimate.deb?job=linux%3Adeb">Deb Package</a></td>
<td><a href="{artifacts_url}/checksum.txt?job=linux%3Adeb">SHA1</a></td>
<td><a href="{install_notes_url}#deb-package">Installation Instructions</a></td>
</tr>
<tr>
<td><a href="{artifacts_url}/glaxnimate-x86_64.zip?job=mxe%3Abuild">Windows Zip</a></td>
<td><a href="{artifacts_url}/checksum.txt?job=mxe%3Abuild">SHA1</a></td>
<td><a href="{install_notes_url}#windows-zip">Installation Instructions</a></td>
</tr>
<tr>
<td><a href="{bintray_url}/MacOs/glaxnimate.dmg">Mac dmg</a></td>
<td><a href="{bintray_url}/MacOs/checksum.txt">SHA1</a></td>
<td><a href="{install_notes_url}#mac-dmg">Installation Instructions</a></td>
</tr>
</table>

## User Manual

See the [Documentation](https://glaxnimate.mattbas.org/manual/) page.

## Changes
{chlog}

""".format(
    version=api.tag,
    chlog=chlog,
    project_url=api.project_url,
    artifacts_url=api.project_url+"/-/jobs/artifacts/" + download_tag + "/raw/build",
    install_notes_url="https://glaxnimate.mattbas.org/download/",
    bintray_url="https://dl.bintray.com/mattbas/Glaxnimate/" + download_tag
)


old_release = api.project_request("get", ["repository", "tags", api.tag])["release"]
release = api.project_request(
    "put" if old_release else "post",
    ["repository", "tags", api.tag, "release"],
    json={
        "description": notes
    }
)
api.project_request("put", ["releases", api.tag], can_fail=True, json={
    "milestones": [api.tag]
})
