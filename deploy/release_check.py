#!/usr/bin/env python3
import os
import re
import sys
import time
import hashlib
import argparse
import requests
from urllib.parse import urljoin, urlparse
from xml.etree import ElementTree


class GitlabApi:
    def __init__(self, project_id="19921167"):
        self.api_url = "https://gitlab.com/api/v4/projects/%s/repository" % project_id

    def get(self, *url, **kwargs):
        url = "/".join((self.api_url,) + url)
        res = requests.request("get", url, **kwargs)
        return (res.json(), res.status_code)


def download(indent, url):
    res = requests.get(url)
    if res.status_code != 200:
        error(indent, "Failed to fetch %s (%s)" % (url, res.status_code))
        return None
    time.sleep(0.1)
    return res.content


def log(indent, msg):
    print(" " * (indent*4) + msg)


def error(indent, msg):
    global retcode
    retcode += 1
    sys.stderr.write(" " * (indent*4) + "\x1b[31m%s\x1b[m\n" % msg)


class ReleaseRow:
    def __init__(self, name, package_url, hash_algo, hash_url, notes_url):
        self.name = name
        self.package_url = package_url
        self.hash_algo = hash_algo
        self.hash_url = hash_url
        self.notes_url = notes_url

    def print(self):
        log(1, self.name)
        log(2, self.package_url)
        if self.notes_url:
            log(2, self.notes_url)
        log(2, self.hash_algo)
        log(3, self.hash_url)

    def check_hash(self):
        log(1, self.name)

        hasher = getattr(hashlib, self.hash_algo.lower(), None)
        if not hasher:
            error(2, "Unknown hashing algorithm: %s" % self.hash_algo)
            return

        checksum = download(2, self.hash_url)
        if checksum is None:
            return

        data = download(2, self.package_url)
        if data is None:
            return

        if self.notes_url:
            download(2, self.notes_url)

        eval_hash = hasher(data).hexdigest()
        dl_hash = re.search(b'^[0-9a-f]+', checksum).group(0).decode("utf-8")
        if dl_hash != eval_hash:
            error(2, "Hash mismatch: got %s, should be %s" % (dl_hash, eval_hash))

    def download_url(self, path, url, basename):
        full_path = os.path.join(path, basename)
        with open(full_path, "wb") as f:
            f.write(download(1, url))

    def download(self, path):
        basename = os.path.basename(urlparse(self.package_url).path)
        print(basename)
        self.download_url(path, self.package_url, basename)
        basename_hash = "%s.%s.txt" % (basename, self.hash_algo)
        self.download_url(path, self.hash_url, basename_hash)


class ReleaseTable:
    def __init__(self):
        self.rows = []

    def get_link(self, td, row, col):
        found = None

        for a in td:
            if a.tag == "a":
                if found is not None:
                    error(2, "Too many links in row %s col %s" % (row, col))
                    return None
                found = a

        if found is None:
            error(2, "No link in row %s col %s" % (row, col))
            return None

        return [found.attrib["href"], found.text]

    def check_download_table_row_common(self, e, i):
        if e.tag != "tr":
            error(1, "Expected <tr> for row %s" % i)
            return False
        if len(e) != 3:
            error(1, "Unexpected number of elements in row %s" % i)
            return False
        return True

    def check_download_table_row_head(self, e):
        if self.check_download_table_row_common(e, 0):
            if e[0].tag != "th":
                error(1, "First row should be headers")

    def check_download_table_row_body(self, e, i, rel_url):
        if self.check_download_table_row_common(e, i):
            if any(c.tag != "td" for c in e):
                error(1, "Invalid elements in row %s" % i)

            links = [self.get_link(td, i, col) for col, td in enumerate(e)]
            if any(x is None for x in links):
                return

            download_link, download_name = links[0]
            sha_link, sha_algo = links[1]
            notes_link = links[2][0]

            if notes_link.startswith("#"):
                notes_link = urljoin(rel_url, notes_link)
            else:
                notes_link = None

            self.rows.append(
                ReleaseRow(download_name, download_link, sha_algo, sha_link, notes_link)
            )

    def check_download_table(self, description, rel_url=""):
        match_s = re.search("<table", description)
        match_e = re.search("</table>", description)
        if not match_s or not match_e:
            error(1, "No download table")
            return

        html_text = description[match_s.start(0):match_e.end(0)]
        try:
            html = ElementTree.fromstring(html_text)
        except ElementTree.ParseError as e:
            print(html_text)
            error(1, "Invalid download table: %s" % e)
            return

        if len(html) < 2:
            error(1, "Too few row in the table")
            return

        if html[0].tag == "tr":
            for i, e in enumerate(html):
                if i == 0:
                    self.check_download_table_row_head(e)
                else:
                    self.check_download_table_row_body(e, i, rel_url)
        else:
            if html[0].tag != "thead":
                error(1, "Invalid html table (missing head)")
            elif len(html[0]) != 1:
                error(1, "Wrong number of rows in the table header")
            else:
                self.check_download_table_row_head(html[0][0])

            if html[1].tag != "tbody":
                error(1, "Invalid html table (missing body)")
            elif len(html[1]) < 1:
                error(1, "Wrong number of rows in the table body")
            else:
                for i, row in enumerate(html[1]):
                    self.check_download_table_row_body(row, i, rel_url)

    def check_download_page(self):
        url = "https://glaxnimate.mattbas.org/download"
        html = download(1, url)
        if not html:
            return
        self.check_download_table(html.decode("utf-8"), url)


#def check_tag():
    #log(0, "Checking tag")
    #response, status = api.get("tags", ns.version)
    #if status != 200:
        #error(1, "No tag")
        #return

    #if "release" not in response or "description" not in response["release"]:
        #error(1, "No release")
        #return

    #log(0, "Validating Release Page Downloads")
    #description = response["release"]["description"]
    #check_download_table(description)


retcode = 0
parser = argparse.ArgumentParser()
#parser.add_argument("version")
parser.add_argument("--action", default="check_hash", choices=["check_hash", "list", "download"])
parser.add_argument("--download-path", default="/tmp")
parser.add_argument("--package", default=None, nargs="+")

ns = parser.parse_args()

api = GitlabApi()
#log(0, "Checking %s" % ns.version)
#check_tag()
if retcode == 0:
    release = ReleaseTable()
    release.check_download_page()

    rows = release.rows
    if ns.package:
        rows = [row for row in rows if row.name in ns.package]

        if len(rows) != len(ns.package):
            error(0, "Not all packages found")
            log(0, "Available packages")
            for row in release.rows:
                log(1, row.name)
            log(0, "Requested packages")
            for row in ns.package:
                log(1, row)

    if ns.action == "check_hash":
        log(0, "Validating Website Download Page")
        for row in rows:
            row.check_hash()
    elif ns.action == "list":
        for row in rows:
            row.print()
    elif ns.action == "download":
        for row in rows:
            row.download(ns.download_path)
sys.exit(retcode)
