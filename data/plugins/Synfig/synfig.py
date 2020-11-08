import io
import json
import gzip

from lottie.parsers.sif import parse_sif_file
from lottie.parsers.sif.builder import to_sif
from lottie.objects.animation import Animation

import glaxnimate


def save_synfig(window, document, file, fname, import_export, settings):
    lottie_str = glaxnimate.io.registry.from_extension("json").save(document)
    lottie_dict = json.loads(lottie_str)
    animation = Animation.load(lottie_dict)
    dom = to_sif(animation).to_xml()
    compressed = fname.endswith("sifz")
    fp = io.StringIO()
    dom.writexml(fp, "", "  ", "\n", "utf-8")
    if compressed:
        with gzip.open(file, "wb") as gzfile:
            gzfile.write(fp.getvalue().encode("utf-8"))
    else:
        file.write(fp.getvalue().encode("utf-8"))


def open_synfig(window, document, file, fname, import_export, settings):
    animation = parse_sif_file(file)
    lottie_str = json.dumps(animation.to_dict()).encode("utf-8")
    glaxnimate.io.registry.from_extension("json").load(document, lottie_str)
