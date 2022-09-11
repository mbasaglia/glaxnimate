import re
import os
import sys
from pathlib import Path

import xml.etree.ElementTree as etree
from xml.etree.ElementTree import parse as parse_xml

from markdown.inlinepatterns import InlineProcessor
from markdown.blockprocessors import BlockProcessor
from markdown.extensions import Extension
from babel import Locale

project_root = Path(__file__).parent.parent.parent

sys.path.append(str(project_root / "deploy"))

from gitlab_api import GitlabApi

gitlab = GitlabApi()

def etree_fontawesome(icon, group="fas"):
    el = etree.Element("i")
    el.attrib["class"] = "%s fa-%s" % (group, icon)
    return el


def clean_link(filename):
    if not filename.startswith("/") and not filename.startswith("."):
        filename = "../" + filename
    return filename


def css_style(**args):
    string = ""
    for k, v in args.items():
        string += "%s:%s;" % (k.replace("_", "-"), v)

    return string


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
        elif download_file == "-":
            download_file = filename

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
            if download_file.endswith("rawr"):
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
        def __init__(self, name, icon_group, icon, filename, job, notes=None, parent="build/"):
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

            checksum_filename = "checksum.txt"

            if self.job_path.startswith("-"):
                url = "https://github.com/mbasaglia/glaxnimate/releases/download/{0}/{{0}}".format(branch)
                checksum_filename = "checksum%s.txt" % self.job_path
            else:
                url = gitlab.artifact_url(branch, self.job_path, self.parent + "{0}")

            etree.SubElement(pack, "a", {"href": url.format(self.filename)}).text = self.name

            check = etree.SubElement(tr, "td")
            etree.SubElement(check, "a", {"href": url.format(checksum_filename)}).text = self.checksum

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
            self.Row("Windows Zip",     "fab", "windows",  "glaxnimate-x86_64.zip",        "-win"),
            self.Row("Mac DMG",         "fab", "apple",    "glaxnimate.dmg",               "-mac"),
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


class TranslationTable(InlineProcessor):
    def __init__(self, pattern, md):
        super().__init__(pattern, md)
        self.data = None

    def fetch_data(self):
        self.data = []

        source = project_root / "data" / "translations"

        for file in source.glob("*.ts"):
            locale_name = file.stem.split("_", 1)[1]
            locale = Locale.parse(locale_name)

            dom = parse_xml(open(file))
            translated = 0
            untranslated = 0
            obsolete = 0
            for message in dom.findall(".//message"):
                type = message.find("translation").attrib.get("type")
                if type == "unfinished":
                    untranslated += 1
                elif not type:
                    translated += 1
                elif type == "vanished" or type == "obsolete":
                    obsolete += 1

            total = untranslated + translated
            self.data.append({
                "total": total,
                "translated": translated,
                "language": locale.display_name,
                "obsolete": obsolete,
                "untranslated": untranslated,
            })

        self.data = sorted(self.data, key=lambda x: (-x["translated"], -x["obsolete"]))

    def _th(self, tr, text):
        cell = etree.SubElement(tr, "th")
        cell.attrib["style"] = "text-align:right;"
        cell.text = text

    def _cell(self, tr, text, style=None):
        cell = etree.SubElement(tr, "td")
        if style:
            cell.attrib["style"] = style
        cell.text = text

    def handleMatch(self, m, data):
        if self.data is None:
            self.fetch_data()

        table = etree.Element("table")

        thead = etree.SubElement(table, "thead")
        tr = etree.SubElement(thead, "tr")
        etree.SubElement(tr, "th").text = "Language"
        self._th(tr, "Completion")
        self._th(tr, "Translated")
        self._th(tr, "Missing")
        self._th(tr, "Obsolete")

        tbody = etree.SubElement(table, "tbody")

        for row in self.data:
            tr = etree.SubElement(tbody, "tr")

            percent = round(row["translated"]/row["total"]*100)
            if percent > 80:
                color = "#0a0"
            elif percent > 50:
                color = "#b80"
            else:
                color = "#c00"

            self._cell(tr, row["language"])
            data_style = "text-align:right; font-family: monospace;"
            self._cell(tr, "%s%%" % percent, data_style + "color: %s;" % color)
            self._cell(tr, str(row["translated"]), data_style)
            self._cell(tr, str(row["untranslated"]), data_style)
            self._cell(tr, str(row["obsolete"]), data_style)

        return table, m.start(0), m.end(0)


class LottieColor(InlineProcessor):
    def __init__(self, pattern, md, mult):
        super().__init__(pattern, md)
        self.mult = mult

    def handleMatch(self, match, data):
        span = etree.Element("span")
        span.attrib["style"] = "font-family: right"

        comp = [float(match.group(i)) / self.mult for i in range(2, 5)]

        hex = "#" + "".join("%02x" % round(x * 255) for x in comp)
        color = etree.SubElement(span, "span")
        color.attrib["style"] = css_style(
            width="24px",
            height="24px",
            background_color=hex,
            border="1px solid black",
            display="inline-block",
            vertical_align="middle",
            margin_right="0.5ex"
        )

        etree.SubElement(span, "code").text = "[%s]" % ", ".join("%.3g" % x for x in comp)

        return span, match.start(0), match.end(0)


class Matrix(BlockProcessor):
    RE_FENCE_START = r'^\s*\{matrix\}\s*\n'

    def test(self, parent, block):
        return re.match(self.RE_FENCE_START, block)

    def run(self, parent, blocks):
        table = etree.SubElement(parent, "table")
        table.attrib["style"] = "font-family: monospace; text-align: center; background-color: #fcfdff; border: 1px solid #ccc;"
        table.attrib["class"] = "table-plain"
        rows = blocks.pop(0)
        for row in rows.split("\n")[1:]:
            tr = etree.SubElement(table, "tr")
            for cell in row.split():
                td = etree.SubElement(tr, "td")
                td.text = cell
                td.attrib["style"] = "width: 25%;"
        return True


class GlaxnimateExtension(Extension):
    def extendMarkdown(self, md):
        md.inlinePatterns.register(LottieInlineProcessor(md), 'lottie', 175)
        md.inlinePatterns.register(DownloadTable(r'{download_table:([^:]+)}', md, False), 'download_table', 175)
        md.inlinePatterns.register(DownloadTable(r'{download_table:([^:]+):git}', md, True), 'download_table_git', 175)
        md.inlinePatterns.register(TranslationTable(r'{translation_table}', md), 'translation_table', 175)
        md.inlinePatterns.register(LottieColor(r'{lottie_color:(([^,]+),\s*([^,]+),\s*([^,]+))}', md, 1), 'lottie_color', 175)
        md.inlinePatterns.register(LottieColor(r'{lottie_color_255:(([^,]+),\s*([^,]+),\s*([^,]+))}', md, 255), 'lottie_color_255', 175)
        md.parser.blockprocessors.register(Matrix(md.parser), 'matrix', 175)


def makeExtension(**kwargs):
    return GlaxnimateExtension(**kwargs)
