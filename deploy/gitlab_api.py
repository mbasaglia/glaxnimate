import os
import sys
import json
import shlex
import requests
from urllib.parse import urljoin, quote_plus


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

        self.debug_curl(method, url, **kwargs)
        res = requests.request(method, url, **kwargs)
        if can_fail:
            res.raise_for_status()
        return res.json()

    def project_request(self, method, url, **kwargs):
        return self.request(method, "/".join([self.api_url] + url), **kwargs)

    def debug_curl(self, method, url, **kwargs):
        command = "curl -i -X %s %s" % (method, shlex.quote(url))

        if "data" in kwargs:
            command += " -d " + shlex.quote(kwargs.pop("data"))
        elif "json" in kwargs:
            command += " -d " + shlex.quote(json.dumps(kwargs.pop("json")))

        for header, value in kwargs.pop("headers", {}).items():
            command += " -H \"%s: " % shlex.quote(header)
            if value == self.api_key:
                command += "$GITLAB_ACCESS_TOKEN"
            else:
                command += shlex.quote(value)
            command += '"'

        print("CURL:")
        print(command)
        if kwargs:
            print(kwargs)

    @staticmethod
    def fake_env():
        os.environ.setdefault("CI_PROJECT_URL", "https://gitlab.com/mattbas/glaxnimate")
        os.environ.setdefault("CI_PROJECT_ID", "19921167")
