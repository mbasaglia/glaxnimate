#!/usr/bin/env python3

import os
import re
import sys
import json
import base64
from argparse import ArgumentParser
from urllib.parse import urljoin, urlencode, quote
from urllib.request import urlopen, HTTPError, Request
from pathlib import Path


class GitlabApi:
    def __init__(self, api_key):
        self.project_id = "mattbas%2Fglaxnimate-artifacts"
        self.gitlab = "https://gitlab.com/"
        self.api_version = "v4"
        self.api_url = urljoin(self.gitlab, "/api/%s/" % self.api_version)
        self.api_key = api_key

    def request(self, method, url, **kwargs):
        kwargs.setdefault("headers", {})
        kwargs["headers"]["PRIVATE-TOKEN"] = self.api_key
        kwargs["method"] = method.upper()

        if "json" in kwargs:
            kwargs["headers"]["Content-Type"] = "application/json"
            kwargs["data"] = json.dumps(kwargs.pop("json")).encode("utf-8")
        elif method == "get" and "data" in kwargs:
            url += "?" + urlencode(kwargs.pop("data"))

        req = Request(self.api_url + url, **kwargs)
        res = urlopen(req)
        return json.load(res)

    def project_request(self, method, url, **kwargs):
        return self.request(method, "/".join(["projects", self.project_id] + url), **kwargs)

    def file_exists(self, path):
        resp = self.project_request("get", ["repository", "tree"], data={"path": path.rsplit("/", 1)[0]})
        for file in resp:
            if file["path"] == path:
                return True
        return False

    def file_get(self, path):
        return self.project_request("get", ["repository", "files", quote(path, "")], data={"ref": "master"})["content"]


api_key = os.environ.get("GITLAB_ACCESS_TOKEN", "")
if not api_key:
    print("Missing GITLAB_ACCESS_TOKEN. Skipping deployment.")
    sys.exit(0)


parser = ArgumentParser()
parser.add_argument("input_file")
parser.add_argument("output_file")
parser.add_argument("--copy", action="store_true")
args = parser.parse_args()

api = GitlabApi(api_key)

if args.copy:
    data = api.file_get(args.input_file)
else:
    with open(args.input_file, "rb") as f:
        data = base64.b64encode(f.read()).decode("ascii")

method = "put" if api.file_exists(args.output_file) else "post"

resp = api.project_request(method, ["repository", "files", quote(args.output_file, "")], json={
    "branch": "master",
    "author_email": "ci@dragon.best",
    "author_name": "CI",
    "content": data,
    "encoding": "base64",
    "commit_message": "Upload %s" % args.output_file
})

print(resp)
