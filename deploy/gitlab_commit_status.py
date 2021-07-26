#!/usr/bin/env python3

import os
import subprocess
from argparse import ArgumentParser
from gitlab_api import GitlabApi, environ


def git(*args):
    pipe = subprocess.Popen(["git", "-C", os.path.dirname(__file__)] + list(args), stdout=subprocess.PIPE)
    out, _ = pipe.communicate()
    return out.decode('ascii').strip()


parser = ArgumentParser()
parser.add_argument("--fake-env", action="store_true")
parser.add_argument("state", choices=["pending", "running", "success", "failed", "canceled", "cancelled", "failure"])

ns = parser.parse_args()

if ns.fake_env:
    commit = git("rev-parse",  "HEAD")
    name = "Test"
    url = "https://github.com/mbasaglia/glaxnimate/actions"
    ref = "refs/heads/" + git("branch", "--show-current")
else:
    commit = environ("GITHUB_SHA")
    name = "%s: %s" % (environ("GITHUB_WORKFLOW"), environ("GITHUB_JOB"))
    url = environ("GITHUB_SERVER_URL") + "/" + environ("GITHUB_REPOSITORY") + "/runs/" + environ("GITHUB_RUN_ID")
    ref = environ("GITHUB_REF")


if ref.startswith("refs/"):
    ref = ref.rsplit("/", 1)[1]


GitlabApi.fake_env()
api = GitlabApi()

status = ns.state
# Convert GitHub => GitLab
if status == "cancelled":
    status = "canceled"
elif status == "failure":
    status = "failed"


data = {
    "id": api.project_id,
    "sha": commit,
    "state": status,
    "ref": ref,
    "name": name,
    "target_url": url
}

api.project_request("post", ["statuses", commit], json=data)
