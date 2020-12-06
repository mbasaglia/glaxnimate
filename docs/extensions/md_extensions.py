from markdown.inlinepatterns import InlineProcessor
from markdown.extensions import Extension
import os
import xml.etree.ElementTree as etree


def etree_fontawesome(icon):
    el = etree.Element("i")
    el.attrib["class"] = "fas fa-%s" % icon
    return el


def clean_link(filename):
    if not filename.startswith("/") and not filename.startswith("."):
        filename = "../" + filename
    return filename



class LottieInlineProcessor(InlineProcessor):
    def __init__(self, pattern, md):
        super().__init__(pattern, md)
        self._id = 0

    def handleMatch(self, m, data):
        el = etree.Element("div")
        el.attrib["class"] = "lottie-container"

        animation = etree.Element("div");
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
        play.attrib["title"] = "Play";
        play.attrib["style"] = "display:none";

        pause = etree.Element("button")
        el.append(pause)
        pause.attrib["id"] = "lottie_pause_{id}".format(id=self._id)
        pause.attrib["onclick"] = "anim_{id}.pause(); document.getElementById('lottie_play_{id}').style.display = 'inline-block'; this.style.display = 'none'".format(id=self._id)
        pause.append(etree_fontawesome("pause"))
        pause.attrib["title"] = "Pause";

        if download_file:
            download = etree.Element("a")
            el.append(download)
            download.attrib["href"] = download_file
            download.attrib["download"] = ""
            download.attrib["title"] = "Download";
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


class GlaxnimateExtension(Extension):
    def extendMarkdown(self, md):
        DEL_PATTERN = r'{lottie:([^:]+)(?::([0-9]+):([0-9]+))(?::([^:]*))}'
        md.inlinePatterns.register(LottieInlineProcessor(DEL_PATTERN, md), 'lottie', 175)


def makeExtension(**kwargs):
    return GlaxnimateExtension(**kwargs)
