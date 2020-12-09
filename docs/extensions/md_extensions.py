from markdown.inlinepatterns import InlineProcessor
from markdown.extensions import Extension
import os
import xml.etree.ElementTree as etree


def etree_fontawesome(icon, group="fas"):
    el = etree.Element("i")
    el.attrib["class"] = "%s fa-%s" % (group, icon)
    return el


def clean_link(filename):
    if not filename.startswith("/") and not filename.startswith("."):
        filename = "../" + filename
    return filename


class LottieInlineProcessor(InlineProcessor):
    def __init__(self, md):
        pattern = r'{lottie:([^:]+)(?::([0-9]+):([0-9]+))(?::([^:]*))}'
        super().__init__(pattern, md)
        self._id = 0

    def handleMatch(self, m, data):
        el = etree.Element("div")
        el.attrib["class"] = "lottie-container"

        animation = etree.Element("div")
        el.append(animation)
        animation.attrib["class"] = "alpha_checkered"
        animation.attrib["id"] = "lottie_target_%s" % self._id

        filename = clean_link(m.group(1))
        download_file = m.group(4)
        if download_file is None:
            download_file = clean_link(download_file)

        if m.group(2):
            animation.attrib["style"] = "width:%spx;height:%spx" % (m.group(2), m.group(3))

        play = etree.Element("button")
        el.append(play)
        play.attrib["id"] = "lottie_play_{id}".format(id=self._id)
        play.attrib["onclick"] = "anim_{id}.play(); document.getElementById('lottie_pause_{id}').style.display = 'inline-block'; this.style.display = 'none'".format(id=self._id)
        play.append(etree_fontawesome("play"))
        play.attrib["title"] = "Play"
        play.attrib["style"] = "display:none"

        pause = etree.Element("button")
        el.append(pause)
        pause.attrib["id"] = "lottie_pause_{id}".format(id=self._id)
        pause.attrib["onclick"] = "anim_{id}.pause(); document.getElementById('lottie_play_{id}').style.display = 'inline-block'; this.style.display = 'none'".format(id=self._id)
        pause.append(etree_fontawesome("pause"))
        pause.attrib["title"] = "Pause"

        if download_file:
            download = etree.Element("a")
            el.append(download)
            download.attrib["href"] = download_file
            download.attrib["download"] = ""
            download.attrib["title"] = "Download"
            download_button = etree.Element("button")
            download.append(download_button)
            download_button.append(etree_fontawesome("download"))

        script = etree.Element("script")
        el.append(script)
        script.text = """
            var anim_{id} = bodymovin.loadAnimation({{
                container: document.getElementById('lottie_target_{id}'),
                renderer: 'svg',
                loop: true,
                autoplay: true,
                path: '{file}'
            }});
        """.format(id=self._id, file=filename)

        self._id += 1
        return el, m.start(0), m.end(0)


class DownloadTable(InlineProcessor):
    def __init__(self, pattern, md, git):
        super().__init__(pattern, md)
        self.git = git

    class Row:
        def __init__(self, name, icon_group, icon, filename, job, notes=None, parent="/build"):
            self.name = name
            self.icon_group = icon_group
            self.icon = icon
            self.filename = filename
            self.checksum = "SHA1"
            self.notes = notes if notes else "#" + name.lower().replace(" ", "-")
            self.job_path = job
            self.parent = parent

        def element(self, parent, branch):
            tr = etree.SubElement(parent, "tr")

            pack = etree.SubElement(tr, "td")
            icon = etree_fontawesome(self.icon, self.icon_group)
            icon.tail = " "
            pack.append(icon)
            if "/" in self.job_path:
                url = "https://gitlab.com/mattbas/glaxnimate-artifacts/-/raw/master/{0}{1}{{0}}".format(branch, self.job_path)
            else:
                url = "https://gitlab.com/mattbas/glaxnimate/-/jobs/artifacts/{0}/raw{2}/{{0}}?job={1}".format(
                    branch, self.job_path.replace(":", "%3A"), self.parent
                )

            etree.SubElement(pack, "a", {"href": url.format(self.filename)}).text = self.name

            check = etree.SubElement(tr, "td")
            etree.SubElement(check, "a", {"href": url.format("checksum.txt")}).text = self.checksum

            notes = etree.SubElement(tr, "td")
            etree.SubElement(notes, "a", {"href": self.notes}).text = "Notes"

    def handleMatch(self, m, data):
        table = etree.Element("table")

        thead = etree.SubElement(table, "thead")
        tr = etree.SubElement(thead, "tr")
        etree.SubElement(tr, "th").text = "Package"
        etree.SubElement(tr, "th").text = "Checksum"
        etree.SubElement(tr, "th").text = "Notes"

        tbody = etree.SubElement(table, "tbody")
        branch = m.group(1)
        rows = [
            self.Row("Linux Appimage",  "fab", "linux",    "glaxnimate-x86_64.AppImage",   "linux:appimage"),
            self.Row("Deb Package",     "fab", "ubuntu",   "glaxnimate.deb",               "linux:deb"),
            self.Row("Windows Zip",     "fab", "windows",  "glaxnimate-x86_64.zip",        "/Win/"),
            self.Row("Mac DMG",         "fab", "apple",    "glaxnimate.dmg",               "/MacOs/"),
            self.Row("Source Tarball",  "fas", "wrench",   "glaxnimate-src.tar.gz",        "tarball", "/contributing/read_me", ""),
        ]

        for row in rows:
            row.element(tbody, branch)

        if self.git:
            tr = etree.SubElement(tbody, "tr")
            pack = etree.SubElement(tr, "td")
            icon = etree_fontawesome("code-branch", "fas")
            icon.tail = " "
            pack.append(icon)
            etree.SubElement(pack, "a", {"href": "https://gitlab.com/mattbas/glaxnimate.git"}).text = "Git Repo"
            etree.SubElement(tr, "td")
            notes = etree.SubElement(tr, "td")
            etree.SubElement(notes, "a", {"href": "/contributing/read_me/"}).text = "Notes"

        return table, m.start(0), m.end(0)


class GlaxnimateExtension(Extension):
    def extendMarkdown(self, md):
        md.inlinePatterns.register(LottieInlineProcessor(md), 'lottie', 175)
        md.inlinePatterns.register(DownloadTable(r'{download_table:([^:]+)}', md, False), 'download_table', 175)
        md.inlinePatterns.register(DownloadTable(r'{download_table:([^:]+):git}', md, True), 'download_table_git', 175)


def makeExtension(**kwargs):
    return GlaxnimateExtension(**kwargs)
