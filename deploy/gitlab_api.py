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
    default_project_url = "https://gitlab.com/mattbas/glaxnimate"
    default_project_id = "19921167"

    def __init__(self, project_url=default_project_url, project_id=default_project_id, api_key=None):
        self.project_url = project_url
        self.project_id = project_id
        self.api_version = "v4"
        self.api_url = urljoin(self.project_url, "/api/%s/projects/%s" % (self.api_version, self.project_id))
        self.api_key = api_key

    def request(self, method, url, **kwargs):
        kwargs.setdefault("headers", {})

        if self.api_key:
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
        command = "curl -i -X %s %s" % (method.upper(), shlex.quote(url))

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

    def artifact_url(self, ref, job, file):
        return self.artifacts_url(ref) + "/%s?job=%s" % (file, job.replace(":", "%3A"))

    def artifacts_url(self, ref):
        return self.api_url + "/jobs/artifacts/%s/raw" % ref.replace(".", "%2E")

    @classmethod
    def from_env(cls):
        project_url = environ("CI_PROJECT_URL")
        project_id = environ("CI_PROJECT_ID")
        api_key = environ("GITLAB_ACCESS_TOKEN", "You must specify an access token. See https://gitlab.com/-/profile/personal_access_tokens")
        return cls(project_url, project_id, api_key)

    @classmethod
    def fake_env(cls):
        os.environ.setdefault("CI_PROJECT_URL", cls.default_project_url)
        os.environ.setdefault("CI_PROJECT_ID", cls.default_project_id)
